"""Build the existing game's sealed spiral door and embedded, static city scenery.
Run after blender_build_assets.py, using the saved source as input. All assets are
decorative; no geometry, interaction, room, or light-rule change is implied.
"""
import bpy, bmesh, math, json, struct
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets'/'blender'; REVIEW=OUT/'review'; MODELS=ROOT/'public'/'models'
HEADER=ROOT/'src'/'generated'/'foundry_assets.h';HEADER.parent.mkdir(parents=True,exist_ok=True)
scene=bpy.data.scenes['Celestial_Foundry_Source'];bpy.context.window.scene=scene
for other_scene in list(bpy.data.scenes):
    if other_scene!=scene:bpy.data.scenes.remove(other_scene)
TAU=math.tau
def mat(prefix):return next(m for m in bpy.data.materials if m.name.startswith(prefix))
clay_material=next((m for m in bpy.data.materials if m.name.startswith('REVIEW | neutral')),None)
if clay_material is None:
    clay_material=bpy.data.materials.new('REVIEW | neutral clay');clay_material.use_nodes=True
    p=next(n for n in clay_material.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    p.inputs['Base Color'].default_value=(.40,.43,.45,1);p.inputs['Roughness'].default_value=.65
clay_material.use_fake_user=True
bronze,brass,oxide,stone,dark,cyan,amber=[mat('%02d'%i) for i in range(1,8)]
# The premise's palette is a semantic contract. City emissions are green-white.
for m in (cyan,amber):
    p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    p.inputs['Base Color'].default_value=(.34,.68,.45,1)
    p.inputs['Emission Color'].default_value=(.34,.68,.45,1)
    p.inputs['Emission Strength'].default_value=.45
    m.diffuse_color=(.34,.68,.45,1)
amber.name='07 | city green-white glass'
# The earlier blue enamel is replaced by the same city-owned green-white family.
cyan.name='06 | city green-white enamel'
for m,c in [(bronze,(.23,.16,.073)),(brass,(.44,.31,.13)),(oxide,(.036,.13,.092)),(stone,(.042,.061,.064))]:
    p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');p.inputs['Base Color'].default_value=(*c,1);m.diffuse_color=(*c,1)
    if m==bronze:p.inputs['Roughness'].default_value=.43

existing=bpy.data.collections.get('ASSET | Sealed Spiral Door')
if existing:
    for o in list(existing.objects):bpy.data.objects.remove(o,do_unlink=True)
    bpy.data.collections.remove(existing)
door=bpy.data.collections.new('ASSET | Sealed Spiral Door');scene.collection.children.link(door)
def bind(o,name,material):
    for c in list(o.users_collection):c.objects.unlink(o)
    door.objects.link(o);o.name=name;o.data.materials.append(material);return o
def mesh(name,verts,faces,material,bevel=0,smooth=True):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(me);bm.free()
    o=bind(bpy.data.objects.new(name,me),name,material)
    for p in me.polygons:p.use_smooth=smooth
    if bevel:
        m=o.modifiers.new('Soft carved edge','BEVEL');m.width=bevel;m.segments=2
    return o
def box(name,loc,scale,material,bevel=.04):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bind(bpy.context.object,name,material);o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    m=o.modifiers.new('Soft block edge','BEVEL');m.width=bevel;m.segments=2
    return o
def annular_sector(name,r0,r1,t0,t1,depth,front,center,material,n=20,bevel=.018):
    verts=[];faces=[]
    for i in range(n+1):
        a=t0+(t1-t0)*i/n
        for r,y in ((r0,front),(r1,front),(r1,front+depth),(r0,front+depth)):
            verts.append((center[0]+r*math.cos(a),y,center[1]+r*math.sin(a)))
    for i in range(n):
        for j in range(4):faces.append((i*4+j,(i+1)*4+j,(i+1)*4+(j+1)%4,i*4+(j+1)%4))
    faces.append((3,2,1,0));faces.append(tuple(n*4+j for j in range(4)))
    return mesh(name,verts,faces,material,bevel)
def extrude_shape(name,coords,front,depth,material,bevel=.02):
    n=len(coords);verts=[(x,y,z) for y in (front,front+depth) for x,z in coords]
    faces=[tuple(reversed(range(n))),tuple(n+i for i in range(n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,verts,faces,material,bevel,False)
def ribbon(name,points,width,depth,material):
    verts=[];faces=[]
    for i,p in enumerate(points):
        p=Vector(p);d=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])).normalized()
        side=Vector((-d.z,0,d.x))*width*.5
        for q in (p-side,p+side,p+side+Vector((0,depth,0)),p-side+Vector((0,depth,0))):verts.append(q)
    for i in range(len(points)-1):
        for j in range(4):faces.append((i*4+j,(i+1)*4+j,(i+1)*4+(j+1)%4,i*4+(j+1)%4))
    faces.append((3,2,1,0));n=(len(points)-1)*4;faces.append((n,n+1,n+2,n+3))
    return mesh(name,verts,faces,material,.006)

# The source sprite spans exactly five tiles by six; round head, sealed face, flat foot.
# Keep silhouette within x[-2.5,2.5], y(source up)[0,6]. Front towards Blender -Y.
archz=3.5
shape=[(-2.48,.12)]
shape += [(2.48*math.cos(math.pi-i*math.pi/64),archz+2.48*math.sin(math.pi-i*math.pi/64)) for i in range(65)]
shape += [(2.48,.12)]
extrude_shape('Door sealed backing - never opens',shape,-.03,.24,dark,.018)
inner=[(-2.06,.25)]
inner += [(2.06*math.cos(math.pi-i*math.pi/64),archz+2.06*math.sin(math.pi-i*math.pi/64)) for i in range(65)]
inner += [(2.06,.25)]
extrude_shape('Sealed cast metal face',inner,-.30,.22,bronze,.025)

# Individual keyed voussoirs: visible quiet seams, weight seated on engaged jambs.
for i in range(11):
    a0=i*math.pi/11+.012;a1=(i+1)*math.pi/11-.012
    annular_sector('Arch keyed stone %02d'%i,2.10,2.48,a0,a1,.49,-.37,(0,archz),stone,8,.025)
annular_sector('Inner arch bronze reveal',2.035,2.106,0,math.pi,.10,-.418,(0,archz),brass,56,.012)
annular_sector('Outer arch worn raised rim',2.43,2.488,0,math.pi,.055,-.412,(0,archz),stone,56,.012)
for s in (-1,1):
    for i in range(4):
        box('Engaged jamb block',(s*2.30,-.105,.51+i*.845),(.38,.52,.81),stone,.035)
    box('Bronze jamb reveal',(s*2.07,-.41,1.875),(.07,.09,3.23),brass,.012)
    for z in (.40,1.60,2.80):
        box('Recessed jamb staple',(s*2.30,-.391,z),(.19,.035,.085),dark,.012)
        box('Staple rubbed face',(s*2.30,-.417,z),(.13,.027,.046),brass,.007)
box('Single sealed threshold',(0,-.13,.10),(4.99,.75,.20),stone,.038)
box('Threshold metal step',(0,-.405,.225),(4.12,.20,.065),brass,.015)

# Six interlocking, curved cast petals form a real shallow relief over an unbroken door.
# Each patch has thickness, a rounded lip, and inward taper. Gaps reveal sealed bronze.
cz=2.86; radial_steps=20;cross_steps=6
for petal in range(6):
    verts=[];faces=[]
    for i in range(radial_steps+1):
        u=i/radial_steps;r=.22+1.67*u;twist=1.3*(1-u)**1.25
        for j in range(cross_steps+1):
            v=j/cross_steps;a=petal*TAU/6+twist+(v-.5)*(.98-.14*(1-u))
            relief=.045+.12*math.sin(math.pi*v)*math.sin(math.pi*u*.92)
            verts.append((r*math.cos(a),-.322-relief,cz+r*math.sin(a)))
    width=cross_steps+1
    for i in range(radial_steps):
        for j in range(cross_steps):
            q=i*width+j;faces.append((q,q+1,q+1+width,q+width))
    o=mesh('Spiral petal casting %d'%petal,verts,faces,bronze if petal%2 else brass,0)
    solid=o.modifiers.new('Cast plate thickness','SOLIDIFY');solid.thickness=.050
    bevel=o.modifiers.new('Soft plate edge','BEVEL');bevel.width=.011;bevel.segments=2
    pts=[]
    for i in range(33):
        u=i/32;r=.25+1.60*u;a=petal*TAU/6+1.3*(1-u)**1.25-.5*(.98-.14*(1-u))
        pts.append((r*math.cos(a),-.394,cz+r*math.sin(a)))
    ribbon('Recessed spiral patina %d'%petal,pts,.039,.022,oxide)

# The long continuous spiral matches the existing one-time light glint's reading.
pts=[]
for i in range(145):
    t=i/144;a=-1.55+t*TAU*1.57;r=.22+1.71*t
    u=min(1,max(0,(r-.22)/1.67));twist=1.3*(1-u)**1.25
    local=(a-twist+math.pi/6)%(TAU/6)-math.pi/6
    v=local/(.98-.14*(1-u))+.5
    relief=.045+.12*math.sin(math.pi*max(0,min(1,v)))*math.sin(math.pi*u*.92)
    pts.append((r*math.cos(a),-.322-relief-.020,cz+r*math.sin(a)))
# Six curved, patinated petal seams carry the spiral. A former continuous bead
# crossed the relief as apparent floating fragments and was removed after review.
annular_sector('Central glass bezel',.19,.29,0,TAU,.09,-.535,(0,cz),dark,48,.012)
annular_sector('Rubbed bezel rim',.183,.207,0,TAU,.044,-.57,(0,cz),brass,48,.006)
bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,radius=1,location=(0,-.51,cz))
o=bind(bpy.context.object,'City green-white glass hub',cyan);o.scale=(.182,.115,.182)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
for p in o.data.polygons:p.use_smooth=True
# Calm lower field and a single upper astral crest. No text or introduced affordance.
for j in range(3):
    z=5.02+j*.15
    box('Astral crest mark',(0,-.341,z),(.21-j*.048,.035,.035),oxide,.010)

# Export each asset through evaluated delivery copies. Decimation is delivery-only.
source_assets=[('vault-door',door,'FOUNDRY_VAULT_DOOR',.38),('orrery',bpy.data.collections['ASSET | Orrery'],'FOUNDRY_ORRERY',.22)]
manifest={'version':bpy.app.version_string,'premise':'claude/PREMISE.md and claude/ROOMS.md','scope':'sealed door and decorative city orrery; no bell gameplay','source_z_up':True,'runtime_axes':'x right, y up, front +z','embedded_assets':{}}
lines=['/* Generated by scripts/blender_city_assets.py. Do not hand edit. */','#ifndef FOUNDRY_ASSETS_H','#define FOUNDRY_ASSETS_H','typedef struct FoundryMeshData { const float *positions; const float *normals; const unsigned short *indices; int vertex_count; int index_count; unsigned char color[4]; float metallic; float roughness; float emission; } FoundryMeshData;','typedef struct FoundryAssetData { const FoundryMeshData *meshes; int mesh_count; float size[3]; } FoundryAssetData;']
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
def fmt(v):
    s=f'{v:.6f}'.rstrip('0').rstrip('.')
    return (s if '.' in s else s+'.0')+'f'
def arr(name,ctype,values,cols=12):
    lines.append('static const '+ctype+' '+name+'[] = {')
    for i in range(0,len(values),cols):lines.append(','.join(fmt(x) if ctype=='float' else str(x) for x in values[i:i+cols])+',')
    lines.append('};')

for slug,col,ident,ratio in source_assets:
    for c in scene.collection.children:c.hide_viewport=False
    scene.view_layers[0].update();deps=bpy.context.evaluated_depsgraph_get()
    delivery=bpy.data.collections.new('DELIVERY | '+slug);scene.collection.children.link(delivery)
    copies=[]
    for src in col.objects:
        if src.type!='MESH':continue
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        o=bpy.data.objects.new(src.name+' delivery',me);delivery.objects.link(o);o.matrix_world=src.matrix_world.copy()
        for x in scene.objects:x.select_set(False)
        o.select_set(True);bpy.context.view_layer.objects.active=o
        dec=o.modifiers.new('Runtime detail reduction','DECIMATE');dec.ratio=1.0 if src.name.startswith('Spiral petal casting') else ratio
        bpy.ops.object.modifier_apply(modifier=dec.name)
        # Remove collapsed bevel slivers before producing either delivery format.
        bm=bmesh.new();bm.from_mesh(o.data)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        bmesh.ops.dissolve_degenerate(bm,dist=.0000001,edges=list(bm.edges))
        dead=[f for f in bm.faces if f.calc_area()<1e-10]
        if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
        bm.to_mesh(o.data);bm.free();o.data.update()
        # Preserve the source's broad manufactured highlights across reduced topology.
        transfer=o.modifiers.new('Preserve authored source normals','DATA_TRANSFER')
        transfer.object=src;transfer.use_loop_data=True;transfer.data_types_loops={'CUSTOM_NORMAL'}
        transfer.loop_mapping='POLYINTERP_NEAREST'
        bpy.ops.object.modifier_apply(modifier=transfer.name)
        copies.append(o)
    # Join by material to bound draw calls. Build C arrays directly from final evaluated mesh.
    groups={};lo=[float('inf')]*3;hi=[-float('inf')]*3
    for o in copies:
        me=o.data;me.calc_loop_triangles();normal_matrix=o.matrix_world.to_3x3().inverted().transposed()
        for tri in me.loop_triangles:
            m=me.materials[tri.material_index];g=groups.setdefault(m.name,{'mat':m,'p':[],'n':[],'idx':[],'map':{}})
            for vi,li in zip(tri.vertices,tri.loops):
                p=o.matrix_world@me.vertices[vi].co
                n=normal_matrix@me.corner_normals[li].vector;n.normalize()
                p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y)
                key=tuple(round(v,6) for v in (*p,*n))
                idx=g['map'].get(key)
                if idx is None:
                    idx=len(g['p'])//3;g['map'][key]=idx;g['p'].extend(p);g['n'].extend(n)
                    for j in range(3):lo[j]=min(lo[j],p[j]);hi[j]=max(hi[j],p[j])
                g['idx'].append(idx)
    descriptors=[];triangles=0
    for k,g in enumerate(groups.values()):
        prefix=slug.replace('-','_')+'_'+str(k);count=len(g['p'])//3
        assert count<65536,(slug,count)
        arr(prefix+'_p','float',g['p']);arr(prefix+'_n','float',g['n']);arr(prefix+'_i','unsigned short',g['idx'])
        n=next(n for n in g['mat'].node_tree.nodes if n.type=='BSDF_PRINCIPLED')
        rgb=[max(0,min(255,round(srgb(c)*255))) for c in n.inputs['Base Color'].default_value[:3]]
        descriptors.append('{'+','.join([prefix+'_p',prefix+'_n',prefix+'_i',str(count),str(len(g['idx'])),'{'+','.join(map(str,rgb+[255]))+'}',fmt(n.inputs['Metallic'].default_value),fmt(n.inputs['Roughness'].default_value),fmt(n.inputs['Emission Strength'].default_value if any(n.inputs['Emission Color'].default_value[:3]) else 0)])+'}')
        triangles+=len(g['idx'])//3
    lines.append('static const FoundryMeshData '+ident+'_MESHES[] = {'+','.join(descriptors)+'};')
    size=[hi[i]-lo[i] for i in range(3)]
    lines.append('static const FoundryAssetData '+ident+' = {'+ident+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(x) for x in size)+'}};')
    for x in scene.objects:x.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0]
    # Merge source parts; glTF subsequently splits into material primitives.
    bpy.ops.object.join();joined=copies[0];joined.name=slug
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    # Scope export to exactly the owned delivery object, including previously hidden source parts.
    for x in scene.objects:x.select_set(False)
    joined.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_apply=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
    manifest['embedded_assets'][slug]={'triangles':triangles,'material_meshes':len(groups),'bounds_min':lo,'bounds_max':hi,'dimensions':size,'role':'sealed non-opening portal' if slug=='vault-door' else 'decorative distant city astronomy','emission':'green-white only','material_names':list(groups)}
lines.append('#endif')
HEADER.write_text('\n'.join(lines)+'\n')
assert sum(a['triangles'] for a in manifest['embedded_assets'].values())<30000
(OUT/'runtime-manifest.json').write_text(json.dumps(manifest,indent=2))

# Remove the factory default scene from this generated file only; keep authored alternatives.
for s in list(bpy.data.scenes):
    if s!=scene:bpy.data.scenes.remove(s)
stage=bpy.data.collections['PRESENTATION | Excluded from export']
for c in scene.collection.children:c.hide_render=c not in (door,stage);c.hide_viewport=c not in (door,stage)
cam=scene.camera;center=Vector((0,0,3));cam.location=center+Vector((6,-15,4));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=6.7
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'celestial-foundry.blend'))
for name,vector,clay in [('door-beauty.png',(5,-15,3),False),('door-front-clay.png',(0,-1,0),True),('door-side-clay.png',(1,-.1,0),True),('door-back-clay.png',(0,1,0),True)]:
    cam.location=center+Vector(vector).normalized()*15;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    scene.view_layers[0].material_override=clay_material if clay else None
    scene.render.resolution_x=scene.render.resolution_y=900;scene.render.filepath=str(REVIEW/name)
    bpy.ops.render.render(write_still=True)
scene.view_layers[0].material_override=None
print('CITY_ASSETS_COMPLETE '+json.dumps(manifest))
