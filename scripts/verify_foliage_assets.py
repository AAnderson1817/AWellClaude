"""Source reopen and independent clean GLB import checks; run with Blender."""
import bpy, json, math, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/blender'
source=bpy.data.scenes['FOLIAGE | rooted branches and curled leaves']
manifest=json.loads((OUT/'foliage-manifest.json').read_text())
stage=bpy.data.collections['PRESENTATION | excluded from exports']
report={'source_reopened':True,'blender_version':bpy.app.version_string,
        'source_sha256':hashlib.sha256((OUT/'foliage.blend').read_bytes()).hexdigest(),
        'artistic_state':'work_in_progress','assets':{}}
for slug,expected in manifest['assets'].items():
    sc=bpy.data.scenes.new('REIMPORT | '+slug);bpy.context.window.scene=sc;sc.world=source.world
    for o in stage.objects:
        if o.type in ('CAMERA','LIGHT'):sc.collection.objects.link(o)
    sc.camera=source.camera
    path=ROOT/'public/models'/(slug+'.glb');bpy.ops.import_scene.gltf(filepath=str(path))
    objects=[o for o in sc.objects if o.type=='MESH'];assert len(objects)==1
    triangles=vertices=degenerate=nonfinite=invalid_normals=0;bounds=[];materials=[]
    for o in objects:
        me=o.data;me.calc_loop_triangles();triangles+=len(me.loop_triangles);vertices+=len(me.vertices)
        bounds.extend(o.matrix_world@v.co for v in me.vertices)
        for t in me.loop_triangles:
            a,b,c=[me.vertices[i].co for i in t.vertices]
            if (b-a).cross(c-a).length<1e-11:degenerate+=1
        for v in me.vertices:
            if not all(math.isfinite(c) for c in (*v.co,*v.normal)):nonfinite+=1
            if not .999<v.normal.length<1.001:invalid_normals+=1
        materials.extend(m.name for m in me.materials)
    lo=[min(p[i] for p in bounds) for i in range(3)];hi=[max(p[i] for p in bounds) for i in range(3)]
    dims=[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]]
    assert max(abs(a-b) for a,b in zip(dims,expected['dimensions']))<1e-5
    assert triangles==expected['triangles']
    assert degenerate==nonfinite==invalid_normals==0,(slug,degenerate,nonfinite,invalid_normals)
    report['assets'][slug]={'triangles':triangles,'vertices':vertices,'materials':materials,'dimensions':dims,
        'degenerate':degenerate,'nonfinite':nonfinite,'invalid_normals':invalid_normals,
        'export_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'technical_check':'pass'}
    center=Vector(tuple((hi[i]+lo[i])/2 for i in range(3)))
    sc.camera.location=center+Vector((3,-5,2));sc.camera.rotation_euler=(center-sc.camera.location).to_track_quat('-Z','Y').to_euler()
    sc.camera.data.ortho_scale=1.8;sc.render.engine='CYCLES';sc.cycles.samples=24;sc.cycles.use_denoising=True
    sc.render.resolution_x=sc.render.resolution_y=900;sc.render.resolution_percentage=100
    sc.render.image_settings.file_format='PNG';sc.render.filepath=str(OUT/'review/foliage'/(slug+'-reimport.png'))
    bpy.ops.render.render(write_still=True)
(OUT/'foliage-verification.json').write_text(json.dumps(report,indent=2))
print('FOLIAGE_REIMPORT_VERIFIED '+json.dumps(report))
