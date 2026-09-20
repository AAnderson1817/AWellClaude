"""Authored Drowned Quarter relief: editable stone shell, bronze repair, opal eyes.

Run in an isolated Blender background process. Runtime coordinates are X right,
Y up, Z front; the source is Z up with its front towards Blender -Y.
"""
import bpy, bmesh, math, json
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'assets' / 'blender'
REVIEW = OUT / 'review' / 'city-face'
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

def g(x, y, cx, cy, sx, sy): return math.exp(-(((x-cx)/sx)**2+((y-cy)/sy)**2))

def halfwidth(t):
    # A broad architectural carving: worn temple corners and flat broken base.
    # Catmull-Rom interpolation avoids a coin silhouette without jagged edges.
    profile=[(0,1.65),(.10,2.25),(.28,2.98),(.52,3.55),(.76,3.69),(.90,3.27),(1,2.34)]
    for i in range(len(profile)-1):
        a,b=profile[i],profile[i+1]
        if a[0]<=t<=b[0]:
            u=(t-a[0])/(b[0]-a[0]);p0=profile[max(0,i-1)][1];p1=a[1];p2=b[1];p3=profile[min(len(profile)-1,i+2)][1]
            w=.5*((2*p1)+(-p0+p2)*u+(2*p0-5*p1+4*p2-p3)*u*u+(-p0+3*p1-3*p2+p3)*u*u*u)
            return w*(1-.009*math.sin(t*57)-.006*math.sin(t*113+.8))
    return 2.34

def face_z(x, y):
    t = max(.0001,min(.9999,y/5))
    w = max(.01,halfwidth(t))
    u = max(-1,min(1,x/w))
    dome = .06+.28*max(0,1-u*u)**.6*math.sin(math.pi*t)**.4
    # Large cheek and brow planes are continuous with the main carving.
    for s in (-1,1):
        dome += .22*g(x,y,s*1.72,2.40,1.10,.74)
        dome += .30*g(x,y,s*1.43,3.99,1.12,.24)
        dome -= .43*g(x,y,s*1.50,3.49,.94,.35)
        dome -= .10*g(x,y,s*2.87,3.13,.20,.83)
    # The broad authored wedge below is sunk into this supporting bridge.
    dome += .19*g(x,y,0,3.24,.42,.94)
    # Broad, archaic nonhuman mask: low philtrum and a small closed mouth.
    lip_x=max(0,1-(abs(x)/1.12)**4)
    dome += .16*lip_x*max(0,min(1,(.16-abs(y-1.57))/.055))
    dome += .20*lip_x*max(0,min(1,(.17-abs(y-1.19))/.055))
    lipline = 1.39-.045*(x/1.05)**2
    dome -= .23*math.exp(-((y-lipline)/.053)**2-(x/1.10)**6)
    dome += .11*g(x,y,0,.74,1.22,.37)
    # Restrained tool-cut strata; true geometry, no painted sculpt illusion.
    dome += .008*(math.sin(x*11+y*4.1)*math.sin(y*13.4)+.5*math.sin(x*21.4-y*19.2))*max(0,1-u*u)
    # Broken outer stone surface, concentrated on exposed rim rather than a
    # uniform all-over noise filter. The small planar chips affect true geometry.
    edge=max(0,(abs(u)-.72)/.28)
    dome-=edge*.054*(.5+.5*math.sin(x*29+y*33))
    for cx,cy,sx,sy in ((-2.8,4.2,.13,.22),(-3.3,3.0,.19,.25),(2.9,4.5,.18,.20),(3.15,2.1,.14,.24),(-1.35,.35,.17,.14)):
        r=abs((x-cx)/sx)+abs((y-cy)/sy)
        dome-=.095*max(0,1-r)
    return dome

def front_point(x,y,offset=0): return (x,y,face_z(x,y)+offset)

def ribbon(name, points, width, depth, mat, bevel=.008):
    # Seat every strip against the sculpt, with a real rectangular cross section.
    verts=[]; faces=[]
    for i,(x,y) in enumerate(points):
        a=Vector(points[max(0,i-1)]);b=Vector(points[min(len(points)-1,i+1)])
        tangent=(b-a).normalized();side=Vector((-tangent.y,tangent.x))*width*.5
        for p,zoff in ((Vector((x,y))-side,.003),(Vector((x,y))+side,.003),
                       (Vector((x,y))+side,depth),(Vector((x,y))-side,depth)):
            verts.append(front_point(p.x,p.y,zoff))
    for i in range(len(points)-1):
        for j in range(4): faces.append((i*4+j,(i+1)*4+j,(i+1)*4+(j+1)%4,i*4+(j+1)%4))
    faces += [(3,2,1,0), tuple((len(points)-1)*4+j for j in range(4))]
    o=mesh(name,verts,faces,mat)
    if bevel:
        mod=o.modifiers.new('Cast edge radius','BEVEL');mod.width=bevel;mod.segments=2
    return o

def disk(name, x,y,rx,ry,z,mat,depth=.03,n=40):
    verts=[(x,y,z+depth)]
    for k in range(n):
        a=math.tau*k/n;verts.append((x+rx*math.cos(a),y+ry*math.sin(a),z))
    verts.append((x,y,z-.04));faces=[]
    for k in range(n): faces.extend([(0,1+k,1+(k+1)%n),(n+1,1+(k+1)%n,1+k)])
    return mesh(name,verts,faces,mat)

facecol=collection('city-face','FOUNDRY_CITY_FACE','base center; add world(25,1,-1.8)')
NX,NY=100,80
verts=[];faces=[];mi=[]
for j in range(NY+1):
    y=.025+4.95*j/NY;w=halfwidth(y/5)
    for i in range(NX+1):
        x=w*(2*i/NX-1);verts.append(front_point(x,y))
stride=NX+1
for j in range(NY):
    for i in range(NX):
        q=j*stride+i;faces.append((q,q+1,q+1+stride,q+stride))
        x,y,_=verts[q]
        # Low-frequency material patches, no uniform per-face confetti.
        tide=math.sin(x*1.27+y*.6)+.53*math.sin(x*.42-y*2.3)
        mi.append(0)
# Real slab thickness and an editable simple back. No unsupported open plane.
edge=list(range(stride))+[j*stride+NX for j in range(1,NY+1)]+list(range(NY*stride+NX-1,NY*stride-1,-1))+[j*stride for j in range(NY-1,0,-1)]
back_start=len(verts)
verts += [(verts[q][0],verts[q][1],-.26) for q in edge]
for k,q in enumerate(edge):
    qn=edge[(k+1)%len(edge)]
    faces.append((q,back_start+k,back_start+(k+1)%len(edge),qn));mi.append(1)
faces.append(tuple(reversed(range(back_start,len(verts)))));mi.append(1)
body=mesh('Continuous carved limestone face and integral back',verts,faces,[stone,stone_dark,stone_light],mi=mi)
# The unseen flat masonry seat is intentionally flat shaded, so a large back
# n-gon cannot smear the side normals into a false convex rear face.
body.data.polygons[-1].use_smooth=False

# Authored planar nose: a solid stone keystone, embedded in the carving. The
# five deliberate front planes remain legible from the side and at room scale;
# shallow bevels are real radii and the unseen back is seated within the shell.
v=[(-.25,4.04,.36),(.25,4.04,.36),(-.31,3.36,.56),(.31,3.36,.56),
   (-.66,2.64,.70),(.66,2.64,.70),(-.54,2.43,.54),(.54,2.43,.54),
   (0,4.04,.56),(0,3.35,.93),(0,2.61,1.16),(0,2.41,.78),
   (-.30,4.05,.13),(.30,4.05,.13),(-.69,2.40,.13),(.69,2.40,.13)]
f=[(0,8,9,2),(8,1,3,9),(2,9,10,4),(9,3,5,10),
   (4,10,11,6),(10,5,7,11),(0,2,4,6,14,12),(1,13,15,7,5,3),
   (6,11,7,15,14),(0,12,13,1,8),(12,14,15,13)]
o=mesh('Carved nose keystone with broad chisel planes',v,f,stone,smooth=False)
mod=o.modifiers.new('Worn chisel radii','BEVEL');mod.width=.035;mod.segments=2
# The recesses sit in the lowest angled planes instead of pasted round nostrils.
for s in (-1,1):
    verts=[(s*.21,2.44,.822),(s*.48,2.46,.660),(s*.44,2.52,.688),(s*.21,2.51,.850)]
    mesh('Quiet cut beneath nose '+str(s),verts,[(0,1,2,3)],cut,False)

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
        for k in range(41):
            a=.05+math.pi*.93*k/40
            x=s*(1.5+1.19*math.cos(a));y=3.48+(.59+.14*level)*math.sin(a)+level*.055
            pts.append((x,y))
        ribbon(('Left' if s<0 else 'Right')+' seated orbital brow '+str(level),pts,.088 if level==0 else .048,.052,bronze if level==0 else oxide)
    for level in range(3):
        pts=[]
        for k in range(40):
            inset=.18+level*.27
            a=math.pi+inset+(math.pi-2*inset)*k/39
            x=s*(1.5+(1.07-.05*level)*math.cos(a));y=3.46+(.50+.14*level)*math.sin(a)
            pts.append((x,y))
        ribbon(('Left' if s<0 else 'Right')+' layered lower orbital casting '+str(level),pts,.070-.008*level,.034,bronze if level!=1 else oxide,.006)
    # Long tapered cheek engraving: one clear orbital family, calm low field.
    pts=[(s*(2.10+.23*math.sin(math.pi*k/30)),2.99-1.82*k/30) for k in range(31)]
    ribbon('Cheek meridian inlay '+str(s),pts,.038,.016,oxide,.002)
    # Recess around the glass lens. Stone/socket remains visible outside it.
    cx=s*1.5;cy=3.5
    ring=[(cx+.265*math.cos(math.tau*k/48),cy+.268*math.sin(math.tau*k/48)) for k in range(49)]
    ribbon('Seated eye lens bronze retaining ring '+str(s),ring,.042,.030,bronze,.004)

# A single eclipse mark is the ritual/astronomy focus above the eyes. The disc
# is physically inserted in a shallow circular seat, with three sunk pin heads.
cy=4.54;cz=face_z(0,cy)+.014
disk('Eclipse bronze disc',0,cy,.224,.224,cz,bronze,.022)
for k in range(31):
    if k==0:pts=[]
    a=math.pi*.16+math.pi*1.55*k/30;pts.append((.29*math.cos(a),cy+.29*math.sin(a)))
ribbon('Partial orbit around eclipse seal',pts,.030,.025,oxide,.003)
for s in (-1,1):
    pts=[(s*(.065+.11*k/14),4.30-.31*k/14) for k in range(15)]
    ribbon('Seal cast stem '+str(s),pts,.030,.021,bronze,.003)
for x in (-.11,0,.11):disk('Seal sunk rivet',x,cy-.02,.027,.027,cz+.022,oxide,.01,n=12)

# A fracture at the right temple is visibly repaired by three short cast staples.
crack=[(2.88,4.16),(2.71,3.93),(2.80,3.74),(2.67,3.53),(2.78,3.25),(2.67,3.03),(2.75,2.80)]
ribbon('Old temple fracture',crack,.023,.012,cut,.002)
for x,y in ((2.77,3.88),(2.72,3.47),(2.73,3.07)):
    pts=[(x-.18,y-.045),(x-.11,y-.062),(x+.10,y+.036),(x+.18,y+.043)]
    ribbon('Cast bronze fracture staple',pts,.080,.048,bronze,.007)
    for sx,sy in ((x-.14,y-.05),(x+.14,y+.04)):
        disk('Staple seated pin',sx,sy,.023,.023,face_z(sx,sy)+.052,oxide,.007,n=12)

# Mouth darkness is modeled as a closed inset sliver following the carved groove.
pts=[]
for k in range(39):
    x=-.94+1.88*k/38;pts.append((x,1.39-.045*(x/1.05)**2))
ribbon('Quiet mouth incised line',pts,.028,.009,cut,.002)

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
          'eye_centers_local':[[-1.5,3.5,.165],[1.5,3.5,.165]],'mouth_local':[0,1.37,face_z(0,1.37)+.02],
          'governing_reference':'public/art/references/drowned-quarter/03-drowned-face-v2.png',
          'interpretation':'original broad, quiet mask; rounded temple silhouette and eclipse repairs translate the reference into fixed-view carved relief',
          'source':'assets/blender/city-face.blend','budget':{'triangles_max':43000,'material_groups_max':7,'textures':0},'assets':{}}
for slug,symbol,col,pivot in assets:
    bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();groups={};lo=[1e9]*3;hi=[-1e9]*3;copies=[]
    delivery=bpy.data.collections.new('DELIVERY temporary');scene.collection.children.link(delivery)
    for src in col.objects:
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.triangulate(bm,faces=list(bm.faces))
        dead=[f for f in bm.faces if f.calc_area()<1e-10]
        if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
        bm.to_mesh(me);bm.free();me.calc_loop_triangles()
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
    for o in scene.objects:o.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
    bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
    manifest['assets'][slug]={'symbol':symbol,'triangles':tris,'material_meshes':len(groups),'bounds_min':lo,'bounds_max':hi,'dimensions':[hi[i]-lo[i] for i in range(3)],'pivot':pivot,'materials':list(groups)}
lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n')
assert sum(a['triangles'] for a in manifest['assets'].values())<manifest['budget']['triangles_max']
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
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=1100;scene.render.resolution_y=800;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
scene.view_settings.view_transform='AgX'
for label,pos,override in [('front',(0,-15,2.5),None),('three-quarter',(7,-15,5.2),None),('side-clay',(12,-1,2.5),clay),('back-clay',(0,15,2.5),clay),('front-clay',(0,-15,2.5),clay)]:
    cam.location=pos;cam.rotation_euler=(Vector((0,0,2.5))-cam.location).to_track_quat('-Z','Y').to_euler();scene.view_layers[0].material_override=override
    scene.render.filepath=str(REVIEW/(label+'.png'));bpy.ops.render.render(write_still=True)
scene.view_layers[0].material_override=None;cam.location=(0,-15,2.5);cam.rotation_euler=(Vector((0,0,2.5))-cam.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'city-face.blend'))
print('CITY_FACE_COMPLETE '+json.dumps(manifest))
