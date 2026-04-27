# Mandelbrot Explorer

An interactive real-time Mandelbrot set explorer built with C++17, OpenGL 3.3, and GPU fragment shaders. All computation happens on the GPU — each pixel's escape-time iteration runs in parallel in the fragment shader, giving fluid interactive performance even on integrated graphics.

This is a modern revisitation of a high-performance x86 assembly Mandelbrot renderer from 35 years ago, this time using GPU shaders to achieve the same goal.

![Mandelbrot Explorer](https://upload.wikimedia.org/wikipedia/commons/thumb/2/21/Mandel_zoom_00_mandelbrot_set.jpg/640px-Mandel_zoom_00_mandelbrot_set.jpg)

---

## Features

- **GPU-accelerated rendering** — escape-time computation runs entirely in GLSL on the fragment shader; no CPU iteration loops
- **Smooth coloring** — Inigo Quilez's smooth iteration count formula eliminates harsh banding
- **6 color schemes** — Classic, Fire, Ocean, Grayscale, Psychedelic, Ultra, selectable via dropdown
- **Palette cycling** — animates the color offset within the current scheme; click once to start, again to stop
- **Cursor-invariant zoom** — the point under the cursor stays fixed as you zoom in and out
- **Drag to pan** — click and drag to move around the complex plane
- **Adjustable iteration depth** — 50 to 2000 iterations via slider
- **Save to JPEG** — captures the current view (without UI overlay) as a timestamped `.jpg`
- **Live frame time** — millisecond render cost shown in the overlay
- **Float precision warning** — the UI alerts you when zoom depth exceeds float accuracy (~1e6×)
- **Orbit visualization** — right-click any point to watch its Mandelbrot iteration animate step by step inside a circular portal overlay (see below)
- **Demand-driven rendering** — redraws only on actual input; idles at zero GPU load when nothing is happening
- **Static binary** — ships as a single `.exe` with no external DLL dependencies

---

## Controls

| Input | Action |
|-------|--------|
| Scroll wheel up | Zoom in (centered on cursor) |
| Scroll wheel down | Zoom out (centered on cursor) |
| Left click + drag | Pan |
| Right click | Pick a point and start orbit visualization |
| `H` | Toggle overlay |
| `ESC` | Quit |

The overlay also provides:
- **Zoom In / Zoom Out / Reset** buttons
- **Cycle Colors** — toggles animated palette cycling within the current scheme (green when active)
- **Save JPG** — writes `mandelbrot_YYYYMMDD_HHMMSS.jpg` to the working directory; a confirmation message appears for 3 seconds
- **Orbit Visualization** controls — Play/Pause, Restart, speed slider (0.5–30 iter/s), Loop toggle, and Clear

---

## Building

The project cross-compiles from Linux to a Windows `.exe` using MinGW-w64.

### Required Dependencies

Install the following on your Linux build machine:

```bash
sudo apt-get install -y \
    cmake \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64
```

| Package | Purpose |
|---------|---------|
| `cmake` (≥ 3.15) | Build system |
| `gcc-mingw-w64-x86-64` | MinGW C cross-compiler |
| `g++-mingw-w64-x86-64` | MinGW C++ cross-compiler |

The following libraries are fetched automatically by CMake at configure time (no manual installation needed):

| Library | Version | Role |
|---------|---------|------|
| [GLFW](https://www.glfw.org/) | 3.4 | Window creation and input |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.90.1 | Immediate-mode UI overlay |
| [GLM](https://github.com/g-truc/glm) | 0.9.9.8 | Math types (header-only) |
| [GLAD](https://glad.dav1d.de/) | bundled in `external/` | OpenGL function loader |
| [stb_image_write](https://github.com/nothings/stb) | bundled in `external/` | JPEG export |

### Build

```bash
bash build-windows.sh
```

The script cleans any previous build, configures with CMake using the MinGW toolchain, and compiles in Release mode. On first run CMake will download GLFW, ImGui, and GLM via FetchContent — this requires an internet connection.

Output is placed in `build-windows/bin/`:

```
build-windows/bin/
├── MandelbrotExplorer.exe
└── shaders/
    ├── mandelbrot.vert
    └── mandelbrot.frag
```

The shaders directory must be kept alongside the executable.

### Verify no external DLLs

```bash
objdump -p build-windows/bin/MandelbrotExplorer.exe | grep "DLL Name"
```

Expected output lists only Windows system DLLs: `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `OPENGL32.dll`.

---

## Project Structure

```
mandelbrot/
├── CMakeLists.txt              Build definition
├── toolchain-mingw.cmake       CMake cross-compilation toolchain for MinGW
├── build-windows.sh            One-shot build script
├── external/
│   ├── glad/                   OpenGL loader (bundled)
│   └── stb_image_write.h       JPEG writer (bundled, single-header)
├── shaders/
│   ├── mandelbrot.vert         Passthrough vertex shader (fullscreen quad)
│   └── mandelbrot.frag         Mandelbrot iteration + smooth coloring (all 6 schemes)
└── src/
    ├── main.cpp                GLFW init, callbacks, main loop
    ├── mandelbrot.h/cpp        Explorer state, coordinate math, and orbit computation
    ├── renderer.h/cpp          OpenGL setup, uniform upload, draw call
    └── ui.h/ui.cpp             ImGui overlay and orbit portal rendering
```

---

## Architecture

### GPU Fragment Shader

A single fullscreen quad (two triangles) covers the viewport. For every pixel, the fragment shader:

1. Maps the pixel's screen coordinate to a point `c` in the complex plane using the current center and zoom uniforms
2. Iterates `z = z² + c` up to `u_maxIter` times
3. Breaks when `|z|² > 4` (escaped)
4. Applies the smooth iteration count formula to eliminate color banding
5. Looks up the final color from the selected color scheme

Because each pixel is independent, this maps perfectly to the GPU's massively parallel architecture.

### Coordinate System

The complex plane is mapped so that:
- The Y axis is flipped to match mathematical convention (+i is up, screen Y is down)
- `zoom` represents fractal units per half-height of the window
- At the default `zoom = 1.5`, the full Mandelbrot set is visible with margin

### Smooth Coloring

Uses the Inigo Quilez formulation:

```glsl
float smooth_i = float(i) - log2(log2(dot(z, z))) + 1.0;
```

This produces a continuous value that eliminates the discrete iteration bands visible in classic escape-time coloring.

---

## Color Schemes

| # | Name | Description |
|---|------|-------------|
| 0 | Classic | HSV hue cycling |
| 1 | Fire | Black → red → orange → yellow → white |
| 2 | Ocean | Black → navy → cyan → white |
| 3 | Grayscale | Perceptually even via sqrt |
| 4 | Psychedelic | High-frequency HSV with phase shift |
| 5 | Ultra | Black → purple → blue → gold → white |

---

## Performance Notes

Tested target: notebook with integrated Intel/AMD Xe graphics.

| Setting | Expected performance |
|---------|---------------------|
| 1024×768, 256 iterations | ~60 fps (VSync limited) |
| 1024×768, 1000 iterations | ~20–30 fps |
| 1024×768, 2000 iterations | ~10–15 fps |

VSync is enabled by default to prevent the GPU from running at 100% when idle. The frame time display in the overlay shows real render cost so you can tune iteration count for your hardware.

**Float precision limit**: at zoom levels beyond ~1e6×, 32-bit float coordinates in the shader begin to show pixelation artifacts. The overlay displays a warning when this threshold is approached. True deep-zoom requires either GLSL `double` (OpenGL 4.0+) or software split-float arithmetic.

---

## Orbit Visualization

Right-click anywhere on the fractal to pick a complex number `c` and watch its orbit — the sequence z₀=0, z₁=z₀²+c, z₂=z₁²+c, … — animate one step at a time.

The visualization appears as a circular portal in the bottom-right corner of the window:

- **Dark background** with subtle Re/Im axes and the escape circle |z|=2 always visible as a reference, independent of the fractal's current zoom level
- **Amber trail** connecting each z_n, fading from dim (older) to bright (recent)
- **White dot** at z₀=0 (the fixed starting point), **cyan dot** for the current step, **red dot** when a point escapes
- **Amber crosshair** marks the picked point `c` both in the portal and on the fractal itself
- Points with |z|>2.35 are clamped to the portal edge so escaped orbits never overflow the circle

The **Orbit Visualization** section in the overlay panel provides:

| Control | Description |
|---------|-------------|
| Play / Pause | Start or freeze the animation |
| Restart | Reset to z₀ and replay |
| Clear | Dismiss the portal |
| Speed slider | 0.5 – 30 iterations per second |
| Loop checkbox | Automatically restart after reaching the final step |

The step counter at the bottom of the portal shows the current iteration and, for escaping points, which step first crossed the boundary.

---

## Possible Future Extensions

- **Double precision** — GLSL `double` (OpenGL 4.0) or DS (double-single) split-float arithmetic for OpenGL 3.3 compatibility, enabling zoom beyond 1e15
- **Keyframe animation** — record (center, zoom) sequences and render offline at high iteration counts
- **Julia set mode** — fix `c` as a parameter and iterate from each pixel position as the starting `z`; the orbit visualization already computes the same iteration so the two features share math

- **Height map / 3D view** — use the smooth iteration value as elevation for a second rendering pass
