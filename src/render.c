// render.c -- the palette, the 320x180 target, and the CRT pass over the top.
#include "aw.h"
#include <stdio.h>
#include <math.h>

RenderTexture2D screenRT;
static Shader post;
static int uTexSize;
int lookNew = 1;

// ---------------------------------------------------------------- the palette
// The new look is drawn in these and nothing else: every pixel the composite writes is the
// nearest of them. Ramps, not a spread: five of stone, four of flame (yours), four of the
// city's light (theirs), bone, two of water, one accent. Most of any frame is the first.
const Color PAL[PL_COUNT] = {
    // shadow and stone: violet-blue, never grey
    [PL_VOID]   = {   7,   6,  15, 255 },
    [PL_DEEP]   = {  18,  15,  36, 255 },
    [PL_DARK]   = {  33,  27,  61, 255 },
    [PL_STONE]  = {  58,  49,  96, 255 },
    [PL_STONEL] = {  94,  83, 144, 255 },
    [PL_STONEH] = { 151, 141, 196, 255 },
    [PL_VIOLET] = { 107,  63, 160, 255 },
    // flame: the hunters'
    [PL_WARMD]  = {  74,  30,  34, 255 },
    [PL_WARM]   = { 154,  63,  36, 255 },
    [PL_AMBER]  = { 240, 138,  44, 255 },
    [PL_PEACH]  = { 255, 176, 122, 255 },
    [PL_AMBERH] = { 255, 212, 106, 255 },
    // the city's light
    [PL_COOLD]  = {  12,  48,  64, 255 },
    [PL_COOLM]  = {  22, 112, 106, 255 },
    [PL_CITY]   = {  63, 214, 166, 255 },
    [PL_CITYH]  = { 189, 251, 224, 255 },
    [PL_CYAN]   = { 127, 232, 255, 255 },
    // water
    [PL_WATERD] = {  11,  33,  72, 255 },
    [PL_WATER]  = {  26,  74, 138, 255 },
    [PL_WATERL] = {  78, 168, 216, 255 },
    // growing things
    [PL_MOSSD]  = {  29,  59,  36, 255 },
    [PL_MOSS]   = {  63, 122,  58, 255 },
    [PL_MOSSL]  = { 140, 200,  90, 255 },
    // rose, and bone
    [PL_ROSED]  = { 106,  31,  74, 255 },
    [PL_ACCENT] = { 224,  72, 126, 255 },
    [PL_ROSEL]  = { 255, 158, 192, 255 },
    [PL_BONE]   = { 244, 236, 214, 255 },
};

// ---------------------------------------------------------------- the new look
// The frame is drawn in three layers -- what is lit (albedo), what gives its own light
// (eyes, the lamp's glass), and the far city -- and one pass puts them together. That pass
// is where the look lives, and it follows Animal Well's own rules as its maker describes
// them: lighting posterised into a few hard bands with dithering, never a smooth gradient;
// rim light on every edge that faces a light; and here, one palette.
static RenderTexture2D albedoRT, emisRT, backRT;
static Shader comp;
static int cBack, cEmis, cBake, cLP, cLC, cLN, cPal, cAmb, cBands, cDebug, cLab, cUW, cPalN;
int dbgAlbedo = 0;

static const char *COMP_BODY =
"uniform sampler2D texture0;\n"     // albedo: alpha 0 where the wall is broken
"uniform sampler2D uBack;\n"        // the far city
"uniform sampler2D uEmis;\n"        // what gives its own light
"uniform sampler2D uBake;\n"        // 41x23: r warm/2, g cool/2, b water; a = tile code at texel (tx,ty)
"uniform vec4 uLP[16];\n"           // x, y (room px), radius px, peak
"uniform vec4 uLC[16];\n"           // x: 1 = the city's light
"uniform int uLN;\n"
"uniform vec3 uPal[32];\n"
"uniform vec3 uLab[32];\n"                                   // the palette in Oklab, for matching by eye
"uniform float uUW[32];\n"                                   // 1: allowed under the water
"uniform int uPalN;\n"
"uniform vec3 uAmb;\n"
"uniform float uBands;\n"
"uniform float uDebug;\n"                                   // 1: show the art under flat light
"vec2 uvOf(vec2 p) { return vec2((p.x + 0.5) / 320.0, 1.0 - (p.y + 0.5) / 180.0); }\n"
"float code(vec2 t) {\n"
"  if (t.x < 0.0 || t.x > 39.0 || t.y < 0.0 || t.y > 21.0) return 1.0;\n"
"  return TEX(uBake, (t + 0.5) / vec2(41.0, 23.0)).a;\n"
"}\n"
"float maskAt(vec2 p) {\n"                                   // stone and shelves as drawn: alpha 253
"  if (p.x < 0.0 || p.x > 319.0 || p.y < 2.0 || p.y > 177.0) return 1.0;\n"   // past the room: stone
"  float a = TEX(texture0, uvOf(p)).a;\n"
"  return (a > 0.5 && a < 0.994) ? 1.0 : 0.0;\n"
"}\n"
"float bayer(vec2 p) {\n"
"  vec2 a = mod(floor(p), 4.0); vec2 a1 = mod(a, 2.0); vec2 a2 = floor(a / 2.0);\n"
"  return (4.0 * mod(2.0*a1.x + 3.0*a1.y, 4.0) + mod(2.0*a2.x + 3.0*a2.y, 4.0) + 0.5) / 16.0;\n"
"}\n"
"vec3 lab(vec3 c) {\n"                                       // sRGB to Oklab
"  c = pow(max(c, vec3(0.0)), vec3(2.2));\n"
"  float l = 0.4122214708*c.r + 0.5363325363*c.g + 0.0514459929*c.b;\n"
"  float m = 0.2119034982*c.r + 0.6806995451*c.g + 0.1073969566*c.b;\n"
"  float s = 0.0883024619*c.r + 0.2817188376*c.g + 0.6299787005*c.b;\n"
"  l = pow(l + 1e-6, 1.0/3.0); m = pow(m + 1e-6, 1.0/3.0); s = pow(s + 1e-6, 1.0/3.0);\n"
"  return vec3(0.2104542553*l + 0.7936177850*m - 0.0040720468*s,\n"
"              1.9779984951*l - 2.4285922050*m + 0.4505937099*s,\n"
"              0.0259040371*l + 0.7827717662*m - 0.8086757660*s);\n"
"}\n"
"vec3 nearest(vec3 c, bool uw) {\n"          // by eye, and hue counts double: lit stone keeps the colour of its light
"  vec3 q = lab(c); vec3 best = uPal[0]; float bd = 1000.0;\n"
"  for (int i = 0; i < 32; i++) {\n"
"    if (i >= uPalN) break;\n"
"    if (uw && uUW[i] < 0.5) continue;\n"
"    vec3 d = q - uLab[i]; float e = d.x * d.x + 2.2 * (d.y * d.y + d.z * d.z);\n"
"    if (e < bd) { bd = e; best = uPal[i]; }\n"
"  }\n"
"  return best;\n"
"}\n"
"void main() {\n"
"  vec2 p = floor(vec2(gl_FragCoord.x, 180.0 - gl_FragCoord.y));\n"
"  vec2 rp = p - vec2(0.0, 2.0) + 0.5;\n"
"  vec2 uv = uvOf(p);\n"
"  vec4 em = TEX(uEmis, uv);\n"
"  if (em.a > 0.5) { OUT(nearest(em.rgb, false)); return; }\n"
"  vec4 alb = TEX(texture0, uv);\n"
"  if (alb.a < 0.5) { OUT(TEX(uBack, uv).rgb); return; }\n"
"  float tc = code(floor(rp / 8.0));\n"                       // this pixel's tile: stone, seam, shelf, water, air
"  float al = dot(alb.rgb, vec3(0.30, 0.59, 0.11));\n"
"  if (tc > 0.9 && al > 0.45) { OUT(nearest(alb.rgb, false)); return; }\n"   // the bright of a seam or their glass gives its own light
"  float solid = (alb.a > 0.5 && alb.a < 0.994) ? 1.0 : 0.0;\n"   // the drawn silhouette, not the tile
"  if (uDebug > 0.5) { OUT(nearest(alb.rgb, false)); return; }\n"
"  vec2 n = vec2(0.0); float edge = 0.0;\n"
"  if (solid > 0.5) {\n"
"    if (maskAt(p + vec2(0.0, -1.0)) < 0.5) n += vec2(0.0, -1.0);\n"
"    if (maskAt(p + vec2(0.0,  1.0)) < 0.5) n += vec2(0.0,  1.0);\n"
"    if (maskAt(p + vec2(-1.0, 0.0)) < 0.5) n += vec2(-1.0, 0.0);\n"
"    if (maskAt(p + vec2( 1.0, 0.0)) < 0.5) n += vec2( 1.0, 0.0);\n"
"    if (dot(n, n) > 0.0) edge = 1.0;\n"
"    else {\n"
"      if (maskAt(p + vec2(0.0, -2.0)) < 0.5) n += vec2(0.0, -1.0);\n"
"      if (maskAt(p + vec2(0.0,  2.0)) < 0.5) n += vec2(0.0,  1.0);\n"
"      if (maskAt(p + vec2(-2.0, 0.0)) < 0.5) n += vec2(-1.0, 0.0);\n"
"      if (maskAt(p + vec2( 2.0, 0.0)) < 0.5) n += vec2( 1.0, 0.0);\n"
"      if (dot(n, n) > 0.0) edge = 0.45;\n"
"    }\n"
"    if (edge > 0.0) n = normalize(n);\n"
"    if (edge > 0.0 && code(floor(rp / 8.0)) < 0.75 && n.y > -0.5) edge = 0.0;\n"   // a shelf catches light on its top only
"  }\n"
"  vec4 bk = TEX(uBake, (rp / 8.0 + 0.5) / vec2(41.0, 23.0));\n"
"  float w = bk.r * 2.0, c = bk.g * 2.0, rw = 0.0, rc = 0.0;\n"
"  if (edge > 0.0) {\n"
"    vec4 bo = TEX(uBake, ((rp + n * 6.0) / 8.0 + 0.5) / vec2(41.0, 23.0));\n"
"    rw = bo.r * 2.0 * edge; rc = bo.g * 2.0 * edge;\n"
"    vec2 q = p + n * 2.0;\n"
"    if (q.x >= 0.0 && q.x <= 319.0 && q.y >= 2.0 && q.y <= 177.0 && code(floor(rp / 8.0)) > 0.75 && TEX(texture0, uvOf(q)).a < 0.5) rc += 0.42 * edge;\n"  // stone against the far city: backlit
"  }\n"
"  vec2 own = floor(rp / 8.0);\n"
"  for (int i = 0; i < 16; i++) {\n"
"    if (i >= uLN) break;\n"
"    vec2 d = uLP[i].xy - rp; float dist = length(d); float R = uLP[i].z;\n"
"    if (dist >= R) continue;\n"
"    float f = 1.0 - dist / R; float v = uLP[i].w * f * f;\n"
"    for (int s = 1; s < 24; s++) {\n"
"      vec2 tq = floor((rp + d * (float(s) / 24.0)) / 8.0);\n"
"      if (tq == own) continue;\n"
"      if (code(tq) > 0.75) { v = 0.0; break; }\n"
"    }\n"
"    if (v <= 0.0) continue;\n"
"    float ndl = edge > 0.0 ? max(dot(n, d / max(dist, 0.001)), 0.0) : 0.0;\n"
"    if (uLC[i].x > 0.5) { c += v; rc += v * ndl * edge; } else { w += v; rw += v * ndl * edge; }\n"
"  }\n"
"  float k = solid > 0.5 ? 0.52 : 1.0;\n"                   // stone is a silhouette, lit at its rim
"  w *= k; c *= k;\n"
"  vec3 warm = vec3(1.0, 0.62, 0.28), cool = vec3(0.32, 1.0, 0.84);\n"   // saturated: the light has a colour
"  float I = w + c; vec3 tint = I > 0.001 ? (warm * w + cool * c) / I : warm;\n"
"  float th = alb.a < 0.998 ? 0.0 : bayer(p) - 0.5;\n"      // no dither on drawn things: a clean edge
"  I = max(I - 0.06, 0.0) * 1.15;\n"                         // below a floor, dark is black, not noise
"  float Iq = floor(I * uBands + 0.5 + th * 0.7) / uBands;\n"    // hard bands; dither only across each edge
"  float Rl = max(rw + rc - 0.12, 0.0) * 1.4; vec3 rt = (rw + rc) > 0.001 ? (warm * rw + cool * rc) / (rw + rc) : warm;\n"
"  float Rq = floor(Rl * uBands + 0.5 + th * 0.7) / uBands;\n"
"  vec3 lit = alb.rgb * (uAmb + Iq * tint * 1.65) + rt * Rq * 0.8;\n"
"  lit *= mix(vec3(1.0), vec3(0.55, 0.85, 1.05), bk.b);\n"   // under the water, cold
"  if (solid > 0.5) {\n"                                     // the dark keeps faint shapes:
"    vec3 fl = uPal[1];\n"                                    // unlit stone one step above black,
"    if (edge >= 1.0 && n.y < -0.5) fl = uPal[2];\n"          // and a dim line on every standable top,
"    lit = max(lit, fl);\n"                                   // so the climb reads without a light
"  }\n"
"  if (tc > 0.1 && tc < 0.25) {\n"                            // water: one blue ramp, never a hue per band
"    float L = dot(lit, vec3(0.30, 0.59, 0.11)) + th * 0.03;\n"
"    vec3 o = uPal[18];\n"
"    if (L > 0.065) o = uPal[15];\n"
"    if (L > 0.24) o = uPal[16];\n"
"    if (al > 0.5) o = L > 0.10 ? uPal[16] : uPal[15];\n"     // the surface line: always a shape, bright when lit
"    OUT(o); return;\n"
"  }\n"
"  OUT(nearest(lit, bk.b > 0.5));\n"
"}\n";

#if defined(PLATFORM_WEB)
static const char *COMP_HEAD =
"#version 100\n"
"#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n"
"varying vec2 fragTexCoord;\nvarying vec4 fragColor;\n"
"#define TEX texture2D\n#define OUT(c) gl_FragColor = vec4((c), 1.0)\n";
#else
static const char *COMP_HEAD =
"#version 330\n"
"in vec2 fragTexCoord;\nin vec4 fragColor;\nout vec4 finalColor;\n"
"#define TEX texture\n#define OUT(c) finalColor = vec4((c), 1.0)\n";
#endif


// Everything is authored a stop brighter than it wants to look, because the light
// pass multiplies the whole frame down. A colour here is what a thing would be if
// something were shining directly on it.
Color palVoid     = {   4,   4,   8, 255 };
Color palBack     = {  46,  43,  66, 255 };
Color palBackLit  = {  60,  56,  86, 255 };
Color palRock     = {  58,  54,  76, 255 };
Color palRockDeep = {  32,  30,  44, 255 };
Color palRockLit  = { 106, 101, 134, 255 };
Color palLedge    = {  96,  80,  62, 255 };
Color palLedgeLit = { 158, 136, 104, 255 };
Color palVein     = { 176, 122,  62, 255 };
Color palVeinHot  = { 255, 216, 142, 255 };
Color palMoss     = {  62,  98,  70, 255 };
Color palSkin     = { 224, 216, 196, 255 };
Color palSkinDeep = { 150, 143, 128, 255 };
Color palEye      = { 236, 244, 255, 255 };
Color palPupil    = {  22,  20,  32, 255 };
Color palDrop     = { 130, 168, 196, 255 };
Color palBulb     = { 156, 104, 148, 255 };   // something grown, not cut
Color palBulbLit  = { 224, 178, 214, 255 };
Color palBulbDeep = {  92,  58,  90, 255 };
Color palWater    = {  38,  74, 104, 255 };
Color palWaterLit = { 112, 164, 182, 255 };   // the surface line
Color palWaterFleck={  60, 108, 140, 255 };
Color palSkinWet  = { 124, 134, 132, 255 };   // the part of you that is under
Color palBush     = {  44,  84,  56, 255 };
Color palBushLit  = {  92, 148,  96, 255 };
Color palBerry    = { 204,  84,  96, 255 };
Color palStalk    = {  56, 100,  62, 255 };
Color palLeaf     = {  70, 128,  78, 255 };
Color palPod      = { 214, 112, 150, 255 };   // the fruit: something that would be sweet
Color palPodLit   = { 246, 186, 206, 255 };
Color palPodDeep  = { 120,  50,  80, 255 };
Color palBird     = {  60,  62,  80, 255 };
Color palBirdLight= { 128, 130, 148, 255 };
Color palFur      = {  86,  68,  60, 255 };
Color palFurLight = { 138, 116, 100, 255 };
Color palEyeGreen = { 170, 240, 190, 255 };
Color palLampIron = {  58,  54,  66, 255 };
Color palLampGlass= { 255, 226, 172, 255 };
Color palLampHot  = { 255, 250, 232, 255 };
Color palStone    = {  98,  92,  96, 255 };   // warmer than the walls: it is not part of them
Color palStoneLit = { 150, 142, 140, 255 };
Color palStoneDeep= {  54,  50,  56, 255 };
// the city: dressed stone, a shade greyer and greener than the rock it was cut from
Color palAshlar    = {  64,  66,  82, 255 };
Color palAshlarLit = { 118, 122, 142, 255 };
Color palMortar    = {  40,  40,  54, 255 };
Color palCornice   = {  92,  96, 104, 255 };
Color palCorniceLit= { 152, 158, 164, 255 };
Color palLichen    = { 108, 128, 108, 255 };
Color palDoor      = {  40,  56,  54, 255 };   // the door's face: iron gone green
Color palDoorGroove= {  66, 102,  90, 255 };
Color palCityGlass = { 120, 186, 150, 255 };   // their light. Not yours
Color palCityGlassLit={ 200, 244, 214, 255 };
Color palIron      = {  70,  68,  78, 255 };
Color palRope      = { 132, 104,  72, 255 };
Color palCloth     = {  48, 108, 110, 255 };   // dyed teal, the one saturated thing they left
Color palClothLit  = {  92, 160, 158, 255 };
Color palClay      = { 156,  96,  74, 255 };
Color palClayLit   = { 206, 146, 112, 255 };
Color palBone      = { 214, 206, 186, 255 };
Color palEmber     = { 196,  70,  40, 255 };
Color palFlame     = { 244, 150,  60, 255 };
Color palFlameHot  = { 255, 232, 170, 255 };

// Not an overlay: scanline depth is modulated per pixel by luminosity, so a bright
// pixel blooms across the gap and a dark one sinks into it. X stays bilinear, Y is
// snapped to source pixel centres. The dither at the end is there to break up the
// banding that 8-bit colour puts across a large flat dark field.
#if defined(PLATFORM_WEB)
static const char *FS =
"#version 100\n"
"precision mediump float;\n"
"varying vec2 fragTexCoord;\n"
"varying vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uTexSize;\n"
"void main() {\n"
"  vec2 uv = fragTexCoord;\n"
"  float py = (floor(uv.y * uTexSize.y) + 0.5) / uTexSize.y;\n"
"  vec3 c = texture2D(texture0, vec2(uv.x, py)).rgb;\n"
"  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
"  float f = fract(uv.y * uTexSize.y);\n"
"  float d = abs(f - 0.5) * 2.0;\n"
"  float depth = mix(0.30, 0.04, smoothstep(0.0, 0.70, lum));\n"
"  c *= 1.0 - depth * pow(d, 1.55);\n"
"  vec2 q = uv - 0.5;\n"
"  float vig = clamp(1.0 - dot(q * vec2(1.0, 0.86), q * vec2(1.0, 0.86)) * 0.55, 0.0, 1.0);\n"
"  c *= mix(1.0, vig, 0.50);\n"
"  float dth = fract(sin(dot(floor(gl_FragCoord.xy), vec2(12.9898, 78.233))) * 43758.5453);\n"
"  c += (dth - 0.5) * (1.6 / 255.0);\n"
"  gl_FragColor = vec4(c, 1.0);\n"
"}\n";
#else
static const char *FS =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uTexSize;\n"
"void main() {\n"
"  vec2 uv = fragTexCoord;\n"
"  float py = (floor(uv.y * uTexSize.y) + 0.5) / uTexSize.y;\n"
"  vec3 c = texture(texture0, vec2(uv.x, py)).rgb;\n"
"  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
"  float f = fract(uv.y * uTexSize.y);\n"
"  float d = abs(f - 0.5) * 2.0;\n"
"  float depth = mix(0.30, 0.04, smoothstep(0.0, 0.70, lum));\n"
"  c *= 1.0 - depth * pow(d, 1.55);\n"
"  vec2 q = uv - 0.5;\n"
"  float vig = clamp(1.0 - dot(q * vec2(1.0, 0.86), q * vec2(1.0, 0.86)) * 0.55, 0.0, 1.0);\n"
"  c *= mix(1.0, vig, 0.50);\n"
"  float dth = fract(sin(dot(floor(gl_FragCoord.xy), vec2(12.9898, 78.233))) * 43758.5453);\n"
"  c += (dth - 0.5) * (1.6 / 255.0);\n"
"  finalColor = vec4(c, 1.0);\n"
"}\n";
#endif

void RenderInit(void) {
    screenRT = LoadRenderTexture(GW, GH);
    SetTextureFilter(screenRT.texture, TEXTURE_FILTER_BILINEAR);
    post = LoadShaderFromMemory(0, FS);
    uTexSize = GetShaderLocation(post, "uTexSize");
    albedoRT = LoadRenderTexture(GW, GH);
    emisRT   = LoadRenderTexture(GW, GH);
    backRT   = LoadRenderTexture(GW, GH);
    static char src[12000];
    snprintf(src, sizeof src, "%s%s", COMP_HEAD, COMP_BODY);
    comp = LoadShaderFromMemory(0, src);
    cBack = GetShaderLocation(comp, "uBack");  cEmis = GetShaderLocation(comp, "uEmis");
    cBake = GetShaderLocation(comp, "uBake");  cLP = GetShaderLocation(comp, "uLP");
    cLC = GetShaderLocation(comp, "uLC");      cLN = GetShaderLocation(comp, "uLN");
    cPal = GetShaderLocation(comp, "uPal");    cAmb = GetShaderLocation(comp, "uAmb");
    cBands = GetShaderLocation(comp, "uBands");
    cDebug = GetShaderLocation(comp, "uDebug");
    cLab = GetShaderLocation(comp, "uLab"); cUW = GetShaderLocation(comp, "uUW"); cPalN = GetShaderLocation(comp, "uPalN");
}

void RenderBegin(void) {
    if (LOOK_NEW) { BeginTextureMode(albedoRT); ClearBackground(BLANK); return; }
    BeginTextureMode(screenRT);
    ClearBackground(palVoid);
}

// Change which layer is being drawn: RL_EMIS for things that give their own light, RL_BACK
// for the far city.
void RenderLayer(int which) {
    EndTextureMode();
    BeginTextureMode(which == RL_EMIS ? emisRT : backRT);
    ClearBackground(BLANK);
}

// Put the layers together into the frame. Leaves the frame open for what is drawn over the
// light: your eyes, your lids, the debug tags.
void RenderComposite(void) {
    EndTextureMode();
    BeginTextureMode(screenRT);
    ClearBackground(BLACK);
    static float lp[16 * 4], lc[16 * 4], pal[PAL_MAX * 3], labv[PAL_MAX * 3], uw[PAL_MAX];
    static int labDone;
    int n = LightPoints(lp, lc, 16);
    if (!labDone) {
        // The palette in Oklab, once, the same way the shader converts a pixel.
        for (int i = 0; i < PL_COUNT; i++) {
            float c[3] = { PAL[i].r / 255.0f, PAL[i].g / 255.0f, PAL[i].b / 255.0f };
            pal[i*3] = c[0]; pal[i*3+1] = c[1]; pal[i*3+2] = c[2];
            for (int k = 0; k < 3; k++) c[k] = powf(c[k], 2.2f);
            float l = 0.4122214708f*c[0] + 0.5363325363f*c[1] + 0.0514459929f*c[2];
            float m = 0.2119034982f*c[0] + 0.6806995451f*c[1] + 0.1073969566f*c[2];
            float s = 0.0883024619f*c[0] + 0.2817188376f*c[1] + 0.6299787005f*c[2];
            l = cbrtf(l + 1e-6f); m = cbrtf(m + 1e-6f); s = cbrtf(s + 1e-6f);
            labv[i*3]   = 0.2104542553f*l + 0.7936177850f*m - 0.0040720468f*s;
            labv[i*3+1] = 1.9779984951f*l - 2.4285922050f*m + 0.4505937099f*s;
            labv[i*3+2] = 0.0259040371f*l + 0.7827717662f*m - 0.8086757660f*s;
            // under the water only stone, violet and blue exist
            uw[i] = (i <= PL_STONEH || i == PL_VIOLET || i == PL_WATER || i == PL_WATERL || i == PL_WATERD || i == PL_CYAN) ? 1.0f : 0.0f;
        }
        labDone = 1;
    }
    int palN = PL_COUNT;
    float amb[3] = { 0.10f, 0.085f, 0.20f }, bands = 5.0f;     // an indigo dark, not a grey one
    BeginShaderMode(comp);
        SetShaderValueTexture(comp, cBack, backRT.texture);
        SetShaderValueTexture(comp, cEmis, emisRT.texture);
        SetShaderValueTexture(comp, cBake, LightBakeTexture());
        SetShaderValueV(comp, cLP, lp, SHADER_UNIFORM_VEC4, 16);
        SetShaderValueV(comp, cLC, lc, SHADER_UNIFORM_VEC4, 16);
        SetShaderValue(comp, cLN, &n, SHADER_UNIFORM_INT);
        SetShaderValueV(comp, cPal, pal, SHADER_UNIFORM_VEC3, PL_COUNT);
        SetShaderValueV(comp, cLab, labv, SHADER_UNIFORM_VEC3, PL_COUNT);
        SetShaderValueV(comp, cUW, uw, SHADER_UNIFORM_FLOAT, PL_COUNT);
        SetShaderValue(comp, cPalN, &palN, SHADER_UNIFORM_INT);
        SetShaderValue(comp, cAmb, amb, SHADER_UNIFORM_VEC3);
        SetShaderValue(comp, cBands, &bands, SHADER_UNIFORM_FLOAT);
        float dbg = (float)dbgAlbedo;
        SetShaderValue(comp, cDebug, &dbg, SHADER_UNIFORM_FLOAT);
        DrawTexturePro(albedoRT.texture, (Rectangle){ 0, 0, (float)GW, -(float)GH },
                       (Rectangle){ 0, 0, (float)GW, (float)GH }, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
}

void RenderPresent(void) {
    EndTextureMode();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int s = sw / GW, sy = sh / GH;
    if (sy < s) s = sy;
    if (s < 1) s = 1;
    int ox = (sw - GW * s) / 2, oy = (sh - GH * s) / 2;
    float ts[2] = { (float)GW, (float)GH };

    BeginDrawing();
        ClearBackground(BLACK);
        SetShaderValue(post, uTexSize, ts, SHADER_UNIFORM_VEC2);
        BeginShaderMode(post);
            DrawTexturePro(screenRT.texture,
                (Rectangle){ 0, 0, (float)GW, -(float)GH },
                (Rectangle){ (float)ox, (float)oy, (float)(GW * s), (float)(GH * s) },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
    EndDrawing();
}
