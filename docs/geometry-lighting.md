# Geometry contact shading

The renderer now captures the visible opaque geometry into a 960×540 buffer.
Octahedrally encoded world normals occupy RG; a two-channel world-Z value occupies
BA. The buffer disables color blending while writing these values. It uses the
same fixed camera and poses as the 1920×1080 color pass. Background paintings,
atmosphere, water and response particles do not contribute false geometry.

A screen-space pass reconstructs position using the calibrated camera, samples
16 neighbors within 0.67 tile, and darkens only neighboring geometry above the
surface's tangent plane. Attenuation ends at 0.95 tile and is bounded to 42%.
This replaces the previous world-Z darkening, which did not measure contacts.
Water absorption/reflection and tone mapping follow the contact pass. The
simulation advances only once; the extra draw reads existing detached snapshots.

This is local screen-space ambient occlusion, not ray tracing, physically complete
indirect light or cast shadows. Offscreen and hidden objects cannot occlude it.
Half-resolution edges and very thin parts need visual review. No temporal noise,
camera motion or additional flashes are introduced.

`tools/capture-architecture-win.ps1` compiles the exact same source and geometry
with and without `AWELL_DEPTH_NO_AO`. It captures the two rooms at the same fixed
frame with ambient movement held still, then measures 600 real frame-loop wall
times after 120 warmup frames. The comparison control is developer-only and does
not change any player rule. These are local host throughput observations, not
GPU-only timings or a cross-hardware performance guarantee. The generated report
records the source and asset hashes so later revisions cannot inherit acceptance.
