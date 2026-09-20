"""Editable rooted plants for the existing fixed-camera stage. Run in Blender."""
import bpy, bmesh, math, json, hashlib, sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'assets/blender'
REVIEW = OUT/'review/foliage'
MODELS = ROOT/'public/models'
HEADER = ROOT/'src/generated/foliage_assets.h'
for p in (OUT, REVIEW, MODELS): p.mkdir(parents=True, exist_ok=True)
scene=bpy.data.scenes.new('FOLIAGE | rooted branches and curled leaves')
bpy.context.window.scene=scene
for s in list(bpy.data.scenes):
    if s != scene: bpy.data.scenes.remove(s)

def linear(c):
    c/=255
    return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
def srgb(c): return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
def material(name, rgb, rough=.85):
    m=bpy.data.materials.new(name);m.use_nodes=True
    n=m.node_tree.nodes.get('Principled BSDF');c=tuple(linear(v) for v in rgb)
    n.inputs['Base Color'].default_value=(*c,1);n.inputs['Roughness'].default_value=rough
    m.diffuse_color=(*c,1)
    return m
wood=material('Rooted wood | umber', (91,77,49))
bark=material('Rooted wood | aged ridge', (111,101,64))
leaf=material('Vault | olive lamina', (77,105,68),.78)
young=material('Vault | young lamina', (103,125,78),.73)
under=material('Leaf | subdued underside', (61,82,55))
rib=material('Leaf | attached midrib', (72,96,57),.88)
kelp=material('Drowned | mineral green lamina', (48,94,79),.68)
kelpedge=material('Drowned | worn olive margin', (76,112,82),.76)
current=None;assets=[]
def collection(slug, symbol):
    global current
    current=bpy.data.collections.new('ASSET | '+slug);scene.collection.children.link(current)
    assets.append((slug,symbol,current));return current
def xyz(p): return (p[0],-p[2],p[1])
def mesh(name, verts, faces, mats, mi=None):
    me=bpy.data.meshes.new(name);me.from_pydata([xyz(p) for p in verts], [], faces);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new(name,me);current.objects.link(o)
    for m in mats: me.materials.append(m)
    for i,p in enumerate(me.polygons):p.use_smooth=True;p.material_index=mi[i] if mi else 0
    return o
def sweep(name, points, radius, mats, sides=7):
    verts=[];faces=[];mi=[]
    for i,q in enumerate(points):
        p=Vector(q);t=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])).normalized()
        s=t.cross(Vector((0,0,1)))
        if s.length<.1:s=t.cross(Vector((1,0,0)))
        s.normalize();b=t.cross(s).normalized()
        r=radius*(1-.76*i/(len(points)-1))
        for k in range(sides):
            a=math.tau*k/sides;rr=r*(1+.10*math.sin(k*7+i*.67))
            verts.append(p+rr*(math.cos(a)*s+math.sin(a)*b))
    for i in range(len(points)-1):
        for k in range(sides):
            faces.append((i*sides+k,i*sides+(k+1)%sides,(i+1)*sides+(k+1)%sides,(i+1)*sides+k));mi.append(k%len(mats))
    faces.extend([tuple(reversed(range(sides))),tuple((len(points)-1)*sides+k for k in range(sides))]);mi.extend([0,0])
    return mesh(name,verts,faces,mats,mi)
def curve(a,b,bow=0,n=9):
    a=Vector(a);b=Vector(b)
    return [a.lerp(b,i/(n-1))+Vector((bow*math.sin(math.pi*i/(n-1)),0,.035*math.sin(math.pi*i/(n-1)))) for i in range(n)]

def blade(name, base, direction, length, width, phase, aquatic=False):
    """Closed cambered lamina: a curled midline and corrugated margin, no cards."""
    d=Vector(direction).normalized();side=d.cross(Vector((0,0,1))).normalized()
    normal=side.cross(d).normalized();base=Vector(base)
    rows=32 if aquatic else 24;cols=8;verts=[];faces=[];mi=[]
    def point(t,v):
        # The petiole narrows continuously into the lamina. Tip droop and twist
        # are intentionally different on each leaf, with camber across the rib.
        breadth=(math.sin(math.pi*t)**(.7+.38*(.5+.5*math.sin(phase*2.7))))*width*(.88+.12*t)
        edge=1+.040*math.sin(t*27+phase)+.020*math.sin(t*51+phase*2)
        # Sparse losses at a margin, broad enough to survive the source mesh.
        if v>0 and int(phase)%3==0:edge-=.19*math.exp(-((t-.62)/.065)**2)
        if v<0 and int(phase)%4==1:edge-=.14*math.exp(-((t-.41)/.055)**2)
        curl=.15*length*math.sin(math.pi*t*.95+phase*.15)*t
        droop=(.25+.08*math.sin(phase) if not aquatic else .12)*length*t*t*t
        rip=(.009 if not aquatic else .014)*math.sin(t*14+phase)*abs(v)**1.5
        twist=math.sin(t*3.7+phase)*.28
        return base+d*(length*t)+side*(breadth*v*edge+(.08 if aquatic else .04)*length*math.sin(t*4+phase)*t)+normal*(curl+(1-v*v)*width*.07+v*breadth*twist+rip)-Vector((0,droop,0))
    for layer in (0,1):
        for i in range(rows+1):
            t=.003+(.994*i/rows)
            for j in range(cols+1):
                v=2*j/cols-1;p=point(t,v)+normal*(.0015 if layer==0 else -.0015)
                verts.append(p)
    stride=cols+1;plane=(rows+1)*stride
    for layer in (0,1):
        off=layer*plane
        for i in range(rows):
            for j in range(cols):
                f=(off+i*stride+j,off+i*stride+j+1,off+(i+1)*stride+j+1,off+(i+1)*stride+j)
                faces.append(f if layer==0 else tuple(reversed(f)))
                mi.append(1 if layer else 0)
    boundary=list(range(stride))+[i*stride+cols for i in range(1,rows+1)]+[rows*stride+j for j in range(cols-1,-1,-1)]+[i*stride for i in range(rows-1,0,-1)]
    for i,a in enumerate(boundary):
        b=boundary[(i+1)%len(boundary)];faces.append((a,b,b+plane,a+plane));mi.append(2)
    mats=[kelp if aquatic else (young if int(phase)%4==0 else leaf),under,kelpedge if aquatic else rib]
    mesh(name,verts,faces,mats,mi)
    pts=[point(i/rows,0)+normal*.003 for i in range(rows+1)]
    sweep(name+' | seated midrib',pts,.0025 if aquatic else .0015,[kelpedge if aquatic else rib],5)
    # Secondary veins are actual attached geometry; kept shallow and sparse.
    if not aquatic:
        for i in (6,10,14,18):
            for sign in (-1,1):
                pts=[point((i+1.8*s)/rows,sign*s*.87)+normal*.002 for s in (0,.25,.5,.75,1)]
                sweep(name+' | side vein',pts,.0008,[rib],4)

collection('vault-rooted-bush','FOUNDRY_VAULT_BUSH')
for k in range(7):
    a=math.tau*k/7
    sweep('Root | buried taper',[(0,.13,0),(.10*math.cos(a),.018,.06*math.sin(a)),(.22*math.cos(a),-.075,.15*math.sin(a))],.024,[wood,bark],7)
branches=[((0,.05,0),(-.40,.77,-.05)),((.02,.06,0),(.38,.94,.01)),((-.02,.10,-.02),(-.08,1.15,-.16)),((0,.08,0),(.54,.54,.08)),((0,.07,.03),(-.49,.43,.19))]
for k,(a,b) in enumerate(branches):
    pts=curve(a,b,(-1 if k%2 else 1)*.05,12);sweep('Woody fork %02d'%k,pts,.029,[wood,bark],8)
    for j,t in enumerate((.33,.55,.76,.95)):
        attach=Vector(a).lerp(Vector(b),t);attach.x+=(-1 if k%2 else 1)*.05*math.sin(math.pi*t)
        attach.z+=.035*math.sin(math.pi*t)
        sign=1 if (j+k)%2 else -1
        tip=attach+Vector((sign*(.075+.02*j),.085,.025*math.sin(k+j)))
        sweep('Attached petiole',curve(attach,tip,.012,5),.008,[wood],6)
        direction=(sign*(.68+.11*j),.70-.12*j,.18*math.sin(k*2+j))
        blade('Broad curled leaf %02d.%02d'%(k,j),tip,direction,.29+.05*((k+j)%4),.071+.014*((k*2+j)%4),k*7+j)

collection('drowned-rooted-fronds','FOUNDRY_DROWNED_FRONDS')
frond_lengths=(.48,.87,1.35,.65,1.10,1.55,.42,.95,1.24,.30,.36,.56)
frond_widths=(.043,.075,.062,.052,.085,.069,.038,.059,.082,.026,.038,.031)
for k in range(len(frond_lengths)):
    a=k*2.399963;base=(.075*math.cos(a),.015,.075*math.sin(a))
    sweep('Root crown',[(base[0],.13,base[2]),base,(base[0]*1.7,-.08,base[2]*1.7)],.016,[wood,kelpedge],7)
    blade('Water curled ribbon %02d'%k,(base[0],.075,base[2]),(.34*math.cos(a),1,.20*math.sin(a)),frond_lengths[k],frond_widths[k],k*3.37,True)

def fmt(v):
    s=f'{v:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
lines=['/* Generated by scripts/blender_foliage_assets.py; rooted volumetric plants. */','#ifndef FOLIAGE_ASSETS_H','#define FOLIAGE_ASSETS_H','#include "foundry_assets.h"']
def array(name,ctype,values):
    lines.append('static const '+ctype+' '+name+'[] = {')
    for i in range(0,len(values),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in values[i:i+12])+',')
    lines.append('};')
manifest={'axes':'X right,Y up,Z toward camera','pivot':'root at origin','blender_version':bpy.app.version_string,'profile':'conventional real-time','external_dependencies':[], 'artistic_state':'work_in_progress','assets':{}}
for slug,symbol,col in assets:
    bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();groups={};lo=[1e9]*3;hi=[-1e9]*3;copies=[]
    delivery=bpy.data.collections.new('EXPORT | temporary');scene.collection.children.link(delivery)
    for src in col.objects:
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.triangulate(bm,faces=list(bm.faces))
        assert all(f.calc_area()>1e-12 for f in bm.faces),src.name
        bm.to_mesh(me);bm.free();me.calc_loop_triangles()
        o=bpy.data.objects.new(src.name+' export',me);delivery.objects.link(o);copies.append(o)
        for t in me.loop_triangles:
            mat=me.materials[t.material_index];g=groups.setdefault(mat.name,{'mat':mat,'p':[],'n':[],'i':[],'map':{}})
            for vi,li in zip(t.vertices,t.loops):
                p=me.vertices[vi].co;n=me.corner_normals[li].vector;p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y)
                assert all(math.isfinite(v) for v in (*p,*n))
                assert abs(sum(v*v for v in n)-1)<1e-4
                key=tuple(round(v,6) for v in (*p,*n));idx=g['map'].get(key)
                if idx is None:
                    idx=len(g['p'])//3;g['map'][key]=idx;g['p'].extend(p);g['n'].extend(n)
                    for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                g['i'].append(idx)
    desc=[];tris=0
    for k,g in enumerate(groups.values()):
        pre=slug.replace('-','_')+'_'+str(k);nv=len(g['p'])//3;assert nv<65536
        for suffix,ctype in (('p','float'),('n','float'),('i','unsigned short')):array(pre+'_'+suffix,ctype,g[suffix])
        n=g['mat'].node_tree.nodes.get('Principled BSDF');rgba=[round(srgb(c)*255) for c in n.inputs['Base Color'].default_value[:3]]+[255]
        desc.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(g['i'])),'{'+','.join(map(str,rgba))+'}','0.0f',fmt(n.inputs['Roughness'].default_value),'0.0f'])+'}');tris+=len(g['i'])//3
    assert len(groups)<=8
    lines.append('static const FoundryMeshData '+symbol+'_MESHES[] = {'+','.join(desc)+'};')
    lines.append('static const FoundryAssetData '+symbol+' = {'+symbol+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(hi[i]-lo[i]) for i in range(3))+'}};')
    for o in scene.objects:o.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
    path=MODELS/(slug+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(path),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
    manifest['assets'][slug]={'symbol':symbol,'triangles':tris,'material_meshes':len(groups),'bounds_min':lo,'bounds_max':hi,'dimensions':[hi[i]-lo[i] for i in range(3)],'materials':list(groups),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n')
(OUT/'foliage-manifest.json').write_text(json.dumps(manifest,indent=2))

stage=bpy.data.collections.new('PRESENTATION | excluded from exports');scene.collection.children.link(stage)
world=bpy.data.worlds.new('Neutral studio');world.use_nodes=True;scene.world=world
world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.11,.14,.16,1)
world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.42
for name,loc,power,color,size in [('Key',(-3,-4,5),480,(1,.91,.79),4),('Fill',(3,-1,3),220,(.73,.86,1),3),('Rim',(1,3,3),350,(.8,1,.90),3)]:
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.color=color;d.size=size
    o=bpy.data.objects.new(name,d);stage.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,.5))-o.location).to_track_quat('-Z','Y').to_euler()
cd=bpy.data.cameras.new('Plant review');cam=bpy.data.objects.new('Plant review',cd);stage.objects.link(cam);scene.camera=cam;cd.type='ORTHO';cd.ortho_scale=1.8
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
for slug,symbol,col in assets:
    for _,_,c in assets:c.hide_render=c!=col;c.hide_viewport=c!=col
    a=manifest['assets'][slug];center=Vector(xyz(tuple((a['bounds_min'][i]+a['bounds_max'][i])/2 for i in range(3))))
    for name,offset in [('front',(0,-5,.15)),('side',(5,0,.5)),('three-quarter',(3,-5,2))]:
        cam.location=center+Vector(offset);cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(REVIEW/(slug+'-'+name+'.png'));bpy.ops.render.render(write_still=True)
for _,_,c in assets:c.hide_render=c!=assets[0][2];c.hide_viewport=c!=assets[0][2]
a=manifest['assets'][assets[0][0]]
center=Vector(xyz(tuple((a['bounds_min'][i]+a['bounds_max'][i])/2 for i in range(3))))
cam.location=center+Vector((3,-5,2));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'foliage.blend'))
print('FOLIAGE_COMPLETE '+json.dumps(manifest))
