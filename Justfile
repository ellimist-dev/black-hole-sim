# Builds against the raydom tag pinned in CMakeLists.txt.
build:
    cmake -S . -B build && cmake --build build

run: build
    ./build/black-hole-sim

# Builds and runs against a local raydom checkout instead of the pinned tag.
local path="../raydom":
    cmake -S . -B build-local -DFETCHCONTENT_SOURCE_DIR_RAYDOM={{absolute_path(path)}} && cmake --build build-local
    ./build-local/black-hole-sim

# Builds for the browser with Emscripten (emcmake on the PATH).
web:
    emcmake cmake -S . -B build-web -DPLATFORM=Web && cmake --build build-web

# Builds for the browser and serves it at http://localhost:8080.
serve: web
    cd serve && go run . -dir ../build-web

# The web build against a local raydom checkout, served at http://localhost:8080.
serve-local path="../raydom":
    emcmake cmake -S . -B build-web-local -DPLATFORM=Web -DFETCHCONTENT_SOURCE_DIR_RAYDOM={{absolute_path(path)}} && cmake --build build-web-local
    cd serve && go run . -dir ../build-web-local

clean:
    rm -rf build build-local build-web build-web-local
