# Modeled foliage contract

The Vault bush and Drowned fronds replace the six-point leaf fans in the existing
fixed-camera renderer. They retain the existing roots, collision, bush response,
and quiet motion. Runtime sizes are roughly 40–100 pixels at 1080p; close studio
views expose construction defects but do not substitute for that camera review.

The governing references are `25-vault-bush.png` and `11-submerged-fronds.png` in
their respective reference directories. The bush has rooted woody forks,
alternating broad curled leaves, clear petiole attachment, and subdued olive
surfaces. The aquatic plant has a compact root crown and long irregular ribbons,
with a real camber and central rib. Hidden rear surfaces are inferred. Neither
plant uses transparency cards or an SVG image. No externally sourced assets.

Delivery is editable Blender 5.2 source, portable GLB, and embedded indexed mesh
data for the existing C/raylib renderer. X right, Y up, Z toward camera; root pivot
is (0,0,0). Every leaf has thickness; intended separate branches, ribs, and leaves
intersect at their biological attachment points. Source meshes and material
families remain individually editable. Conventional real-time profile, no textures
or external dependencies. Runtime groups must remain below eight materials and
65,536 vertices per group; measured counts are recorded rather than advertised as
proof of quality.

Acceptance requires visibly curved overlapping leaves, rooted branching rather
than a radial icon, restrained tonal variation, and clear gameplay at the actual
camera. Wind bends the upper plant while the root stays fixed. Technical delivery
requires finite indexed geometry, normalized normals, no zero-area triangles,
source reopen and clean export reimport. The artistic state stays work in progress
until reviewed against the references in the actual game.
