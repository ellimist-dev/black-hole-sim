// A black hole with an accretion disc of particles, built on raydom.

#include <raylib.h>

#include <cstdlib>
#include <raydom/raydom.hh>

#include "app.hh"
#include "theme.hh"

namespace {

const unsigned char kFont[] = {
#embed "anonymous_pro_bold.ttf"
};

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape): std::bad_alloc should terminate.
  const char* shot  = std::getenv("BH_SCREENSHOT");
  const char* f     = std::getenv("BH_FRAME");
  const int   final = f ? std::atoi(f) : 120;
  SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(1280, 820, "black hole");
  SetWindowMinSize(760, 480);
  SetExitKey(KEY_NULL);
  {
    rd::Context ctx;
    App         app(ctx);  // destroyed first: the context then frees the callbacks pointing at it without calling them
    if (!shot) rd_set_event_waiting(ctx.get(), true);
    ctx.load_font(kFont, sizeof kFont, false);
    theme::apply(ctx);
    build_ui(app);
    for (int frame = 0; !WindowShouldClose(); frame++) {
      ctx.update();
      if (const char* c = std::getenv("BH_CLICK"); shot && c && frame == 1) {
        char* end = nullptr;
        float x   = std::strtof(c, &end);
        float y   = std::strtof(end + 1, nullptr);
        ctx.layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
        ctx.pointer(x, y, RD_BUTTON_PRIMARY);
        ctx.pointer(x, y, 0);
      }
      ctx.layout(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
      BeginDrawing();
      ClearBackground(BLACK);
      ctx.draw();
      EndDrawing();
      if (shot && frame == final) {
        TakeScreenshot(shot);
        break;
      }
    }
  }
  CloseWindow();
}
