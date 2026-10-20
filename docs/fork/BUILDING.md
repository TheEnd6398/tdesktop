# Fork Windows x64 builds

The `Fork Windows x64` workflow runs on pushes to `feature/liquid-glass-ui`
and supports manual dispatch. The fork builds with
`TDESKTOP_FORK_LIQUID_GLASS=ON`; this option also defaults ON. The original
baseline artifact predates the visual changes. Download an artifact from the
actual UI commit being reviewed.

## Interface controls and current rendering

Settings → Advanced → Performance exposes **Liquid Glass interface**. Power
Saving exposes glass blur and smooth scrolling independently; automatic battery
saving disables costly effects while retaining the floating layout.

The header and both composers use cached, downsampled samples of the current
chat wallpaper. They retain reserved reading space, so messages do not extend
behind those bars. Corner buttons sample the actual message-list widget in
bounded regions outside paint events. The main menu samples the underlying
content widget before opening and refreshes its bounded backdrop when that
content paints. Chat rows, folder buttons, search, settings navigation and
profile actions share rounded geometry and material highlights. Date badges retain translucent palette
colors with rounded edge highlights; the recorder uses a rounded opaque
fallback. The fork does not implement native Windows desktop blur or optical
refraction. No full-window continuous capture is used.

Focused Qt checks exercise blur boundaries, premultiplied alpha, buffer reuse,
light/dark materials, five DPR values and rendering a small region of a tall
source widget. A third check covers discrete-wheel classification, fractional
deltas, easing, accumulation, reversal, range limits and cancellation. Mouse
wheels use the existing scroll APIs; touchpad, phased, horizontal and modified
wheel input keeps its native path. Flexible settings pages retain their existing
wheel policy. These checks do not establish Windows GUI correctness. Test
normal/narrow windows, light/dark/custom themes, multi-line input, reply/edit,
recording, pinned navigation, account switches and keyboard/mouse interaction
with the freshly built client before treating the interface as visually
verified.

## Required repository configuration

In GitHub, open **Settings → Secrets and variables → Actions → New repository
secret** and add `API_ID` and `API_HASH` using your own application credentials
from [my.telegram.org](https://my.telegram.org/apps). Enter values only through
the secure settings UI. The workflow checks presence before dependency work
and does not substitute public test credentials. The current integration
cannot enumerate these secrets; only an actual preflight establishes their
availability to Actions.

Actions must be enabled and allowed to run the referenced checkout, cache,
MSVC setup, sccache and artifact actions. The workflow needs read access to
repository contents. Its pushes must be authorized to update workflow files.

## Build and cache behavior

A credentials job runs first. Separate Windows jobs prepare dependencies and
compile/package the application, each with a 350-minute timeout. Dependencies
use this checkout's pinned preparation script, SDK 10.0.26100.0 and MSVC
14.44. Each Windows job checks matching ATL headers and libraries and installs
the versioned `Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL` component
when missing. This avoids Breakpad's `atlbase.h`/`atlcomcli.h` failures when a
runner has the old compiler but only the newest ATL. The bootstrap exports
the actual versioned include/library paths to subsequent steps. The component
identifier is listed in Microsoft's [Visual Studio 2026 component directory](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=visualstudio).
Debug and Release library configurations are prepared because the
upstream script does not expose a supported skip-debug switch. The application
is built in Release only.

ThirdParty tools, non-Qt Libraries and Qt have separate recipe/toolchain keys,
with a unique revision for each run attempt. Preparation restores the latest
matching snapshot and saves tools/library progress even when a later stage
fails. The preparation script's stage keys decide which outputs can be reused;
a failed stage without its success key is rebuilt. Qt is saved after its stage
key exists. The build job requires all three exact caches from its successful
dependency job; it fails on missing handoff
instead of silently compiling against incomplete dependencies. Completed Qt
snapshots keep the installed prefix, including its libraries, tools, headers,
plugins and CMake files. The duplicate source/build tree is removed while its
stage directory and key remain, so preparation can reuse the installation.
Rust source trees are retained for their generated bindings and rebuilds.
MSVC compilation uses sccache. The application disables debug information
through `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=`, which selects the project's
`/DEBUG:NONE` linker path. Release optimization remains enabled; the portable
artifact has no app PDB. This avoids the hosted runner's observed `LNK1102`
failure while generating a full debug database. API values are read from the
process environment by an initial CMake script; the application build tree
and CMake cache are neither cached nor uploaded.

The first successful dependency preparation took about 114 minutes. Its
untrimmed snapshots occupied about 11.5 GB, which caused tools/library cache
eviction during app compilation. Trimming the duplicate Qt tree reduces that
handoff; its final size must be checked in Actions. Cold dependency preparation
may still exceed the available runner disk, cache quota or six-hour job limit.
If it does, stop and
use the named stage groups in `prepare.py` to split preparation further,
increase cache capacity or choose a suitable runner. Do not claim that the
separate app job solves an oversized dependency job. An interrupted or failed
dependency job cannot trigger the app build; its partial cache is solely for
the next preparation attempt. GitHub's required check/result and
the actual uploaded artifact are the source of truth.

## Download and side-by-side use

Public preview builds are available under the repository's **Releases** page.
Download `LiquidGlass-Windows-x64.zip` and extract it into a new writable
directory. Release assets can be downloaded without signing in to GitHub.
For a build that has not been published, sign in to GitHub, open
**Actions → Fork Windows x64 → the successful run → Artifacts**, download
`LiquidGlass-Windows-x64-<commit>`, then extract the artifact and its included
portable zip. GitHub Actions artifact downloads require an authenticated
session even in a public repository.
Run `LiquidGlass/Start Liquid Glass.cmd`, which passes an isolated `profile`
folder through the supported `-workdir` flag. Direct EXE launches also default
to that folder. If it cannot be created or written, the Windows fork uses
`%APPDATA%/Liquid Glass/`. An explicit `-workdir` remains supported. Do not copy
official Telegram's `tdata` or install over its directory.
The portable package includes the ANGLE `modules` directory when generated.

The build sets `DESKTOP_APP_DISABLE_AUTOUPDATE=ON`; the package excludes the
Updater. Updates to this fork require downloading a new portable package. Crash
report uploads are disabled for this portable build. The package preserves
license notices. It is unsigned, and no installer or signing identity is
provided. Windows SmartScreen and environment-specific runtime dependencies
must be evaluated on the resulting executable.

The Windows fork uses its own AppUserModelID, toast activator CLSID, startup,
Send To and Start Menu shortcut names. It skips automatic Telegram protocol
registration and legacy official-data/installer migration. The package records
`dumpbin /dependents` output. After compiling, CI launches an unauthenticated
client with a temporary `-workdir`, requires a native window and ten seconds of
responsiveness, then removes that test profile. Check the actual smoke-test
step result; it does not establish account, chat or coexistence behavior.

Use the Windows verification matrix in [IMPLEMENTATION.md](IMPLEMENTATION.md)
for interactive acceptance and known rendering limitations.

To publish an existing successful native build without recompiling it, run
**Verify Windows fork package** manually with its native `artifact_id` and
`publish_release` enabled. Verification runs first; publication creates a
prerelease tagged with the binary's source commit, containing the portable ZIP,
`verification.json` and `SHA256SUMS.txt`. The publication job needs repository
contents write permission and leaves existing releases unchanged.
