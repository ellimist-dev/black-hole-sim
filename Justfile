# Builds against the raydom tag pinned in CMakeLists.txt.
build:
    cmake -S . -B build && cmake --build build

run: build
    ./build/black-hole-sim

# Builds and runs against a local raydom checkout instead of the pinned tag.
local path="../raydom":
    cmake -S . -B build-local -DFETCHCONTENT_SOURCE_DIR_RAYDOM={{absolute_path(path)}} && cmake --build build-local
    ./build-local/black-hole-sim

clean:
    rm -rf build build-local
