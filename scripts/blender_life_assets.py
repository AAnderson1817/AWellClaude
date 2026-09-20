"""Small original creature bodies and props using the existing game's palettes.
Rigid source bodies only: runtime retains eyes, feet, wings, tails and behavior.
"""
import bpy,bmesh,math,re,json,hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review';MODELS=ROOT/'public'/'models';HEADER=ROOT/'src'/'generated'/'life_assets.h'
scene=bpy.data.scenes.new('LIFE | original compact forms');bpy.context.window.scene=scene
for s in list(bpy.data.scenes):
    if s!=scene:bpy.data.scenes.remove(s)
palette={n:tuple(map(int,v.split(',')[:3])) for n,v in re.findall(r'Color\s+(pal\w+)\s*=\s*\{\s*([^}]+)\}',(ROOT/'src'/'render.c').read_text())}
def linear(c):
    c/=255;return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
def material(name,rgb,rough=.7,metal=0,emit=0):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');color=tuple(linear(c) for c in rgb)
    n.inputs['Base Color'].default_value=(*color,1);n.inputs['Roughness'].default_value=rough;n.inputs['Metallic'].default_value=metal
    if emit:n.inputs['Emission Color'].default_value=(*color,1);n.inputs['Emission Strength'].default_value=emit
    m.diffuse_color=(*color,1);return m
skin=material('Life | original palSkin',palette['palSkin'],.66)
skinunder=material('Life | original palSkinDeep',palette['palSkinDeep'],.75)
fur=material('Life | original palFur',palette['palFur'],.81)
furunder=material('Life | original palFurLight',palette['palFurLight'],.79)
bird=material('Life | original palBird',palette['palBird'],.73)
birdlight=material('Life | original palBirdLight',palette['palBirdLight'],.71)
clay=material('Prop | original palClay',palette['palClay'],.78)
claylip=material('Prop | original palClayLit',palette['palClayLit'],.64)
claydark=material('Prop | ceramic cavity',(80,49,39),.87)
iron=material('Prop | original palLampIron',palette['palLampIron'],.39,.80)
ironworn=material('Prop | worn lamp iron',(110,101,88),.35,.8)
opal=material('Prop | opaque warm lamp glass',palette['palLampGlass'],.28,.05,.72)
current=None;assets=[]
def collection(slug,symbol,pivot):
    global current
    current=bpy.data.collections.new('ASSET | '+slug);scene.collection.children.link(current);assets.append((slug,symbol,current,pivot));return current
def xyz(p):return (p[0],-p[2],p[1])
def mesh(name,verts,faces,mat,bevel=0,smooth=True,mi=None):
    me=bpy.data.meshes.new(name);me.from_pydata([xyz(p) for p in verts],[],faces);me.update();bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new(name,me);current.objects.link(o)
    mats=mat if isinstance(mat,list) else [mat]
    for m in mats:me.materials.append(m)
    for i,p in enumerate(me.polygons):p.use_smooth=smooth;p.material_index=mi[i] if mi else 0
    if bevel:
        mod=o.modifiers.new('Soft manufactured edge','BEVEL');mod.width=bevel;mod.segments=2
    return o
def cubic(points,steps=3):
    out=[]
    for i in range(len(points)-1):
        p0=Vector(points[max(0,i-1)]);p1=Vector(points[i]);p2=Vector(points[i+1]);p3=Vector(points[min(i+2,len(points)-1)])
        for j in range(steps):
            t=j/steps;out.append(tuple(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t)))
    out.append(points[-1]);return out
def lathe(name,profile,mat,n=40,elliptic=1,closed=True,band=None):
    verts=[];faces=[];mi=[]
    for r,y in profile:
        for i in range(n):
            a=math.tau*i/n;verts.append((max(0,r)*math.cos(a),y,max(0,r)*math.sin(a)*elliptic))
    pairs=len(profile) if closed else len(profile)-1
    for j in range(pairs):
        jj=(j+1)%len(profile)
        for i in range(n):
            faces.append((j*n+i,j*n+(i+1)%n,jj*n+(i+1)%n,jj*n+i));mi.append(band(j,i) if band else 0)
    return mesh(name,verts,faces,mat,mi=mi)
def longitudinal(name,profile,mat,n=32,steps=3,underside=False):
    # x, vertical center, vertical radius, depth radius. Rounded compact shapes
    # are modeled as one surface rather than overlapping body primitives.
    pp=cubic(profile,steps);verts=[];faces=[];mi=[]
    for x,y,ry,rz in pp:
        for i in range(n):
            a=math.tau*i/n;verts.append((x,y+max(0,ry)*math.cos(a),max(0,rz)*math.sin(a)))
    for j in range(len(pp)-1):
        for i in range(n):
            faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i));mi.append(1 if underside and math.cos(math.tau*(i+.5)/n)<-.70 else 0)
    faces.append(tuple(reversed(range(n))));mi.append(0)
    faces.append(tuple((len(pp)-1)*n+i for i in range(n)));mi.append(0)
    return mesh(name,verts,faces,mat,mi=mi)
def sweep(name,points,r,mat,sides=8):
    verts=[];faces=[]
    for i,p in enumerate(points):
        p=Vector(p);t=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(i-1,0)])).normalized()
        axis=Vector((0,0,1));s=t.cross(axis).normalized();b=t.cross(s).normalized()
        if s.length<.1:s=Vector((1,0,0));b=t.cross(s).normalized()
        for k in range(sides):a=math.tau*k/sides;verts.append(p+r*(math.cos(a)*s+math.sin(a)*b))
    for i in range(len(points)-1):
        for k in range(sides):faces.append((i*sides+k,i*sides+(k+1)%sides,(i+1)*sides+(k+1)%sides,(i+1)*sides+k))
    faces.append(tuple(reversed(range(sides))));faces.append(tuple((len(points)-1)*sides+k for k in range(sides)))
    return mesh(name,verts,faces,mat)
def box(name,lo,hi,mat,bevel=.008):
    v=[(x,y,z) for z in (lo[2],hi[2]) for y in (lo[1],hi[1]) for x in (lo[0],hi[0])]
    return mesh(name,v,[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)],mat,bevel,False)

# A hollow, wheel-thrown pot. The mouth descends to a genuine interior floor.
collection('ceramic-pot','FOUNDRY_POT','ground-center')
pot_profile=[(0,.045),(.12,.045),(.16,.055),(.195,.105),(.25,.18),(.285,.29),(.30,.37),(.288,.455),(.25,.525),(.204,.58),(.185,.62),(.191,.67),(.217,.684),(.222,.702),(.214,.718),(.189,.721),(.174,.703),(.169,.672),(.167,.621),(.184,.585),(.23,.514),(.264,.45),(.273,.37),(.26,.29),(.225,.19),(.166,.10),(.11,.083),(0,.083)]
lathe('Thrown ceramic body and cavity',pot_profile,[clay,claylip,claydark],48,band=lambda j,i:1 if 11<=j<=15 else (2 if j>=16 else 0))
lathe('Substantial foot ring',[(.13,0),(.161,.0),(.175,.025),(.17,.052),(.143,.059),(.13,.045)],claydark,40)
# The reference's orbital motif hugs the vessel's actual varying shoulder radius.
def potr(y):
    outer=pot_profile[1:12]
    for (r0,y0),(r1,y1) in zip(outer,outer[1:]):
        if y0<=y<=y1:return r0+(r1-r0)*(y-y0)/(y1-y0)
    return .28
for offset in (-.012,.012):
    pts=[]
    for i in range(81):
        a=math.tau*i/80;y=.41+.058*math.cos(a)+offset;r=potr(y)+.0005;pts.append((r*math.cos(a),y,r*math.sin(a)))
    sweep('Incised orbital slip line',pts,.0032,claydark,6)

# Hunter lamp: seated fuel tank, opal amber chamber, iron side rails, vented cap,
# riveted handle. The compact height follows the existing carried-item silhouette.
collection('hunter-lantern','FOUNDRY_HUNTER_LANTERN','body-center; main tank/glass y[-.31,.31], handle above')
lathe('Fuel reservoir casting',[(0,-.3125),(.185,-.3125),(.228,-.29),(.25,-.265),(.247,-.235),(.205,-.215),(.155,-.194),(.13,-.175),(0,-.175)],iron,40,elliptic=.72)
lathe('Rubbed tank foot',[(.23,-.277),(.251,-.267),(.251,-.254),(.23,-.25)],ironworn,40,elliptic=.72)
lathe('Opaque warm opal chamber',[(0,-.188),(.106,-.188),(.132,-.145),(.145,-.060),(.139,.054),(.119,.139),(.102,.184),(0,.184)],opal,40,elliptic=.76)
lathe('Iron chimney collar',[(.115,.165),(.147,.168),(.156,.183),(.151,.198),(.12,.208),(.10,.218),(.084,.253),(.112,.263),(.129,.275),(.127,.286),(.066,.302),(0,.302),(0,.165)],iron,40,elliptic=.8)
lathe('Upper rolled chimney edge',[(.117,.265),(.133,.274),(.133,.283),(.119,.286)],ironworn,36,elliptic=.8)
for s in (-1,1):
    p=cubic([(s*.185,-.24,0),(s*.214,-.17,0),(s*.212,.075,0),(s*.183,.194,0),(s*.12,.216,0)],4)
    sweep('Single forged side rail',p,.022,iron,8)
    box('Rail seated rivet',(s*.19-.025,-.18,.015),(s*.19+.025,-.135,.042),ironworn,.01)
for i in range(8):
    a=math.tau*i/8;r=.105;y=.229
    # Recessed ventilation marks on the solid alpha-free cap.
    o=box('Dark vent recess',(r*math.cos(a)-.007,y-.009,r*.8*math.sin(a)-.007),(r*math.cos(a)+.007,y+.009,r*.8*math.sin(a)+.007),iron,.004)
pts=[]
for i in range(37):
    a=math.pi*i/36;pts.append((.133*math.cos(a),.278+.155*math.sin(a),0))
sweep('Arched carrying handle',pts,.013,ironworn,8)
for s in (-1,1):sweep('Handle hinge pin',[(s*.129,.278,-.033),(s*.129,.278,.033)],.019,iron,8)
# Two seated diagonal guards communicate real protection at the small screen scale.
for s in (-1,1):sweep('Front glass guard',[(s*-.105,-.196,.075),(0,-.01,.128),(s*.105,.174,.077)],.008,ironworn,8)

# Rigid original seed body. No eyes, ear nubs, feet, mouth or implied squash rig.
collection('player-seed','FOUNDRY_PLAYER_SEED','ground-center; rigid original6x11pixel body')
seed=[(0,0),(.13,.025),(.235,.08),(.316,.18),(.363,.34),(.375,.50),(.366,.72),(.343,.94),(.298,1.12),(.228,1.27),(.125,1.35),(0,1.375)]
seed=cubic(seed,2)
lathe('Continuous rigid seed body',seed,[skin,skinunder],40,elliptic=.773333,closed=False,band=lambda j,i:1 if j<3 else 0)

# Quiet long-backed native animal of the original room; expressive extras stay in code.
collection('beast-body','FOUNDRY_BEAST_BODY','body-center; longitudinal axis+X')
longitudinal('Long-backed compact animal body',[(-.75,-.01,.025,.035),(-.66,.00,.18,.17),(-.43,.005,.28,.25),(-.1,0,.32,.27),(.27,.0,.29,.245),(.54,.025,.24,.21),(.71,.055,.15,.15),(.75,.06,.045,.06)],[fur,furunder],32,2,True)
collection('beast-head','FOUNDRY_BEAST_HEAD','head-center; faces+X, includes ears; no eyes')
longitudinal('Rounded wedge head',[(-.26,.012,.02,.025),(-.19,.017,.13,.135),(-.07,.018,.20,.19),(.10,-.015,.16,.17),(.23,-.066,.089,.115),(.27,-.067,.032,.05)],[fur,furunder],28,2,True)
for z in (-.108,.108):
    # Rounded triangular ear cages retain small upright, silent-creature identity.
    v=[(-.18,.13,z-.046),(-.025,.135,z-.040),(-.10,.35,z-.015),(-.18,.13,z+.046),(-.025,.135,z+.040),(-.10,.35,z+.015)]
    o=mesh('Small rounded animal ear',v,[(0,2,1),(3,4,5),(0,3,5,2),(1,2,5,4),(0,1,4,3)],fur,.015)

collection('bird-body','FOUNDRY_BIRD_BODY','body-center; faces+X; no eyes,beak,feet,wings')
longitudinal('Single pear-shaped small bird',[(-.29,-.065,.018,.024),(-.22,-.036,.065,.075),(-.10,-.015,.158,.14),(.035,.0,.18,.16),(.15,.07,.139,.13),(.23,.145,.13,.114),(.31,.143,.06,.062),(.325,.14,.014,.024)],[bird,birdlight],28,2,True)

def fmt(v):
    s=f'{v:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
lines=['/* Generated by scripts/blender_life_assets.py; original source palettes. */','#ifndef LIFE_ASSETS_H','#define LIFE_ASSETS_H','#include "foundry_assets.h"']
def array(name,ctype,values):
    lines.append('static const '+ctype+' '+name+'[] = {')
    for i in range(0,len(values),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in values[i:i+12])+',')
    lines.append('};')
manifest={'source_palette':'src/render.c','source_shape':'src/player.c,src/items.c,src/life.c','references':['public/art/references/vault-mouth/19-clay-pots.png','public/art/references/vault-mouth/10-pack-dead-lamp.png'],'axes':'X right,Y up,Z front','assets':{}}
total=0
for slug,symbol,col,pivot in assets:
    bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();groups={};lo=[1e9]*3;hi=[-1e9]*3;copies=[]
    delivery=bpy.data.collections.new('Delivery temporary');scene.collection.children.link(delivery)
    for src in col.objects:
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.triangulate(bm,faces=list(bm.faces));dead=[f for f in bm.faces if f.calc_area()<1e-10]
        if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
        bm.to_mesh(me);bm.free();me.calc_loop_triangles();o=bpy.data.objects.new(src.name+' delivery',me);delivery.objects.link(o);copies.append(o)
        for t in me.loop_triangles:
            m=me.materials[t.material_index];g=groups.setdefault(m.name,{'mat':m,'p':[],'n':[],'i':[],'map':{}})
            for vi,li in zip(t.vertices,t.loops):
                p=me.vertices[vi].co;n=me.corner_normals[li].vector;p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y);key=tuple(round(v,6) for v in (*p,*n));idx=g['map'].get(key)
                if idx is None:
                    idx=len(g['p'])//3;g['map'][key]=idx;g['p'].extend(p);g['n'].extend(n)
                    for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                g['i'].append(idx)
    desc=[];tris=0
    for k,g in enumerate(groups.values()):
        pre=slug.replace('-','_')+'_'+str(k);nv=len(g['p'])//3;assert nv<65536
        for suffix,ctype in (('p','float'),('n','float'),('i','unsigned short')):array(pre+'_'+suffix,ctype,g[suffix])
        n=next(n for n in g['mat'].node_tree.nodes if n.type=='BSDF_PRINCIPLED');rgba=[round(srgb(c)*255) for c in n.inputs['Base Color'].default_value[:3]]+[255]
        emit=n.inputs['Emission Strength'].default_value if any(n.inputs['Emission Color'].default_value[:3]) else 0
        desc.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(g['i'])),'{'+','.join(map(str,rgba))+'}',fmt(n.inputs['Metallic'].default_value),fmt(n.inputs['Roughness'].default_value),fmt(emit)])+'}');tris+=len(g['i'])//3
    lines.append('static const FoundryMeshData '+symbol+'_MESHES[] = {'+','.join(desc)+'};')
    lines.append('static const FoundryAssetData '+symbol+' = {'+symbol+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(hi[i]-lo[i]) for i in range(3))+'}};')
    for o in scene.objects:o.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
    bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
    manifest['assets'][slug]={'symbol':symbol,'triangles':tris,'material_meshes':len(groups),'bounds_min':lo,'bounds_max':hi,'dimensions':[hi[i]-lo[i] for i in range(3)],'pivot':pivot,'materials':list(groups)};total+=tris
lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n');manifest['total_triangles']=total;assert total<18000,total
(OUT/'life-manifest.json').write_text(json.dumps(manifest,indent=2))

# Diagnostic staging: no baked lights/textures and no stage geometry in exports.
stage=bpy.data.collections.new('PRESENTATION | small asset studio');scene.collection.children.link(stage);current=stage
floor=box('Studio ground',(-100,-.03,-100),(100,-.025,100),material('Studio | ground',(42,49,54),.85),0)
world=bpy.data.worlds.new('Small asset world');world.use_nodes=True;scene.world=world
bg=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.10,.14,.17,1);bg.inputs['Strength'].default_value=.32
def light(name,loc,power,color,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.color=color;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(name,d);stage.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,.4))-o.location).to_track_quat('-Z','Y').to_euler()
light('Soft neutral key',(-3,-4,5),450,(1,.9,.77),4);light('Quiet cool fill',(3,-1,3),260,(.66,.85,1),3);light('Rim',(1,3,4),400,(.83,1,.91),3)
cd=bpy.data.cameras.new('Small asset review');cam=bpy.data.objects.new('Small asset review',cd);stage.objects.link(cam);scene.camera=cam;cd.type='ORTHO'
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
for slug,symbol,col,pivot in assets:
    for _,_,c,_ in assets:c.hide_render=c!=col;c.hide_viewport=c!=col
    a=manifest['assets'][slug];lo=a['bounds_min'];hi=a['bounds_max'];center=Vector(xyz(tuple((lo[i]+hi[i])/2 for i in range(3))))
    floor.hide_render=lo[1]<-.02
    cam.location=center+Vector((3,-8,3));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cd.ortho_scale=max(a['dimensions'])*1.35
    scene.render.filepath=str(REVIEW/(slug+'-beauty.png'));bpy.ops.render.render(write_still=True)
for _,_,c,_ in assets:c.hide_render=c!=assets[0][2];c.hide_viewport=c!=assets[0][2]
floor.hide_render=False;cam.location=(2,-5,2.3);cam.rotation_euler=(Vector((0,0,.36))-cam.location).to_track_quat('-Z','Y').to_euler();cd.ortho_scale=1.0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'life-props.blend'))
print('LIFE_ASSETS_COMPLETE '+json.dumps(manifest))
