# Fork Windows x64 builds

The `Fork Windows x64` workflow runs on pushes to `feature/liquid-glass-ui`
and supports manual dispatch. The initial CI checkpoint builds the existing
client; the UI changes described in `PLAN.md` are not implemented yet.
Do not interpret this checkpoint's artifact as the completed redesign.

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
instead of silently compiling against incomplete dependencies. MSVC compilation
uses sccache with embedded debug information. API values are read from the
process environment by an initial CMake script; the application build tree
and CMake cache are neither cached nor uploaded.

Cold dependency preparation may exceed the available runner disk, cache quota
or six-hour job limit. That has not been measured yet. If it does, stop and
use the named stage groups in `prepare.py` to split preparation further,
increase cache capacity or choose a suitable runner. Do not claim that the
separate app job solves an oversized dependency job. An interrupted or failed
dependency job cannot trigger the app build; its partial cache is solely for
the next preparation attempt. GitHub's required check/result and
the actual uploaded artifact are the source of truth.

## Download and side-by-side use

After a successful run, open **Actions → Fork Windows x64 → the successful
run → Artifacts**, download `LiquidGlass-Windows-x64-<commit>`, then extract
the artifact and its included portable zip into a new writable directory.
Run `LiquidGlass/Start Liquid Glass.cmd`, which passes an isolated `profile`
folder through the supported `-workdir` flag. Do not copy official Telegram's
`tdata`, install over its directory, or launch this fork's executable directly
if you want to ensure the launcher always chooses the isolated directory.
The portable package includes the ANGLE `modules` directory when generated.

The build sets `DESKTOP_APP_DISABLE_AUTOUPDATE=ON`; the package excludes the
Updater. Updates to this fork require downloading a new artifact. Crash
report uploads are disabled for this portable build. The package preserves
license notices. It is unsigned, and no installer or signing identity is
provided. Windows SmartScreen and environment-specific runtime dependencies
must be evaluated on the resulting executable.

Working-directory isolation is the documented coexistence mechanism at this
checkpoint. Full taskbar/notification/protocol branding and simultaneous-run
behavior have not been validated; do not register this build as the default
Telegram protocol handler. The CI package records `dumpbin /dependents`
output, but import inspection alone is not a startup smoke test.
