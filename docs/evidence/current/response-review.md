# Rendered response observations

These are observations of actual native game captures from the depth-v10 renderer,
not concept art. All runs use the inherited simulation and fixed-step CLI plans.
The root agent inspected the listed frames at their original output sizes.

| Evidence | Observation | Scope |
|---|---|---|
| `assets/review/responses/bulb/f0008.png` and `f0012.png` | Body descends toward the bulb, then rises above the compressed, brighter dome with contact particles. The crown uses the original six-pixel height. | Visible response in a controlled landing; trace comparison separately confirms original motion. |
| `assets/review/responses/reset/f0035.png` | Bright eye points remain visible over the dimmed scene during the initial eye-closing phase. | Frame evidence for post-fade eye visibility; source preserves half-close/shut thresholds. |
| `assets/review/responses/dead-lamp/f0013.png` | Camp remains dark away from the nearby portable lamp. The pack's dead lamp remains unlit, with a small conditional highlight. | The exact original proximity/phase condition is retained; the wide frame makes this deliberately small acknowledgment subtle. |
| `assets/review/responses/fire-clear/f0060.png`, `f0120.png`, `f0180.png` | The lamp is beside the cold fire, then the fire acquires its persistent warm contribution and event particles. | The lamp overlaps part of the small flame silhouette. Further player observation should check whether this response is sufficiently apparent. |
| `assets/review/depth-v10/1080-vault/f0005.png` | Player, landing lips, city masonry, raw vault, plants, pots and sealed door are visible at 1920×1080. | Full-size visual inspection, not artistic acceptance. |
| `assets/review/depth-v10/1080-drowned/f0120.png` | The body crosses the readable waterline, with submerged tint and reflected silhouette. Solid architecture remains outside the water mask. | Full-size inspection supplements the 1280×720 review. |

The source corrections and independent judgments are recorded in
[artistic-review.md](../../artistic-review.md). This does not certify blind discovery,
comfort, listening quality, every possible object state, or AAA craft.

Earlier black first-frame captures were excluded from the review set: native screen
readback at the first buffer swap did not yet contain the rendered frame. They remain
in the ignored local study archive, not as evidence of an object response.
