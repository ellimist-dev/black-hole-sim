#include "render.hh"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

struct Stop {
  float         t;
  std::uint32_t c;
};

const Stop kStops[][5] = {
    {{0, 0x3a0500}, {0.3f, 0xd83200}, {0.55f, 0xff8a24}, {0.8f, 0xffe1b8}, {1, 0xd9e6ff}},
    // A stellar-mass hole's disc is hotter (T scales as M^-1/4), peaking in X-rays: blue-white.
    {{0, 0x020a3a}, {0.3f, 0x1638a8}, {0.55f, 0x2f7dff}, {0.8f, 0x8fd4ff}, {1, 0xf0f8ff}},
    {{0, 0x2b1000}, {0.3f, 0xa04a00}, {0.55f, 0xf0a030}, {0.8f, 0xffe6a8}, {1, 0xfffaf0}},
    {{0, 0x1b0436}, {0.3f, 0x6a1070}, {0.55f, 0xd1424a}, {0.8f, 0xfb9b2a}, {1, 0xf6f7a5}},
    {{0, 0x300000}, {0.3f, 0x9a1d00}, {0.55f, 0xe55a00}, {0.8f, 0xffb347}, {1, 0xfff1c8}},
    // The UI's colors: purple, pink, orange, yellow.
    {{0, 0x2a1a4a}, {0.3f, 0xae81ff}, {0.55f, 0xf92672}, {0.8f, 0xfd971f}, {1, 0xe6db74}},
};

Color hex(std::uint32_t c) {
  return {static_cast<unsigned char>(c >> 16), static_cast<unsigned char>(c >> 8), static_cast<unsigned char>(c), 255};
}

unsigned char lerp(unsigned char a, unsigned char b, float k) {
  return static_cast<unsigned char>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * k);
}

Color mix(Color a, Color b, float k) { return {lerp(a.r, b.r, k), lerp(a.g, b.g, k), lerp(a.b, b.b, k), 255}; }

Color palette(int which, float t) {
  const Stop* s = kStops[which];
  t             = std::clamp(t, 0.0f, 1.0f);
  int i         = 0;
  while (i < 3 && t > s[i + 1].t) i++;
  return mix(hex(s[i].c), hex(s[i + 1].c), (t - s[i].t) / (s[i + 1].t - s[i].t));
}

unsigned char byte(float v) { return static_cast<unsigned char>(std::clamp(v, 0.0f, 1.0f) * 255); }

// Point-lens images of a source at sky offset (x, y) and distance ds behind the hole: the scale
// factors for the primary and secondary positions, and their magnifications.
void lens(float x, float y, float ds, float mass, float& k1, float& m1, float& k2, float& m2) {
  float b  = std::max(std::hypot(x, y), 1e-4f);
  float re = std::sqrt(2 * mass * ds);
  float u  = b / re;
  float q  = std::sqrt(b * b + 4 * re * re);
  k1       = (b + q) / 2 / b;
  k2       = (b - q) / 2 / b;
  float mu = (u * u + 2) / (2 * u * std::sqrt(u * u + 4));
  m1       = std::min(mu + 0.5f, 4.0f);
  m2       = std::min(mu - 0.5f, 3.0f);
}

void composite(const RenderTexture2D& t, const rd_rect& r) {
  float w = static_cast<float>(t.texture.width), h = static_cast<float>(t.texture.height);
  BeginBlendMode(BLEND_ADD_COLORS);
  DrawTexturePro(t.texture, {0, 0, w, -h}, {r.x, r.y, r.width, r.height}, {0, 0}, 0, WHITE);
  EndBlendMode();
}

// The photon ring, brighter on the side whose disc material approaches.
void draw_ring(const Params& p, float si, Vector2 c, float radius) {
  BeginBlendMode(BLEND_ADDITIVE);
  constexpr int n = 96;
  for (int i = 0; i < n; i++) {
    float a0 = 360.0f * static_cast<float>(i) / n, a1 = 360.0f * static_cast<float>(i + 1) / n;
    float k  = 1;
    if (p.beaming) {
      float los = -0.45f * si * std::cos((a0 + a1) / 2 * kPi / 180);
      k         = std::pow(std::sqrt(1 - 0.2f) / (1 - los), 2.2f);
    }
    Color col = palette(p.palette, 0.75f);
    col.a     = byte(0.85f * k * p.exposure);
    DrawRing(c, radius, radius * 1.035f, a0, a1, 2, col);
    col.a = byte(0.18f * k * p.exposure);
    DrawRing(c, radius * 1.035f, radius * 1.13f, a0, a1, 2, col);
  }
  EndBlendMode();
}

}  // namespace

const char* const kPalettes[] = {"Blackbody", "Stellar", "Interstellar", "Inferno", "Event Horizon Telescope", "Monokai"};
const int         kPaletteCount = sizeof kPalettes / sizeof kPalettes[0];

Renderer::Renderer() {
  std::uint32_t rng  = 0x2545f491u;
  auto          rand = [&rng] {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return static_cast<float>(rng >> 8) / 16777216.0f;
  };
  for (int i = 0; i < 700; i++) {
    float b = rand();
    stars_.push_back({rand(), rand(), 0.15f + 0.85f * b * b * b});
  }
}

Renderer::~Renderer() {
  if (back_.id) UnloadRenderTexture(back_), UnloadRenderTexture(front_);
  if (dot_.id) UnloadTexture(dot_);
}

void Renderer::add_sprites(const Params& p, const std::vector<Particle>& ps, const View& v) {
  const Hole h     = hole(p);
  float      shade = kShadow * p.mass;
  for (const Particle& q : ps) {
    float xr = q.x * v.cp - q.y * v.sp, yr = q.x * v.sp + q.y * v.cp;
    float vyr = q.vx * v.sp + q.vy * v.cp;
    float sx = xr, sy = yr * v.ci + q.z * v.si, depth = -yr * v.si + q.z * v.ci;
    float r  = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
    float rc = std::max(std::hypot(q.x, q.y), h.isco * 0.5f);

    float temp   = std::pow(h.isco / rc, 0.75f);
    float bright = 0.35f + 0.65f * std::pow(temp, 0.8f);
    float hue    = temp;
    if (p.beaming) {
      float speed2 = std::min(q.vx * q.vx + q.vy * q.vy + q.vz * q.vz, 0.81f);
      float los    = std::clamp(-vyr * v.si + q.vz * v.ci, -0.9f, 0.9f);
      float dop    = std::sqrt(1 - speed2) / (1 - los);
      bright *= std::pow(dop, 2.2f);  // softened from D^3, which blacks out the receding side
      hue *= std::sqrt(dop);
    }
    if (p.redshift) {
      float g = std::sqrt(std::max(0.0f, 1 - p.mass / r));
      bright *= g * g;
      hue *= g;
    }
    Color c     = palette(p.palette, (hue - 0.12f) / 1.3f);
    float alpha = std::clamp(bright * p.exposure * 0.4f, 0.03f, 1.0f);
    float size  = p.size * (0.8f + 0.4f * std::min(bright, 2.0f));
    bool  solid = false;
    if (q.color) {  // a body keeps its colors until it heats up near the hole
      float heat = std::clamp((temp - 0.45f) / 0.5f, 0.0f, 1.0f);
      c          = mix(hex(q.color & 0xffffff), c, heat);
      alpha      = 0.9f + (alpha - 0.9f) * heat;
      size       = p.size * (1.4f - 0.4f * heat);
      solid      = heat < 0.5f;
    }

    auto push = [&](float x, float y, float mu) {
      Color k = c;
      k.a     = byte(alpha * mu);
      sprites_.push_back({v.cx + x * v.scale, v.cy - y * v.scale, size * std::sqrt(mu), k, depth < 0, solid});
    };
    if (depth < 0 && p.lensing) {
      float k1, m1, k2, m2;
      lens(sx, sy, -depth, p.mass, k1, m1, k2, m2);
      push(sx * k1, sy * k1, m1);
      if (std::fabs(k2) * std::hypot(sx, sy) > shade * 1.02f && m2 > 0.02f) push(sx * k2, sy * k2, m2);
    } else {
      push(sx, sy, 1);
    }
  }
}

void Renderer::ensure_targets(int w, int h) {
  if (!dot_.id) {
    constexpr int      n = 32;
    std::vector<Color> px(n * n);
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) {
        float dx = (static_cast<float>(x) + 0.5f) / n * 2 - 1, dy = (static_cast<float>(y) + 0.5f) / n * 2 - 1;
        px[static_cast<std::size_t>(y * n + x)] = {255, 255, 255, byte(std::exp(-(dx * dx + dy * dy) * 5))};
      }
    Image img{px.data(), n, n, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    dot_ = LoadTextureFromImage(img);
    SetTextureFilter(dot_, TEXTURE_FILTER_BILINEAR);
  }
  if (back_.id && back_.texture.width == w && back_.texture.height == h) return;
  if (back_.id) UnloadRenderTexture(back_), UnloadRenderTexture(front_);
  back_  = LoadRenderTexture(w, h);
  front_ = LoadRenderTexture(w, h);
  clear_ = true;
}

// Fades a layer toward black for trails, then draws its sprites: glowing ones added, solid ones over them.
void Renderer::draw_layer(RenderTexture2D& target, bool behind, float keep, float dpi) {
  BeginTextureMode(target);
  if (clear_ || keep <= 0.01f) {
    ClearBackground(BLANK);
  } else {
    float w = static_cast<float>(target.texture.width), h = static_cast<float>(target.texture.height);
    DrawRectangleRec({0, 0, w, h}, {0, 0, 0, byte(1 - keep)});
    // 8-bit fades stall at small values; subtract a little to finish them.
    rlSetBlendFactors(RL_ONE, RL_ONE, RL_FUNC_REVERSE_SUBTRACT);
    BeginBlendMode(BLEND_CUSTOM);
    DrawRectangleRec({0, 0, w, h}, {2, 2, 2, 0});
    EndBlendMode();
  }
  Rectangle src{0, 0, static_cast<float>(dot_.width), static_cast<float>(dot_.height)};
  for (bool solid : {false, true}) {
    BeginBlendMode(solid ? BLEND_ALPHA : BLEND_ADDITIVE);
    for (const Sprite& s : sprites_) {
      if (s.behind != behind || s.solid != solid) continue;
      float d = s.size * 2.5f * dpi;
      DrawTexturePro(dot_, src, {s.x * dpi - d / 2, s.y * dpi - d / 2, d, d}, {0, 0}, 0, s.color);
    }
    EndBlendMode();
  }
  EndTextureMode();
}

// Background stars, lensed as from far behind the hole; orbiting the view slides them.
void Renderer::draw_stars(const Params& p, const View& v, const rd_rect& r) const {
  float shade = kShadow * p.mass;
  float ox = p.azimuth * 0.9f, oy = (p.inclination - 78) * 0.9f;
  float span = 2 * std::max(r.width, r.height) / v.scale;
  for (const Star& s : stars_) {
    float x   = std::fmod(s.x * span + ox + span * 10, span) - span / 2;
    float y   = std::fmod(s.y * span + oy + span * 10, span) - span / 2;
    auto  dot = [&](float sx, float sy, float mu) {
      float a = std::min(s.b * mu, 1.0f);
      DrawCircleV({r.x + v.cx + sx * v.scale, r.y + v.cy - sy * v.scale}, 0.6f + 0.6f * a, {230, 228, 215, byte(a * 0.9f)});
    };
    if (!p.lensing) {
      dot(x, y, 1);
      continue;
    }
    float k1, m1, k2, m2;
    lens(x, y, 60, p.mass, k1, m1, k2, m2);
    dot(x * k1, y * k1, m1);
    if (std::fabs(k2) * std::hypot(x, y) > shade * 1.05f) dot(x * k2, y * k2, m2);
  }
}

void Renderer::paint(const Sim& sim, const rd_paint_info& info, double now) {
  const rd_rect& r = info.content;
  if (r.width < 4 || r.height < 4) return;
  ensure_targets(static_cast<int>(r.width * info.dpi), static_cast<int>(r.height * info.dpi));

  const Params& p  = sim.p;
  float         az = p.azimuth * kPi / 180, in = p.inclination * kPi / 180;
  View          v{std::cos(az), std::sin(az), std::cos(in), std::sin(in),
                  std::min(r.width, r.height) * 0.46f / (p.outer * 1.05f) * p.zoom, r.width / 2, r.height / 2};
  extent_ = std::min(r.width, r.height) / 2 / v.scale;

  sprites_.clear();
  add_sprites(p, sim.disc, v);
  add_sprites(p, sim.bodies, v);
  float keep = std::pow(p.trails, static_cast<float>(std::clamp(now - faded_, 0.0, 0.1) * 60));
  faded_     = now;
  draw_layer(back_, true, keep, info.dpi);
  draw_layer(front_, false, keep, info.dpi);
  rd_paint_restore(&info);  // EndTextureMode reset the transform
  clear_ = false;

  rd_paint_scissor(&info);
  if (p.stars) draw_stars(p, v, r);
  composite(back_, r);
  Vector2 c{r.x + v.cx, r.y + v.cy};
  float   shade = kShadow * p.mass * v.scale;
  DrawCircleV(c, shade, BLACK);
  if (p.ring) draw_ring(p, v.si, c, shade);
  composite(front_, r);
}
