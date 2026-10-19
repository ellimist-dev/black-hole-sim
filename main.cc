// A black hole with an accretion disc of particles, built on raydom.

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <cmath>
#include <cstdlib>
#include <memory>
#include <raydom/raydom.hh>

#include "app.hh"
#include "theme.hh"

namespace {

const unsigned char kFont[] = {
#include "font.inc"  // generated from the font file by CMakeLists.txt
};

// Everything the frames need, kept off main's stack: on the web, main returns while frames continue.
struct Program {
  rd::Context ctx;
  App         app{ctx};  // destroyed first: the context then frees the callbacks pointing at it without calling them
  const char* shot  = std::getenv("BH_SCREENSHOT");
  int         final = 120;
  int         frame = 0;
};

// One frame; false when the program should end.
#ifdef __EMSCRIPTEN__
// raylib renders the web at the canvas's own resolution. Keeping the canvas at the page's size in
// device pixels, shown at CSS size, and telling raydom the ratio keeps text and edges sharp.
void fit_canvas(const rd::Context& ctx) {
  double ratio = emscripten_get_device_pixel_ratio();
  int    w     = static_cast<int>(std::lround(EM_ASM_DOUBLE(return window.innerWidth;) * ratio));
  int    h     = static_cast<int>(std::lround(EM_ASM_DOUBLE(return window.innerHeight;) * ratio));
  if (w != GetScreenWidth() || h != GetScreenHeight()) SetWindowSize(w, h);
  ctx.set_pixel_ratio(static_cast<float>(ratio));
}
#endif

bool frame(Program& p) {
  rd::Context& ctx = p.ctx;
#ifdef __EMSCRIPTEN__
  fit_canvas(ctx);
#endif
  ctx.update();
  float width  = static_cast<float>(GetScreenWidth()) / ctx.pixel_ratio();
  float height = static_cast<float>(GetScreenHeight()) / ctx.pixel_ratio();
  // Screenshot harness: BH_CLICK=<x>,<y>[;<x>,<y>...] clicks and BH_KEYS=<letters> presses keys on the second frame.
  if (const char* c = std::getenv("BH_CLICK"); p.shot && c && p.frame == 1) {
    while (*c) {
      char* end = nullptr;
      float x   = std::strtof(c, &end);
      float y   = std::strtof(end + 1, &end);
      ctx.layout(width, height);
      ctx.pointer(x, y, RD_BUTTON_PRIMARY);
      ctx.pointer(x, y, 0);
      c = *end == ';' ? end + 1 : end;
    }
  }
  if (const char* k = std::getenv("BH_KEYS"); p.shot && k && p.frame == 1)
    for (; *k >= 'a' && *k <= 'z'; k++) ctx.press(static_cast<rd_key>(RD_KEY_A + (*k - 'a')));
  ctx.layout(width, height);
  BeginDrawing();
  ClearBackground(BLACK);
  ctx.draw();
  EndDrawing();
  if (p.shot && p.frame == p.final) {
    TakeScreenshot(p.shot);
    return false;
  }
  p.frame++;
  return true;
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape): std::bad_alloc should terminate.
#ifdef __EMSCRIPTEN__
  // No FLAG_WINDOW_HIGHDPI, whose scissor scaling raylib applies on the web without rendering at high
  // DPI, and no FLAG_WINDOW_RESIZABLE, whose resizing sets the canvas to CSS pixels: fit_canvas sizes it.
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  double ratio = emscripten_get_device_pixel_ratio();
  InitWindow(static_cast<int>(EM_ASM_DOUBLE(return window.innerWidth;) * ratio),
             static_cast<int>(EM_ASM_DOUBLE(return window.innerHeight;) * ratio), "black hole");
#else
  SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(1280, 820, "Interactive Black Hole");
#endif
  SetWindowMinSize(320, 480);
  if (const char* size = std::getenv("BH_SIZE")) {  // <w>,<h>, for screenshots
    char* end = nullptr;
    int   w   = static_cast<int>(std::strtol(size, &end, 10));
    SetWindowSize(w, static_cast<int>(std::strtol(end + 1, nullptr, 10)));
  }
  SetExitKey(KEY_NULL);

  auto program = std::make_unique<Program>();
  if (const char* f = std::getenv("BH_FRAME")) program->final = std::atoi(f);
  if (!program->shot) rd_set_event_waiting(program->ctx.get(), true);
  program->ctx.load_font(kFont, sizeof kFont, false);
  theme::apply(program->ctx);
  build_ui(program->app);

#ifdef __EMSCRIPTEN__
  // The browser drives frames; the program lives as long as the page.
  emscripten_set_main_loop_arg([](void* p) { frame(*static_cast<Program*>(p)); }, program.release(), 0, true);
#else
  while (!WindowShouldClose() && frame(*program)) {
  }
  program.reset();
  CloseWindow();
#endif
}
