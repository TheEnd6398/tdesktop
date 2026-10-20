# Liquid Glass implementation and Windows acceptance

The fork changes active Telegram widgets, with the design enabled by default.
The review and build results are tracked in [PR #1](https://github.com/TheEnd6398/tdesktop/pull/1).
Use a successful [Fork Windows x64 run](https://github.com/TheEnd6398/tdesktop/actions/workflows/fork-win-x64.yml)
whose recorded source commit includes the changes under review. The earlier
baseline artifact has no redesigned UI.

## Implemented interface

| Area | Source integration | Result |
| --- | --- | --- |
| Chat header | `history/view/history_view_top_bar_widget.*` | Inset avatar/title/status capsule, independent circular back/call/search/menu controls, matching action masks and title hit testing; search and selection retain their native controls |
| Normal composer | `history/history_widget.*` | Floating input capsule, separate attachment/emoji surfaces, focus illumination, native send/record control, reply/edit/forward state and bottom-space adjustment |
| Other composers | `history/view/controls/history_view_compose_controls.*` | Equivalent treatment in chat/replies/scheduled views; existing custom rounded contexts keep their native style |
| Pinned messages | `ui/chat/pinned_bar.*`, `history_view_top_controls.cpp` | Separate rounded bar with inset content and original navigation/close controls |
| Corner controls | `history_view_corner_buttons.*`, `ui/controls/jump_down_button.*` | Region-limited message-content backdrop, blur and tint for down/mentions/reactions/poll/stash controls; native unread counters remain |
| Dates and recording | `history_view_service_message.cpp`, `history_view_voice_record_bar.cpp` | Rounded date highlights and a rounded recorder fallback |
| Chat list and search | `dialogs/dialogs_widget.*`, `dialogs/ui/dialogs_layout.cpp` | Rounded active/hover rows and ripple clipping, raised search field and hamburger control; existing text, counters and ordering remain native |
| Folders | `window/window_filters_menu.cpp` | Rounded active/hover folder surfaces, with native icons, badges and accessibility roles |
| Main menu | `window/window_main_menu.*`, `mainwindow.cpp` | Inset rounded menu, transparent scrolling content and a cached backdrop of the content below the layer stack; existing layer opening animation retained |
| Settings and profiles | `settings_common.cpp`, `settings_information.cpp`, `info_top_bar.cpp`, `info_profile_top_bar.cpp`, `window_main_menu_helpers.cpp` | Shared rounded navigation hover treatment, top-bar capsules and profile action highlights; native toggles and section behavior retained |
| Motion | `ui/widgets/smooth_wheel_*`, adoption sites | 220 ms cubic easing for ordinary mouse wheels over both native scroll types, accumulated fractional deltas and immediate cancellation on user navigation or native position/range changes |
| Preferences | `settings_advanced.cpp`, `settings_power_saving.cpp`, `ui/power_saving.h`, `lang.strings` | Live Liquid Glass layout switch; independent blur and smooth-scroll power-saving controls with translated labels |

The header and composer retain the original reading viewport contract, enlarged
by their floating gaps. They sample the current chat wallpaper; messages do not
underlap those bars. Corner controls sample the message list, while the main
menu samples the content widget excluding its own layer stack. Source rendering
is bounded and queued outside overlay painting. Transparent sources retain their
alpha. Capture-mode guards exclude read receipts, reaction/view tracking and
translation collection from the extra message-list paints.

The shared material retains downsampled source, output and blur scratch images.
Unchanged source pixels skip blur; output checksums suppress idle repaint loops.
Disabled blur clears retained images and uses an opaque palette tint with the
same rounded geometry. Hidden overlays do not request capture refreshes. No
submodule edits or additional dependencies are required.

## Verification boundaries

The three focused Qt executables passed locally in Debug and with
AddressSanitizer plus UndefinedBehaviorSanitizer. They cover a reference blur,
thin images, premultiplied alpha, retained buffers, sibling coordinates, bounded
QWidget rendering, transparent source composition, capture-scope restoration,
light/dark tint, fallback opacity, five DPR values and wheel classification,
easing, fractions, accumulation, reversal, bounds and cancellation. They compile
the production blur, material and wheel-model sources directly.

The Windows workflow checks these components before dependencies, compiles the
full application with MSVC, then launches it with a temporary isolated profile.
That smoke check requires a native window, ten seconds of responsiveness and
profile initialization. It then terminates only its own process and removes
the temporary profile. Consult the actual run for compile and startup results.
The package includes architecture/import inspection and license notices.

There is no interactive Windows GUI with an authenticated Telegram account in
this cloud workspace. No screenshots of the redesigned client, account tests,
visual acceptance results or performance measurements have been produced here.
Compilation and an unauthenticated startup check cannot establish those results.

Remaining rendering limits: no native desktop blur or optical refraction;
wallpaper-only sampling under header/composers; date badges, recording and
most settings/profile navigation use a palette fallback rather than sampled
blur. Multi-line composer height follows native resizing without a height
tween. Flexible settings pages keep their specialized native wheel policy.
Some uncommon dialogs, media viewers and mini-app surfaces keep their native
appearance. Narrow/RTL layouts, theme contrast, overlay refresh cost with
animated media and complete interaction compatibility need Windows acceptance.

## Windows acceptance matrix

Extract the new artifact into a separate writable directory and start it using
`Start Liquid Glass.cmd`. Direct EXE startup also chooses `profile` beside the
executable, with `%APPDATA%/Liquid Glass/` as its write-failure fallback. Keep
the official installation and data untouched. Use a test account or log in
manually; API application credentials are not account login credentials.

1. Open a conversation at Windows scaling 100%, 150% and 200%. Save real
   screenshots in light, dark and one custom theme. Confirm independent title
   and action capsules, visible wallpaper gaps, floating input and pinned bar;
   compare normal width and the narrowest supported window.
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

The following inventory covers the UI implementation and integration after the
Windows baseline; earlier build-foundation commits are also part of the PR.

- `.github/workflows/fork-win-x64.yml`
- `Telegram/CMakeLists.txt`
- `Telegram/Resources/langs/lang.strings`
- `Telegram/Resources/winrc/Telegram.rc`
- `Telegram/SourceFiles/core/application.cpp`
- `Telegram/SourceFiles/core/core_settings.cpp`
- `Telegram/SourceFiles/core/launcher.cpp`
- `Telegram/SourceFiles/dialogs/dialogs_widget.cpp`
- `Telegram/SourceFiles/dialogs/dialogs_widget.h`
- `Telegram/SourceFiles/dialogs/ui/dialogs_layout.cpp`
- `Telegram/SourceFiles/history/history_inner_widget.cpp`
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
- `Telegram/SourceFiles/tests/fork_glass/test_glass_blur.cpp`
- `Telegram/SourceFiles/tests/fork_glass/test_glass_material.cpp`
- `Telegram/SourceFiles/tests/fork_glass/test_smooth_wheel.cpp`
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
- `Telegram/SourceFiles/ui/power_saving.h`
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
- `Telegram/build/fork/smoke-win.ps1`
- `Telegram/cmake/td_ui.cmake`
- `Telegram/cmake/telegram_options.cmake`
- `docs/fork/BUILDING.md`
- `docs/fork/IMPLEMENTATION.md`
- `docs/fork/PLAN.md`
