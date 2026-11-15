file(GLOB SOURCES CONFIGURE_DEPENDS
    common/*.cpp control/*.cpp config/*.cpp event/*.cpp framework/*.cpp
    graphic/*.cpp theme/*.cpp locale/*.cpp system/*.cpp window/*.cpp)
if(WIN32)
    list(REMOVE_ITEM SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/system/udev_handler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/window/listener.cpp")
endif()
if(APPLE OR EMSCRIPTEN)
    list(REMOVE_ITEM SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/graphic/graphic.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/graphic/primitive_container.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/system/udev_handler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/window/listener.cpp")
    if(EMSCRIPTEN)
        file(GLOB WASM_SOURCES CONFIGURE_DEPENDS wasm/*.cpp)
        list(APPEND SOURCES ${WASM_SOURCES})
    else()
        file(GLOB MAC_SOURCES CONFIGURE_DEPENDS macos/*.mm)
        if(NOT WUI_ENABLE_MEDIA)
            list(REMOVE_ITEM MAC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/macos/media.mm")
        endif()
        list(APPEND SOURCES ${MAC_SOURCES})
    endif()
endif()
add_library(wui STATIC ${SOURCES})
target_compile_features(wui PUBLIC cxx_std_17)
target_include_directories(wui PUBLIC ../include PRIVATE . ../thirdparty)
if(EMSCRIPTEN)
    target_compile_options(wui PUBLIC -fexceptions)
    target_link_options(wui PUBLIC
        -fexceptions -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY=1
        "-sEXPORTED_RUNTIME_METHODS=['ccall']"
        "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/wasm/browser.js")
    set_property(TARGET wui APPEND PROPERTY INTERFACE_LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/wasm/browser.js")
elseif(APPLE)
    set_target_properties(wui PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES)
    target_compile_options(wui PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
    target_link_libraries(wui PUBLIC "-framework Cocoa" "-framework CoreGraphics"
        "-framework CoreText" "-framework ImageIO")
    if(WUI_ENABLE_MEDIA)
        target_compile_definitions(wui PUBLIC WUI_HAS_NATIVE_MEDIA=1)
        target_link_libraries(wui PUBLIC "-framework AVFoundation" "-framework AudioToolbox"
            "-framework CoreAudio" "-framework CoreMedia" "-framework CoreVideo"
            "-weak_framework ScreenCaptureKit")
    endif()
elseif(WIN32)
    target_compile_definitions(wui PUBLIC NOMINMAX UNICODE _UNICODE)
    # Windows uses only Nowide's inline UTF conversion helpers.
    target_compile_definitions(wui PRIVATE BOOST_NOWIDE_NO_LIB)
    if(MSVC)
        target_compile_options(wui PRIVATE /utf-8)
    endif()
    target_link_libraries(wui PUBLIC gdiplus setupapi)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
    find_package(Threads REQUIRED)
    target_link_libraries(wui PUBLIC PkgConfig::CAIRO xcb xcb-cursor xcb-ewmh
        xcb-icccm xcb-image X11 X11-xcb Threads::Threads udev)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(wui INTERFACE -static-libstdc++ -static-libgcc)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        target_link_libraries(wui PUBLIC stdc++fs)
    endif()
endif()
