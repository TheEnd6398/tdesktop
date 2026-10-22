# Aurora Glass for Telegram Desktop

Aurora changes the native C++/Qt client. Enable `TDESKTOP_FORK_LIQUID_GLASS`
at configure time; the Windows fork workflow enables it. The fork applies three
versioned toolkit patches before style generation. The submodule revision remains
pinned, and a clean checkout can reproduce the changes without unpublished
submodule commits.

## Install and run

Extract `AuroraGlass-Windows-x64.zip` into a new directory and run
`Start Aurora Glass.cmd`. The launcher creates `profile` beside the executable.
The Windows app identity and default application-data directory are separate from
official Telegram and the earlier Liquid Glass preview. Do not copy the executable
into an existing Telegram installation. Official self-updates are disabled in this
distribution, and the package excludes the updater and personal account data.

New profiles start in Aurora's midnight appearance. Existing theme preferences are
retained. The normal night-mode switch selects dark or pearl appearances, and
custom themes retain their own palette. The Advanced setting named “Aurora Glass
interface” controls material painting and floating chat controls. Geometry and
the bundled palettes are part of this fork even when that switch is disabled.

## Design and implementation coverage

The shared material renderer has five levels: crystal, frosted, smoked, soft and
solid content. It paints restrained reflections, accent diffusion and adaptive
edge lighting using Qt gradients and the active theme's colors. Existing chat
backdrop capture supplies bounded, downsampled, cached blur. Messages and settings
text retain opaque content surfaces. No shader, continuous lighting loop, web UI,
or new runtime dependency is introduced.

| Surface | Production implementation | Verification still needed |
| --- | --- | --- |
| Main shell | Native Windows title controls, taller title bar, coordinated chrome | Windows Snap, maximize/restore, high DPI and native frame review |
| Navigation | Inset smoked rail, folder selections, account/menu materials and badges | Folder drag, keyboard navigation, long localized names |
| Chat list | New row proportions, avatar spacing, highlighted selected row and directional accent | Real-account density, unread/muted states, RTL, custom themes |
| Search and archive | Search field geometry, matching result and collapsed archive surfaces | Filter combinations, narrow layout and long results |
| Conversation | Floating header/actions/composer, differentiated bubbles, larger text insets and reply surfaces | Text/media groups, bot keyboards, long Persian and Arabic messages |
| Profiles | Material profile cover, navigation and shared settings/action controls | User/group/channel profiles, stories rings and custom profile colors |
| Settings | Shared inset controls, hover feedback, section spacing and coherent palette | Every settings section, keyboard focus, large text and accessibility |
| Menus and dialogs | Toolkit integration for menu background, inset selection, dialog material and radii | Submenus, disabled/destructive items, custom dialog corners |
| Notifications | Smoked in-app toasts | Different lengths and links; native Windows toast layout is retained |
| Login and lock | Smoked background, coordinated inputs/buttons and isolated new-profile startup | Login/code/password/passcode flows and both appearances |
| Media | Floating playback controller and material music-player bar | Video/PiP/fullscreen, gallery and download interactions |
| Calls | Material fallback call background and call buttons | Actual calls, video, screen sharing and group-call controls |
| Emoji and stickers | Shared palette, panel radii, menus, scrollbars and existing animations | Picker layouts, GIFs, sticker browsing and reactions |
| Stories, Premium and remaining utility pages | Inherit palettes, common controls, menus and dialogs | Their bespoke layouts and effects retain upstream composition |
| Icons | Retain the existing high-DPI icon pipeline and action semantics; title glyphs realigned | A complete new icon family has not been implemented |

Shared styling is not a claim that every specialized screen was individually
reconstructed. Stories, Premium, group voice chat, polls, attachment layouts and
administrative flows retain substantial upstream composition. Native OS backdrop
blur and physically accurate refraction are not implemented.

## Quality audit

For each surface above, acceptance requires checking identity consistency,
material treatment, typography, spacing/alignment, geometry, color harmony,
motion, usability/accessibility, resizing/scaling/RTL, and runtime completeness.
The source work establishes common tokens and paint paths; account-dependent
visual acceptance remains pending until genuine application captures are reviewed.

Automated checks cover five material levels in dark/light at 100%, 125%, 150%,
200% and 300% device pixel ratios, clipping and painter isolation, opaque fallback,
transparent controls, backdrop capture/filtering, wheel motion and chat insets.
The palette check covers 34 normal-text contrast pairs at a minimum 4.5:1.
This does not certify contrast for every custom theme or arbitrary wallpaper.

Hover feedback reuses Telegram's animation infrastructure and checks reduced
motion and power-saving preferences. Blur uses the existing independent power
saving flag. The new lighting remains static and adds no repeated full-window
backdrop capture. End-to-end performance still needs measurement in a populated
account.

## Reproduce checks and resources

`Telegram/Resources/aurora/palettes.json` is the readable palette source.
After editing it, run `python3 Telegram/build/fork/prepare-aurora.py`, regenerate
the toolkit patch if the light palette changed, then run:

```sh
python3 Telegram/build/fork/prepare-aurora.py --check
python3 Telegram/build/fork/check-aurora.py
cmake -S Telegram/SourceFiles/tests/fork_glass -B out/aurora-checks -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/aurora-checks --parallel 4
ctest --test-dir out/aurora-checks --output-on-failure
```

The Windows workflow requires repository API secrets and builds the actual client,
runs an isolated unauthenticated startup check, attempts genuine desktop/narrow
login-window captures, verifies package architecture and source provenance, then
uploads the portable ZIP. A build is successful only when that workflow completes.
Login captures do not verify authenticated conversations, profiles or settings.
