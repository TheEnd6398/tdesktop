# Aurora Motion Engine

Aurora keeps Telegram's native Qt Widgets interface and its animation manager.
The motion extension supplies shared timing, interruptible spring feedback,
interaction lighting and refresh-aware scheduling. It does not replace the
existing Aurora layout, message model or navigation controllers.

## Motion policy

Settings > Advanced > Performance contains **Reduce Aurora motion**. Reduced
motion removes spring feedback, button compression, message flight and spatial
page transitions; short opacity transitions remain. Expandable sections and
multiline composer height changes settle immediately. On Windows the client also
reads `SPI_GETCLIENTAREAANIMATION`, refreshing on `WM_SETTINGCHANGE` and application
activation. Other platforms have the explicit preference and Telegram's existing
animation/power-saving controls; additional native OS preference adapters are not
implemented.

Telegram's disabled-animation and power-saving settings take priority. The
existing separate smooth-scrolling and glass-blur flags remain independent.
Disabling the Aurora Glass interface returns the shared hooks to their original
timing and paint behavior.

The toolkit `ui/effects/motion_model.h` defines the common response times:

| Purpose | Response |
| --- | --- |
| Hover/focus feedback | 140 ms |
| Button compression/release | 150 ms |
| Selection and control state | 190 ms |
| Popups and toasts | 220 ms |
| Page/panel navigation | 280 ms |
| New message appearance | 180 ms |
| Badge feedback | 160 ms |
| Discrete-wheel interpolation | 220 ms |

Finite transitions use ease-out entrances, ease-in exits or ease-in-out state
changes. Interactive springs use the analytic critically damped solution and
retain position and velocity when retargeted. A spring's response is its visible
settling interval; its clock stops at a small remaining error, within 1.5 times
that response. Paint opacity and compression remain bounded even during rapid
reversals. Existing `Animations::Simple` transitions retain their position on
interruption; they do not acquire the spring's velocity-preserving behavior.

The existing animation manager selects a millisecond timer interval from the
focused native window's screen refresh rate, falling back to the primary screen.
This supports 60/120/144/240 Hz and higher rates within Qt's timer precision.
It is scheduling support, not a measured frame-rate guarantee or a vsync promise.
No animation timer is introduced for idle decoration.

## Active production paths

| Surface | Motion implementation |
| --- | --- |
| Main window and conversation navigation | Existing cached `Window::SlideAnimation` uses a short directional entrance and crossfade, including the captured header/composer. RTL reverses horizontal travel. Navigation state changes before the visual transition. |
| Vertical folders and account/menu controls | The live `FiltersMenu` moves a spring-driven selection indicator between folders. `SideBarButton` supplies bounded badge feedback and shared hover/press lighting. Shared menu/layer animations coordinate the hamburger menu and dimming. |
| Conversation rows | `BasicRow` lazily owns hover/selection springs. The inner list invalidates only the affected row; the material background and active indicator respond. Ordinary scrolling clears these row transitions and uses the existing row cache. |
| Search and archive | Shared field focus, popup/menu transitions, row materials and the existing archive/navigation paths. Filtered search result rows retain their existing selection geometry. |
| Regular conversation messages | Only `newItemAdded` events in the visible, bottom-anchored history get a short opacity/paint-offset entrance. The set is capped at twelve messages; no item height, ordering or scroll position is changed. Historical loading is excluded. Existing outgoing-flight messages are not painted twice. |
| List-based conversations and replies | The existing `ListWidget` item-reveal and anchor-restoration implementation uses the message profile. Its existing checks for new items and the newest edge are retained. |
| Outgoing messages | The native sending controller uses the message profile. Reduced motion bypasses message flight and clears pending visual-flight records. |
| Composer | Message fields opt into bounded height interpolation, retaining the live text editor and cursor. Width changes and new size limits settle synchronously. Send/record state morphing uses the state profile; send/attachment/emoji controls use press feedback. |
| Reply/edit/keyboard/expandable controls | Existing `SlideWrap`, `FadeAnimation` and send-state paths use the common profiles, preserving their visibility callbacks and layout ownership. |
| Menus, emoji/sticker panels and dialogs | Shared popup/panel renderers use common entrance/exit curves and cached rounded shadows. Menu selection fades with spring lighting. Emoji/sticker tab transitions use the panel profile. |
| Settings and profiles | Shared page transitions, toggles, tabs, expandable sections and settings/icon buttons receive the same profiles. Existing toggle callbacks, slider values and accessibility semantics remain unchanged. |
| Media and calls | Common icon buttons and sliders gain feedback; call buttons also compress and illuminate on press. Existing audio/video/recording indicators retain their semantic animations. |
| Tooltips and notifications | Important tooltips and in-app toasts use coordinated opacity timing. Reduced motion removes toast sliding. Native Windows notification layout remains upstream. |
| Reactions, typing, loading and animated emoji | Existing Telegram effects remain in use with their own power-saving flags. A new icon-morph family, replacement reaction choreography and replacement typing/loading artwork have not been created. |

The renderer moves a soft highlight and adjusts a restrained rim inside the
control's rounded clip. It restores painter state and uses the toolkit's scaled
line-width token for the rim. Button compression affects painting, not hit targets or layout.
The new effects do not add another blur pass or continuous full-window capture.
Spring drivers use the existing animation manager, stop when their owner is
hidden or destroyed, and do not retain a repeating decorative timer.

Classic mouse-wheel events retain a bounded target position. Repeated events
carry forward the current velocity into a monotone Hermite segment, with slope
limits that prevent overshoot. Pixel-precise input, touchpad phases and modified
wheel events bypass interpolation. External scroll changes, keyboard navigation,
scrollbar dragging, resizing, changed limits and hidden windows cancel it.

## Verification and remaining acceptance

The focused suite contains seven checks: the original blur, material, Aurora
material, wheel and chat-inset tests, plus motion physics and interaction painting.
Motion checks cover frame-rate-independent spring results, rapid reversal,
cancellation, 60/120/144/240/360 Hz timer selection, five device pixel ratios,
idle/hover/press differences, clipping and painter-state isolation. The existing
34 stock palette text-contrast checks remain required.

CI also configures the applied toolkit patch stack twice and checks that neither
the patched source files nor the real Git index changed. The build checks the
overlapping patches in reverse order using a private temporary index.

The optimized CI benchmark renders the actual Qt interaction-lighting component
and writes `motion-benchmark.json` with median and 95th-percentile raster times.
Its workload includes a small image allocation and paint setup. It measures a
component, not full application frame pacing, CPU/GPU use in a populated account
or sustained 120 FPS.

The Windows workflow compiles the complete application, starts an isolated
unauthenticated client, attempts real desktop/narrow screenshots and a 36-frame
welcome-button hover/press/release sequence, then verifies the portable package.
The captures artifact contains `motion-capture.json` or a clear pending-capture
status. Capture-pipeline latency is recorded separately and must not be presented
as application frame time. An observed image change is not a complete visual
acceptance result.

Authenticated chat/profile/settings interactions, multiline typing and IME,
rapid navigation, reaction/media/call interactions, high-DPI resizing, light/dark
and RTL visual review, reduced-motion runtime review and representative
application frame pacing still require manual testing with a real account.
Unit checks and successful compilation do not establish these results.

Run the delivered build through `Start Aurora Glass.cmd` in a new directory.
Use its separate profile for the acceptance pass. Do not copy official Telegram
account data into the package. The source retains the incomplete custom icon
family and bespoke Stories/Premium/group-call layout work documented in
[Aurora Glass coverage](AURORA_GLASS.md).
