# xiaomi-be7000-uboot Repository Instructions

These instructions apply to all agents working in this repository.

## Scope

- Write repository `AGENTS.md` files entirely in English.
- Confirm the repository root, target branch, worktree status, and applicable instructions before editing. Keep changes within the authorized task and preserve existing work.
- Track build source, build configuration and scripts, test and validation code, and essential resources and documentation. Maintained configuration and rule data are source. Keep temporary files in the task directory. Deliver build packages and validation outputs through the project artifact channel when authorized.

## GitHub

### Repository changes

- Make fixes directly in the maintained source code, configuration, or build logic that owns the behavior, including in forks. Do not add patch files, build-time source rewrites, or redundant wrappers as substitutes for direct fixes.
- Start new upstream-based work and upstream updates from the latest commit on the applicable upstream branch. Verify the branch tip remotely and record the upstream repository, branch, and full commit SHA. Use the upstream default branch unless the task specifies another branch or revision.
- Run all repository builds and compilation exclusively in GitHub Actions; do not build or compile locally. Deliver the verified artifacts from the successful workflow run.
- Keep GitHub Actions on the latest stable Linux runner (`ubuntu-latest`) as the sole build and packaging path. Use a different GitHub Actions runner only when the deliverable cannot be built on `ubuntu-latest`.

### GitHub CLI

- Use official `gh` first for GitHub operations. Resolve PATH candidates without executing them. On Windows also check known locations, including `%ProgramFiles%\GitHub CLI\gh.exe`, and available installer records. Before `--version`, verify the actual file using trusted installer evidence binding its path and contents to the installed package, or compare its bytes with the executable from a verified official release package. Product name, publisher text, version, or expected path alone is insufficient. Invoke the verified absolute path; reuse verification only while that path and its SHA-256 match the verified copy. Report a broken installation separately. If absent and writes are permitted, install a verified official package in a task-local tools directory or an explicitly authorized installation directory. Obtain authorization for any other destination. Verify the installed executable and run `--version`.
- Use `gh api` for REST or GraphQL operations without a dedicated command. A connector or browser may handle operations the CLI does not support. For the same authorized read-only operation, use an already authorized connector if `gh` provenance cannot be verified within scope, or if `gh` is confirmed missing, broken, or unable to start and installation or repair is outside scope. Leave an unverified executable unexecuted. Never switch channels to bypass an access denial.

### Pull requests and merges

- Every GitHub repository must have an active `Protect main` ruleset for its default branch. Require pull requests and block branch deletion and force pushes. Require status checks that actually run on pull requests targeting the protected branch.
- Enable and verify **Automatically delete head branches** (`delete_branch_on_merge: true`) in every GitHub repository when creating or configuring it. Before merging a pull request, check the setting on the source repository, including the fork for a fork-based pull request. Enable it when authorized, and report any permission or branch protection rule that prevents automatic deletion. Report any other limitation on automatic branch deletion.
- Before writing to a shared branch, verify the current head and check for competing writers. Follow the repository rules for reviews and checks, and verify the remote commit and source branch cleanup after merging.

### README and Release pages

- Write the primary README and GitHub Release titles and bodies in Simplified Chinese by default, including workflow-generated notes. Follow explicit language requests and keep official names and identifiers. Translate the headings listed below into the document language.
- Start the README with one project-name H1 and a brief overview. Use applicable H2s in the order Features, Quick Start, Configuration, Build and Test, Repository Structure, Releases, and License. Cover prerequisites, installation and basic usage in Quick Start.
- Title each Release `<project> <version>` and open with a short summary. Use applicable H2s in the order Main Changes, Downloads and Verification, Installation and Upgrade, and Verification Status. Identify verified assets and `SHA256SUMS.txt`. Distinguish CI from device or production acceptance.
- Use GitHub Markdown with tagged code fences, relative README file links and tag-pinned Release links to repository files. Verify commands, paths, links and licenses. Omit empty sections and placeholders. Preserve accurate content. Name and link upstreams, record known refs, credit imports and retain license and copyright notices.

## Build and Verification

- Target the Xiaomi BE7000 with Qualcomm IPQ9574. Maintain `configs/ipq9574_xiaomi_be7000_defconfig` and `scripts/build-be7000.sh` as the authoritative configuration and GitHub Actions build entry points. Preserve shared code and device-tree dependencies used by this board.
- Verify the Qualcomm QSDK U-Boot provenance in the README when modifying upstream code. Verify the Xiaomi stock firmware layout and existing test coverage when changing ART or MAC handling.
- Run Webfailsafe, NAND backup, and ART regression checks under `tests/` and U-Boot compilation exclusively through the `Verify BE7000 U-Boot` GitHub Actions workflow (`.github/workflows/build-ipq-uboot.yml`) on pull requests to `main` or authorized workflow runs.
- Deliver MBN images and checksums through GitHub Actions artifacts or Releases. Check automatic release conditions before triggering a workflow run. Keep device operations and publication within task authorization, and report GitHub Actions verification and build results separately from hardware acceptance.

## Versioned deliverables

- When a repository change affects a versioned deliverable, build and deliver the latest authorized repository state. Advance the version using the project's versioning rules or the user's specified version. Use one release version for the output filename, archive directory, package metadata, and the executable or application's reported version where applicable. Record upstream component versions and source revisions separately.
- Verify the version at each applicable surface in the actual delivered artifact and refresh its checksums. Build, publication and deployment follow their respective authorization scopes.

## Release asset integrity

- Deliver native usable files directly with `SHA256SUMS.txt`. Publish the same verified build outputs to GitHub Releases with identical filenames and SHA-256 hashes.
- Match the Release tag to the verified build commit. Check every published asset against the delivery files and checksum manifest. Report missing source, build or Release evidence.
- Use the repository's native format and honor the requested packaging format.
