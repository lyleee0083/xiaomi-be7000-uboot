# 小米 BE7000 U-Boot

本项目基于 [1980490718/u-boot-2016](https://github.com/1980490718/u-boot-2016) 的 Qualcomm QSDK 12.5 U-Boot，提供小米 BE7000 的构建配置和基于 lwIP 的 Webfailsafe 恢复界面。

| 平台 | 构建目标 | 设备 |
| --- | --- | --- |
| IPQ9574 | `ipq9574_xiaomi_be7000` | 小米 BE7000 |

## 源码范围

`board/` 仅保留 IPQ9574 板级实现和 Qualcomm 共用代码。`arch/arm/dts/` 只构建 BE7000 的设备树，板级 DTS 引用的基础描述作为 DTSI 保留。仓库保留 BE7000 的 ARMv7、闪存、网络与 Webfailsafe 源码、构建脚本和配置生成器，以及必要的使用说明和许可证。

## ART/MAC 定制

本项目依据从小米 BE7000 原厂固件获取的 ART 布局信息，实现 ART/MAC 读写与校验，支持六个有线和六个无线 MAC 槽位。无线校准区位于 ART 的 `0x1000`、`0x33000`、`0x65000`，读写和 Webfailsafe 校验使用各校准区对应的 MAC 与 XOR16 字段。

写入通过 NAND MTD 完成，整块读取并保留同一擦除块内的其他 ART 数据，写入后逐字节回读核对整个擦除块。读写范围由 SMEM 的实际分区长度和 NAND 几何约束。

## Linux 构建

Webfailsafe 保留分区备份和刷写功能。配对后的专用 MBN 在进入恢复模式时提供 HTTPS 和完整 U-Boot 网页终端。关闭自动重启后，可由配对管理员通过终端重启。ART 恢复接受与当前设备分区等长、通过 BE7000 无线校准校验的完整镜像。恢复前检查所有擦除块，按分区内的物理偏移写入并回读校验。

安装依赖（Ubuntu 24.04，使用与 CI 相同的 APT 快照及 Node.js 24.15.0）

```bash
sudo apt-get --snapshot 20261118T000000Z update
sudo apt-get --snapshot 20261118T000000Z install -y build-essential libncurses-dev gawk git gettext libssl-dev python3 \
  wget cpio flex bison bc rsync gzip zopfli device-tree-compiler
```

将固定版本的 ARM OpenWrt 工具链放在源码库同级目录

```bash
git clone https://github.com/lyleee0083/xiaomi-be7000-uboot.git
git init staging_dir
git -C staging_dir fetch --no-tags --depth=1 https://github.com/1980490718/toolchain-arm_cortex-a7_gcc-5.2.0.git 0daf0879219130c46812cd5029fbcc2940316b16
git -C staging_dir checkout --detach FETCH_HEAD
cd xiaomi-be7000-uboot
npm ci --prefix failsafe/html --ignore-scripts --no-audit --no-fund
./build.sh ipq9574_xiaomi_be7000
```

产物为 `bin/openwrt-ipq9574_xiaomi_be7000-u-boot.mbn`，格式为 MBN v6（SHA-384）。
`./build.sh clean_all` 清理 MBN 文件，`./build.sh clean` 同时清理构建中间文件。

GitHub Actions 的 **Build Xiaomi BE7000 U-Boot** 工作流支持 `main` 推送和手动运行，成功后可在该次运行的 Artifacts 直接下载 `openwrt-ipq9574_xiaomi_be7000-u-boot-<版本>.mbn`。
版本由 `.github/workflows/build-ipq-uboot.yml` 中的 `ARTIFACT_VERSION` 指定。更新仓库并准备生成下一版产物时递增版本，例如从 `0.0.1` 升到 `0.0.2`。同一版本的重跑沿用原版本号。

## 使用网页终端

从 Actions 取得编译好的通用 MBN 模板。要用网线和浏览器访问完整终端，在自己的电脑生成专用 MBN 和浏览器证书。配对证书及专用 MBN 存放在个人目录。

~~~bash
python -X utf8 -m pip install cryptography
python -X utf8 tools/pair_webterm.py openwrt-ipq9574_xiaomi_be7000-u-boot-0.0.4.mbn private-be7000
~~~

工具会要求为浏览器证书文件设置密码。将 `BE7000-trusted-ca.cer` 导入当前用户的受信任根证书存储，将 `BE7000-browser.pfx` 导入当前用户的个人证书存储。刷入 `BE7000-personal.mbn` 后，进入 Webfailsafe 并直接打开 https://192.168.1.1/，浏览器选择已导入的管理员证书即可看到 U-Boot 终端。如果恢复地址不同，生成材料时用 `--ip` 指定设备实际使用的 IPv4 地址。

网页终端可执行设备编译支持的 U-Boot 命令。遇到命令提示输入时，在同一输入框填写后点击「发送输入」。

专用 MBN 使用客户端证书保护整个 Webfailsafe 管理入口。通用模板沿用 HTTP 恢复页面，终端入口只接受已配对固件上的 HTTPS 连接。

## 许可与声明

- 本项目使用 GPLv2 开源，详见 LICENSE 文件
- 任何借鉴、复刻、二次开发须注明出处并以 GPLv2 开源，保留原始版权声明
- 本项目未经充分测试，使用风险自负
- 作者不承担因使用本代码而产生的任何损失或损害责任
- 仅供学习和研究使用，禁止用于侵犯他人利益的用途
