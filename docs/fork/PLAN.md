# Liquid Glass fork implementation plan

This is the original design plan. The delivered scope and verification status
are tracked in [IMPLEMENTATION.md](IMPLEMENTATION.md). In particular, the
original reserved-viewport delivery is superseded by Iteration 2 below.

Phase 1 is committed in `RESEARCH.md` and `DESIGN_NOTES.md`. This plan must be
committed before production or workflow implementation. Work on
`feature/liquid-glass-ui`, with small commits and a PR targeting `dev`.

## Sequence and acceptance gates

The complete-redesign request supersedes the earlier dependency-first order.
The Windows baseline has passed; subsequent artifacts must include visible UI
changes. The fork option now defaults ON and the Windows workflow explicitly
enables it.

1. Implement the active chat header, both composers, pinned bars and corner
   controls with shared materials and translated effect settings. Keep the
   current reading viewport and its existing anchoring contract while exposing
   wallpaper between the floating controls.
2. Validate bounded capture, blur, light/dark materials, DPI and fallback
   behavior locally. Compile and package the changed Windows application.
   Runtime screenshots and interactions require a Windows GUI with an account;
   component raster checks do not establish that evidence.
3. Extend the material to chat lists, navigation, profiles, settings and menus
   after the first chat milestone. Add subtle motion and wheel smoothing using
   the actual ScrollArea and ElasticScroll contracts.
4. Stabilize theme changes, live toggles, narrow layouts and account switches.
   Update the existing PR and report each implemented, compiled and visually
   verified feature separately.

The user explicitly requests a Release distribution build in GitHub Actions.
Use that there, while local development checks remain Debug; do not run a
full local Release build in this constrained Linux environment.

## Files and ownership

Paths below are relative to the repository root. New product helpers live
inside the tdesktop checkout. No submodule edit or repin is planned.

| Area | Files/classes | Intended responsibility |
| --- | --- | --- |
| Build isolation | `Telegram/cmake/telegram_options.cmake`, `Telegram/CMakeLists.txt` | Default-on fork build option; explicit registration and compile gating of new sources |
| Windows artifact | `.github/workflows/fork-win-x64.yml`, `Telegram/build/fork/` helper scripts, `docs/fork/BUILDING.md` | Secrets preflight, staged dependency cache, Release build and side-by-side portable package |
| Runtime switches | New `Telegram/SourceFiles/ui/fork_effects.h/.cpp`, `ui/power_saving.h`, `settings/settings_power_saving.cpp`, `settings/sections/settings_advanced.cpp`, `Telegram/Resources/langs/lang.strings` | Query effects, persist layout preference, expose localized settings and live changes |
| Style constants | New `Telegram/SourceFiles/ui/chat/fork_glass.style`, `Telegram/cmake/td_ui.cmake` | Scaled spacing, capture scale, surface treatment and action sizes through style code generation |
| Wheel smoothing | New `Telegram/SourceFiles/ui/widgets/smooth_wheel_scroll.h/.cpp` | Optional adapter over `Ui::ScrollArea`, not a replacement scroll implementation |
| Adoption sites | `settings/settings_common.cpp` and section scroll creation sites; `dialogs/dialogs_widget.cpp`; `history/history_widget.cpp`; `history/view/history_view_chat_section.cpp`, `history_view_scheduled_section.cpp`, `history_view_pinned_section.cpp` | Install adapter only for requested areas, respecting existing wheel handlers |
| Inset policy | New `Telegram/SourceFiles/ui/chat/chat_content_insets.h/.cpp`; `history/history_widget.*`, `history/history_inner_widget.*`, `history/view/history_view_list_widget.*` | Shared transforms and exposed reading area, implemented in both list/layout contracts |
| Section layout | `history/view/history_view_chat_section.*`, `history_view_scheduled_section.*`, `history_view_pinned_section.*`, `history_view_corner_buttons.*`, subsection tab integration | Full viewport underlap, dynamic footer/header reserves, overlay and autocomplete anchors |
| Glass rendering | New `Telegram/SourceFiles/ui/chat/glass_surface.h/.cpp` | Content-only strip source, retained reduced buffers, blur, tint, clipping and cache invalidation |
| Header | `history/view/history_view_top_bar_widget.*`, application styles | Back/title/actions groups, search and selection modes, matching visible and interactive shapes |
| Pinned bar | `ui/chat/pinned_bar.*`, `ui/chat/message_bar.*` if its child background requires it, `history/view/history_view_pinned_bar.*` | Floating geometry and reusable surface; preserve tracker and pinned navigation behavior |
| Composer | `history/history_widget.*`, `history/view/controls/history_view_compose_controls.*`, existing field/button styles where needed | Both composer implementations, previews, dynamic heights and circular actions |
| Menu | `window/window_main_menu.*`, `mainwindow.cpp` only for backdrop-source wiring | Content snapshot below the layer stack, opaque fallback and existing menu behavior |
| Verification | `Telegram/SourceFiles/tests/`, relevant CMake test registration; existing Debug harness for interactive checks | Geometry/state tests plus Windows UI and performance evidence |

Recheck the source atlas and exact adoption constructors immediately before
each change. The table identifies candidate integration sites, not permission
to refactor all of them. If a submodule API is insufficient, stop and describe
the missing API first. Any unavoidable submodule fix should be a small,
reviewable `fork-patches/*.patch` applied consistently in CI and documented
locally; do not silently modify a submodule checkout or require an undisclosed
manual fork.

## Build and runtime gating

Use the default-on CMake option, `TDESKTOP_FORK_LIQUID_GLASS`. The new workflow
enables it. Builds may explicitly disable the option to retain their original interface. Product
helpers may be platform-generic; Windows-only defaults/integration use the
repository's established platform guards. Do not equate the project's CMake
`LINUX` with only the Linux kernel platform.

Add disable bits `PowerSaving::kGlassBlur` at the next unused bit and
`kSmoothScrolling` after it, extending `kAll` without renumbering earlier
bits. New unset bits mean both effects default enabled in the fork. Add
localized rows through `PowerSavingLabels()`; glass consumers use `OnValue()`
so battery `SetForceAll()` is honored. Smooth scrolling also stops when
`kAnimations` becomes active. Do not insert fields in sequential settings
serialization: the existing tagged Power Saving integer already holds the
new bits. Check `kAll`, inverted checkbox masks and forced mode together.

Keep capsule geometry separately controllable through a boolean application
KV preference, `fork/capsule-layout`, read/written through
`Core::Settings::readPref<bool>` and `writePref<bool>`, with a localized
Advanced setting. Notify live owners through the fork-effects helper and
save with `saveSettingsDelayed()`. The fork build defaults to capsule layout;
other builds remain unchanged. Layout-off restores the original bar geometry
and reading range. Blur-off may keep enabled capsule geometry, but each
surface becomes fully opaque with readable palette colors and no capture
work. Disabling capsule layout must also restore matching content reserves.

Every new visible label/help text uses the lang system. Do not add an
untranslated experimental option name merely because existing options use
hard-coded English. Keep opaque fallbacks valid in custom palettes.

## Insets and scroll invariants

Use explicit top and bottom reserves rather than moving the scroll widget
without modifying its content contract. The physical viewport spans the
chat region. Define the unobscured reading range independently from painted
underlap. Top reserve includes current header, pinned and other stacked bars,
their gaps and applicable subsection tab offsets. Bottom reserve follows
the actual current compose/restriction/recording/keyboard geometry, not a
fixed input-field height.

For a reserve change, record a message anchor and offset, current scroll
maximum and whether the user is at bottom. Recompute content height with
reserves, then preserve bottom state or restore that message anchor. Treat
short histories, first-load positioning, migrated messages, about/bot views,
collapse gaps and animated removal explicitly. Do not save transient
underlap offsets as a different chat's persisted position.

Carry the unoccluded range into read metrics, selection clamping/auto-scroll,
preload decisions where appropriate, date/corner overlay placement, floating
player geometry, drag/drop and autocomplete bounds. Paint and hit-test
transforms must agree. Programmatic jump and highlight targets must place
their content in the reading area, not behind the glass. Header/pinned/compose
geometry and content reserves update in one layout transaction to prevent a
one-frame anchor jump. Fallback and runtime toggling must use the same
transaction.

## Glass source and cache

Start with the CPU path: the UI is raster-painted and existing `Ui::GL`
surfaces do not supply a sibling-widget backdrop. Avoid an incidental
whole-window RHI migration or a GL widget per capsule. Full GPU refraction
is outside this iteration; the design goal is responsive, legible glass.

The source callback paints only wallpaper plus list content beneath a surface
into a retained reduced-resolution image. Do not call recursive parent
`render()` from a paint handler or sample the glass itself. Include blur
support beyond clip bounds; map logical coordinates, DPR and sample origin
deterministically. Start with a quarter-resolution candidate and tune against
Windows measurements rather than asserting it is fast by inspection.

Use retained input/output images and retained blur scratch buffers. The
existing stack-blur implementation is algorithm guidance, but its allocating
large-image API is not the final per-frame contract. A small application-owned
separable filter can avoid submodule changes and should be checked against
reference image cases, alpha handling and row stride. Coalesce dirty source
regions, invalidate only intersecting strips, and avoid an always-running
capture timer. Theme, DPR, resize, scroll, history load, new/removed content,
selection and animated media paints must participate. Cap each surface to at
most one refresh per presentation opportunity; hidden/disabled surfaces do
none. A stale callback must not retain destroyed chat/list owners.

Tint from existing palette colors with one uniform alpha. No gradients,
strokes, highlights, noise, extra shadow or optical effects are used. Use one
coordinated top-edge treatment for stacked floating bars. Avoid glass-on-glass
captures. The main menu samples below its layer/dimming surfaces; menu child
backgrounds must not repaint full opaque rectangles over the result.

## Capsule interaction and dynamic states

Keep original action controls and callbacks. Group back, title/avatar/status
and actions with outer gaps; use half-height radii for single-line capsules
and circular buttons. Multiline composer geometry uses a radius bounded by
its minimum control height. Keep previews/recording/keyboard surfaces aligned
with the same footer reserve. Use style values, not raw layout pixels.

Clip ripples and hover/pressed painting to each actual shape, and match
hit-testing to that shape. Preserve tab order, accessible names, title clicks,
typing indicators, topic/thread navigation, selection toolbar and in-header
search. In narrow layouts, prioritize back/navigation and current controls
without allowing decorative margins to hide actions. Custom themes and RTL
need their normal coordinate helpers.

## Smooth wheel adapter

Reuse the animation-clock pattern from `Info::SetupFlexibleRegularScroll`,
without profile-cover snap points. An adapter state accumulates an angle-wheel
target, retains fractional deltas, clamps against the live range and eases
from the current position. Pixel deltas and phased touchpad gestures cancel
the animation and pass through unchanged. Horizontal/modifier input and
nested lists retain their existing routing.

Use `scrollToY()` for intermediate positions so normal visible-range/loading
notifications still fire. Track own writes separately from external changes;
cancel on keyboard navigation, scrollbar press/drag, range or layout changes,
chat switches, animation disable and programmatic jumps. Synchronous history
corrections must win over a pending wheel target. Reversal reanchors to the
current visible position; do not overshoot and bounce at the ends. Avoid
replacing an existing custom hook silently; add explicit composition at any
adoption site that already owns one. Leave Info's specialized helper intact.

## Windows pipeline and coexistence

The workflow starts with repository-secret presence checks and clear errors
before any dependency downloads. Do not write secret values into output,
artifacts, reusable caches or committed files. Build-only CMake cache paths
containing API values must be excluded from artifacts and dependency caches.

Use a GitHub-hosted Windows x64 runner with the documented SDK/toolset and
Qt 6.11.2 patched preparation. Keep `BuildPath/tdesktop`, `Libraries/win64`
and `ThirdParty` aligned. Dependency preparation needs resumable stages or
separate jobs so cold builds can complete within six hours; retain the
repository's stage keys and split Qt from other large dependencies. Pin cache
identity to recipe/toolchain inputs. Never call `skip-release` for a Release
toolchain accidentally. Add a compiler cache only with verified MSVC/PCH
compatibility; it is secondary to restoring complete dependency outputs.

Configure x64 Release with `TDESKTOP_API_TEST=OFF`, both API secrets,
`DESKTOP_APP_DISABLE_AUTOUPDATE=ON` and the fork option. Preserve the source's
license notices. Package the executable and required runtime files, verify
the package's imports, and upload an artifact containing a portable zip.
Provide a launcher using an isolated local workdir; do not register official
protocols or overwrite the official executable/data. Document unsigned
distribution and any WebView2 runtime requirement discovered by actual
packaging. A native installer is optional only if existing packaging supports
safe fork identity; do not fabricate signing credentials.

## Validation evidence and stop conditions

Run focused deterministic checks for inset transforms, anchoring, changing
footer heights, old/new Power Saving masks, blur pixel output/DPR handling,
and wheel accumulation/cancellation. Use existing tests where applicable;
new tests must exercise observable policy rather than merely duplicate code.

On Windows verify both chat implementations, scheduled/pinned views and forum
tabs; narrow/normal/three-column layouts; light/dark/custom palettes; 100%,
fractional and high DPI; selection/search/unread/jump/restore; top/bottom
loading and stickiness; drag/drop and autoscroll; multiline, previews,
recording, restrictions and bot keyboard; battery saving and live toggles;
mouse wheel/touchpad/keyboard/scrollbar/programmatic scrolls. Confirm first
and last messages remain accessible at rest and underlap never changes read
state incorrectly.

Profile steady idle, sustained scroll with text and animated media, resize
and theme changes. Record strip dimensions, DPR, capture/filter/composite
times, allocation counts and frame cadence on a named Windows configuration.
Target no refreshes while unchanged and no steady-state image/scratch
allocations. Compare frame costs to opaque mode; do not present an unmeasured
frame-time budget as achieved.

Every push must be followed by inspection of its Actions result and logs.
Missing secrets, denied workflow updates, disabled Actions, unavailable SDK,
cache/runner limits or an unavoidable dependency-fork requirement are human
prerequisites: stop, preserve commits and report the exact repair. Never
claim a green build or downloadable artifact without an actual successful
run, and never advance risky UI work while its compiler path is blocked.

## Iteration 2

The requested material is a floating capsule with live backdrop blur and a
single flat palette tint. `glassTintOpacity` controls its alpha; Power Saving
blur-off keeps the same capsules with opaque fills. Extra lighting, strokes,
shadows and composer focus glow are removed. Native ripples and flat button
hover feedback remain.

Real message viewports now extend beneath the header/pinned and composer
overlays with explicit reading reserves. Masks cover interactive capsules,
header actions are grouped, and bounded expanded blur strips refresh from live
content. The existing vertical wheel easing remains unchanged with wider
installation coverage. Menu shade covers the complete client layer window.

Header actions share one pill, sized from the currently visible buttons;
selection and in-header search retain one unified toolbar capsule. Composer
attachment and emoji controls share the input surface, with the native
send/record control separate. This follows the input grouping in Telegram
macOS (`Telegram-Mac/ChatInputView.swift` in `overtake/TelegramSwift`), while
retaining this client's native event handlers and optional bot controls.

The message list is owned by a transparent content wrapper with top/bottom
reserves equal to the overlaid bars. The physical scroll widget expands by
those same reserves, preserving the original scroll maximum and list-local
message coordinates. Reading height, read tracking, jumps, autocomplete,
corner buttons and pull controls use the unobscured rectangle. Classic
composer painting moves onto a masked sibling overlay above the list; header,
pinned and intermediary containers are masked to their actual controls so
empty space reaches the list. Backdrop composition paints the wallpaper and
then the bounded live list region, excluding sibling glass widgets.

Live blur refresh runs on the window's UpdateRequest before backing-store
painting, reading current list/animation state rather than a previous-frame
snapshot. Each observer updates its bounded strip only when its output hash
changes. No timer runs at idle. Capture renders only the list subtree over
wallpaper, with the existing capture guard suppressing read/view side effects.
The reduced sample grid is aligned in window coordinates at each DPR. Three
box passes use retained premultiplied buffers; their full support plus two
filter pixels is captured beyond the capsule, then cropped only at composition.
CPU raster strips avoid a separate GL context/widget per capsule and preserve
the current Qt Widgets architecture. Performance measurements below distinguish
synthetic strip costs from real authenticated-chat frame costs.

The existing layer-stack fade/slide clock is retained. A small toolkit switch
makes its shade fill the entire window, including the floating menu margins.
Menu backdrop capture includes the final shade color before blur; the cached
slide uses that same material so handing over to the live panel does not
change its tint. Click-to-close stays on the existing layer stack. The toolkit
change is versioned as `fork-patches/0001-full-window-menu-shade.patch` and
applied idempotently before `lib_ui` is configured; it is not a gitlink repin.

Smooth scrolling now has a default toolkit integration hook after each local
wheel policy. A local handler that consumes an event keeps priority. The same
220 ms model and native scroll APIs are reused lazily for all remaining
ScrollArea and ElasticScroll instances, including native horizontal wheels;
pixel/phase/modifier input
continues untouched. Horizontal emoji/filter/topic/search tab handlers compose
the same model explicitly after their native axis checks and stop their own
programmatic tab animation first. Power Saving's animation flag also cancels
wheel motion. The central hook is versioned in
`fork-patches/0002-default-scroll-wheel-hook.patch`.

Inset regression checks instantiate the production Qt content wrapper and
exercise unchanged scroll range, message anchors, bottom padding, multiline
footer growth, layout-off coordinates and short histories. The classic
layout transaction suppresses intermediate scroll/read updates until its
saved position has been restored. Share-selector anchors map through the list
itself rather than assuming its old direct scroll parent. Grouped header
width animates for 140 ms using the existing animation clock; title clipping
and hit regions follow the same width, with instant reduced-motion fallback.

Before a classic-history capture, pending message layout is flushed using the
same updateListSize path as native painting. This prevents sampling the
wallpaper-only early return of a list with pending resize work. Blur accesses
retained image storage once per pass, instead of asking QImage for a scanline
on every pixel. The focused CI job also runs an optimized synthetic strip
benchmark with `test_glass_material --benchmark`; it measures bounded QWidget
capture, hashing and filtering, not Telegram message rendering or Windows FPS.
Native horizontal mouse wheels reuse the same easing, while both axes cancel
immediately on pixel/phased/modified input. The original vertical curve,
duration and distance calculation stay unchanged.

The sampler includes non-glass scroll overlays (including pull controls and
the native scrollbar) in stacking order, not just the list child. The glass
corner-button subtree is excluded entirely to prevent self capture. Wallpaper
is mapped in section coordinates, so a small corner button samples the same
wallpaper scale and position as the full chat. Both classic chat-removal paths
destroy the owned wrapper as QWidget and clear the old composer surface.

The modern chat and pinned sections now paint wallpaper across the header
region too, rather than relying on the removed opaque toolbar. In-chat
horizontal topic/subsection tabs share one masked floating capsule with live
sampling and no separator line; their native controls and vertical sidebar
layout are preserved. Their top/bottom reserves follow the actual pill height.
The modern suggested-post options strip also paints on a masked sibling
overlay, with its own live surface and reading reserve, instead of leaving
its informational text under the expanded message viewport.

Local optimized material-only measurements (Qt 6.8.2, Linux, shared 5-CPU
Xeon Platinum 8573C, three trials of 80 changing frames) put the combined
960 px header/composer/pinned preparation at 1.39–1.50 ms at DPR 1 and
5.53–5.81 ms at DPR 2. A 320×760 menu costs 1.03–1.07 / 4.17–4.42 ms
respectively. This includes synthetic QWidget capture, hashing and the CPU
filter, excluding Telegram message/media painting and final composition.
The DPR-2 chat strips consume about 35% of a 16.67 ms frame before that
additional work; this is a budget estimate, not measured Windows FPS.
Debug costs were about 21.4 ms for those three strips at DPR 2. CI publishes
its own optimized benchmark and CPU JSON; real animated-chat costs and lower
power hardware still require Windows profiling. Opaque Power Saving fallback
avoids captures. The bounded CPU path is retained rather than adding a GL
context per capsule or migrating the widget renderer.
