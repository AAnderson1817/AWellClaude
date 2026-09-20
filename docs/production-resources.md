# Production resource allowance

The user authorized up to 75% CPU/GPU utilization within safe limits on 2026-09-20.
This is a ceiling, not a requirement to keep the machine saturated. The current
host exposes 24 logical CPUs and an NVIDIA RTX 5080. Blender CPU authoring/baking
runs use `--background -t 16`, leaving headroom for the editor, browser and system.
Do not run competing heavy renders during a performance measurement.

For subsequent concurrent work, our PowerShell tool processes set their own
`ProcessorAffinity` to `65535` (the same first 16 logical CPUs). Child compilers,
Blender and capture processes inherit that mask. Together those jobs can occupy
at most 16 of this host's 24 logical processors; Blender also retains its 16-thread
limit. This applies only to our tool processes, not the user's applications or
global system policy. It is specific to this host and is not a portable build
requirement. Other applications can still contribute to total utilization, so
sampling and render-slot coordination remain necessary.

The native comparison harness now rests 4 ms between frames, outside its timed
`Frame()` call, in both comparison builds. This reduces sustained GPU demand
during review. Its timings describe this bounded local workload; they are not
maximum-throughput results or directly interchangeable with earlier unrestricted
frame-loop measurements.
Rendered scripted game runs also retain the normal 60 fps ceiling. Their fixed
simulation tick is unchanged; headless route checks remain unrestricted.

Monitor CPU with the Windows total-processor performance counter and GPU load,
temperature and power with `nvidia-smi`. Stagger or reduce our jobs as the ceiling
is approached; do not change GPU power limits, fan policy, or operating-system
security settings. Sampled observations are not a hardware-enforced utilization
cap. See `evidence/architecture/resource-observation.json` for one recorded sample.
