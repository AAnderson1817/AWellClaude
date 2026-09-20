# Rock Face 01 source

Rock Face 01 by **Dario Barresi**, supplied by [Poly Haven](https://polyhaven.com/a/rock_face_01) under [CC0](https://polyhaven.com/license). Powered by Poly Haven.

The original Blender file and its 1K base-color, OpenGL normal and roughness dependencies are retained unchanged. `provenance.json` pins their official download URLs, sizes, MD5 and SHA-256 hashes. This is a third-party scan, not a scan authored for this project.

`scripts/blender_scanned_rock.py` imports the mesh as data, preserves an untouched object and an undeformed aligned crop in the terrain source, then fits one existing Vault rock footprint. The delivered game mesh uses the existing basalt material family. These vendor texture files are source references and portable dependencies; they do not replace the game's current material maps.

Open the vendor file with Blender's automatic script execution disabled. The generation commands also use `--disable-autoexec`.
