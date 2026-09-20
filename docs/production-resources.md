# Production resource allowance

The user authorized up to 75% CPU/GPU utilization within safe limits on 2026-09-20.
This is a ceiling, not a requirement to keep the machine saturated. The current
host exposes 24 logical CPUs and an NVIDIA RTX 5080. Blender CPU authoring/baking
runs use `--background -t 16`, leaving headroom for the editor, browser and system.
Do not run competing heavy renders during a performance measurement.

Monitor CPU with the Windows total-processor performance counter and GPU load,
temperature and power with `nvidia-smi`. Stagger or reduce our jobs as the ceiling
is approached; do not change GPU power limits, fan policy, or operating-system
security settings. Sampled observations are not a hardware-enforced utilization
cap. See `evidence/architecture/resource-observation.json` for one recorded sample.
