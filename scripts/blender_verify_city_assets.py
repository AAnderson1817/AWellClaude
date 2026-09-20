"""Clean GLB reimports, dimensional/geometry checks, and material review renders."""
import bpy, json, math, struct, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review'
source=bpy.data.scenes['Celestial_Foundry_Source'];stage=bpy.data.collections['PRESENTATION | Excluded from export']
runtime=json.loads((OUT/'runtime-manifest.json').read_text())
report={'source_reopened':True,'blender_version':bpy.app.version_string,'source_collections':[c.name for c in source.collection.children],'assets':{}}
for slug in ('vault-door','orrery'):
    sc=bpy.data.scenes.new('QA clean reimport '+slug);bpy.context.window.scene=sc
    sc.world=source.world;sc.collection.children.link(stage);stage.hide_render=False;stage.hide_viewport=False
    path=ROOT/'public'/'models'/(slug+'.glb')
    bpy.ops.import_scene.gltf(filepath=str(path))
    imported=[o for o in sc.objects if o.type=='MESH' and o.name!='Review ground']
    triangles=0;vertices=0;degenerate=0;invalid_normals=0;bounds=[];names=[]
    for o in imported:
        me=o.data;me.calc_loop_triangles();triangles+=len(me.loop_triangles);vertices+=len(me.vertices)
        bounds.extend(o.matrix_world@v.co for v in me.vertices)
        for t in me.loop_triangles:
            a,b,c=[me.vertices[i].co for i in t.vertices]
            if (b-a).cross(c-a).length<1e-10:degenerate+=1
        for v in me.vertices:
            if not all(math.isfinite(c) for c in (*v.co,*v.normal)) or v.normal.length<.95:invalid_normals+=1
        names.extend(m.name for m in me.materials)
    lo=[min(p[i] for p in bounds) for i in range(3)];hi=[max(p[i] for p in bounds) for i in range(3)]
    # Blender importer converts glTF back into source Z-up coordinates.
    dims=[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]]
    expected=runtime['embedded_assets'][slug]['dimensions']
    assert max(abs(a-b) for a,b in zip(dims,expected))<.005,(slug,dims,expected)
    assert triangles==runtime['embedded_assets'][slug]['triangles'],(slug,triangles)
    assert invalid_normals==0,(slug,invalid_normals)
    report['assets'][slug]={'triangles':triangles,'vertices':vertices,'mesh_objects':len(imported),'degenerate_triangles':degenerate,'invalid_normals':invalid_normals,'dimensions_xyz':dims,'material_names':names,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'dimension_check':'pass','triangle_count_check':'pass'}
    cam=source.camera;sc.camera=cam
    center=Vector((0,0,3 if slug=='vault-door' else 2.65));cam.location=center+Vector((4,-15,2) if slug=='vault-door' else (5,-10,4))
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=6.7 if slug=='vault-door' else 6.2
    sc.render.engine=source.render.engine
    if sc.render.engine=='CYCLES':sc.cycles.samples=24;sc.cycles.use_denoising=True
    sc.render.resolution_x=sc.render.resolution_y=900;sc.render.resolution_percentage=100
    sc.render.image_settings.file_format='PNG';sc.render.filepath=str(REVIEW/(slug+'-runtime-reimport.png'))
    bpy.ops.render.render(write_still=True)
(OUT/'verification.json').write_text(json.dumps(report,indent=2))
print('CLEAN_REIMPORT_VERIFIED '+json.dumps(report))
