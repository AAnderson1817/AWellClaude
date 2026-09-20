"""Reopen/import checks plus dense projected-mask regression against source rooms."""
import bpy,json,hashlib,math,re
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'blender'
manifest=json.loads((OUT/'terrain-manifest.json').read_text());maps=manifest['mask_source']
room_text=(ROOT/'src'/'room.c').read_text().split('static const char *MAPS')[1].split('u8  tiles')[0]
current_rows=re.findall(r'"([#.*~,bofmsP\-]+)"',room_text)
assert current_rows==maps[0]+maps[1], 'Source maps changed; regenerate terrain before verification.'
source=bpy.data.scenes['TERRAIN | two existing rooms'];stage=bpy.data.collections['PRESENTATION | terrain review']
report={'source_reopened':True,'source_room_hash':manifest['room_source_sha256'],'assets':{}}
for room,slug in enumerate(('vault-terrain','drowned-terrain')):
    sc=bpy.data.scenes.new('Clean terrain verification '+slug);bpy.context.window.scene=sc;sc.world=source.world
    sc.collection.children.link(stage);sc.camera=source.camera
    path=ROOT/'public'/'models'/(slug+'.glb');bpy.ops.import_scene.gltf(filepath=str(path))
    obs=[o for o in sc.objects if o.type=='MESH'];verts=[];faces=[];triangles=0;degen=0;badnorm=0
    for o in obs:
        me=o.data;me.calc_loop_triangles();start=len(verts);verts.extend(o.matrix_world@v.co for v in me.vertices)
        for t in me.loop_triangles:
            face=tuple(start+i for i in t.vertices);faces.append(face);triangles+=1
            a,b,c=[verts[i] for i in face]
            if (b-a).cross(c-a).length<1e-10:degen+=1
        for v in me.vertices:
            if v.normal.length<.95 or not all(math.isfinite(vv) for vv in (*v.co,*v.normal)):badnorm+=1
    tree=BVHTree.FromPolygons(verts,faces,all_triangles=True);failures=[];samples=0
    for y in range(22):
        for x in range(40):
            expected=maps[room][y][x] in '#*'
            for fx in (.04,.50,.96):
                for fy in (.04,.50,.96):
                    hit=tree.ray_cast(Vector((x+fx,-10,22-y-fy)),Vector((0,1,0)),20)[0] is not None;samples+=1
                    if hit!=expected:failures.append({'tile':[x,y],'fraction':[fx,fy],'expected':expected,'hit':hit})
    assert not failures,(slug,failures[:10]);assert degen==0,(slug,degen);assert badnorm==0,(slug,badnorm)
    assert triangles==manifest['assets'][slug]['triangles'],(slug,triangles)
    assert max(-v.y for v in verts)<.00001
    report['assets'][slug]={'triangles':triangles,'vertices':len(verts),'degenerate_triangles':degen,'invalid_normals':badnorm,'projected_mask_samples':samples,'mask_mismatches':len(failures),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'external_dependencies':[]}
    cam=sc.camera;center=Vector((20,0,11));cam.location=(20,-48,14);cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=41
    sc.render.engine='CYCLES';sc.cycles.samples=20;sc.cycles.use_denoising=True
    sc.render.resolution_x=1440;sc.render.resolution_y=810;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG'
    sc.render.filepath=str(OUT/'review'/(slug+'-runtime-reimport.png'));bpy.ops.render.render(write_still=True)
(OUT/'terrain-verification.json').write_text(json.dumps(report,indent=2))
print('TERRAIN_VERIFIED '+json.dumps(report))
