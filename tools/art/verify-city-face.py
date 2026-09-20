"""Reopen source, clean-reimport both delivery models, inspect all mesh data.

Invoke Blender --background --disable-autoexec assets/blender/city-face.blend
--python-exit-code 1 --python tools/art/verify-city-face.py.
"""
import bpy, json, math, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review'/'city-face'
manifest=json.loads((OUT/'city-face-manifest.json').read_text())
source=bpy.data.scenes['CITY FACE | submerged carved witness']
stage=bpy.data.collections['PRESENTATION | excluded from export']
report={'source_reopened':True,'blender_version':bpy.app.version_string,'assets':{},
        'source_file_sha256':hashlib.sha256((OUT/'city-face.blend').read_bytes()).hexdigest(),
        'source_collections':[c.name for c in source.collection.children],
        'scope':'geometry, dimensions, materials and clean package import; artistic judgment separate'}
for slug in ('city-face','city-eye'):
    sc=bpy.data.scenes.new('QA clean import '+slug);bpy.context.window.scene=sc
    sc.world=source.world
    for o in stage.objects:
        if o.type in ('LIGHT','CAMERA'):sc.collection.objects.link(o)
    sc.camera=source.camera
    path=ROOT/'public'/'models'/(slug+'.glb')
    bpy.ops.import_scene.gltf(filepath=str(path))
    imported=[o for o in sc.objects if o.type=='MESH']
    triangles=vertices=degenerate=invalid_normals=nonfinite=0
    bounds=[];materials=[]
    for o in imported:
        me=o.data;me.calc_loop_triangles();triangles+=len(me.loop_triangles);vertices+=len(me.vertices)
        bounds.extend(o.matrix_world@v.co for v in me.vertices)
        for t in me.loop_triangles:
            a,b,c=[me.vertices[i].co for i in t.vertices]
            if (b-a).cross(c-a).length<1e-10:degenerate+=1
        for v in me.vertices:
            if not all(math.isfinite(c) for c in (*v.co,*v.normal)):nonfinite+=1
            if v.normal.length<.95:invalid_normals+=1
        materials += [m.name for m in me.materials]
    lo=[min(p[i] for p in bounds) for i in range(3)];hi=[max(p[i] for p in bounds) for i in range(3)]
    dims=[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]]
    expected=manifest['assets'][slug]
    assert max(abs(a-b) for a,b in zip(dims,expected['dimensions']))<.0001,(slug,dims,expected)
    assert triangles==expected['triangles'],(slug,triangles,expected['triangles'])
    assert degenerate==invalid_normals==nonfinite==0,(slug,degenerate,invalid_normals,nonfinite)
    assert len(imported)==1,(slug,'unexpected exported objects',[o.name for o in imported])
    report['assets'][slug]={'triangles':triangles,'vertices':vertices,'mesh_objects':len(imported),
        'degenerate_triangles':degenerate,'invalid_normals':invalid_normals,'nonfinite_values':nonfinite,
        'dimensions_xyz':dims,'material_names':materials,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
        'bytes':path.stat().st_size,'dimension_check':'pass','triangle_count_check':'pass','scoped_export_check':'pass'}
    if slug=='city-face':
        # Eye delivery geometry is genuinely reimported, rather than a source
        # duplicate, so the whole static assembly is visible for comparison.
        for x in (-1.5,1.5):
            before=set(sc.objects)
            bpy.ops.import_scene.gltf(filepath=str(ROOT/'public'/'models'/'city-eye.glb'))
            for o in set(sc.objects)-before:
                if o.type=='MESH':o.location=(x,-.165,3.5)
        sc.camera.location=(7,-15,5.2);target=Vector((0,0,2.5));sc.camera.data.ortho_scale=8.6
    else:
        sc.camera.location=(.4,-2,.3);target=Vector((0,0,0));sc.camera.data.ortho_scale=.66
    sc.camera.rotation_euler=(target-sc.camera.location).to_track_quat('-Z','Y').to_euler()
    sc.render.engine='CYCLES';sc.cycles.samples=24;sc.cycles.use_denoising=True
    sc.render.resolution_x=1100;sc.render.resolution_y=800;sc.render.resolution_percentage=100
    sc.view_settings.view_transform='AgX';sc.render.image_settings.file_format='PNG'
    sc.render.filepath=str(REVIEW/(slug+'-reimport.png'));bpy.ops.render.render(write_still=True)
(OUT/'city-face-verification.json').write_text(json.dumps(report,indent=2))
print('CITY_FACE_REIMPORT_VERIFIED '+json.dumps(report))
