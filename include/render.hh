// Imaging: temperature falls as r^-3/4; Doppler beaming and gravitational redshift scale brightness
// and color. Material behind the hole is lensed as by a point mass, giving the arch over the shadow
// and a fainter secondary image just outside it. Particles draw into two fading render textures,
// one behind the shadow and one in front, which gives trails.
#pragma once

#include <raydom/raydom.h>
#include <raylib.h>

#include <vector>

#include "sim.hh"

extern const char* const kPalettes[];
extern const int         kPaletteCount;

class Renderer {
public:
  Renderer();
  ~Renderer();
  Renderer(const Renderer&)            = delete;
  Renderer& operator=(const Renderer&) = delete;

  // Draws the simulation into a canvas node's content box. Call from a paint callback.
  void paint(const Sim& sim, const rd_paint_info& info, double now);
  // Drops the trails, after a reset.
  void clear() { clear_ = true; }
  // Half the canvas's smaller side, in simulation units, as of the last paint.
  float extent() const { return extent_; }

private:
  struct Sprite {
    float x, y, size;
    Color color;
    bool  behind, solid;  // solid sprites cover what's under them instead of adding light
  };
  struct Star {
    float x, y, b;
  };
  struct View {
    float cp, sp, ci, si;  // azimuth and inclination
    float scale, cx, cy;   // logical pixels per unit, and the hole's position in the canvas
  };

  void add_sprites(const Params& p, const std::vector<Particle>& ps, const View& v);
  void ensure_targets(int w, int h);
  void draw_layer(RenderTexture2D& target, bool behind, float keep, float dpi);
  void draw_stars(const Params& p, const View& v, const rd_rect& r) const;

  std::vector<Sprite> sprites_;
  std::vector<Star>   stars_;
  RenderTexture2D     back_{}, front_{};
  Texture2D           dot_{};
  bool                clear_  = true;
  double              faded_  = 0;
  float               extent_ = 16;
};
