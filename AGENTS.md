# xiaomi-be7000-uboot Repository Instructions

## Scope

- Confirm the repository root, target branch, worktree status, and applicable instructions before editing. Keep changes within the authorized task and preserve existing work.
- Track build source, build configuration and scripts, test and validation code, and essential resources and documentation. Maintained configuration and rule data are source. Keep temporary files in the task directory. Deliver build packages and validation outputs through the project artifact channel when authorized.

## GitHub

- Make fixes directly in the maintained source code, configuration, or build logic that owns the behavior, including in forks. Do not add patch files, build-time source rewrites, or redundant wrappers as substitutes for direct fixes.
- Prefer the verified official `gh`. On Windows, resolve the executable without running it and verify its contents with trusted installer evidence or an official release package. Use the verified absolute path and SHA-256. Use `gh api` for operations without a dedicated command. An authorized connector may perform the same read-only operation if the CLI is unavailable or unverified and repair is outside scope. Respect access denials.
- Every GitHub repository must have an active `Protect main` ruleset for its default branch. Require pull requests and block branch deletion and force pushes. Require status checks that actually run on pull requests targeting the protected branch.
- Enable and verify `delete_branch_on_merge: true` when creating or configuring a repository. Before merging a pull request, verify this setting in its source repository, including a fork, and report any limit on automatic branch deletion.
- Before writing to a shared branch, verify the current head and check for competing writers. Follow the repository rules for reviews and checks, and verify the remote commit and source branch cleanup after merging.
- Maintain one H1 in the primary README, with the project name and purpose. Where applicable, keep the H2 order Features, Quick Start, Configuration, Build and Test, Repository Structure, Releases, and License. Use the established language, fenced code blocks with language identifiers, and relative links for repository files.
- Name and link the upstream repository in the README, and record the source tag or commit when known. Credit imported components in the README or linked third-party notices and preserve applicable license and copyright notices.

## Build and Verification

- Target the Xiaomi BE7000 with Qualcomm IPQ9574. Use `configs/ipq9574_xiaomi_be7000_defconfig` and `scripts/build-be7000.sh`. Preserve shared code and device-tree dependencies used by this board.
- Verify the Qualcomm QSDK U-Boot provenance in the README when modifying upstream code. Verify the Xiaomi stock firmware layout and existing test coverage when changing ART or MAC handling.
- Run the applicable offline regressions under `tests/` for Webfailsafe, NAND backup, and ART changes. The `Verify BE7000 U-Boot` check covers pull requests to `main`.
- Deliver MBN images and checksums through Actions or Releases. Check automatic release conditions before triggering a build. Keep device operations and publication within task authorization, and report offline verification separately from hardware acceptance.

## Versioned deliverables

- When a change affects a versioned deliverable, build and deliver the latest authorized repository state. Advance the project release version and use it consistently in the output filename, archive directory, package metadata and the executable or application's reported version where applicable.
- Record upstream versions and revisions separately. Check the version and checksum in the final artifact. Follow the authorized scope for publishing and deployment.

## Release asset integrity

- Deliver native usable files directly with `SHA256SUMS.txt`. Publish the same verified build outputs to GitHub Releases with identical filenames and SHA-256 hashes.
- Match the Release tag to the verified build commit. Check every published asset against the delivery files and checksum manifest. Report missing source, build or Release evidence.
