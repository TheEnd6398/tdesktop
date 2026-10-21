# Liquid Glass implementation and Windows acceptance

The fork changes active Telegram widgets, with the design enabled by default.
The review and build results are tracked in [PR #1](https://github.com/TheEnd6398/tdesktop/pull/1).
Use a successful [Fork Windows x64 run](https://github.com/TheEnd6398/tdesktop/actions/workflows/fork-win-x64.yml)
whose recorded source commit includes the changes under review. The earlier
baseline artifact has no redesigned UI.

## Iteration 2 interface

The material is a plain backdrop blur plus one uniform palette tint, controlled
by `glassTintOpacity` (0.68). It has no added gradient, highlight, glow, stroke,
noise, saturation adjustment, refraction or shadow. Existing native ripples
remain. Blur-off uses opaque capsule fills; layout-off restores native bars.

| Correction | Active source integration | Behavior |
| --- | --- | --- |
| Flat material | `ui/chat/glass_material.*`, `fork_glass.style`, profile/date callers | One flat tint; native hover feedback remains without shine or focus borders |
| Chat underlap | `history_widget.*`, `history_inner_widget.cpp`, chat/scheduled/pinned sections, `ui/chat/chat_content_insets.*` | The physical viewport extends beneath floating controls; wrapper reserves preserve message coordinates and the reading range. Masks pass empty-space input to the list |
| Header/composer groups | `history_view_top_bar_widget.*`, `history_view_compose_controls.*`, `history_widget.cpp` | One right-hand action pill, animated width and matching title clipping; attachment/emoji share the input surface, with native send/record separate |
| Live sampling | `ui/chat/glass_surface.*`, `glass_material.*`, `glass_blur.cpp`, `pinned_bar.*`, corner controls | Capture before backing-store painting; compose wallpaper, messages, dates and non-glass scroll overlays. Exclude glass corner controls and capture side effects |
| Main menu | `window_main_menu.*`, `mainwindow.cpp`, `fork-patches/0001-full-window-menu-shade.patch` | Full client-area shade behind the rounded panel, including its margins. Existing layer fade/slide and outside-click handling remain |
| Smooth coverage | `ui/widgets/smooth_wheel_*`, `core/ui_integration.*`, tab handlers, `fork-patches/0002-default-scroll-wheel-hook.patch` | Default hook for every toolkit scroll area after its local wheel policy; same 220 ms easing, native scroll APIs and cancellation rules |

The message wrapper adds the same top/bottom reserves to content height that
are added to physical viewport height. This preserves the scroll maximum and
list-local jump/restore coordinates. Read tracking, preload, selection clamps,
autocomplete, corner controls, floating players and pull controls use the
unobscured reading rectangle. Classic composer painting is a masked sibling
above the list. Reply/edit/forward/link previews retain their separate capsule.
Modern chat and pinned-section wallpaper also covers the header region.
Horizontal topic/subsection tabs inside conversations share one masked live
capsule; the vertical topic sidebar keeps its native layout. Suggested-post
options use a masked sibling overlay with a matching bottom reading reserve.

Each visible blur strip refreshes at the window UpdateRequest before painting.
Classic pending message layout is flushed first. Sampling expands beyond the
capsule by all three filter passes plus interpolation support, aligns to the
reduced pixel grid at the current DPR, and clips only the final composite.
Source, output and scratch buffers are retained. Identical pixels skip the
filter; identical output skips another overlay repaint. Hidden/minimized or
reduced-effects views avoid captures. There is no idle capture timer.

The sampler deliberately renders the content subtree with capture guards;
it does not grab the full window each frame. The menu samples below its layer
stack and includes the final shade in its cached slide material. The existing
short open/close animation uses that cached panel; the fully open panel uses
live captures. This retains the native animation and avoids a material change
at its handover. Interactive flicker/edge acceptance still requires Windows.

Toolkit changes are tracked as two patches against the pinned `lib_ui` source,
applied idempotently by `Telegram/cmake/fork_patches.cmake` before configuration.
No gitlink repin or manual unrecorded submodule edit is required. Patch
application was checked on clean LF and CRLF working trees and by configuring
twice. No new application runtime dependency is introduced.

## Smooth-scrolling coverage

The default toolkit hook covers the chat list and archive/folder/forum variants,
global and in-chat results, contacts and members, emoji/sticker/GIF content,
profile/shared-media lists, popup/box/forward/share lists, long menus, settings,
right-side information and conversation/replies/thread views wherever they use
`Ui::ScrollArea` or `Ui::ElasticScroll`. Existing local wheel handlers keep
priority. Emoji/filter/topic/search tab handlers explicitly compose horizontal
motion; native horizontal detents also use the same model.

No standard toolkit list is deliberately excluded. Specialized flexible-info
scrolling retains its existing smooth policy rather than stacking an animation.
Ctrl-wheel media zoom, precision pixel/phased touchpad input, modifier gestures,
keyboard/scrollbar control and programmatic jumps keep their native paths.
WebView/mini-app documents, the media viewer's zoom gestures, native OS lists
and text editors do not use this toolkit list mechanism and remain native.

## Verification boundaries

Four focused Qt executables directly compile production blur/material,
wheel-model and content-wrapper sources. Their checks cover reference pixel
values, thin/padded rows, alpha and buffer reuse, sibling coordinates, bounded
capture and child-layer exclusion, capture-scope restoration, uniform light/dark
tints, opaque fallback, five DPR values, both mouse-wheel axes, gesture exclusion,
easing/fractions/reversal/bounds/cancellation, scroll anchors/range, composer
height changes, short histories, toggles and wrapped-content destruction.
Debug and ASan/UBSan checks have passed during this iteration. Check the current
workflow for the exact delivered source's validation and native build result.

The focused CI job also builds the material/blur checks in optimized Release
and runs `test_glass_material --benchmark`. Its JSON and CPU description are
uploaded as `LiquidGlass-strip-benchmark-<commit>`. It measures synthetic bounded
QWidget capture, hashes and filtering; it excludes real Telegram message/media
painting and final composition. These are preparation costs, not Windows FPS.
Local optimized synthetic trials on the shared Xeon 8573C measured the three
960 px chat strips together at 1.39–1.50 ms (DPR 1) and 5.53–5.81 ms (DPR 2),
before real message/media painting. See PLAN.md for the measurement limits.

The Windows workflow compiles the full client with MSVC and the glass option ON,
then starts it in a temporary isolated profile and requires a native window and
ten seconds of responsiveness. Packaging verifies ZIP CRC, AMD64 GUI images and
DLLs, fork resources, isolated launcher, excluded files and the source/executable
hash in `BUILD-INFO.json`. The portable ZIP and `verification.json` are uploaded.
Use the successful run and commit being reviewed; earlier artifacts cannot
validate the new code.

This workspace has no authenticated interactive Windows chat. No real-chat
screenshots, visual/theme/narrow/RTL acceptance or measured Windows scrolling
FPS are claimed. The acceptance matrix below is still required. The business
welcome-message editor and other specialized compose contexts retain their
reserved viewports and palette fallbacks; ordinary chats, replies, scheduled
and pinned views use the new underlap. Date/recording and most settings/profile
controls use palette fallbacks. Multiline input retains native height changes.
No desktop/OS blur or optical simulation is implemented.

## Windows acceptance matrix

Extract the new artifact into a separate writable directory and start it using
`Start Liquid Glass.cmd`. Direct EXE startup also chooses `profile` beside the
executable, with `%APPDATA%/Liquid Glass/` as its write-failure fallback. Keep
the official installation and data untouched. Use a test account or log in
manually; API application credentials are not account login credentials.

1. Open a conversation at Windows scaling 100%, 150% and 200%. Save real
   screenshots in light, dark and one custom theme. Confirm separate back/title/action-group capsules, live message content
   behind and between them, floating input and pinned bar;
   Compare normal width and the narrowest supported window.
2. Exercise avatar/profile opening, back, call, menu, Ctrl+F, search cancellation,
   message selection, forwarding, scheduled messages, pinned-message navigation
   and forum/replies views. Check both left-to-right and right-to-left layouts.
3. Type one and several lines, paste text, reply, edit, forward, attach files,
   use emoji and bot keyboards, and start/cancel voice recording. Confirm text,
   previews and buttons remain readable and clickable without overlaps.
4. Scroll a long history and chat list with an ordinary wheel, then a precision
   touchpad. Drag the scrollbar, use keyboard navigation, jump to a search
   result and return to bottom during wheel motion. Check short histories,
   incoming messages at bottom, pagination and resize anchoring.
5. Toggle Liquid Glass while a chat/menu/settings page is visible. Toggle blur,
   smooth scrolling and global animations separately, then force Power Saving.
   Check geometry restoration, opaque fallbacks and immediate motion stopping.
   Compare idle and scrolling CPU/frame behavior with animated media visible.
6. Leave unread messages outside the reading area, open the main menu and move
   over corner controls. Confirm backdrop updates do not independently mark
   unseen messages/reactions read. Switch accounts and close/reopen the client.
7. Run official Telegram and the fork together. Confirm independent sessions,
   taskbar/notification identities and shortcuts. The fork skips automatic
   protocol registration, uses its own toast CLSID and shortcut names, skips
   legacy official migration and has the official updater disabled. Verify the
   existing default `tg:` handler stays unchanged.

## Modified files

The inventory covers the complete feature branch and current integration.
Toolkit implementation changes are recorded in the two versioned patches;
the pinned submodule revision is unchanged.

- `.github/workflows/fork-win-x64.yml`
- `.github/workflows/mac.yml`
- `.github/workflows/verify-fork-artifact.yml`
- `.github/workflows/win.yml`
- `Telegram/CMakeLists.txt`
- `Telegram/Resources/langs/lang.strings`
- `Telegram/Resources/winrc/Telegram.rc`
- `Telegram/SourceFiles/core/application.cpp`
- `Telegram/SourceFiles/core/core_settings.cpp`
- `Telegram/SourceFiles/core/launcher.cpp`
- `Telegram/SourceFiles/core/ui_integration.cpp`
- `Telegram/SourceFiles/core/ui_integration.h`
- `Telegram/SourceFiles/dialogs/dialogs_widget.cpp`
- `Telegram/SourceFiles/dialogs/dialogs_widget.h`
- `Telegram/SourceFiles/dialogs/ui/dialogs_layout.cpp`
- `Telegram/SourceFiles/dialogs/ui/dialogs_suggestions.cpp`
- `Telegram/SourceFiles/history/history_inner_widget.cpp`
- `Telegram/SourceFiles/history/history_view_pull_to_next_channel.cpp`
- `Telegram/SourceFiles/history/history_widget.cpp`
- `Telegram/SourceFiles/history/history_widget.h`
- `Telegram/SourceFiles/history/view/controls/history_view_compose_controls.cpp`
- `Telegram/SourceFiles/history/view/controls/history_view_compose_controls.h`
- `Telegram/SourceFiles/history/view/controls/history_view_top_controls.cpp`
- `Telegram/SourceFiles/history/view/controls/history_view_voice_record_bar.cpp`
- `Telegram/SourceFiles/history/view/history_view_chat_section.cpp`
- `Telegram/SourceFiles/history/view/history_view_corner_buttons.cpp`
- `Telegram/SourceFiles/history/view/history_view_corner_buttons.h`
- `Telegram/SourceFiles/history/view/history_view_list_widget.cpp`
- `Telegram/SourceFiles/history/view/history_view_pinned_section.cpp`
- `Telegram/SourceFiles/history/view/history_view_scheduled_section.cpp`
- `Telegram/SourceFiles/history/view/history_view_service_message.cpp`
- `Telegram/SourceFiles/history/view/history_view_subsection_tabs.cpp`
- `Telegram/SourceFiles/history/view/history_view_top_bar_widget.cpp`
- `Telegram/SourceFiles/history/view/history_view_top_bar_widget.h`
- `Telegram/SourceFiles/info/info_content_widget.cpp`
- `Telegram/SourceFiles/info/info_top_bar.cpp`
- `Telegram/SourceFiles/info/profile/info_profile_top_bar.cpp`
- `Telegram/SourceFiles/info/settings/info_settings_widget.cpp`
- `Telegram/SourceFiles/logs.cpp`
- `Telegram/SourceFiles/mainwindow.cpp`
- `Telegram/SourceFiles/platform/win/specific_win.cpp`
- `Telegram/SourceFiles/platform/win/windows_app_user_model_id.cpp`
- `Telegram/SourceFiles/platform/win/windows_toast_activator.h`
- `Telegram/SourceFiles/settings/sections/settings_advanced.cpp`
- `Telegram/SourceFiles/settings/sections/settings_information.cpp`
- `Telegram/SourceFiles/settings/settings_common.cpp`
- `Telegram/SourceFiles/settings/settings_power_saving.cpp`
- `Telegram/SourceFiles/tests/fork_glass/CMakeLists.txt`
- `Telegram/SourceFiles/tests/fork_glass/test_chat_insets.cpp`
- `Telegram/SourceFiles/tests/fork_glass/test_glass_blur.cpp`
- `Telegram/SourceFiles/tests/fork_glass/test_glass_material.cpp`
- `Telegram/SourceFiles/tests/fork_glass/test_smooth_wheel.cpp`
- `Telegram/SourceFiles/ui/chat/chat_content_insets.cpp`
- `Telegram/SourceFiles/ui/chat/chat_content_insets.h`
- `Telegram/SourceFiles/ui/chat/fork_glass.style`
- `Telegram/SourceFiles/ui/chat/glass_blur.cpp`
- `Telegram/SourceFiles/ui/chat/glass_blur.h`
- `Telegram/SourceFiles/ui/chat/glass_material.cpp`
- `Telegram/SourceFiles/ui/chat/glass_material.h`
- `Telegram/SourceFiles/ui/chat/glass_surface.cpp`
- `Telegram/SourceFiles/ui/chat/glass_surface.h`
- `Telegram/SourceFiles/ui/chat/pinned_bar.cpp`
- `Telegram/SourceFiles/ui/chat/pinned_bar.h`
- `Telegram/SourceFiles/ui/controls/jump_down_button.cpp`
- `Telegram/SourceFiles/ui/controls/jump_down_button.h`
- `Telegram/SourceFiles/ui/controls/labeled_emoji_tabs.cpp`
- `Telegram/SourceFiles/ui/controls/labeled_emoji_tabs.h`
- `Telegram/SourceFiles/ui/power_saving.h`
- `Telegram/SourceFiles/ui/widgets/chat_filters_tabs_strip.cpp`
- `Telegram/SourceFiles/ui/widgets/glass_navigation_button.cpp`
- `Telegram/SourceFiles/ui/widgets/glass_navigation_button.h`
- `Telegram/SourceFiles/ui/widgets/smooth_wheel_model.cpp`
- `Telegram/SourceFiles/ui/widgets/smooth_wheel_model.h`
- `Telegram/SourceFiles/ui/widgets/smooth_wheel_scroll.cpp`
- `Telegram/SourceFiles/ui/widgets/smooth_wheel_scroll.h`
- `Telegram/SourceFiles/window/window_filters_menu.cpp`
- `Telegram/SourceFiles/window/window_main_menu.cpp`
- `Telegram/SourceFiles/window/window_main_menu.h`
- `Telegram/SourceFiles/window/window_main_menu_helpers.cpp`
- `Telegram/build/fork/ensure-atl.ps1`
- `Telegram/build/fork/package-win.ps1`
- `Telegram/build/fork/smoke-win.ps1`
- `Telegram/build/fork/trim-qt-cache.ps1`
- `Telegram/build/fork/verify-package.py`
- `Telegram/cmake/fork_patches.cmake`
- `Telegram/cmake/td_ui.cmake`
- `Telegram/cmake/telegram_options.cmake`
- `docs/fork/BUILDING.md`
- `docs/fork/DESIGN_NOTES.md`
- `docs/fork/IMPLEMENTATION.md`
- `docs/fork/PLAN.md`
- `docs/fork/RESEARCH.md`
- `fork-patches/0001-full-window-menu-shade.patch`
- `fork-patches/0002-default-scroll-wheel-hook.patch`
