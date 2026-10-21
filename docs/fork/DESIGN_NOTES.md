# Liquid Glass fork: design source study

Status: Phase 1 source study complete. Apple HIG materials and three WWDC25
session transcripts were retrieved alongside the Telegram iOS source study.
These notes distinguish inspected code, the user's visual specification and
engineering hypotheses; no redesigned application has been built yet.

## Visual specification supplied by the user

The target is the described macOS-style arrangement: separate floating
capsules for back navigation, avatar/title/status and actions; a pinned
capsule below with a gap; a floating composer and circular buttons; content
visible underneath glass. The screenshot description is the specification;
no actual screenshots have been attached. The closed-source macOS client is
not used as an implementation reference.

The chat list, folders bar and settings appearance are outside the capsule
redesign scope. Settings and the chat list are included only in the requested
smooth-scrolling behavior. The hamburger menu is separately in scope for
glass. Maintain opaque fallback, keyboard navigation, text contrast,
selection states, narrow-window navigation and theme compatibility.

## Apple primary sources

Sources read:

- [Human Interface Guidelines: Materials](https://developer.apple.com/design/human-interface-guidelines/materials)
- [WWDC25: Meet Liquid Glass](https://developer.apple.com/videos/play/wwdc2025/219/)
- [WWDC25: Get to know the new design system](https://developer.apple.com/videos/play/wwdc2025/356/)
- [WWDC25: Build a UIKit app with the new design](https://developer.apple.com/videos/play/wwdc2025/284/)

The HIG distinguishes a functional Liquid Glass layer for navigation and
controls from the content layer. It recommends sparse use on custom controls;
it does not recommend making message bubbles or all content glass. Its regular
variant adjusts blur and background luminosity for legible text. The clear
variant prioritizes media-rich backgrounds and may need a dark dimming layer
for bright content; the HIG gives 35% as a consideration for that case. That
value is not a universal toolbar opacity.

The Meet Liquid Glass transcript describes rounded floating forms and one
coherent functional plane. Adaptive material changes include tint, shadows
and dynamic range; simply making a background transparent does not reproduce
the material. Regular and clear have distinct use cases and should not be
casually mixed. Tint should emphasize selected primary actions, rather than
color every control. Reduced Transparency, Increased Contrast and Reduced
Motion adapt the system material, which informs our need for a cheap opaque
fallback and animation disable behavior.

Get to know the new design system explains fixed, capsule and concentric
shapes. Capsule geometry supports nested layouts, while container-relative
corners avoid pinched curves. It also says scroll edge effects support
legibility rather than decoration: use them where floating UI overlaps
content and avoid stacking multiple independent edge effects in one view.
The Qt design should therefore coordinate the header/pinned overlap region
instead of layering multiple full-width blur gradients blindly.

Build a UIKit app with the new design shows navigation/toolbars floating over
content and grouping related bar actions over shared glass backgrounds.
Scroll views extend underneath the bars and receive an edge effect for
contrast. Container-relative corners adapt as views move. Qt Widgets has no
automatic equivalent of these UIKit layout/material APIs, so this fork must
explicitly implement insets, capture, clipping, contrast and fallback.

Apple does not prescribe the numeric desktop dimensions or CPU blur radii
below. The iOS measurements are source observations, and proposed CPU
parameters remain subject to Windows performance and visual validation.

## Telegram iOS sources actually inspected

The public repository was fetched read-only, without its dependency
submodules, at revision `f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838`.
The following links pin the inspected code rather than following a moving
branch. Paths starting with `submodules/` here are directories in the iOS
repository and do not imply changes to tdesktop submodules.

- [GlassBackgroundComponent](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/GlassBackgroundComponent/Sources/GlassBackgroundComponent.swift)
  defines `GlassBackgroundView`, theme-aware tint kinds, native visual-effect
  surfaces, rounded shapes, highlights and legacy rendering paths. The native
  path uses `UIGlassEffect` and `UIVisualEffectView`; its companion container
  uses `UIGlassContainerEffect`. A native blur radius is not exposed as a
  portable parameter by that path.
- [LegacyGlassView](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/GlassBackgroundComponent/Sources/LegacyGlassView.swift)
  constructs backdrop-layer/filter behavior through runtime interfaces. It
  uses blur radii 2 or 6 in its clear variants, a radius 2 normal path and a
  color matrix. This is Apple-specific implementation evidence, not an API
  to port or a Qt radius calibration.
- [GlassControlGroup](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/GlassControls/Sources/GlassControlGroup.swift)
  groups existing controls over one background. Its background radius is
  `size.height * 0.5`, and item minimum width generally follows available
  height. [GlassControlPanel](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/GlassControls/Sources/GlassControlPanel.swift)
  separately arranges left, center and right groups. This supports separated
  capsules while preserving each control's own behavior.
- [NavigationBarImpl](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/NavigationBarImpl/Sources/NavigationBarImpl.swift)
  uses 44-point action backgrounds and a 16-point side inset in the inspected
  layout, with a radius of half the action-background height. These values
  describe an iOS configuration and should be scaled/tuned through tdesktop
  styles, not copied as universal desktop metrics.
- [ChatTitleView](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/ChatTitleView/Sources/ChatTitleView.swift)
  and the adjacent `ChatTitleComponent.swift` retain dedicated chat-title
  state; capsule composition does not eliminate title/status semantics.
- [ChatTextInputPanelNode](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/Chat/ChatTextInputPanelNode/Sources/ChatTextInputPanelNode.swift)
  owns a `GlassBackgroundContainerView`, input `GlassBackgroundView`, menu
  background and separate attachment background. The input corner radius is
  based on the minimum input height; the attachment background uses a
  40-point diameter and half-height radius. The start button uses a 50-point
  height and radius 25. These distinct shapes matter when the input grows:
  not every tall reply/input container is simply rounded by half its total
  height. `ChatTextInputPanelComponent.swift` forwards layout and inset state
  to the panel rather than assuming fixed keyboard/footer dimensions.
- [LiquidGlassShapes shader](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/TelegramUI/Components/LiquidGlassShapes/Metal/LiquidGlassShapes.metal)
  contains Metal-specific layer, saturation, rim and refraction parameters.
  It is a custom GPU shape pipeline, not proof that navigation-bar glass is
  implemented by CPU image blur or that QWidget backdrops can be sampled the
  same way.
- [FastBlur](https://github.com/TelegramMessenger/Telegram-iOS/blob/f1dd7a2dbd02cbbf513e75d5695d8d36d1cf5838/submodules/FastBlur/Sources/FastBlur.m)
  includes a radius-three separable CPU blur and temporary allocation.
  Its existence is useful algorithm evidence, but does not establish that
  the current native glass bars call it for their real-time backdrop.

### Inspected tint and highlight treatment

In the native `GlassBackgroundView` panel path, the regular effect receives
a white tint with alpha 0.025 in dark mode and 0.1 in light mode. Clear mode
uses a clear effect, with a black alpha-0.28 tint in dark mode and no tint in
light mode. Those are tints applied to an operating-system material, not
the resulting surface opacity. A Qt CPU renderer would need its own measured
tint/contrast calibration.

Legacy paths construct shadows, overlay fills and fine bright edges. The
inspected shadow helper exposes an intensity of 0.04 and blur 40; edge
treatment elsewhere uses sub-point offsets and several blend modes. This
is evidence for restrained layered highlights, not a reason to reproduce a
large shadow or many offscreen passes for every desktop frame. TouchEffect
also contains pointer/touch highlights and deformations specific to that UI.

## CPU rendering candidates and costs

The existing tdesktop stack blur is a better starting point than introducing
a new heavy dependency. A candidate pipeline is: capture the actual content
strip with kernel support beyond the surface edge, reduce resolution, blur,
apply restrained theme-derived tint/saturation, then scale and clip to the
capsule. The capture must include message content and wallpaper in one
coordinate system; blurring wallpaper alone does not satisfy the spec.

At one-quarter linear resolution, the blurred pixel count is one-sixteenth
of the full-resolution strip. One-eighth reduces it to one-sixty-fourth but
can lose text silhouettes or produce unstable edges; compare visually at
multiple DPRs. These are engineering estimates, not benchmark results.
Rendering a full-size strip and only then downscaling may leave capture as
the dominant cost. Measure reduced-resolution painting separately from the
filter and final compositing.

| Candidate | Useful property | Constraint in this renderer |
| --- | --- | --- |
| Separable box passes | Linear cost and reusable rolling sums | Several passes approximate a blur; account for pass support and borders |
| Existing stack blur | Already available, with SIMD backends | Current large-radius helper allocates scratch buffers per call |
| Downsample plus fixed-radius blur | Small working set and predictable cost | Effective radius, sample origin and DPR must be stable |
| Dual Kawase | Efficient GPU ping-pong at several resolutions | Needs owned textures/framebuffers and a real scene source; CPU resampling can erase its advantage |
| QWidget image capture plus GPU blur | Can reuse some GL infrastructure | Adds texture upload and compositor/fallback costs; no free sibling-widget backdrop texture |

Reuse image storage when sizes/formats are unchanged. An rvalue image API
does not guarantee allocation-free filtering: inspect internal scratch
storage too. Grow/reallocate buffers on size or DPR changes rather than on
every frame. Treat copy-on-write detaches as allocations when profiling.

Coalesce dirty regions and process only intersections with visible glass
strips. Changes include wheel/programmatic scroll, history load, new or
removed messages, selection, playing media, theme/wallpaper changes,
resizing and scaling. Stop work entirely while the glass flag is disabled
or the surface is hidden. Include source revision, transform, surface size,
DPR and palette revision in cache validity. Use a reentrancy guard and a
content-only paint path: rendering a parent that includes the glass again
can recursively capture stale glass or loop through paint invalidation.

Noise must be subtle and cached, and saturation must preserve premultiplied
alpha and legibility. The requested inner highlight should use a device-aware
hairline with clipping that does not enlarge hit regions. Existing palette
colors provide sensible custom-theme fallback; sample contrast over both
bright wallpaper and detailed media before choosing opacity.

No blur backend or new dimensions have been adopted in product code yet.
The eventual plan must specify performance budgets and measure capture,
blur and composition independently on Windows. A static screenshot cannot
prove scrolling latency or invalidation correctness.

## Required design and behavior validation

Complete and commit `PLAN.md` before implementation. It must cover all chat
implementations, shared inset math, cache invalidation, input routing, runtime
flags and Windows CI.

Validation must include light/dark/custom themes; 100%, fractional and high
DPI; narrow/normal/three-column layout; title typing/selection/search modes;
pinned, call and other stacked bars; multiline/reply/edit/forward/recording
and bot keyboard states; opaque Power Saving fallback including battery
activation; and both classic wheel and precision touchpad input. Geometry
tests must preserve a message anchor, first/last message accessibility,
unread/read metrics, loading, selection auto-scroll and chat-position restore.
Use Windows build/run evidence for UI claims. No feature test has executed
and no redesign artifact exists at this research checkpoint.
