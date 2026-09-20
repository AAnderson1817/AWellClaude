"""Reopen source, clean-reimport both delivery models, inspect all mesh data.

Invoke Blender --background --disable-autoexec assets/blender/city-face.blend
--python-exit-code 1 --python tools/art/verify-city-face.py.
"""
import bpy, json, math, hashlib, re, struct
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review'/'city-face-v14'
manifest=json.loads((OUT/'city-face-manifest.json').read_text())
source=bpy.data.scenes['CITY FACE | submerged carved witness']
stage=bpy.data.collections['PRESENTATION | excluded from export']
clay_material=bpy.data.materials.get('REVIEW | neutral clay')
if clay_material is None:
    clay_material=bpy.data.materials.new('REVIEW | neutral clay');clay_material.use_nodes=True
    p=clay_material.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*[((c/255+.055)/1.055)**2.4 for c in (164,167,165)],1)
    p.inputs['Roughness'].default_value=.72
report={'source_reopened':True,'blender_version':bpy.app.version_string,'assets':{},
        'source_file_sha256':hashlib.sha256((OUT/'city-face.blend').read_bytes()).hexdigest(),
        'source_collections':[c.name for c in source.collection.children],
        'scope':'geometry, dimensions, materials and clean package import; artistic judgment separate'}
# Verify the actual six-decimal C representation consumed by the native game.
# Export/reimport alone cannot catch triangles collapsed by numeric formatting.
header=(ROOT/'src'/'generated'/'city_assets.h').read_text()
def float32(value):return struct.unpack('f',struct.pack('f',float(value)))[0]
header_groups=[]
for match in re.finditer(r'static const float (city_(?:face|eye)_\d+)_p\[\] = \{(.*?)\};',header,re.S):
    prefix,raw=match.groups()
    positions=[float32(x.rstrip('f')) for x in raw.replace('\n','').split(',') if x.strip()]
    normals_raw=re.search(r'static const float '+prefix+r'_n\[\] = \{(.*?)\};',header,re.S).group(1)
    normals=[float32(x.rstrip('f')) for x in normals_raw.replace('\n','').split(',') if x.strip()]
    indices_raw=re.search(r'static const unsigned short '+prefix+r'_i\[\] = \{(.*?)\};',header,re.S).group(1)
    indices=[int(x) for x in indices_raw.replace('\n','').split(',') if x.strip()]
    assert len(positions)==len(normals) and len(positions)%3==0 and len(indices)%3==0,prefix
    vertices=[Vector(positions[i:i+3]) for i in range(0,len(positions),3)]
    assert all(math.isfinite(x) for x in positions+normals),prefix
    assert all(.999<sum(n*n for n in normals[i:i+3])<1.001 for i in range(0,len(normals),3)),prefix
    for i in range(0,len(indices),3):
        ids=indices[i:i+3];assert len(set(ids))==3 and max(ids)<len(vertices),(prefix,i,ids)
        a,b,c=[vertices[j] for j in ids];assert (b-a).cross(c-a).length>1e-10,(prefix,i,'collapsed C triangle')
    header_groups.append({'group':prefix,'vertices':len(vertices),'triangles':len(indices)//3,'float32_geometry':'pass'})
assert header_groups,'No embedded face arrays found'
report['embedded_header']={'groups':header_groups,'sha256':hashlib.sha256((ROOT/'src'/'generated'/'city_assets.h').read_bytes()).hexdigest()}
envelope_min=[-3.738533,.024999,-.260002];envelope_max=[3.738533,4.975001,1.136234]
actual=manifest['assets']['city-face']
assert all(a>=b for a,b in zip(actual['bounds_min'],envelope_min)),actual
assert all(a<=b for a,b in zip(actual['bounds_max'],envelope_max)),actual
assert manifest['runtime_anchor']==[25,1,-1.8]
assert manifest['eye_centers_local']==[[-1.5,3.5,.165],[1.5,3.5,.165]]
assert manifest['assets']['city-eye']['triangles']==880,'Eye mesh changed'
report['protected_contract']={'prior_occupied_envelope':'pass','world_anchor':'pass','eye_response_anchors':'pass','eye_triangles':880}
for slug in ('city-face','city-eye'):
    sc=bpy.data.scenes.new('QA clean import '+slug);bpy.context.window.scene=sc
    sc.world=source.world
    for o in stage.objects:
        if o.type in ('LIGHT','CAMERA'):sc.collection.objects.link(o)
    sc.camera=source.camera
    path=ROOT/'public'/'models'/(slug+'.glb')
    bpy.ops.import_scene.gltf(filepath=str(path))
    imported=[o for o in sc.objects if o.type=='MESH']
    triangles=vertices=degenerate=invalid_normals=nonfinite=invalid_corner_normals=0
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
        for n in me.corner_normals:
            if not all(math.isfinite(c) for c in n.vector) or not .95<n.vector.length<1.05:invalid_corner_normals+=1
        materials += [m.name for m in me.materials]
    lo=[min(p[i] for p in bounds) for i in range(3)];hi=[max(p[i] for p in bounds) for i in range(3)]
    dims=[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]]
    expected=manifest['assets'][slug]
    assert max(abs(a-b) for a,b in zip(dims,expected['dimensions']))<.0001,(slug,dims,expected)
    assert triangles==expected['triangles'],(slug,triangles,expected['triangles'])
    assert degenerate==invalid_normals==nonfinite==invalid_corner_normals==0,(slug,degenerate,invalid_normals,nonfinite,invalid_corner_normals)
    assert len(imported)==1,(slug,'unexpected exported objects',[o.name for o in imported])
    report['assets'][slug]={'triangles':triangles,'vertices':vertices,'mesh_objects':len(imported),
        'degenerate_triangles':degenerate,'invalid_normals':invalid_normals,'invalid_corner_normals':invalid_corner_normals,'nonfinite_values':nonfinite,
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
    sc.render.engine='CYCLES';sc.render.threads_mode='FIXED';sc.render.threads=16;sc.cycles.device='CPU';sc.cycles.samples=16;sc.cycles.use_denoising=True
    sc.render.resolution_x=1100;sc.render.resolution_y=800;sc.render.resolution_percentage=100
    sc.view_settings.view_transform='AgX';sc.render.image_settings.file_format='PNG'
    sc.render.filepath=str(REVIEW/(slug+'-reimport.png'));bpy.ops.render.render(write_still=True)
    if slug=='city-face':
        for label,position,clay in [('reimport-front',(0,-15,2.5),False),('reimport-front-clay',(0,-15,2.5),True),('reimport-three-quarter-clay',(7,-15,5.2),True)]:
            sc.camera.location=position;sc.camera.rotation_euler=(Vector((0,0,2.5))-sc.camera.location).to_track_quat('-Z','Y').to_euler()
            sc.view_layers[0].material_override=clay_material if clay else None
            sc.render.filepath=str(REVIEW/(label+'.png'));bpy.ops.render.render(write_still=True)
        sc.camera.location=(0,-15,2.5);sc.camera.rotation_euler=(Vector((0,0,2.5))-sc.camera.location).to_track_quat('-Z','Y').to_euler()
        sc.render.resolution_x=320;sc.render.resolution_y=224
        sc.render.filepath=str(REVIEW/'reimport-target-size-clay.png');bpy.ops.render.render(write_still=True)
(OUT/'city-face-verification.json').write_text(json.dumps(report,indent=2))
print('CITY_FACE_REIMPORT_VERIFIED '+json.dumps(report))
