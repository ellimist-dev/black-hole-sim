#include <raydom/extras/controls.hh>
#include <raydom/extras/popups.hh>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "app.hh"
#include "theme.hh"

using namespace rd::literals;
using namespace theme;

namespace {

std::string fmt(const char* f, double v) {
  char s[48];
  std::snprintf(s, sizeof s, f, v);
  return s;
}

void refresh_stats(App& app) {
  double now = GetTime();
  if (now - app.stat_time < 0.25) return;
  app.rate      = app.rate * 0.6f + 0.4f * static_cast<float>(app.sim.take_recent() / (now - app.stat_time));
  app.stat_time = now;
  app.stat_particles.text(std::to_string(app.sim.disc.size() + app.sim.bodies.size()));
  app.stat_accreted.text(std::to_string(app.sim.accreted));
  app.stat_rate.text(fmt("%.0f /s", static_cast<double>(app.rate)));
  app.stat_fps.text(std::to_string(GetFPS()));
}

void refresh_radii(App& app) {
  app.radii.text(fmt("horizon %.2f   ", static_cast<double>(horizon_rs(app.sim.p.spin))) +
                 fmt("ISCO %.2f   ", static_cast<double>(isco_rs(app.sim.p.spin))) + fmt("shadow %.2f\n(in r_s)", static_cast<double>(kShadow)));
}

// Runs the simulation while the canvas is drawn. A timer keeps frames coming; it stops itself when
// the canvas hasn't been painted lately (the window is hidden or minimized) and painting restarts it.
void paint(App& app, const rd_paint_info& info) {
  double now  = GetTime();
  app.painted = now;
  if (app.playing && !app.running) {
    app.running = true;
    app.last    = now;
    app.ticker  = app.ctx.set_interval(1.0 / 120, [&app] {
      double t = GetTime();
      if (t - app.painted > 0.5) {
        app.ctx.clear_timer(app.ticker);
        app.running = false;
        return;
      }
      float dt = static_cast<float>(std::min(t - app.last, 0.05));
      app.last = t;
      app.sim.step(dt * kRate * app.sim.p.speed);
      refresh_stats(app);
    });
  }
  app.renderer.paint(app.sim, info, now);
}

void reset(App& app) {
  app.sim.reset();
  app.renderer.clear();
}

rd::Node button(const rd::Context& ctx, rd::Class cls, std::string_view label, rd::Node* text_out = nullptr) {
  rd::Node b = ctx.create().add_class(cls);
  rd::Node t = ctx.create_text(label, rd::Style().pointer_events(RD_POINTER_EVENTS_NONE));
  b.append(t);
  if (text_out) *text_out = t;
  return b;
}

struct Ui {
  const rd::Context& ctx;
  rd::Class          muted, heading, number;

  rd::Node section(rd::Node side, std::string_view title) const {
    rd::Node s = ctx.create(rd::Style().row_gap(12_px).padding(14_px, 0_px).border_width(0_px, 0_px, 1_px, 0_px).border_color(rd::hex(kLine)));
    s.append(ctx.create_text(title).add_class(heading));
    side.append(s);
    return s;
  }

  // A labeled slider with its value; set(v) runs as it moves.
  template <class F>
  rd::Node knob(rd::Node parent, std::string_view name, const char* format, rdx_slider_desc d, F set,
                rd::Node* value_out = nullptr, std::string_view tip = {}) const {
    rd::Node row   = ctx.create(rd::Style().flex_direction(RD_FLEX_ROW).align_items(RD_ALIGN_CENTER));
    rd::Node label = ctx.create_text(name, rd::Style().flex(1));
    rd::Node value = ctx.create_text(fmt(format, static_cast<double>(d.value))).add_class(number);
    row.append(label).append(value);
    rd::Node s = rdx::slider(ctx, d);
    s.merge_style(rd::Style().width(100_pct));
    s.on(RD_EVENT_INPUT, [s, value, format, set] {
      float v = rdx::slider_value(s);
      value.text(fmt(format, static_cast<double>(v)));
      set(v);
    });
    if (!tip.empty()) rdx::tooltip(label, std::string(tip));
    parent.append(ctx.create(rd::Style().row_gap(6_px)).append(row).append(s));
    if (value_out) *value_out = value;
    return s;
  }

  template <class F>
  void toggle(rd::Node parent, std::string_view name, bool on, F set, std::string_view tip = {}) const {
    rd::Node s = rdx::toggle_switch(ctx, std::string(name));
    rdx::set_checked(s, on);
    s.on(RD_EVENT_CHANGE, [s, set] { set(rdx::checked(s)); });
    if (!tip.empty()) rdx::tooltip(s, std::string(tip));
    parent.append(s);
  }
};

rd::Node build_header(App& app, const Ui& ui) {
  const rd::Context& ctx = app.ctx;
  rd::Class          btn = ctx.create_class(rd::Style()
                                                .padding(6_px, 14_px)
                                                .border_radius(7_px)
                                                .border_width(1_px)
                                                .border_color(rd::hex(kHover))
                                                .background(rd::hex(kRaised))
                                                .focusable(RD_FOCUSABLE_TAB)
                                                .cursor(RD_CURSOR_POINTER)
                                                .user_select(RD_USER_SELECT_NONE)
                                                .transition_duration(120_ms));
  btn.on(RD_STATE_HOVER, rd::Style().background(rd::hex(kHover)));
  btn.on(RD_STATE_ACTIVE, rd::Style().background(rd::hex(kPanel)));
  btn.on(RD_STATE_FOCUS_VISIBLE, rd::Style().box_shadow(0, 0, 0, rd::rgba(249, 38, 114, 200), 2));
  rd::Class primary = ctx.create_class(rd::Style().background(rd::hex(kPinkLo)).border_color(rd::hex(kPink)).color(rd::hex(0xffffff)));
  primary.on(RD_STATE_HOVER, rd::Style().background(rd::hex(kPink)));
  primary.on(RD_STATE_ACTIVE, rd::Style().background(rd::hex(kPinkLo)));

  // The title takes what's left after the buttons, cutting the subtitle short; below the title's own
  // width, the buttons wrap to a second line.
  rd::Node header = ctx.create(rd::Style()
                                   .flex_direction(RD_FLEX_ROW)
                                   .flex_wrap(RD_FLEX_WRAP)
                                   .align_items(RD_ALIGN_CENTER)
                                   .column_gap(16_px)
                                   .row_gap(8_px)
                                   .padding(10_px, 16_px)
                                   .background(rd::hex(kPanel))
                                   .border_width(0_px, 0_px, 1_px, 0_px)
                                   .border_color(rd::hex(kLine)));
  rd::Node brand = ctx.create(rd::Style()
                                  .flex_direction(RD_FLEX_ROW)
                                  .align_items(RD_ALIGN_CENTER)
                                  .column_gap(10_px)
                                  .flex_grow(rd::num(1))
                                  .flex_shrink(rd::num(1))
                                  .flex_basis(140_px)
                                  .min_width(0_px));
  brand.append(ctx.create(rd::Style().width(12_px).height(12_px).border_radius(6_px).background(rd::hex(kPink)).flex_shrink(rd::num(0))));
  brand.append(ctx.create_text("Black Hole", rd::Style().font_size(18_px).white_space(RD_WHITE_SPACE_NOWRAP).flex_shrink(rd::num(0))));
  brand.append(ctx.create_text("// an accretion disc of particles around a Kerr black hole",
                               rd::Style()
                                   .flex_shrink(rd::num(1))
                                   .min_width(0_px)
                                   .white_space(RD_WHITE_SPACE_NOWRAP)
                                   .text_overflow(RD_TEXT_OVERFLOW_ELLIPSIS)
                                   .overflow_x(RD_OVERFLOW_HIDDEN))
                   .add_class(ui.muted));
  rd::Node actions = ctx.create(rd::Style().flex_direction(RD_FLEX_ROW).column_gap(8_px).flex_shrink(rd::num(0)));
  header.append(brand).append(actions);
  rd::Node play  = button(ctx, btn, "Pause", &app.play_label).add_class(primary);
  rd::Node earth = button(ctx, btn, "Drop Earth");
  rd::Node again = button(ctx, btn, "Reset");
  rdx::tooltip(play, "Space");
  rdx::tooltip(earth, "Drop the Earth onto a plunging orbit, to be torn apart by tides (E)");
  rdx::tooltip(again, "R");
  actions.append(play).append(earth).append(again);
  play.on_click([&app] { toggle_play(app); });
  earth.on_click([&app] { app.sim.drop_earth(app.renderer.extent()); });
  again.on_click([&app] { reset(app); });
  return header;
}

rd::Node build_side(App& app, const Ui& ui) {
  const rd::Context& ctx  = app.ctx;
  rd::Node           side = ctx.create();
  Params&            p    = app.sim.p;
  Sim&               sim  = app.sim;

  rd::Node bh = ui.section(side, "black hole");
  ui.knob(bh, "Mass", "%.2f r_s", {.min = 0.4f, .max = 3, .value = p.mass, .step = 0.05f}, [&sim](float v) { sim.p.mass = v; },
          nullptr, "Schwarzschild radius in disc units. A heavier hole swallows the inner disc.");
  ui.knob(bh, "Spin", "%+.2f", {.min = -0.99f, .max = 0.99f, .value = p.spin, .step = 0.01f},
          [&app](float v) {
            app.sim.p.spin = v;
            refresh_radii(app);
          },
          nullptr, "Kerr spin a/M. Negative spins against the disc, moving the ISCO out.");
  app.radii = ctx.create_text("", rd::Style().font_size(12_px).white_space(RD_WHITE_SPACE_PRE).color(rd::hex(kSoft)));
  bh.append(app.radii);
  refresh_radii(app);

  rd::Node disc = ui.section(side, "disc");
  ui.knob(disc, "Particles", "%.0f", {.min = 1000, .max = 100000, .value = static_cast<float>(p.count), .step = 1000},
          [&sim](float v) { sim.set_count(static_cast<int>(v)); });
  ui.knob(disc, "Outer radius", "%.0f", {.min = 12, .max = 60, .value = p.outer, .step = 1}, [&sim](float v) { sim.p.outer = v; });
  ui.knob(disc, "Thickness", "%.2f h/r", {.min = 0, .max = 0.25f, .value = p.thickness, .step = 0.01f},
          [&sim](float v) { sim.set_thickness(v); });
  ui.knob(disc, "Viscosity", "%.2f", {.min = 0, .max = 2, .value = p.viscosity, .step = 0.05f}, [&sim](float v) { sim.p.viscosity = v; },
          nullptr, "How fast the disc loses angular momentum and spirals in.");
  ui.knob(disc, "Turbulence", "%.2f", {.min = 0, .max = 2, .value = p.turbulence, .step = 0.05f}, [&sim](float v) { sim.p.turbulence = v; });
  ui.knob(disc, "Time scale", "%.1fx", {.min = 0.1f, .max = 5, .value = p.speed, .step = 0.1f}, [&sim](float v) { sim.p.speed = v; });

  rd::Node view   = ui.section(side, "view");
  app.incl_slider = ui.knob(view, "Inclination", "%.0f deg", {.min = 0, .max = 89, .value = p.inclination, .step = 1},
                            [&sim](float v) { sim.p.inclination = v; }, &app.incl_value, "Drag the canvas to orbit the view.");
  ui.knob(view, "Exposure", "%.2f", {.min = 0.2f, .max = 3, .value = p.exposure, .step = 0.05f}, [&sim](float v) { sim.p.exposure = v; });
  ui.knob(view, "Particle size", "%.1f px", {.min = 0.5f, .max = 4, .value = p.size, .step = 0.1f}, [&sim](float v) { sim.p.size = v; });
  ui.knob(view, "Trails", "%.2f", {.min = 0, .max = 0.97f, .value = p.trails, .step = 0.01f}, [&sim](float v) { sim.p.trails = v; });
  view.append(ctx.create_text("Palette"));
  rd::Node pal = rdx::select(ctx, std::vector<std::string>(kPalettes, kPalettes + kPaletteCount));
  rdx::set_selected(pal, p.palette);
  pal.on(RD_EVENT_CHANGE, [&sim, pal] { sim.p.palette = std::max(0, rdx::selected(pal)); });
  view.append(pal);

  rd::Node fx = ui.section(side, "effects");
  ui.toggle(fx, "Gravitational lensing", p.lensing, [&sim](bool on) { sim.p.lensing = on; },
            "Light from behind the hole bends around it: the arch over the shadow.");
  ui.toggle(fx, "Doppler beaming", p.beaming, [&sim](bool on) { sim.p.beaming = on; },
            "The side moving toward you is brighter and bluer.");
  ui.toggle(fx, "Gravitational redshift", p.redshift, [&sim](bool on) { sim.p.redshift = on; });
  ui.toggle(fx, "Photon ring", p.ring, [&sim](bool on) { sim.p.ring = on; });
  ui.toggle(fx, "Stars", p.stars, [&sim](bool on) { sim.p.stars = on; });
  side.append(ctx.create(rd::Style().height(16_px)));
  return side;
}

rd::Node build_canvas(App& app, const Ui& ui) {
  const rd::Context& ctx  = app.ctx;
  rd::Class          look = ctx.create_class(rd::Style().flex(1).min_width(0_px).min_height(0_px).background(rd::hex(kSpace)));
  look.on(RD_STATE_ACTIVE, rd::Style().cursor(RD_CURSOR_MOVE));
  rd::Node canvas = ctx.create().add_class(look);
  canvas.paint([&app](const rd_paint_info& info) { paint(app, info); });
  canvas.on(RD_EVENT_POINTER_DOWN, [&app, canvas](rd::Event& e) {
    canvas.capture_pointer();
    app.dragging = true;
    app.drag_x = e.x(), app.drag_y = e.y();
  });
  canvas.on(RD_EVENT_POINTER_MOVE, [&app](rd::Event& e) {
    if (!app.dragging) return;
    Params& p = app.sim.p;
    p.azimuth += (e.x() - app.drag_x) * 0.4f;
    p.inclination = std::clamp(p.inclination - (e.y() - app.drag_y) * 0.3f, 0.0f, 89.0f);
    app.drag_x = e.x(), app.drag_y = e.y();
    rdx::set_slider_value(app.incl_slider, p.inclination);
    app.incl_value.text(fmt("%.0f deg", std::round(static_cast<double>(p.inclination))));
  });
  canvas.on(RD_EVENT_POINTER_UP, [&app, canvas] {
    canvas.release_pointer();
    app.dragging = false;
  });
  canvas.on(RD_EVENT_WHEEL, [&app](rd::Event& e) {
    app.sim.p.zoom = std::clamp(app.sim.p.zoom * std::exp(e.wheel_y() * 0.1f), 0.3f, 8.0f);
    e.prevent_default();
  });

  // Stats: comment-colored labels, number-colored values.
  rd::Node stats = ctx.create(rd::Style()
                                  .position(RD_POSITION_ABSOLUTE)
                                  .inset(12_px, rd::auto_, rd::auto_, 14_px)
                                  .display(RD_DISPLAY_GRID)
                                  .grid_template_columns(ctx.tracks({rd::auto_, rd::auto_}))
                                  .column_gap(12_px)
                                  .row_gap(2_px)
                                  .font_size(12_px)
                                  .pointer_events(RD_POINTER_EVENTS_NONE));
  for (auto [name, value] : {std::pair{"particles", &app.stat_particles}, {"accreted", &app.stat_accreted},
                             {"rate", &app.stat_rate}, {"fps", &app.stat_fps}}) {
    *value = ctx.create_text("0").add_class(ui.number);
    stats.append(ctx.create_text(name).add_class(ui.muted)).append(*value);
  }
  canvas.append(stats);
  canvas.append(ctx.create_text("// drag to orbit, scroll to zoom", rd::Style()
                                                                         .position(RD_POSITION_ABSOLUTE)
                                                                         .inset(rd::auto_, rd::auto_, 12_px, 14_px)
                                                                         .font_size(12_px)
                                                                         .color(rd::hex(kMuted))
                                                                         .pointer_events(RD_POINTER_EVENTS_NONE)));
  return canvas;
}

}  // namespace

void toggle_play(App& app) {
  app.playing = !app.playing;
  app.play_label.text(app.playing ? "Pause" : "Play");
  if (!app.playing && app.running) {
    app.ctx.clear_timer(app.ticker);
    app.running = false;
  }
}

void build_ui(App& app) {
  const rd::Context& ctx = app.ctx;
  Ui ui{ctx, ctx.create_class(rd::Style().color(rd::hex(kMuted))),
        ctx.create_class(rd::Style().font_size(12_px).color(rd::hex(kPink)).user_select(RD_USER_SELECT_NONE)),
        ctx.create_class(rd::Style().color(rd::hex(kPurple)))};

  rd::Node root = ctx.root();
  root.style(rd::Style().background(rd::hex(kBg)).color(rd::hex(kText)).font_size(14_px));
  // Shortcuts, when no control has focus: a focused button already takes Space.
  root.on(RD_EVENT_KEY_DOWN, [&app, root](rd::Event& e) {
    if (e.target() != root) return;
    if (e.key() == RD_KEY_SPACE) toggle_play(app);
    if (e.key() == RD_KEY_R) reset(app);
    if (e.key() == RD_KEY_E) app.sim.drop_earth(app.renderer.extent());
  });
  rd::Node body = ctx.create();
  rd::Node side = build_side(app, ui);
  body.append(side).append(build_canvas(app, ui));
  root.append(build_header(app, ui)).append(body);

  // Wide windows put the controls in a column on the left; tall or narrow ones put them under the
  // canvas, in as many columns as fit. Checked after each layout; a change lays out once more.
  rd_tracks columns = ctx.tracks(rd::repeat(rd::auto_fill, {rd::minmax(240_px, 1_fr)}));
  rd::Style panel   = rd::Style().padding(0_px, 16_px).background(rd::hex(kPanel)).border_color(rd::hex(kLine)).overflow_y(RD_OVERFLOW_SCROLL).flex_shrink(rd::num(0));
  int       mode    = -1;
  body.on_layout([body, side, root, columns, panel, mode]() mutable {
    rd_rect r        = root.rect();
    int     portrait = r.width < r.height || r.width < 640 ? 1 : 0;
    if (portrait == mode) return;
    mode = portrait;
    if (portrait) {
      body.style(rd::Style().flex(1).min_height(0_px).flex_direction(RD_FLEX_COLUMN_REVERSE));
      side.style(rd::Style(panel)
                     .height(rd::pct(42))
                     .border_width(1_px, 0_px, 0_px, 0_px)
                     .display(RD_DISPLAY_GRID)
                     .grid_template_columns(columns)
                     .column_gap(24_px)
                     .align_content(RD_ALIGN_START));
    } else {
      body.style(rd::Style().flex(1).min_height(0_px).flex_direction(RD_FLEX_ROW));
      side.style(rd::Style(panel).width(300_px).border_width(0_px, 1_px, 0_px, 0_px));
    }
  });
  reset(app);
}
