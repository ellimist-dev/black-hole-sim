#include "sim.hh"

#include <algorithm>
#include <cmath>

float isco_rs(float a) {
  float s  = std::fabs(a);
  float z1 = 1 + std::cbrt(1 - s * s) * (std::cbrt(1 + s) + std::cbrt(1 - s));
  float z2 = std::sqrt(3 * s * s + z1 * z1);
  float r  = 3 + z2 + (a >= 0 ? -1.0f : 1.0f) * std::sqrt((3 - z1) * (3 + z1 + 2 * z2));
  return r / 2;
}

float horizon_rs(float a) { return (1 + std::sqrt(1 - a * a)) / 2; }

Hole hole(const Params& p) {
  Hole h;
  h.gm      = p.mass / 2;
  h.isco    = isco_rs(p.spin) * p.mass;
  h.horizon = horizon_rs(p.spin) * p.mass;
  h.soft    = h.isco / 3;
  h.capture = std::max(h.horizon, h.soft * 1.05f);
  return h;
}

namespace {

float circular_speed(const Hole& h, float r) { return std::sqrt(h.gm * r) / std::max(r - h.soft, 0.01f); }

// Land on a 6 degree grid, north to south from 90, west to east from -180.
const char* kLand[] = {
    "............................................................",
    "...............############.....###...###.....##............",
    ".........#########..#######............#.....####....##.....",
    "..##################.#####......############################",
    "..############...##...#...##...#############################",
    ".......########..###.........#.##########################...",
    ".........############........########################.......",
    ".........##########.........#######..#################......",
    ".........########...........###.##################.###......",
    "..........#######...........#######.##############..#.......",
    "...........###..#..........#############.#########..........",
    "............##..##.........#########.###..######............",
    "...............#...........############...#...##..#.........",
    "................####.......###########........##..#.........",
    ".................#####.......########.........####..........",
    ".................#######.......######..........###..###.....",
    ".................#######........#####...........#....##.....",
    ".................#######........######.............###......",
    "..................#####.........####.#...........######.....",
    "..................####..........###..............#######....",
    "..................###............##..............##.###.....",
    "..................##..................................#....#",
    "..................#.......................................#.",
    ".................##.........................................",
    "............................................................",
    "...................##.......................................",
    "############################################################",
    "############################################################",
    "############################################################",
    "############################################################",
};

// The Earth's color at a latitude and longitude, in degrees.
std::uint32_t earth_color(float lat, float lon) {
  lon         = std::fmod(lon + 540.0f, 360.0f) - 180;
  int  row    = std::clamp(static_cast<int>((90 - lat) / 6), 0, 29);
  int  col    = std::clamp(static_cast<int>((lon + 180) / 6), 0, 59);
  bool land   = kLand[row][col] == '#';
  float cloud = std::sin(lat * 0.21f + lon * 0.07f) * std::sin(lon * 0.13f - lat * 0.05f + 1.3f);
  if (cloud > 0.55f || std::fabs(lat) > 66) return 0xeef3f8;
  if (!land) return 0x1d5fae;
  bool desert = (lat > 14 && lat < 32 && lon > -18 && lon < 60) || (lat < -18 && lat > -32 && lon > 115 && lon < 145);
  return desert ? 0xc8a46a : 0x3f8a3c;
}

std::uint32_t scale_color(std::uint32_t c, float k) {
  auto ch = [&](int shift) { return static_cast<std::uint32_t>(static_cast<float>((c >> shift) & 0xff) * k) << shift; };
  return ch(16) | ch(8) | ch(0);
}

}  // namespace

float Sim::rand() {
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  return static_cast<float>(rng_ >> 8) / 16777216.0f;
}

float Sim::gauss() { return (rand() + rand() + rand() - 1.5f) * 2; }

Particle Sim::spawn(bool initial) {
  const Hole h  = hole(p);
  float      lo = std::min(h.isco * 1.1f, p.outer * 0.9f);
  float      rc = initial ? lo + (p.outer - lo) * std::pow(rand(), 1.6f) : p.outer * (0.88f + 0.12f * rand());
  float      th = 2 * kPi * rand();
  float      z  = gauss() * p.thickness * rc;
  float      v  = circular_speed(h, std::sqrt(rc * rc + z * z)) * (1 + 0.01f * gauss());
  return {rc * std::cos(th), rc * std::sin(th), z, -v * std::sin(th), v * std::cos(th), 0};
}

void Sim::reset() {
  disc.clear();
  bodies.clear();
  for (int i = 0; i < p.count; i++) disc.push_back(spawn(true));
  accreted = recent_ = 0;
}

void Sim::set_count(int n) {
  p.count = n;
  while (static_cast<int>(disc.size()) > n) disc.pop_back();
  while (static_cast<int>(disc.size()) < n) disc.push_back(spawn(true));
}

// Rescales the disc's vertical extent, so a change shows at once.
void Sim::set_thickness(float t) {
  if (p.thickness <= 0.001f) {
    for (Particle& q : disc) q.z = gauss() * t * std::hypot(q.x, q.y);
  } else {
    float k = t / p.thickness;
    for (Particle& q : disc) q.z *= k, q.vz *= k;
  }
  p.thickness = t;
}

// The particles fill a ball, colored by the point of the surface in front of them as seen from the
// camera when it appears, so it starts as a recognizable globe from any view. Its pericenter is
// near the ISCO, so tides draw it into a stream: part is swallowed, part flung out.
void Sim::drop_earth(float extent) {
  float az = p.azimuth * kPi / 180, in = p.inclination * kPi / 180;
  float cp = std::cos(az), sp = std::sin(az), ci = std::cos(in), si = std::sin(in);
  // View space (right, up, toward the camera) to simulation space.
  auto to_sim = [&](float x, float u, float d, float& ox, float& oy, float& oz) {
    float yr = u * ci - d * si;
    oz       = u * si + d * ci;
    ox       = x * cp + yr * sp;
    oy       = -x * sp + yr * cp;
  };
  float e = std::min(extent, p.outer * 1.1f);
  float cx, cy, cz;
  to_sim(-0.6f * e, 0.35f * e, 0.3f * e, cx, cy, cz);
  const Hole h      = hole(p);
  float      rc     = std::max(std::hypot(cx, cy), 0.01f);
  float      v      = circular_speed(h, std::sqrt(rc * rc + cz * cz)) * 0.55f;
  float      vx     = -v * cy / rc, vy = v * cx / rc;
  float      radius = 0.12f * e;

  for (int i = 0; i < 4000; i++) {
    float u, w;
    do {
      u = rand() * 2 - 1, w = rand() * 2 - 1;
    } while (u * u + w * w > 1);
    float front = std::sqrt(1 - u * u - w * w);
    float d     = (rand() * 2 - 1) * front;
    float lat = std::asin(w) * 180 / kPi, lon = 15 + std::atan2(u, front) * 180 / kPi;
    float ox, oy, oz;
    to_sim(u * radius, w * radius, d * radius, ox, oy, oz);
    Particle q{cx + ox, cy + oy, cz + oz, vx, vy, 0};
    q.color = scale_color(earth_color(lat, lon), 0.45f + 0.55f * front) | 1u << 24;  // limb darkened; the high byte marks a body
    bodies.push_back(q);
  }
  if (bodies.size() > 12000) bodies.erase(bodies.begin(), bodies.end() - 12000);
}

// Advances one particle by dt; false if it was swallowed or escaped.
bool Sim::advance(const Hole& h, Particle& q, float dt) {
  auto accel = [&h](const Particle& s, float& ax, float& ay, float& az) {
    float r = std::sqrt(s.x * s.x + s.y * s.y + s.z * s.z);
    float d = std::max(r - h.soft, 0.01f);
    float k = -h.gm / (r * d * d);
    ax = k * s.x, ay = k * s.y, az = k * s.z;
  };
  // Leapfrog, with substeps a small fraction of the local orbital period.
  float r    = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  float hmax = 0.04f * std::pow(std::max(r - h.soft, 0.05f), 1.5f) / std::sqrt(h.gm);
  int   n    = std::clamp(static_cast<int>(std::ceil(dt / hmax)), 1, 48);
  float step = dt / static_cast<float>(n);
  float ax, ay, az;
  accel(q, ax, ay, az);
  for (int i = 0; i < n; i++) {
    q.vx += ax * step / 2, q.vy += ay * step / 2, q.vz += az * step / 2;
    q.x += q.vx * step, q.y += q.vy * step, q.z += q.vz * step;
    if (q.x * q.x + q.y * q.y + q.z * q.z < h.capture * h.capture) return false;
    accel(q, ax, ay, az);
    q.vx += ax * step / 2, q.vy += ay * step / 2, q.vz += az * step / 2;
  }
  r = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  if (r > p.outer * 4) return false;

  // Viscosity: a fraction of the angular momentum per orbit.
  float rc = std::hypot(q.x, q.y);
  if (rc > 0.01f) {
    float tx = -q.y / rc, ty = q.x / rc;
    float vt = q.vx * tx + q.vy * ty;
    float dv = -vt * 0.02f * p.viscosity * std::fabs(vt) / rc * dt / (2 * kPi);
    q.vx += dv * tx, q.vy += dv * ty;
  }
  float kick = p.turbulence * 0.012f * std::sqrt(h.gm / std::max(r, 0.1f)) * std::sqrt(dt);
  q.vx += kick * gauss(), q.vy += kick * gauss(), q.vz += kick * 0.5f * gauss();
  return true;
}

void Sim::step(float dt) {
  const Hole h = hole(p);
  for (Particle& q : disc)
    if (!advance(h, q, dt)) {
      if (q.x * q.x + q.y * q.y + q.z * q.z < h.capture * h.capture * 1.5f) accreted++, recent_++;
      q = spawn(false);
    }
  for (std::size_t i = 0; i < bodies.size();)
    if (!advance(h, bodies[i], dt)) {
      accreted++, recent_++;
      bodies[i] = bodies.back();
      bodies.pop_back();
    } else {
      i++;
    }
}
