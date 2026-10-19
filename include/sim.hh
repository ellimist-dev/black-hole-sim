// The simulation: test particles around a spinning black hole.
//
// Lengths are in arbitrary units with c = 1; the mass sets the Schwarzschild radius r_s in those
// units, so a heavier hole is larger relative to the disc. Gravity is the Paczynski-Wiita potential
// -GM/(r - R), which has an innermost stable circular orbit (ISCO) at 3R. R is chosen so that the
// ISCO matches the Kerr ISCO for the spin, so material inside it plunges. Viscosity drains angular
// momentum so the disc spirals in, and swallowed particles respawn at the outer edge.
#pragma once

#include <cstdint>
#include <vector>

constexpr float kPi     = 3.14159265f;
constexpr float kShadow = 2.598f;  // sqrt(27)/2: the shadow's radius, in r_s
constexpr float kRate   = 20;      // simulation time per second at 1x

struct Params {
  float mass = 1, spin = 0.6f;
  int   count = 12000;
  float outer = 24, thickness = 0.04f, viscosity = 0.5f, turbulence = 0.3f, speed = 1;
  // The view, which the simulation reads to place the Earth in front of the camera.
  float inclination = 78, azimuth = 0, zoom = 1.7f, size = 1.6f, exposure = 1, trails = 0.7f;
  bool  lensing = true, beaming = true, redshift = true, ring = true, stars = true;
  int   palette = 0;
};

// The hole's radii for a mass and spin, in simulation units.
struct Hole {
  float gm, isco, horizon, soft, capture;
};

float isco_rs(float spin);  // Kerr ISCO in r_s; negative spin is a retrograde disc
float horizon_rs(float spin);
Hole  hole(const Params& p);

struct Particle {
  float         x, y, z, vx, vy, vz;
  std::uint32_t color = 0;  // a body's own color (0xRRGGBB) until it heats up; 0 for disc material
};

class Sim {
public:
  Params                p;
  std::vector<Particle> disc, bodies;
  int                   accreted = 0;

  void reset();
  void step(float dt);  // dt in simulation time
  void set_count(int n);
  void set_thickness(float t);
  // Drops the Earth onto a plunging orbit; extent is half the view's smaller side, in units.
  void drop_earth(float extent);

  // Captures since the last call.
  int take_recent() {
    int n   = recent_;
    recent_ = 0;
    return n;
  }

  float rand();
  float gauss();  // mean 0, deviation 1, within +-3

private:
  Particle spawn(bool initial);
  bool     advance(const Hole& h, Particle& q, float dt);

  std::uint32_t rng_    = 0x9e3779b9u;
  int           recent_ = 0;
};
