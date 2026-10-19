# black hole

A particle accretion disc around a spinning black hole, with gravitational lensing, Doppler beaming
and an Earth to drop in. Built on [raydom](https://github.com/ellimist-dev/raydom) and raylib.

```sh
just run      # configure, build and run
```

Needs CMake 3.25, a C23/C++20 compiler with `#embed` (Clang 19 or Apple Clang 17), and
[just](https://github.com/casey/just). The first configure downloads raydom (the tag pinned in
`CMakeLists.txt`), raylib 6.0 through raydom unless one is installed, and the UI font.

## Controls

Drag the canvas to orbit the view and scroll to zoom. Space pauses, R resets, E drops the Earth.

## How it works

`src/sim.cc` integrates test particles with leapfrog in the Paczynski-Wiita potential
-GM/(r - R), with R set so that the innermost stable orbit matches the Kerr ISCO for the spin.
Viscosity removes a fixed fraction of angular momentum per orbit, so the disc spirals in; particles
swallowed at the horizon respawn at the outer edge.

`src/render.cc` images them on a raydom canvas: temperature falls as r^-3/4, beaming scales
brightness as D^2.2, gravitational redshift dims the inner disc, and material behind the hole is
lensed as by a point mass. Particles draw into two fading render textures, behind and in front of
the shadow, which gives trails.

`src/ui.cc` is the raydom interface, and `src/theme.cc` restyles raydom_extras' controls in Monokai.
