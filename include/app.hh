#pragma once

#include <raydom/raydom.hh>

#include "render.hh"
#include "sim.hh"

struct App {
  explicit App(rd::Context& c) : ctx(c) {}

  rd::Context& ctx;
  Sim          sim;
  Renderer     renderer;

  bool     playing = true, running = false;
  rd_timer ticker{};
  double   last = 0, painted = 0, stat_time = 0;
  float    rate = 0;

  bool  dragging = false;
  float drag_x = 0, drag_y = 0;

  rd::Node play_label, radii, incl_slider, incl_value;
  rd::Node stat_particles, stat_accreted, stat_rate, stat_fps;
};

// Builds the window's contents into the root.
void build_ui(App& app);
// Pauses or plays.
void toggle_play(App& app);
