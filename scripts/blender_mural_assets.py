"""Authored rock return and phosphor procession, original two-room game.

Run in a fresh background Blender process. --verify reopens mural.blend and
checks clean GLB reimports without rebuilding the source. No textures required:
the pigment is intentional surface-conforming open decal geometry.
"""
import bpy, bmesh, math, json, sys, hashlib
from pathlib import Path
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets' / 'blender'
REVIEW = OUT / 'review' / 'mural'
MODELS = ROOT / 'public' / 'models'
HEADER = ROOT / 'src' / 'generated' / 'mural_assets.h'
for path in (OUT, REVIEW, MODELS, HEADER.parent): path.mkdir(parents=True, exist_ok=True)
ASSETS = [('mural-rock', 'FOUNDRY_MURAL_ROCK'), ('mural-pigment', 'FOUNDRY_MURAL_PIGMENT')]

def xyz(p): return (p[0], -p[2], p[1])
def linear(c):
    c /= 255
    return c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4
def srgb(c): return 12.92*c if c <= .0031308 else 1.055*c**(1/2.4)-.055
def material(name, rgb, rough=.94, emit=0):
    mat = bpy.data.materials.new(name); mat.use_nodes = True
    p = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    col = tuple(linear(c) for c in rgb)
    p.inputs['Base Color'].default_value = (*col, 1)
    p.inputs['Roughness'].default_value = rough
    if emit:
        p.inputs['Emission Color'].default_value = (*col, 1)
        p.inputs['Emission Strength'].default_value = emit
    mat.diffuse_color = (*col, 1)
    return mat

def noise(x, y):
    def h(a,b): return (math.sin(a*127.1+b*311.7)*43758.5453) % 1
    ix, iy = math.floor(x), math.floor(y); fx, fy = x-ix, y-iy
    fx = fx*fx*(3-2*fx); fy = fy*fy*(3-2*fy)
    a=h(ix,iy)*(1-fx)+h(ix+1,iy)*fx
    b=h(ix,iy+1)*(1-fx)+h(ix+1,iy+1)*fx
    return a*(1-fy)+b*fy

# Deliberate fissures form large, sparse construction features, not uniform grunge.
FISSURES = [((-2.9,2.5),(-1.68,2.22)),((-1.68,2.22),(-1.23,1.78)),
            ((-1.23,1.78),(-1.03,.95)),((.82,3.30),(.98,2.59)),
            ((.98,2.59),(1.37,2.08)),((1.37,2.08),(1.44,.76)),
            ((1.44,.76),(2.6,.51)),((-2.60,.8),(-2.14,.52))]
def segment_dist(x,y,a,b):
    dx,dy=b[0]-a[0],b[1]-a[1]
    t=max(0,min(1,((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy)))
    return math.hypot(x-a[0]-t*dx,y-a[1]-t*dy)
def fissure(x,y): return min(segment_dist(x,y,a,b) for a,b in FISSURES)
def front(x,y):
    broad = .045*math.sin(x*1.3+y*.7) + .038*math.sin(y*2.3-x*.55)
    grain = .033*(noise(x*5.1+9,y*5.1)-.5)
    crack = .060*math.exp(-fissure(x,y)**2/.00055)
    # This wall face has shallow actual relief. Pigment follows it exactly.
    return broad + grain - crack

def crown(x): return 3.25+.20*math.cos(x*1.37)+.11*math.sin(x*6.1)+.035*math.sin(x*17)
def half_width(t): return 3.15-.29*t**3+.07*math.sin(t*19)+.04*math.sin(t*41)
def surface(x,y):
    t=(y+.48)/(crown(x)+.48)
    return front(x,y)-max(abs(x/half_width(t))**20,t**30)*.24

def mesh(name, verts, faces, mats, collection, indices=None, smooth=False):
    me = bpy.data.meshes.new(name)
    me.from_pydata([xyz(p) for p in verts], [], faces); me.update()
    bm=bmesh.new();bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(me);bm.free()
    ob=bpy.data.objects.new(name,me);collection.objects.link(ob)
    for mat in mats if isinstance(mats,list) else [mats]: me.materials.append(mat)
    for i,p in enumerate(me.polygons):
        p.use_smooth=smooth
        if indices: p.material_index=indices[i]
    return ob

def frame_camera(scene, stage, pos=(0,-13,1.55), center=(0,0,1.55), scale=7.6):
    cam=next((o for o in stage.objects if o.type=='CAMERA'),None)
    if cam is None:
        data=bpy.data.cameras.new('Mural review camera');cam=bpy.data.objects.new('Mural review camera',data);stage.objects.link(cam)
    cam.location=pos;cam.rotation_euler=(Vector(center)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=scale;scene.camera=cam
    return cam

def configure(scene):
    scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
    scene.render.resolution_x=1280;scene.render.resolution_y=720;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.view_settings.view_transform='AgX'

def render(scene,name):
    scene.render.filepath=str(REVIEW/(name+'.png'));bpy.ops.render.render(write_still=True)

def build():
    scene=bpy.data.scenes.new('MURAL | phosphor procession');bpy.context.window.scene=scene
    for old in list(bpy.data.scenes):
        if old!=scene: bpy.data.scenes.remove(old)
    rock=bpy.data.collections.new('ASSET | mural-rock');scene.collection.children.link(rock)
    paint=bpy.data.collections.new('ASSET | mural-pigment');scene.collection.children.link(paint)
    stone=material('Mural | quiet basalt',(57,69,73))
    strata=material('Mural | mineral bedding',(59,71,74))
    core=material('Mural | deep fractured return',(46,57,61))
    vein=material('Mural | recessed natural fissures',(51,64,68))
    ink=material('Pigment | aged phosphor',(112,157,129),.97,.31)
    pale=material('Pigment | surviving lime',(136,177,143),.98,.31)
    rubbed=material('Pigment | old underdrawing',(72,111,91),.99,.14)
    dark=material('Pigment | dark sun and cut marks',(27,45,43),1.0,0)
    # Surface grid, irregular crown, real sides/back; no rectangular panel outline.
    cols,rows=88,56;v=[];f=[];mi=[]
    for j in range(rows+1):
        t=j/rows;half=half_width(t)
        for i in range(cols+1):
            u=i/cols;x=(2*u-1)*half
            top=crown(x)
            y=-.48+(top+.48)*t
            edge=max(abs(2*u-1)**20,t**30)
            z=front(x,y)-edge*.24
            v.append((x,y,z))
    stride=cols+1
    for j in range(rows):
        for i in range(cols):
            k=j*stride+i;f.append((k,k+1,k+1+stride,k+stride))
            cx=(v[k][0]+v[k+1][0])/2;cy=(v[k][1]+v[k+stride][1])/2
            mi.append(3 if fissure(cx,cy)<.025 else (1 if noise(cx*2.6+8,cy*2.1)>.61 else 0))
    perimeter=list(range(stride))+[j*stride+cols for j in range(1,rows+1)]+[rows*stride+i for i in range(cols-1,-1,-1)]+[j*stride for j in range(rows-1,0,-1)]
    back=[]
    for k in perimeter:
        x,y,z=v[k];back.append(len(v));v.append((x,y,-.58+.035*math.sin(x*3.7+y)))
    for i,k in enumerate(perimeter):
        n=(i+1)%len(perimeter);f.append((k,back[i],back[n],perimeter[n]));mi.append(2)
    v.append((0,1.45,-.62));center=len(v)-1
    for i in range(len(back)):
        f.append((back[i],center,back[(i+1)%len(back)]));mi.append(2)
    wall=mesh('Basalt return with buried foot and broken crown',v,f,[stone,strata,core,vein],rock,mi,True)
    wall['role']='Opaque load-bearing decorative rock; buried foot y=-0.48, world floor y2.'

    def pigment(name, coords, mat=ink, wear=True, offset=.017, pieces=None):
        verts=[];faces=[]
        def refine(a,b,c,depth=0):
            edges=[((a-b).length,a,b,c),((b-c).length,b,c,a),((c-a).length,c,a,b)]
            length,p,q,r=max(edges,key=lambda e:e[0])
            if length>.115 and depth<10:
                mid=(p+q)*.5;refine(p,mid,r,depth+1);refine(mid,q,r,depth+1);return
            cen=(a+b+c)/3
            if cen.y>crown(cen.x)-.055: return
            if wear and fissure(cen.x,cen.y)<.007: return
            start=len(verts)
            for pt in (a,b,c): verts.append((pt.x,pt.y,surface(pt.x,pt.y)+offset))
            faces.append((start,start+1,start+2))
        for contour in pieces if pieces else [coords]:
            vectors=[Vector((x,y,0)) for x,y in contour]
            for tri in tessellate_polygon([vectors]):
                refine(*(vectors[p] if isinstance(p,int) else p for p in tri))
        if faces:
            ob=mesh(name,verts,faces,mat,paint,smooth=False)
            # Pigment cards are deliberately open front-facing surface meshes.
            bm=bmesh.new();bm.from_mesh(ob.data)
            for face in bm.faces:
                if face.normal.y>0: face.normal_flip()
            bm.to_mesh(ob.data);bm.free()
            ob['role']='Surface-conforming phosphor pigment; entire collection fades in runtime.'
            # A paint decal does not cast its own shadow; the target renderer has
            # no shadow pass. Keep neutral source previews faithful to that use.
            ob.visible_shadow=False
            return ob
    def ribbon(name, pts, width, mat=ink, wear=True, offset=.020):
        # Widened hand-drawn strokes have real taper, not cylindrical stick limbs.
        if len(pts)<20:
            expanded=[]
            for j in range(len(pts)-1):
                a,b,c,d=[Vector(pts[max(0,min(len(pts)-1,k))]) for k in (j-1,j,j+1,j+2)]
                for k in range(4):
                    t=k/4;v=.5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t);expanded.append(tuple(v))
            pts=expanded+[pts[-1]]
        l=[];r=[]
        for i,(x,y) in enumerate(pts):
            a=Vector(pts[max(0,i-1)]);b=Vector(pts[min(len(pts)-1,i+1)])
            d=(b-a).normalized();w=width*(.87+.13*math.sin(i*1.6))
            l.append((x-d.y*w/2,y+d.x*w/2));r.append((x+d.y*w/2,y-d.x*w/2))
        strips=[[l[i],l[i+1],r[i+1],r[i]] for i in range(len(l)-1)]
        return pigment(name,[],mat,wear,offset,pieces=strips)
    def circle(name,cx,cy,r,mat,wear=True,offset=.021):
        return pigment(name,[(cx+math.cos(a*math.tau/64)*r,cy+math.sin(a*math.tau/64)*r) for a in range(64)],mat,wear,offset)
    def arc(name,cx,cy,rx,ry,start,end,width,mat=ink,wear=True):
        return ribbon(name,[(cx+rx*math.cos(start+(end-start)*i/90),cy+ry*math.sin(start+(end-start)*i/90)) for i in range(91)],width,mat,wear)

    # Six related long-robed bearers: paired procession, distinct gestures, shared
    # swept hood and circular shoulder grammar. No real-world faith symbols/text.
    for i,cx in enumerate((-2.23,-1.36,-.47,.47,1.36,2.23)):
        direction=1 if i<3 else -1;h=.84 if i in (2,3) else 1.0
        def tr(points): return [(cx+direction*x,.25+y*h) for x,y in points]
        robe=[(-.38,0),(-.17,.24),(-.06,.71),(-.02,1.19),(-.10,1.55),(-.04,1.85),(.15,1.94),(.26,1.75),(.21,1.20),(.13,.75),(.11,.28),(.25,.035),(.05,0),(-.09,.05),(-.17,.01)]
        def smooth_outline(points):
            out=[]
            for j in range(len(points)):
                a,b,c,d=[Vector(points[k%len(points)]) for k in (j-1,j,j+1,j+2)]
                for k in range(5):
                    t=k/5;v=.5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t);out.append(tuple(v))
            return out
        pigment('Bearer %d | flowing robe silhouette'%i,tr(smooth_outline(robe)),ink)
        # A crescent hood has a forward prow and a dark open face, not a pin head.
        hood=[(-.11,1.82),(-.21,2.03),(-.14,2.22),(.04,2.36),(.33,2.39),(.53,2.30),(.30,2.26),(.14,2.10),(.10,1.94)]
        pigment('Bearer %d | swept ceremonial hood'%i,tr(smooth_outline(hood)),pale)
        face=[(-.04,1.94),(-.10,2.06),(-.025,2.21),(.14,2.24),(.18,2.16),(.07,2.02),(.08,1.91)]
        pigment('Bearer %d | empty hood interior'%i,tr(smooth_outline(face)),dark,False,.030)
        shoulder=tr([(.02,1.67)])[0];circle('Bearer %d | shoulder eclipse'%i,*shoulder,.08,dark,False,.030)
        # Robe split and fold are negative painted lines following the rock.
        ribbon('Bearer %d | robe fold'%i,tr([(.05,.11),(.035,.47),(.095,.94),(.14,1.31)]),.025,dark,True,.030)
        ribbon('Bearer %d | hem underdrawing'%i,tr([(-.27,.025),(-.10,.22),(-.045,.53)]),.024,rubbed)
        # Inner pair physically raise the dark disc; outer figures share the load
        # through the painted procession's connected arms and cloth.
        if i in (2,3):
            arm=[(.16,1.71),(.30,1.64),(.45,1.78),(.53,2.11),(.66,2.26)]
        else:
            arm=[(.16,1.69),(.34,1.49),(.54,1.63),(.70,1.88)]
        ribbon('Bearer %d | raised broad sleeve'%i,tr(arm),.092,ink)
        ribbon('Bearer %d | trailing sleeve'%i,tr([(-.01,1.70),(-.24,1.37),(-.38,1.58)]),.076,ink)
        # Tiny brush hand split, still above the broad silhouette scale.
        hx,hy=tr([arm[-1]])[0]
        ribbon('Bearer %d | first hand mark'%i,[(hx,hy),(hx+direction*.075,hy+.045)],.028,pale)
        ribbon('Bearer %d | second hand mark'%i,[(hx,hy),(hx+direction*.035,hy+.081)],.022,pale)
    # The object carried is a dark eclipse ring. Its interior remains darker than
    # its weathered phosphor perimeter, distinct from a glowing collectible orb.
    circle('Dark sun | matte center',0,2.78,.51,dark,False,.029)
    arc('Dark sun | carried phosphor perimeter',0,2.78,.535,.535,0,math.tau,.046,pale,True)
    arc('Dark sun | incomplete second orbit',0,2.78,.603,.58,-.42,2.89,.022,ink,True)
    # Sparse stars and one broad trajectory unify astronomy and pigment craft.
    ribbon('Upper orbital trajectory',[(x,2.83+.50*(1-(x/2.6)**2)) for x in [(-2.62+i*5.24/70) for i in range(71)]],.022,rubbed,True)
    for x,y,r in [(-2.57,2.83,.071),(-1.79,3.09,.047),(1.80,3.07,.051),(2.53,2.84,.069)]:
        circle('Orbital mineral mark',x,y,r,ink)
    ribbon('Broken baseline',[(x,.265+.010*math.sin(x*3)) for x in [-2.72+i*5.44/100 for i in range(101)]],.018,rubbed)
    for i in range(9):
        x=-2.5+i*.62
        arc('Procession lower phase',x,.12,.045,.045,.15,math.pi*1.7,.014,rubbed,True)

    manifest=export_assets(scene,rock,paint)
    stage=bpy.data.collections.new('PRESENTATION | excluded from export');scene.collection.children.link(stage)
    world=bpy.data.worlds.new('Mural soft studio');world.use_nodes=True;scene.world=world
    bg=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.055,.07,.085,1);bg.inputs['Strength'].default_value=.40
    for name,loc,power,color,size in [('Broad neutral key',(-4,-6,7),730,(.83,.95,1),5),('Low warm fill',(4,-3,3),180,(1,.82,.66),4)]:
        d=bpy.data.lights.new(name,'AREA');d.energy=power;d.color=color;d.size=size
        ob=bpy.data.objects.new(name,d);stage.objects.link(ob);ob.location=loc
        ob.rotation_euler=(Vector((0,0,1.6))-ob.location).to_track_quat('-Z','Y').to_euler()
    configure(scene);cam=frame_camera(scene,stage)
    render(scene,'source-front')
    cam.location=(4.8,-12,4.8);cam.rotation_euler=(Vector((0,0,1.6))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=7.5
    render(scene,'source-three-quarter')
    paint.hide_render=True;cam=frame_camera(scene,stage)
    render(scene,'source-rock-only')
    paint.hide_render=False
    frame_camera(scene,stage)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'mural.blend'))
    print('MURAL_SOURCE_COMPLETE '+json.dumps(manifest))

def export_assets(scene,rock,paint):
    lines=['/* Generated by scripts/blender_mural_assets.py; surface-following pigment. */','#ifndef MURAL_ASSETS_H','#define MURAL_ASSETS_H','#include "foundry_assets.h"']
    def fmt(v):
        s=f'{v:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
    def array(name,ctype,values):
        lines.append('static const '+ctype+' '+name+'[] = {')
        for i in range(0,len(values),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in values[i:i+12])+',')
        lines.append('};')
    manifest={'blender_version':bpy.app.version_string,'runtime_axes':'X right, Y up, Z front','runtime_anchor':[9,2.3,-1.3],'source':'mural.blend','references':['public/art/references/vault-mouth/13-phosphor-mural.png','public/art/references/vault-mouth/14-mural-lamplit.png'],'contract':{'context':'background rock return, existing mural interaction and layout','dimensions':'approximately 6.4 wide, 4 high including buried foot, .7 deep; no collider','viewing_scale':'about 195x120 screen pixels at1280x720','delivery_budget_triangles':28000,'pigment':'intentionally open nonmetallic geometry following rock surface; fade all pigment meshes together; opaque rock stays','scope':'original ceremonial bearers with a dark disc and orbital records; no text or new verb'},'assets':{}}
    for (slug,symbol),col in zip(ASSETS,(rock,paint)):
        bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();groups={};copies=[];lo=[1e9]*3;hi=[-1e9]*3
        delivery=bpy.data.collections.new('DELIVERY temporary '+slug);scene.collection.children.link(delivery)
        for src in col.objects:
            me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
            bm=bmesh.new();bm.from_mesh(me);bmesh.ops.triangulate(bm,faces=list(bm.faces))
            dead=[f for f in bm.faces if f.calc_area()<1e-11]
            if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
            bm.to_mesh(me);bm.free();me.calc_loop_triangles()
            ob=bpy.data.objects.new(src.name+' delivery',me);delivery.objects.link(ob);copies.append(ob)
            for tri in me.loop_triangles:
                mat=me.materials[tri.material_index];g=groups.setdefault(mat.name,{'mat':mat,'p':[],'n':[],'i':[],'map':{}})
                for vi,li in zip(tri.vertices,tri.loops):
                    p=me.vertices[vi].co;n=me.corner_normals[li].vector;p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y)
                    key=tuple(round(v,6) for v in (*p,*n));idx=g['map'].get(key)
                    if idx is None:
                        idx=len(g['p'])//3;g['map'][key]=idx;g['p'].extend(p);g['n'].extend(n)
                        for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                    g['i'].append(idx)
        desc=[];tris=0
        for i,g in enumerate(groups.values()):
            pre=slug.replace('-','_')+'_'+str(i);nv=len(g['p'])//3;assert nv<65536
            for suffix,ctype in [('p','float'),('n','float'),('i','unsigned short')]:array(pre+'_'+suffix,ctype,g[suffix])
            bs=next(n for n in g['mat'].node_tree.nodes if n.type=='BSDF_PRINCIPLED')
            rgba=[round(srgb(c)*255) for c in bs.inputs['Base Color'].default_value[:3]]+[255]
            em=bs.inputs['Emission Strength'].default_value if any(bs.inputs['Emission Color'].default_value[:3]) else 0
            desc.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(g['i'])),'{'+','.join(map(str,rgba))+'}',fmt(bs.inputs['Metallic'].default_value),fmt(bs.inputs['Roughness'].default_value),fmt(em)])+'}');tris+=len(g['i'])//3
        lines.append('static const FoundryMeshData '+symbol+'_MESHES[] = {'+','.join(desc)+'};')
        lines.append('static const FoundryAssetData '+symbol+' = {'+symbol+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(hi[i]-lo[i]) for i in range(3))+'}};')
        for ob in scene.objects:ob.select_set(False)
        for ob in copies:ob.select_set(True)
        bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
        bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
        bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
        manifest['assets'][slug]={'symbol':symbol,'triangles':tris,'material_meshes':len(groups),'bounds_min':lo,'bounds_max':hi,'dimensions':[hi[i]-lo[i] for i in range(3)],'materials':list(groups),'intentional_open_surfaces':slug=='mural-pigment'}
    lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n')
    manifest['total_triangles']=sum(a['triangles'] for a in manifest['assets'].values())
    assert manifest['total_triangles']<28000,manifest['total_triangles']
    (OUT/'mural-manifest.json').write_text(json.dumps(manifest,indent=2))
    return manifest

def verify():
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'mural.blend'))
    source=bpy.context.scene;stage=bpy.data.collections['PRESENTATION | excluded from export']
    manifest=json.loads((OUT/'mural-manifest.json').read_text())
    report={'source_reopened':True,'blender_version':bpy.app.version_string,'assets':{}}
    sc=bpy.data.scenes.new('MURAL clean GLB reimport');bpy.context.window.scene=sc;sc.world=source.world;sc.collection.children.link(stage)
    for slug,_ in ASSETS:
        previous=set(sc.objects);path=MODELS/(slug+'.glb');bpy.ops.import_scene.gltf(filepath=str(path))
        obs=[ob for ob in sc.objects if ob not in previous and ob.type=='MESH'];points=[];tris=bad=normals=0
        for ob in obs:
            if slug=='mural-pigment':ob.visible_shadow=False
            me=ob.data;me.calc_loop_triangles();tris+=len(me.loop_triangles);points += [ob.matrix_world@v.co for v in me.vertices]
            for t in me.loop_triangles:
                a,b,c=[me.vertices[i].co for i in t.vertices]
                if (b-a).cross(c-a).length<1e-10:bad+=1
            normals+=sum(1 for v in me.vertices if not all(math.isfinite(c) for c in (*v.co,*v.normal)) or v.normal.length<.95)
        lo=[min(p[i] for p in points) for i in range(3)];hi=[max(p[i] for p in points) for i in range(3)]
        dims=[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]];expected=manifest['assets'][slug]
        print('MURAL_QA_COUNTS',slug,{'triangles':tris,'expected':expected['triangles'],'degenerate':bad,'invalid_normals':normals})
        assert tris==expected['triangles'];assert bad==0;assert normals==0
        assert max(abs(a-b) for a,b in zip(dims,expected['dimensions']))<.0001
        report['assets'][slug]={'triangles':tris,'degenerate_triangles':bad,'invalid_normals':normals,'dimensions':dims,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'checks':'pass','intentional_open_surfaces':slug=='mural-pigment'}
    configure(sc);frame_camera(sc,stage);render(sc,'reimport-front')
    cam=frame_camera(sc,stage,pos=(5,-12,4.5),center=(0,0,1.55),scale=7.5);render(sc,'reimport-three-quarter')
    report['source_sha256']=hashlib.sha256((OUT/'mural.blend').read_bytes()).hexdigest()
    report['artistic_status']='awaiting inspection of source/reimport images and runtime integration'
    (OUT/'mural-verification.json').write_text(json.dumps(report,indent=2))
    print('MURAL_REIMPORT_VERIFIED '+json.dumps(report))

if '--verify' in sys.argv: verify()
else: build()
