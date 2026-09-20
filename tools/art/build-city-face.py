"""Authored Drowned Quarter relief: editable stone shell, bronze repair, opal eyes.

Run in an isolated Blender background process. Runtime coordinates are X right,
Y up, Z front; the source is Z up with its front towards Blender -Y.
"""
import bpy, bmesh, math, json, sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from city_face_sculpt import build_sculpt
OUT = ROOT / 'assets' / 'blender'
REVIEW = OUT / 'review' / 'city-face-v14'
MODELS = ROOT / 'public' / 'models'
HEADER = ROOT / 'src' / 'generated' / 'city_assets.h'
REVIEW.mkdir(parents=True, exist_ok=True)
scene = bpy.data.scenes.new('CITY FACE | submerged carved witness')
bpy.context.window.scene = scene
# Factory-startup is a dedicated isolated process. Remove only its other default
# scenes so Blender's exporter cannot capture a selected startup cube elsewhere.
for other in list(bpy.data.scenes):
    if other != scene: bpy.data.scenes.remove(other)
assets = []
current = None

def linear(c):
    c /= 255
    return c/12.92 if c <= .04045 else ((c+.055)/1.055)**2.4

def material(name, rgb, rough=.7, metal=0, emit=0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    n = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    color = tuple(linear(c) for c in rgb)
    n.inputs['Base Color'].default_value = (*color,1)
    n.inputs['Roughness'].default_value = rough
    n.inputs['Metallic'].default_value = metal
    if emit:
        n.inputs['Emission Color'].default_value = (*color,1)
        n.inputs['Emission Strength'].default_value = emit
    m.diffuse_color = (*color,1)
    return m

stone = material('City face | pale submerged limestone', (122,134,124), .88)
stone_dark = material('City face | sheltered mineral tide', (107,130,121), .89)
stone_light = material('City face | worn limestone high planes', (146,154,140), .81)
bronze = material('City face | old cast bronze', (99,88,59), .57, .76)
oxide = material('City face | seated bronze verdigris', (57,89,74), .83, .35)
cut = material('City face | incised shadow', (37,59,54), .91)
glass = material('City eye | opaque green-white opal', (162,226,179), .31, .03, .42)
clay = material('REVIEW | neutral clay', (164,167,165), .72)
clay.use_fake_user=True

def collection(slug, symbol, pivot):
    global current
    current = bpy.data.collections.new('ASSET | '+slug)
    scene.collection.children.link(current)
    assets.append((slug,symbol,current,pivot))
    return current

def xyz(p): return (p[0], -p[2], p[1])

def mesh(name, verts, faces, mats, smooth=True, mi=None):
    me = bpy.data.meshes.new(name)
    me.from_pydata([xyz(p) for p in verts], [], faces)
    me.update()
    bm = bmesh.new(); bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); current.objects.link(o)
    for m in mats if isinstance(mats,list) else [mats]: me.materials.append(m)
    for i,p in enumerate(me.polygons):
        p.use_smooth = smooth
        p.material_index = mi[i] if mi else 0
    uv = me.uv_layers.new(name='Planar relief coordinates')
    for p in me.polygons:
        for li in p.loop_indices:
            v = me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv = (v.x/8+.5, v.z/5)
    return o

def front_point(x,y,offset=0): return (x,y,face_z(x,y)+offset)

def ribbon(name, points, width, depth, mat, bevel=.0025):
    # Thin cast plate with a conforming top grid and a sunk underside. Sampling
    # only the two strip edges caused stone intersections across convex brows.
    # These width samples and length subdivisions follow the actual sculpt BVH.
    path=[]
    for a,b in zip(points,points[1:]):
        va,vb=Vector(a),Vector(b);count=max(1,math.ceil((vb-va).length/.060))
        path.extend(va.lerp(vb,k/count) for k in range(count))
    path.append(Vector(points[-1]))
    verts=[];faces=[];top=max(.026,depth)
    section=[(-.5,-.009),(.5,-.009),(.5,top-.004),(.25,top),(0,top),(-.25,top),(-.5,top-.004)]
    n=len(section)
    for i,p in enumerate(path):
        tangent=(path[min(len(path)-1,i+1)]-path[max(0,i-1)]).normalized()
        side=Vector((-tangent.y,tangent.x))*width
        for u,lift in section:
            q=p+side*u;verts.append(front_point(q.x,q.y,lift))
    for i in range(len(path)-1):
        for k in range(n):faces.append((i*n+k,(i+1)*n+k,(i+1)*n+(k+1)%n,i*n+(k+1)%n))
    faces += [tuple(reversed(range(n))),tuple((len(path)-1)*n+k for k in range(n))]
    return mesh(name,verts,faces,mat)

def disk(name, x,y,rx,ry,z,mat,depth=.03,n=40):
    verts=[(x,y,z+depth)]
    for k in range(n):
        a=math.tau*k/n;verts.append((x+rx*math.cos(a),y+ry*math.sin(a),z))
    verts.append((x,y,z-.04));faces=[]
    for k in range(n): faces.extend([(0,1+k,1+(k+1)%n),(n+1,1+(k+1)%n,1+k)])
    return mesh(name,verts,faces,mat)

facecol=collection('city-face','FOUNDRY_CITY_FACE','base center; add world(25,1,-1.8)')
body,face_z=build_sculpt(facecol,stone)

# The brows follow an orbital path. Each is an inset cast ribbon, not a shelf.
for s in (-1,1):
    # A carved almond cavity darkens toward the lens, matching the undercut eye
    # wells without pretending the entire opening is a luminous eye.
    ev=[];ef=[];rings=5;segments=64
    for j in range(rings+1):
        r=.01+.99*j/rings
        for k in range(segments):
            a=math.tau*k/segments;x=s*1.5+.89*r*math.cos(a)
            y=3.49+.295*r*math.sin(a)*(1-.15*abs(math.cos(a)))
            ev.append(front_point(x,y,.006))
    for j in range(rings):
        for k in range(segments):ef.append((j*segments+k,j*segments+(k+1)%segments,(j+1)*segments+(k+1)%segments,(j+1)*segments+k))
    o=mesh('Carved dark inner eye cavity '+str(s),ev,ef,stone_dark)
    solid=o.modifiers.new('Recess skin thickness','SOLIDIFY');solid.thickness=.008
    for level in (0,1):
        pts=[]
        for k in range(25):
            a=.05+math.pi*.93*k/24
            x=s*(1.5+1.19*math.cos(a));y=3.48+(.59+.14*level)*math.sin(a)+level*.055
            pts.append((x,y))
        ribbon(('Left' if s<0 else 'Right')+' seated orbital brow '+str(level),pts,.105 if level==0 else .060,.016,bronze if level==0 else oxide)
    for level in range(3):
        pts=[]
        for k in range(25):
            inset=.18+level*.27
            a=math.pi+inset+(math.pi-2*inset)*k/24
            x=s*(1.5+(1.07-.05*level)*math.cos(a));y=3.46+(.50+.14*level)*math.sin(a)
            pts.append((x,y))
        ribbon(('Left' if s<0 else 'Right')+' layered lower orbital casting '+str(level),pts,.078-.010*level,.014,bronze if level!=1 else oxide,.002)
    # Long tapered cheek engraving: one clear orbital family, calm low field.
    pts=[(s*(2.10+.23*math.sin(math.pi*k/20)),2.99-1.82*k/20) for k in range(21)]
    ribbon('Cheek meridian inlay '+str(s),pts,.038,.011,oxide,.0015)
    # Recess around the glass lens. Stone/socket remains visible outside it.
    cx=s*1.5;cy=3.5
    ring=[(cx+.265*math.cos(math.tau*k/32),cy+.268*math.sin(math.tau*k/32)) for k in range(33)]
    ribbon('Seated eye lens bronze retaining ring '+str(s),ring,.046,.014,bronze,.002)

# A single eclipse mark is the ritual/astronomy focus above the eyes. The disc
# is physically inserted in a shallow circular seat, with three sunk pin heads.
cy=4.54;cz=face_z(0,cy)+.014
disk('Eclipse bronze disc',0,cy,.224,.224,cz,bronze,.022)
for k in range(31):
    if k==0:pts=[]
    a=math.pi*.16+math.pi*1.55*k/30;pts.append((.29*math.cos(a),cy+.29*math.sin(a)))
ribbon('Partial orbit around eclipse seal',pts,.035,.014,oxide,.002)
for s in (-1,1):
    pts=[(s*(.065+.11*k/14),4.30-.31*k/14) for k in range(15)]
    ribbon('Seal cast stem '+str(s),pts,.035,.014,bronze,.002)
for x in (-.11,0,.11):disk('Seal sunk rivet',x,cy-.02,.027,.027,cz+.022,oxide,.01,n=12)

# A fracture at the right temple is visibly repaired by three short cast staples.
crack=[(2.88,4.16),(2.71,3.93),(2.80,3.74),(2.67,3.53),(2.78,3.25),(2.67,3.03),(2.75,2.80)]
ribbon('Old temple fracture',crack,.023,.012,cut,.002)
for x,y in ((2.77,3.88),(2.72,3.47),(2.73,3.07)):
    pts=[(x-.18,y-.045),(x-.11,y-.062),(x+.10,y+.036),(x+.18,y+.043)]
    ribbon('Cast bronze fracture staple',pts,.086,.023,bronze,.0025)
    for sx,sy in ((x-.14,y-.05),(x+.14,y+.04)):
        disk('Staple seated pin',sx,sy,.023,.023,face_z(sx,sy)+.025,oxide,.007,n=12)

# The mouth is an actual recessed Boolean crease in the stone, with no
# additional painted line or surface strip.

# Dedicated rigid opal lens, centered at its own origin. Runtime places two and
# controls emission; no pulse is baked into source, and no carving vertex moves.
eyecol=collection('city-eye','FOUNDRY_CITY_EYE','lens center; place local ±1.5,3.5,.165')
verts=[];faces=[]
N,R=40,12
for j in range(R+1):
    theta=math.pi*j/R
    for k in range(N):
        a=math.tau*k/N
        verts.append((.213*math.sin(theta)*math.cos(a),.213*math.sin(theta)*math.sin(a),.102*math.cos(theta)))
for j in range(R):
    for k in range(N):faces.append((j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k))
mesh('Solid green-white opal lens',verts,faces,glass)

def fmt(v):
    s=f'{v:.6f}'.rstrip('0').rstrip('.')
    return (s if '.' in s else s+'.0')+'f'
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
lines=['/* Generated by tools/art/build-city-face.py. */','#ifndef CITY_ASSETS_H','#define CITY_ASSETS_H','#include "foundry_assets.h"']
def array(name,ctype,values):
    lines.append('static const '+ctype+' '+name+'[] = {')
    for i in range(0,len(values),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in values[i:i+12])+',')
    lines.append('};')
manifest={'blender_version':bpy.app.version_string,'profile':'conventional real-time static relief','runtime_axes':'X right,Y up,Z front','runtime_anchor':[25,1,-1.8],
          'eye_centers_local':[[-1.5,3.5,.165],[1.5,3.5,.165]],'mouth_local':[0,1.4,face_z(0,1.4)],
          'governing_reference':'public/art/references/drowned-quarter/03-drowned-face-v2.png',
          'interpretation':'continuous mineral sculpture with explicit nasal alae and mouth lofts, irregular architectural burial returns and seated bronze orbital inlays',
          'quality_state':'work_in_progress_pending_native_and_independent_review',
          'editable_source':'skull scaffold, nasal/upper/lower lip control cages, nostril and mouth Boolean cutters, joined surface; separate delivery-only reduction',
          'reduction':{},
          'source':'assets/blender/city-face.blend','budget':{'triangles_max':43000,'material_groups_max':7,'textures':0},'assets':{}}
for slug,symbol,col,pivot in assets:
    bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();groups={};lo=[1e9]*3;hi=[-1e9]*3;copies=[]
    delivery=bpy.data.collections.new('DELIVERY temporary');scene.collection.children.link(delivery)
    for src in col.objects:
        if src.hide_render:continue
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        if 'delivery_head_triangle_target' in src:
            me.calc_loop_triangles();source_tris=len(me.loop_triangles)
            temp=bpy.data.objects.new('DELIVERY only head reduction',me);delivery.objects.link(temp)
            reduction=temp.modifiers.new('Delivery reduction; editable sculpt unchanged','DECIMATE')
            reduction.ratio=min(1,src['delivery_head_triangle_target']/source_tris)
            reduction.use_collapse_triangulate=True
            bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
            reduced=bpy.data.meshes.new_from_object(temp.evaluated_get(deps),depsgraph=deps)
            reduced.calc_loop_triangles()
            manifest['reduction']={'source_head_triangles':source_tris,'delivery_head_triangles':len(reduced.loop_triangles),'method':'delivery-only collapse; source controls and dense sculpt preserved'}
            bpy.data.objects.remove(temp,do_unlink=True);bpy.data.meshes.remove(me);me=reduced
        # Both GLB and C consume this same quantized, cleaned representation.
        # Tiny Boolean/decimation slivers must not become collapsed float arrays.
        for v in me.vertices:
            v.co=tuple(round(c,6) for c in v.co)
            if 'delivery_head_triangle_target' in src:v.co.y=min(.26,v.co.y)
        bm=bmesh.new();bm.from_mesh(me)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=2e-6)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=2e-6)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        dead=[f for f in bm.faces if f.calc_area()<1e-9]
        if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
        deadverts=[v for v in bm.verts if not v.link_faces]
        if deadverts:bmesh.ops.delete(bm,geom=deadverts,context='VERTS')
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(me);bm.free();me.update();me.calc_loop_triangles()
        if not me.uv_layers:
            uv=me.uv_layers.new(name='Planar relief coordinates')
            for poly in me.polygons:
                for li in poly.loop_indices:
                    v=me.vertices[me.loops[li].vertex_index].co;uv.data[li].uv=(v.x/8+.5,v.z/5)
        o=bpy.data.objects.new(src.name+' delivery',me);delivery.objects.link(o);copies.append(o)
        for t in me.loop_triangles:
            m=me.materials[t.material_index];a=groups.setdefault(m.name,{'mat':m,'p':[],'n':[],'i':[],'map':{}})
            for vi,li in zip(t.vertices,t.loops):
                p=me.vertices[vi].co;n=me.corner_normals[li].vector;p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y);key=tuple(round(v,6) for v in (*p,*n));idx=a['map'].get(key)
                if idx is None:
                    idx=len(a['p'])//3;a['map'][key]=idx;a['p'].extend(p);a['n'].extend(n)
                    for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                a['i'].append(idx)
    desc=[];tris=0
    for k,a in enumerate(groups.values()):
        pre=slug.replace('-','_')+'_'+str(k);nv=len(a['p'])//3;assert nv<65536
        for suffix,ctype in (('p','float'),('n','float'),('i','unsigned short')):array(pre+'_'+suffix,ctype,a[suffix])
        n=next(n for n in a['mat'].node_tree.nodes if n.type=='BSDF_PRINCIPLED');rgba=[round(srgb(c)*255) for c in n.inputs['Base Color'].default_value[:3]]+[255]
        emit=n.inputs['Emission Strength'].default_value if any(n.inputs['Emission Color'].default_value[:3]) else 0
        desc.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(a['i'])),'{'+','.join(map(str,rgba))+'}',fmt(n.inputs['Metallic'].default_value),fmt(n.inputs['Roughness'].default_value),fmt(emit)])+'}');tris+=len(a['i'])//3
    lines.append('static const FoundryMeshData '+symbol+'_MESHES[] = {'+','.join(desc)+'};')
    lines.append('static const FoundryAssetData '+symbol+' = {'+symbol+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(hi[i]-lo[i]) for i in range(3))+'}};')
    if slug=='city-face':
        families=[1 if name in (stone.name,stone_dark.name,stone_light.name,cut.name) else 0 for name in groups]
        array('FOUNDRY_CITY_FACE_SUBSTRATES','unsigned char',families)
        manifest['material_families']=[{'slot':i,'material':name,'family':families[i]} for i,name in enumerate(groups)]
    for o in scene.objects:o.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
    bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
    manifest['assets'][slug]={'symbol':symbol,'triangles':tris,'material_meshes':len(groups),'bounds_min':lo,'bounds_max':hi,'dimensions':[hi[i]-lo[i] for i in range(3)],'pivot':pivot,'materials':list(groups)}
lines.append('#endif')
assert sum(a['triangles'] for a in manifest['assets'].values())<manifest['budget']['triangles_max'],manifest
assert sum(a['material_meshes'] for a in manifest['assets'].values())<=manifest['budget']['material_groups_max'],manifest
HEADER.write_text('\n'.join(lines)+'\n')
(OUT/'city-face-manifest.json').write_text(json.dumps(manifest,indent=2))

# Staging is kept in a named collection and excluded from both asset exports.
stage=bpy.data.collections.new('PRESENTATION | excluded from export');scene.collection.children.link(stage)
for x in (-1.5,1.5):
    src=next(iter(eyecol.objects));o=src.copy();o.data=src.data.copy();stage.objects.link(o);o.name='Review eye instance';o.location=xyz((x,3.5,.165))
eyecol.hide_render=True;eyecol.hide_viewport=True
world=bpy.data.worlds.new('Quiet neutral city review');world.use_nodes=True;scene.world=world
bg=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.12,.15,.17,1);bg.inputs['Strength'].default_value=.42
def light(name,loc,power,color,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.color=color;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(name,d);stage.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,2.5))-o.location).to_track_quat('-Z','Y').to_euler()
light('Soft broad key',(-5,-7,9),1600,(1,.94,.83),7)
light('Submerged broad fill',(5,-4,4),650,(.63,.85,.82),6)
light('Quiet stone rim',(1,2,7),1000,(.71,.86,1),5)
cd=bpy.data.cameras.new('City face review');cam=bpy.data.objects.new('City face review',cd);stage.objects.link(cam);scene.camera=cam;cd.type='ORTHO';cd.ortho_scale=8.6
scene.render.engine='CYCLES';scene.render.threads_mode='FIXED';scene.render.threads=16;scene.cycles.device='CPU';scene.cycles.samples=16;scene.cycles.use_denoising=True;scene.render.resolution_x=1100;scene.render.resolution_y=800;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
scene.view_settings.view_transform='AgX'
for label,pos,override in [('front',(0,-15,2.5),None),('three-quarter',(7,-15,5.2),None),('side-clay',(12,-1,2.5),clay),('back-clay',(0,15,2.5),clay),('front-clay',(0,-15,2.5),clay)]:
    cam.location=pos;cam.rotation_euler=(Vector((0,0,2.5))-cam.location).to_track_quat('-Z','Y').to_euler();scene.view_layers[0].material_override=override
    scene.render.filepath=str(REVIEW/(label+'.png'));bpy.ops.render.render(write_still=True)
scene.view_layers[0].material_override=None;cam.location=(0,-15,2.5);cam.rotation_euler=(Vector((0,0,2.5))-cam.location).to_track_quat('-Z','Y').to_euler()
scene.view_layers[0].material_override=clay
scene.render.resolution_x=320;scene.render.resolution_y=224
scene.render.filepath=str(REVIEW/'source-target-size-clay.png');bpy.ops.render.render(write_still=True)
scene.view_layers[0].material_override=None
scene.render.resolution_x=1100;scene.render.resolution_y=800
for o in facecol.objects:
    if o.hide_render:o.hide_set(True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'city-face.blend'))
print('CITY_FACE_COMPLETE '+json.dumps(manifest))
