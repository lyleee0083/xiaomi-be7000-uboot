#!/usr/bin/env python3
"""Personalize the unsigned BE7000 MBN locally for browser certificate access."""

import argparse
import getpass
import hashlib
import ipaddress
import struct
from datetime import datetime, timedelta, timezone
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID


SLOT_MAGIC = b"BE7000-WebTerm-mTLS-provision-v1".ljust(32, b"\0")
SLOT_SIZE = 4096
PARTITION_SIZE = 1024 * 1024


def personalize(image, key, server, ca):
    if image[:7] != b"\x7fELF\x01\x01\x01" or len(image) > PARTITION_SIZE:
        raise ValueError("Expected a BE7000 ARM ELF/MBN within the APPSBL partition size")
    machine, = struct.unpack_from("<H", image, 18)
    ph_offset, = struct.unpack_from("<I", image, 28)
    header_len, ph_size, ph_count = struct.unpack_from("<HHH", image, 40)
    if (machine, header_len, ph_size, ph_count) != (40, 52, 32, 4):
        raise ValueError("Unexpected ELF header")
    ph_end = ph_offset + ph_size * ph_count
    if ph_offset != header_len or ph_end > len(image):
        raise ValueError("Invalid program header range")
    segments = [struct.unpack_from("<8I", image, ph_offset + n * ph_size)
                for n in range(ph_count)]
    _, hash_offset, _, _, hash_len, _, hash_flags, _ = segments[1]
    kind, load_offset, _, _, load_len, _, _, _ = segments[2]
    if (hash_len != 240 or hash_flags != 0x02200000 or kind != 1 or
            not (ph_end <= hash_offset <= hash_offset + hash_len <= load_offset) or
            load_offset + load_len != len(image)):
        raise ValueError("Unexpected BE7000 MBN v6 segments")

    fields = struct.unpack_from("<12I", image, hash_offset)
    if fields[1] != 6 or fields[4:6] != (192, 192) or any(fields[6:]):
        raise ValueError("Expected an unsigned MBN v6 with SHA-384 program hashes")
    hash_start = hash_offset + 48
    load_hash = hash_start + 96
    original_load = image[load_offset:]
    expected = (hashlib.sha384(image[:ph_end]).digest() + bytes(48) +
                hashlib.sha384(original_load).digest() + bytes(48))
    if image[hash_start:hash_offset + hash_len] != expected:
        raise ValueError("The source MBN hashes do not match")
    if original_load.count(SLOT_MAGIC) != 1:
        raise ValueError("Expected one BE7000 WebTerm credential slot")
    offset = load_offset + original_load.index(SLOT_MAGIC)
    if image[offset:offset + SLOT_SIZE] != SLOT_MAGIC.ljust(SLOT_SIZE, b"\0"):
        raise ValueError("Provisioning only supports a fresh, unpaired template")
    if not all((key, server, ca)) or len(key) + len(server) + len(ca) > SLOT_SIZE - 48:
        raise ValueError("Credentials do not fit the reserved slot")
    contents = SLOT_MAGIC + struct.pack("<4I", 1, len(key), len(server), len(ca)) + key + server + ca
    result = bytearray(image)
    result[offset:offset + SLOT_SIZE] = contents.ljust(SLOT_SIZE, b"\0")
    result[load_hash:load_hash + 48] = hashlib.sha384(result[load_offset:]).digest()
    return bytes(result)


def credentials(ip, password):
    now = datetime.now(timezone.utc)
    key_ca = ec.generate_private_key(ec.SECP256R1())
    key_device = ec.generate_private_key(ec.SECP256R1())
    key_browser = ec.generate_private_key(ec.SECP256R1())
    label = f"BE7000 {ip}"
    who_ca = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, label + " recovery CA")])

    def certificate(subject, key, expires):
        return (x509.CertificateBuilder()
                .subject_name(subject).issuer_name(who_ca).public_key(key.public_key())
                .serial_number(x509.random_serial_number())
                .not_valid_before(now - timedelta(days=1)).not_valid_after(expires))

    ca = (certificate(who_ca, key_ca, now + timedelta(days=1095))
          .add_extension(x509.BasicConstraints(ca=True, path_length=0), critical=True)
          .add_extension(x509.KeyUsage(False, False, False, False, False, True, False, False, False),
                         critical=True)
          .sign(key_ca, hashes.SHA256()))

    def leaf(name, key, usage, alternate_names=None):
        builder = (certificate(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, name)]),
                               key, now + timedelta(days=730))
                   .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
                   .add_extension(x509.KeyUsage(True, False, False, False, False, False, False, False, False),
                                  critical=True)
                   .add_extension(x509.ExtendedKeyUsage([usage]), critical=False))
        if alternate_names is not None:
            builder = builder.add_extension(x509.SubjectAlternativeName(alternate_names), critical=False)
        return builder.sign(key_ca, hashes.SHA256())

    device = leaf(str(ip), key_device, ExtendedKeyUsageOID.SERVER_AUTH, [x509.IPAddress(ip)])
    browser = leaf(label + " administrator", key_browser, ExtendedKeyUsageOID.CLIENT_AUTH)
    ca_der = ca.public_bytes(serialization.Encoding.DER)
    device_der = device.public_bytes(serialization.Encoding.DER)
    device_key = key_device.private_bytes(serialization.Encoding.DER,
                                          serialization.PrivateFormat.PKCS8,
                                          serialization.NoEncryption())
    browser_pfx = pkcs12.serialize_key_and_certificates(
        b"BE7000 recovery administrator", key_browser, browser, [ca],
        serialization.BestAvailableEncryption(password.encode("utf-8"))
    )
    return device_key, device_der, ca_der, browser_pfx


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("template", type=Path)
    parser.add_argument("destination", type=Path, help="new private directory")
    parser.add_argument("--ip", type=ipaddress.IPv4Address, default=ipaddress.IPv4Address("192.168.1.1"))
    args = parser.parse_args()
    if not args.ip.is_private or args.ip.is_unspecified or args.ip.is_loopback or args.ip.is_multicast:
        parser.error("Use the private IPv4 address of the BE7000 recovery interface")
    if args.destination.exists():
        parser.error("Destination already exists")
    password = getpass.getpass("New browser certificate password: ")
    if len(password) < 10 or password != getpass.getpass("Repeat password: "):
        parser.error("Use a matching password of at least 10 characters")
    key, server, ca, browser = credentials(args.ip, password)
    image = personalize(args.template.read_bytes(), key, server, ca)
    args.destination.mkdir(mode=0o700)
    for name, contents in (
        ("BE7000-personal.mbn", image),
        ("BE7000-trusted-ca.cer", ca),
        ("BE7000-browser.pfx", browser),
    ):
        with (args.destination / name).open("xb") as target:
            target.write(contents)
    print(f"Private output: {args.destination}")
    print(f"Recovery address: https://{args.ip}/")


if __name__ == "__main__":
    main()
