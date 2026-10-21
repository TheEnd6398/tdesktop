# Liquid Glass fork: repository research

Status: Phase 1 repository, Telegram iOS and Apple primary-source study is
complete. No production UI or workflow changes were made during this phase.
The working branch is `feature/liquid-glass-ui`.

## Sources and build baseline

This study reads the selected checkout rather than assuming upstream paths.
Some names in the request differ from this fork:

- Windows documentation is `docs/building-win.md`; there is no
  `docs/building-win-x64.md` in this checkout.
- Replies and topics use `HistoryView::ChatWidget` in
  `history/view/history_view_chat_section.*`; there is no separate
  `history_view_replies_section.cpp` here.
- The reusable composer is in
  `history/view/controls/history_view_compose_controls.*`.
- The Power Saving flags live in `ui/power_saving.*`, not `core/`.

Paths below are relative to `Telegram/SourceFiles/` unless otherwise stated.
The repository-wide `AGENTS.md`, source atlas, and `REVIEW.md` apply. This is
ordinary requested work, not an external AI task-queue campaign. Use the
existing checkout and preserve LF/no-BOM text in this Linux environment.

The root `CMakeLists.txt` requires CMake 3.25 or newer, includes the `cmake`
submodule, establishes common options, discovers Qt and then adds `Telegram`.
`Telegram/CMakeLists.txt` creates the executable, its component libraries,
code generation, platform sources and optional standalone tests.

`Telegram/build/prepare/prepare.py` is a staged, platform-aware dependency
builder. It resolves Qt through `Telegram/build/qt_version.py`, currently
6.11.2 for Windows, and writes per-stage keys in `cache_keys`. The keys depend
on recipes and declared dependencies; a restored directory alone does not
prove a stage is current. `silent` rebuilds stale stages without prompting.
Named stage arguments select stages explicitly; these selections do not
automatically establish that prerequisite stages have run. `skip-release`
exists for the upstream Debug CI recipe and is unsuitable for preparing a
Release-only artifact without examining each dependency's configuration.

The actual x64 dependency prefix is `BuildPath/Libraries/win64`, as both
`prepare.py` and `cmake/variables.cmake` establish. The checkout lives beside
`Libraries` and `ThirdParty`. Preserve this relative layout in CI. The
Windows docs specify SDK 10.0.26100.0 and MSVC toolset 14.44, despite newer
Visual Studio branding; the Qt preparation also integrates Windows 7/ANGLE
patches. A stock Qt SDK is not an equivalent replacement.

`.gitmodules` pins the UI, base, reactive streams, concurrency, codegen,
storage, WebRTC, webview and other helper libraries and third-party sources.
All 40 recursive submodules were initialized at their recorded commits
during onboarding. They are not to be edited or repinned for this redesign.
The existing `Ui::ScrollArea` hook is particularly useful for avoiding a
`lib_ui` fork.

## Windows CI and distribution constraints

`.github/workflows/win.yml` currently builds a Debug matrix of architectures,
Qt variants and generators. x64 uses the GitHub Windows image for ordinary
branch pushes; other events may select Depot runners. It restores separate
ThirdParty, Libraries and Qt caches, prepares dependencies with
`win.bat skip-release silent`, calls `configure.bat`, and uploads
`Telegram.exe` and `Updater.exe`. Its Debug flags, updater-on option, test API
option and nightly credentials must not be copied into the requested fork
Release workflow.

The fork workflow needs an explicit x64 Release configuration and a narrow
push trigger for `feature/liquid-glass-ui`, plus manual dispatch. Use
repository secrets `API_ID` and `API_HASH`, validate their presence before
dependency work, and pass the supported `TDESKTOP_API_ID` and
`TDESKTOP_API_HASH` CMake variables without printing values. Set
`TDESKTOP_API_TEST=OFF`. The existing public test credentials are inappropriate
for a distributed artifact. Secrets have not been checked: the API connection
returned `Resource not accessible by integration`. This does not establish
that the secrets are absent; the workflow must check their presence without
printing them.

The first complete dependency build cannot be assumed to fit a six-hour job
or the repository's cache quota. The preparation script offers stage-level
selection, making a dependency DAG with durable cache handoff possible, but
the exact grouping must be measured on a Windows runner. Cache keys need the
recipe, architecture, Qt version, compiler and SDK. Save useful completed
dependency stages before an app compilation failure; avoid saving incomplete
stage state as a successful cache. A compiler cache complements dependency
caches but cannot substitute for them. No Windows runner or Release artifact
has been validated here.

`DESKTOP_APP_DISABLE_AUTOUPDATE=ON` adds `TDESKTOP_DISABLE_AUTOUPDATE` through
`Telegram/cmake/telegram_options.cmake`. The external updater interface also
conditions its setup on that CMake option, and `core/update_checker.cpp`
initializes its disabled state from the macro. Verify the resulting binary
does not initiate update checks. Omitting `Updater.exe` alone is insufficient.

`Core::Launcher` in `core/launcher.cpp` recognizes `-workdir` and normalizes
its value. A portable package should include a launcher pointing to a
dedicated folder, then verify coexistence with official Telegram's account
and single-instance routing. Do not copy existing `tdata`. Windows app identity
is handled through `platform/win/windows_app_user_model_id.*` and `specific_win.cpp`;
changing only the executable filename does not separate shortcuts,
notifications, protocol registration or taskbar identity. Branding is an
independent change requiring verification, not a prerequisite to documenting
the supported workdir flag. Preserve GPL and other notices.

## UI toolkit and style generation

`Telegram/lib_ui/ui/rp_widget.h` defines the reactive QWidget abstraction.
`RpWidgetWrap` exposes paint, geometry, size, visibility and input-event
producers with widget lifetimes. A hidden widget may defer move/resize event
delivery; a geometry subscription is not necessarily a synchronous layout
oracle. New callbacks must be tied to the owning widget's lifetime.

`Telegram/lib_ui/cmake/generate_styles.cmake` invokes `codegen_style`, resolves
style imports and emits `styles/style_*.h` and `.cpp` into the build tree.
`Telegram/cmake/td_ui.cmake` registers application styles; the `lib_ui` target
generates shared toolkit styles. Define new product geometry in application
`.style` files and register them there. Generated files are not edited.
Use `st::` scaled dimensions and existing palette colors, with radii derived
from the actual surface height. A growing multiline composer may need bounded
corner radii rather than turning its whole height into a giant pill.

`style::palette`, in `ui/style/style_core_palette.h` under `lib_ui`, wraps
generated palette data and supports serialization, overrides and theme
application. `style::color` provides live palette-backed colors. Cached glass
tints and masks must be invalidated on palette changes rather than retaining
colors sampled at construction.

`Ui::Animations::Basic` and `Simple` in `lib_ui/ui/effects/animations.h` use
the existing manager. `Basic` exposes start/stop and elapsed-time callbacks;
`Simple` interpolates values with an easing function. These are preferable to
introducing a wheel timer per scroll area.

## The two chat implementations and their geometry

`HistoryWidget` owns the legacy normal-chat screen. It uses `HistoryInner`
from `history/history_inner_widget.*`, and owns its composer controls itself.
`updateControlsGeometry()` places a full-width top bar, stacked group-call,
requests, pinned, sponsored, translation and other bars, then positions the
scroll area beneath them. `updateHistoryGeometry()` subtracts these heights
and the current footer states from the viewport height. Footer states include
multiline input, reply/edit/forward previews, bot keyboard, restrictions,
theme choosing and special action bars.

`visibleAreaUpdated()` forwards `[scrollTop, scrollTop + viewportHeight)` to
the list. That feeds item visibility, account read metrics and floating-player
placement. Preload thresholds use scroll position, maximum and viewport
height. `clampMousePosition()` constrains selection to the same visible
range. `updateHistoryGeometry()` records bottom stickiness before resizing
and has special handling while first-load and delayed positioning are pending.
Changing only the scroll area's rectangle would therefore corrupt more than
paint clipping.

`HistoryView::ChatWidget` in `history/view/history_view_chat_section.*` is
the other screen implementation: replies, topics, sublists and related
conversation contexts. It uses `HistoryView::ListWidget` and the reusable
`ComposeControls`. Its layout also subtracts top bars and compose height,
places the scroll area below bars, updates the visible range and autocomplete
bounds, and positions the composer separately. Scheduled messages use
`ScheduledWidget`; pinned messages use `PinnedWidget`, each with its own
viewport sizing. The source atlas explicitly identifies these mirrored
implementations. Forum/topic navigation also involves subsection tabs; their
left, top and bottom skips participate in geometry calculations.

`ListWidget::countItemsTop()` bottom-aligns short histories using `_minHeight`,
message height, collapse gaps and bottom padding, with bot/about-view
exceptions. `setItemsTop()` also shifts active removal-effect gaps. Adding a
top inset at a parent wrapper without accounting for these transforms would
misplace messages, hit tests, scroll anchors and effects. `HistoryInner` has
its own layout and saved top-item/offset mechanism. Both must be studied and
tested when introducing an explicit content-inset contract.

For underlapping glass, distinguish three spaces: the full physical
viewport; the unoccluded reading/selection area; and list content with top
and bottom reserves. Preserve a stable message anchor and bottom state when
insets change. Date badges, corner buttons, selection auto-scroll, drop
targets, floating media, search highlights, jump positioning and read metrics
must use the appropriate space. Full-height painting must not mark an
occluded message as read. Test first load, migration boundaries, short chats,
new messages at/away from bottom, returning to a saved position, and growing
or disappearing compose/pinned bars in both implementations.

## Header, pinned bar, composer and overlays

`HistoryView::TopBarWidget` in `history_view_top_bar_widget.*` currently fills
a full-width rectangle with `st::topBarBg`. `paintTopBar()` draws title/status
content; `updateControlsGeometry()` lays out actions and search/selection
states. Existing child buttons, click routing, accessible names, hover,
ripples and profile navigation must survive capsule regrouping. A rounded
background painted behind an unchanged full-width clickable region would
not provide correct capsule hit testing.

`history_view_pinned_tracker.*` selects pinned-message content; it is not the
surface renderer. `history_view_pinned_bar.*` supplies content to
`Ui::PinnedBar` in `ui/chat/pinned_bar.*`. The latter owns the wrapper,
`MessageBar`, action buttons and a separate shadow. It currently fills an
opaque `st::historyPinnedBg` rectangle and sets the wrapper opaque attribute.
All these layers matter when removing the rectangular card and adding margins.

The primary composer lives in `HistoryWidget`; the alternate composer is
`HistoryView::ComposeControls` in `history/view/controls/`. Its `FieldHeader`
paints an opaque reply-area background, so changing only the bottom wrapper
would leave opaque previews. Respect current `heightCurrent()`, autocomplete
bounds, send/attach/emoji buttons, rich draft previews, recording UI and bot
keyboard/restriction states. Existing child painting also needs inspection
before declaring a surface transparent.

`history_view_corner_buttons.*`, `HistoryInner` and `ListWidget` implement
corner overlays and scroll-date behavior. Their anchors are tied to the
viewport and list coordinates. Consistent glass treatment must preserve date
clicks, message navigation and bottom-return state.

## Main menu and backgrounds

`MainWindow::showMainMenu()` in `mainwindow.cpp` creates `Window::MainMenu`
and hands it to the layer stack. `window/window_main_menu.*` derives from
`Ui::LayerWidget`, handles account switching and swipe behavior, and fills
`st::mainMenuBg`. Cover overlays can paint that color again. The layer stack
and dimming animation affect which underlying image should be sampled.
Capture the content beneath the menu, excluding the menu itself and layer
dimming, rather than recursively rendering the entire parent from its paint
handler. Child menus and account rows need an opacity audit too.

`HistoryWidget::paintEvent()` and `ChatWidget::paintEvent()` delegate wallpaper
drawing to `Window::SectionWidget::PaintBackground`. `ui/chat/chat_theme.*`
owns background images, gradients, tiling and caches. Wallpaper-only blur
would not meet the requested effect: visible message text, bubbles and media
must also contribute to the sampled strips.

## Power Saving persistence and battery integration

`PowerSaving::Flag` in `ui/power_saving.h` currently occupies bits 0 through
10, with `kAll = (1U << 11) - 1`. A set bit means the corresponding effect is
disabled. `On(flag)` includes `ForceAll()`. `OnValue(flag)` emits the current
state and subsequent distinct changes, which is suitable for live fallback.

`Settings::PowerSavingLabels()` in `settings/settings_power_saving.cpp`
defines translated rows. `PowerSavingBox()` presents enabled effects as
`kAll & ~Current()`, then saves the inverse selection. A new bit must extend
`kAll`, have a lang-system label in `Telegram/Resources/langs/lang.strings`,
and be included in the descriptor. With a new disable bit unset, glass is
enabled by default for both existing users and new installations.

Persistence is a tagged `dbiPowerSaving` signed 32-bit integer written in
`storage/localstorage.cpp` and read with `Flags::from_raw()` in
`storage/details/storage_settings_scheme.cpp`. The tag is defined in the
matching header. Existing bit positions must remain unchanged; adding a bit
does not require inserting a field into `Core::Settings`' sequential stream.
Do not add a second unsynchronized preference for the same effect.

`Core::Application` combines battery-saving state with
`ignoreBatterySavingValue()` and calls `SetForceAll()`. Checking only
`Current()` bypasses automatic battery saving. Glass controllers must consume
`OnValue()`/`Changes()`, drop cached work when disabled, repaint the opaque
fallback and keep the resulting layout correct. Smooth scrolling should also
respect `kAnimations`; that flag already controls `anim::SetDisabled()`.

## Blur and GPU feasibility

`Telegram/lib_ui/ui/image/image_prepare.*` offers `Images::Blur` and
`BlurLargeImage`. `Blur` uses a fixed radius of three and normalizes to RGB32
or premultiplied ARGB. `BlurLargeImage` implements separable stack blur with
SIMD helpers for generic/SSE2/NEON backends, but currently allocates stack,
index and intermediate buffers on each call. Reuse is therefore not allocation
free merely because the caller reuses a `QImage`.

Downsampled strip capture is a reasonable candidate for this raster UI.
Include the blur kernel's support outside each surface's visible bounds to
avoid edge seams, and keep a deterministic pixel format, stride and DPR
mapping. Track scroll, geometry, message/selection/media paints, wallpaper
and theme revisions. A cache invalidated only by scrolling will freeze
animated media and typing/selection visuals. Do not add an unconditional
whole-window capture or periodic blur when nothing changed. Queued dirty-region
coalescing must avoid repaint recursion, and buffers must remain GUI-thread
owned unless a safe immutable handoff is explicitly designed.

`Ui::GL::Renderer`/`CreateSurface` in `lib_ui/ui/gl/gl_surface.h` provides
OpenGL/RHI surfaces and raster fallback; `gl_detection.h` checks backend
capabilities. Media/call renderers owning their scene are different from
sampling arbitrary sibling raster widgets. The API does not provide a
ready-made backdrop texture of QWidget siblings. Uploading captured strips
to GPU surfaces adds composition, lifetime and fallback work; GPU blur is not
automatically cheaper than a small CPU strip. Full-window RHI must not be
enabled as an incidental redesign change. Benchmark capture and blur
separately before choosing a backend.

## Scrolling findings

`Ui::ScrollArea` derives from `RpWidgetBase<QScrollArea>`. Its
`viewportEvent()` offers `setCustomWheelProcess()` before default handling;
`scrollToY()` writes the Qt scrollbar, and `scrolled()` emits position and
scroll producers. It also has separate touch scrolling and direction-lock
behavior. This gives an application-level opt-in helper a potential adoption
point without modifying the shared submodule. Preserve existing custom wheel
handlers; assigning the hook twice silently replaces the previous handler.

The observed smooth Profile behavior is in
`Info::SetupFlexibleRegularScroll()` and `FlexibleScrollHelper`, from
`info/info_flexible_scroll.*`. They use the animation clock, a 260 ms
`easeOutQuint` transition, accumulated wheel targets and cover-height snap
points. The regular path consumes classic single-step angle events and lets
other input proceed; it can abort when an external scroll changes the value.
The older helper has a scrollbar event filter and different handling of pixel
input. Neither helper is a general-purpose history scroller: profile-cover
snap points must not be transferred to history/settings.

`Ui::ElasticScroll` exists as a different scrolling implementation in
`lib_ui/ui/widgets/elastic_scroll.*`; its presence does not mean Profile uses
it. Replacing history with ElasticScroll would change substantially more
than wheel smoothing.

An opt-in helper for settings, dialogs, and the two history screens should
accumulate wheel-angle deltas, clamp targets and use the existing clock.
Pixel deltas/phased touchpad events should stop the wheel animation and
remain direct. Cancel on scrollbar dragging, keyboard navigation, range
changes and external/programmatic scrolling. Distinguish the helper's own
scrollbar write from synchronous history corrections so a stale animation
cannot overwrite a jump, history-load anchor or a move-to-bottom operation.
Every animation step must go through normal scroll notifications for read
metrics and loading. Direction reversal, end-of-range input, repeated wheel
events, disabled animations, and fractional wheel steps need coverage.

## Validation and outstanding access

Local onboarding validated a separate Qt 6.8/OpenSSL update-verification
component with 58 passing checks. This is a baseline unrelated to the new UI;
it does not validate this redesign or the Qt 6.11.2 Windows application.
Full Linux preparation was blocked by Rocky Linux egress and Docker `vfs`
storage capacity. Windows compilation and interactive geometry/performance
checks remain necessary.

Read-only Git access works. A dry-run push to the requested feature branch
also succeeded without creating a remote branch. GitHub API and Apple
developer access now work after the required domains were added to the
environment configuration draft. Apple HIG content and three WWDC25
transcripts were retrieved; see `DESIGN_NOTES.md`. The repository's default
branch is `dev`, and the Actions runs endpoint is readable. Repository secret
metadata and Actions-permission settings return `Resource not accessible by
integration`, so secret presence and Actions enablement must be established
by the workflow's early preflight and actual run. No credential values were
requested or printed. A documented preflight failure is a human prerequisite,
not a reason to substitute official test credentials.
