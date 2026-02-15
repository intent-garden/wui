# 1.3.260215
1. Changed maintainer to Intent Garden. Code cleanup. Input fix. (I-74)

# 1.3.251114
1. Restore v143 build, because v145 does not work on Windows 7 (TSKW24-9.1)

# 1.3.251113
1. Fix the window context is missing in the window_created event on Windows (TSKW24-8)
2. Fixed positioning the transient windows (TSKW24-8)
3. Fix menu::update_theme() (TSKW24-8.1)
4. Fix scrollbar (TSKW24-8.2)
5. Migrated to MSVC 2026, remove ATL deps (TSKW24-9)

# 1.3.251014
1. TSKW24-6.1:
- Input and list now use preallocated mem_gr.
- Config save to file only in destructor
- Improve scroll behavior
- Excellent performance

# 1.3.251008
1. Implemented new caching measure_text. Performance improvings and fixes. (TSKW24-6)
2. Fix sometimes crashes on Linux startings. (TSKW24-7)

# 1.3.251006
1. Performance improves, fixes (TSKW24-4)
2. Fix input spaces on Linux (TSKW24-5)

# 1.3.251004
1. Splitter behavior improves (TSKW24-2)
2. Robust input, set_position without redraw, hover borders. Interface changed (TSKW24-3)

# 1.2.250918
1. Improved window's tool buttons and frame draw (I-48)
2. List not change the position of the selected item when new items are added (I-50)
3. Fix window cross on dark mode (I-52)
4. Implemented multiline editor (I-29)
5. Improve editors (TSKW24-1)

# 1.2.250607
1. Implemented symbols limits and numberic content checking in input (I-32)
2. Improved truncate_line() tool (I-34)
3. Cleanup and resolve PVS warns (I-42)
4. Replaced 'const wui::rect&' to 'wui::rect'. Interface changed! (I-42.2)
5. Implemented one thread per many independent windows message handling on Linux. The behavior of ports under windows and linux is completely identical! (I-46)

# 1.1.250510
1. Added udev handler to send device change events on Linux (I-20)
2. Add new method to window: void enable_device_change_handling(bool yes); It is needed to enable the window to receive events about connection / disconnection of devices (will be sent to system_event) (I-20)

# 1.1.250420
1.Implemented determining and sending the device type in system event (I-18)

# 1.1.250418
1.Implemented robust device change event receiver, improved system_event (I-16)

# 1.1.250414
1. Added dll variant for boost nowide in thirdparty, updated boost to 1.88

# 1.1.250410
1. Fixed: in some wm (xfce, fly) window does not remove standard title and buttons (I-9)
2. Fixed incorrect setting of window title in UTF8 (I-11)

# 1.1.250302
1. Fixed selected visibility on list

# 1.1.240326
1. Fix minimizing by click on task bar icon
2. Improved color with alpha
3. Fixed wrong slowly redraw in negative rect points.
4. Added scroll control

# 1.0.240206
1. Added no_redraw method to increase performance of resizing
2. Improved pinned windows resizing
3. Fixed windows sizing on Linux
4. Fixed & improved docs

# 1.0.240121
1. Fixed numpad on linux
2. Improved initialization
3. Fix transient

# 1.0.231113
1. Added scroll control
2. Implemented vert scroll bar
3. Make list with the new different scroll control

# 1.0.231028
1. Updated thirdparty boost to 1.83
2. Added msvc 2022 support
3. Returned a more user-friendly theme and locale interface
4. Replaced const std::string& to std::string_view everything it's possible.
5. Now used C++17

# 1.0.230920
1. Improved multi monitor window expanding behavior
2. Improved performance of graphic::draw_buffer
3. Removed mutex from window, now wui does not care about thread safe

# 1.0.230918
1. First stable release
