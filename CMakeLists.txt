cmake_minimum_required(VERSION 3.25)
project(black_hole_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# raydom brings raylib: an installed raylib 6.0 if there is one, otherwise a download. Build against
# a local checkout instead with -DFETCHCONTENT_SOURCE_DIR_RAYDOM=<path> (just local).
# The sim doesn't use raydom's native file dialogs, which need libdbus on Linux.
set(RAYDOM_FILE_DIALOG OFF)
include(FetchContent)
FetchContent_Declare(raydom
    GIT_REPOSITORY https://github.com/ellimist-dev/raydom.git
    GIT_TAG        v0.2.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(raydom)

# The UI font, Anonymous Pro from raylib's examples: from raylib's sources when raydom downloaded
# them, otherwise downloaded on its own.
set(FONT anonymous_pro_bold.ttf)
FetchContent_GetProperties(raylib)
if (raylib_SOURCE_DIR AND EXISTS "${raylib_SOURCE_DIR}/examples/text/resources/${FONT}")
    set(FONT_DIR "${raylib_SOURCE_DIR}/examples/text/resources")
else ()
    set(FONT_DIR "${CMAKE_BINARY_DIR}/fonts")
    if (NOT EXISTS "${FONT_DIR}/${FONT}")
        file(DOWNLOAD "https://raw.githubusercontent.com/raysan5/raylib/6.0/examples/text/resources/${FONT}"
             "${FONT_DIR}/${FONT}" STATUS status)
        list(GET status 0 code)
        if (NOT code EQUAL 0)
            file(REMOVE "${FONT_DIR}/${FONT}")
            message(FATAL_ERROR "black-hole-sim: couldn't download ${FONT}: ${status}")
        endif ()
    endif ()
endif ()

# The font as a C array for main.cc to include: #embed would do this, but few compilers have it yet.
set(FONT_INC "${CMAKE_BINARY_DIR}/generated/font.inc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${FONT_DIR}/${FONT}")
file(READ "${FONT_DIR}/${FONT}" font_hex HEX)
string(REGEX REPLACE "(..)" "0x\\1," font_bytes "${font_hex}")
string(REGEX REPLACE "((0x..,){32})" "\\1\n" font_bytes "${font_bytes}")
file(CONFIGURE OUTPUT "${FONT_INC}" CONTENT "${font_bytes}\n")

add_executable(black-hole-sim main.cc src/sim.cc src/render.cc src/theme.cc src/ui.cc)
target_include_directories(black-hole-sim PRIVATE include "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(black-hole-sim PRIVATE raydom::extras)
target_compile_options(black-hole-sim PRIVATE -Wall -Wextra -Wconversion)

# The web build (just web): one self-contained script, black-hole-sim.js, with the WebAssembly
# embedded (SINGLE_FILE). It creates its own canvas when the page has none (web/pre.js), and the
# browser drives its frames. web/index.html, copied next to it, is a page for local testing.
if (EMSCRIPTEN)
    set_target_properties(black-hole-sim PROPERTIES SUFFIX ".js")
    target_link_options(black-hole-sim PRIVATE -O1 -sSINGLE_FILE=1 -sENVIRONMENT=web -sUSE_GLFW=3
                        -sALLOW_MEMORY_GROWTH=1 --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js)
    set_property(TARGET black-hole-sim APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/pre.js)
    configure_file(web/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html COPYONLY)
endif ()
