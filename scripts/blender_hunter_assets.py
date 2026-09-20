"""Original tailored hunter hat/pack; isolated source, then reviewed delivery.
--study builds source + diagnostic images without exporting.
--deliver reopens the reviewed source, exports and verifies a clean GLB reimport.
"""
import bpy, bmesh, math, json, hashlib, sys, importlib.util
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/blender'; REVIEW=OUT/'review/hunter-attire'
SOURCE=OUT/'hunter-attire.blend'; GLB=ROOT/'public/models/hunter-attire.glb'
HEADER=ROOT/'src/generated/hunter_assets.h'
REVIEW.mkdir(parents=True,exist_ok=True)
ARGS=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
MODE='deliver' if '--deliver' in ARGS else 'study'
def xyz(p):return (p[0],-p[2],p[1])
def game(p):return (p[0],p[2],-p[1])
def linear(c):
    c/=255;return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def material(name,rgb,rough=.8,metal=0):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes.get('Principled BSDF')
    color=tuple(linear(c) for c in rgb);n.inputs['Base Color'].default_value=(*color,1)
    n.inputs['Roughness'].default_value=rough;n.inputs['Metallic'].default_value=metal;m.diffuse_color=(*color,1)
    return m
def collection(name):
    c=bpy.data.collections.new(name);bpy.context.scene.collection.children.link(c);return c
def mesh(name,verts,faces,mat,col,smooth=True):
    me=bpy.data.meshes.new(name);me.from_pydata([xyz(p) for p in verts],[],faces);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new(name,me);col.objects.link(o);me.materials.append(mat)
    for p in me.polygons:p.use_smooth=smooth
    return o
def thick(o,t):
    m=o.modifiers.new('Tailored thickness and closed hem','SOLIDIFY');m.thickness=t;m.offset=0
def curve(points,steps=3):
    out=[]
    for i in range(len(points)-1):
        p0=points[max(0,i-1)];p1=points[i];p2=points[i+1];p3=points[min(i+2,len(points)-1)]
        for j in range(steps):
            t=j/steps;out.append(tuple(.5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t) for a,b,c,d in zip(p0,p1,p2,p3)))
    out.append(points[-1]);return out
def tube(name,points,r,mat,col,sides=6):
    clean=[]
    for p in points:
        if not clean or (Vector(p)-Vector(clean[-1])).length>1e-7:clean.append(p)
    points=clean
    closed=(Vector(points[0])-Vector(points[-1])).length<1e-7
    if closed:points=points[:-1]
    tangents=[]
    for i in range(len(points)):
        before=points[(i-1)%len(points)] if closed else points[max(i-1,0)];after=points[(i+1)%len(points)] if closed else points[min(i+1,len(points)-1)]
        tangents.append((Vector(after)-Vector(before)).normalized())
    axes=[Vector((1,0,0)),Vector((0,1,0)),Vector((0,0,1))]
    axis=min(axes,key=lambda a:max(abs(t.dot(a)) for t in tangents))
    v=[];f=[]
    for i,p in enumerate(points):
        p=Vector(p);t=tangents[i];s=t.cross(axis)
        assert s.length>.1,(name,'unstable frame')
        s.normalize();b=t.cross(s).normalized()
        for j in range(sides):a=j*math.tau/sides;v.append(p+r*(math.cos(a)*s+math.sin(a)*b))
    for i in range(len(points) if closed else len(points)-1):
        k=(i+1)%len(points)
        for j in range(sides):f.append((i*sides+j,i*sides+(j+1)%sides,k*sides+(j+1)%sides,k*sides+j))
    if not closed:f.extend([tuple(reversed(range(sides))),tuple((len(points)-1)*sides+j for j in range(sides))])
    return mesh(name,v,f,mat,col)
def ribbon(name,points,width,mat,col,thickness=.014):
    pp=curve(points,3);v=[];f=[]
    for x,y,z in pp:v.extend([(x-width*.5,y,z),(x+width*.5,y,z)])
    for i in range(len(pp)-1):f.append((i*2,i*2+1,i*2+3,i*2+2))
    o=mesh(name,v,f,mat,col);thick(o,thickness);return o
def loft(name,rings,mat,col,n=40,tailored=False):
    # Each ring is (centerX,height,centerZ,radiusX,radiusZ), all editable quads.
    v=[];f=[]
    for k,(x,y,z,rx,rz) in enumerate(rings):
        for j in range(n):
            a=j*math.tau/n;fold=1+.012*math.sin(3*a+k*.6)+.007*math.sin(5*a-k*.9)
            ca,sa=math.cos(a),math.sin(a)
            if tailored:
                # Broad sewn panels with soft corners. A few tension creases alter
                # the cloth itself; there are no floating wrinkle strips.
                ca=math.copysign(abs(ca)**.58,ca);sa=math.copysign(abs(sa)**.58,sa)
                tension=math.exp(-((y-.52)/.18)**2)*math.sin(a*4+2.4*y)
                fold=1+.022*tension+.01*math.sin(a*3+.6*k)
            v.append((x+rx*ca*fold,y,z+rz*sa*fold))
    for k in range(len(rings)-1):
        for j in range(n):f.append((k*n+j,k*n+(j+1)%n,(k+1)*n+(j+1)%n,(k+1)*n+j))
    f.extend([tuple(reversed(range(n))),tuple((len(rings)-1)*n+j for j in range(n))])
    return mesh(name,v,f,mat,col)
def buckle(name,x,y,z,mat,col,width=.09,height=.08):
    pts=[(x-width/2,y-height/2,z),(x-width/2,y+height/2,z),(x+width/2,y+height/2,z),(x+width/2,y-height/2,z),(x-width/2,y-height/2,z)]
    tube(name+' forged frame',curve(pts,2),.008,mat,col,6)
    tube(name+' seated tongue',[(x,y-height/2,z),(x,y+height*.25,z-.008)],.0045,mat,col,6)

def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene=bpy.context.scene;scene.name='HUNTER | tailored camp attire'
    scene['protected_seed_source_sha256']=digest(OUT/'life-props.blend')
    scene['asset_contract']='assets/blender/hunter-attire-contract.md'
    hat=collection('ASSET | hunter-hat');pack=collection('ASSET | hunter-pack');reference=collection('REFERENCE | unchanged player seed')
    # Append only this known reference object. Its geometry and original file stay untouched.
    with bpy.data.libraries.load(str(OUT/'life-props.blend'),link=False) as (src,dst):
        dst.objects=[n for n in src.objects if n=='Continuous rigid seed body']
    assert len(dst.objects)==1 and dst.objects[0]
    reference.objects.link(dst.objects[0]);dst.objects[0].name='REFERENCE | unchanged 6x11 seed'
    # Presentation-only eye guides at the existing renderer's positions. These
    # are excluded from all attire exports and do not modify the reference seed.
    eyes=material('REFERENCE | existing eye placement',(13,25,28),.8)
    for side in (-1,1):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,location=xyz((side*.21+.035,1.375*.73,.30)))
        o=bpy.context.object;o.name='REFERENCE | eye guide';o.scale=(.062,.025,.092)
        for c in list(o.users_collection):c.objects.unlink(o)
        reference.objects.link(o);o.data.materials.append(eyes)
        for p in o.data.polygons:p.use_smooth=True
    felt=material('Hunter | warm worn felt',(128,106,67),.91)
    hem=material('Hunter | rubbed binding',(150,126,83),.87)
    canvas=material('Hunter | waxed canvas',(103,87,57),.88)
    leather=material('Hunter | dark leather',(60,43,28),.68)
    bronze=material('Hunter | restrained bronze',(115,96,65),.43,.72)
    lining=material('Hunter | dark cloth interior',(65,56,39),.96)
    # Soft shaped crown: open bottom under solidification, pinched top with a low lean.
    n=48;rings=[(.287,1.184),(.324,1.245),(.316,1.32),(.297,1.45),(.275,1.535),(.236,1.58),(.15,1.595)]
    v=[];f=[]
    for k,(r,y) in enumerate(rings):
        for j in range(n):
            a=j*math.tau/n;pinch=1-.052*math.cos(2*a)*(k/(len(rings)-1))
            v.append((r*math.cos(a)*pinch+.026*k/(len(rings)-1),y+.009*math.sin(a*2+k*.3),r*.79*math.sin(a)))
    for k in range(len(rings)-1):
        for j in range(n):f.append((k*n+j,k*n+(j+1)%n,(k+1)*n+(j+1)%n,(k+1)*n+j))
    f.append(tuple((len(rings)-1)*n+j for j in range(n)))
    crown_points=list(v)
    o=mesh('Felt crown | open fitted underside, shaped crease',v,f,felt,hat);thick(o,.018)
    # Brim is an annular thick cloth surface, not a scaled disk.
    v=[];f=[];rim=[]
    for k in range(5):
        u=k/4;rx=.276+u*.294;rz=.215+u*.192
        for j in range(n):
            a=j*math.tau/n;y=1.218+.013*math.cos(a)-.035*u*math.sin(a)+.018*u*u*math.cos(2*a+.4)
            p=(rx*math.cos(a)+.013*u,y,rz*math.sin(a));v.append(p)
            if k==4:rim.append(p)
    for k in range(4):
        for j in range(n):f.append((k*n+j,k*n+(j+1)%n,(k+1)*n+(j+1)%n,(k+1)*n+j))
    o=mesh('Uneven supple brim | annular cloth',v,f,felt,hat);thick(o,.020)
    tube('Brim rolled hem',rim+[rim[0]],.009,hem,hat,6)
    # The band follows the evaluated crown's ruled surface at every sample.
    # Offset includes crown thickness; lower band face stays on its felt seat.
    bv=[];bf=[]
    for y in (1.226,1.248,1.280):
        for j in range(n):
            k=next(k for k in range(len(rings)-1) if crown_points[k*n+j][1]<=y<=crown_points[(k+1)*n+j][1])
            p0=Vector(crown_points[k*n+j]);p1=Vector(crown_points[(k+1)*n+j]);p=p0.lerp(p1,(y-p0.y)/(p1.y-p0.y))
            direction=Vector((p.x,0,p.z)).normalized();bv.append(tuple(p+direction*.018))
    for k in range(2):
        for j in range(n):bf.append((k*n+j,k*n+(j+1)%n,(k+1)*n+(j+1)%n,(k+1)*n+j))
    band=mesh('Crown band | surface-fitted leather',bv,bf,leather,hat);thick(band,.016)
    liner=loft('Hat | fitted dark inner sweatband',[(0,1.176,0,.280,.218),(0,1.218,0,.282,.218)],lining,hat,48)
    bm=bmesh.new();bm.from_mesh(liner.data);bmesh.ops.delete(bm,geom=[p for p in bm.faces if len(p.verts)>4],context='FACES');bm.to_mesh(liner.data);bm.free();thick(liner,.016)
    buckle('Hat band buckle',-.16,1.251,.252,bronze,hat,.058,.046)
    # Pack mass: broad compressed panels, an asymmetric loaded center of mass.
    packrings=[(-.225,.31,-.40,.19,.12),(-.235,.35,-.42,.27,.16),(-.25,.44,-.43,.306,.174),
               (-.248,.66,-.43,.32,.18),(-.235,.87,-.418,.298,.172),(-.215,.99,-.397,.25,.145),(-.20,1.015,-.39,.19,.11)]
    # Extra intermediate rings make the few broad cloth creases part of the cage.
    packrings=curve(packrings,2)
    loft('Pack | compressed waxed-canvas body',packrings,canvas,pack,48,tailored=True)
    # Broad rear flap follows the back of the bag, ends in a shallow scallop.
    rows=curve([(1.006,-.39,.22),(1.04,-.49,.27),(.99,-.588,.292),(.88,-.62,.289),(.756,-.62,.26)],2)
    v=[];f=[];left=[];right=[]
    for k,(y,z,w) in enumerate(rows):
        for j in range(13):
            u=-1+2*j/12;x=-.23+w*u
            p=(x,y+(.025*u*u if k==len(rows)-1 else .008*math.cos(u*math.pi)),z+.025*u*u)
            v.append(p)
            if j==0:left.append(p)
            if j==12:right.append(p)
    for k in range(len(rows)-1):
        for j in range(12):f.append((k*13+j,k*13+j+1,(k+1)*13+j+1,(k+1)*13+j))
    o=mesh('Pack | overlapping curved flap',v,f,canvas,pack);thick(o,.018)
    outline=left+v[-13:]+list(reversed(right));tube('Flap | worn continuous binding',outline,.009,hem,pack,6)
    # Shoulder straps hug the upper side rather than crossing the eyes/crown.
    for side in (-1,1):
        backx=-.42 if side<0 else -.045
        points=[(backx,.89,-.47),(side*.30,.975,-.23),(side*.338,1.022,-.09),
                (side*.34,1.035,0),(side*.315,1.02,.10),(side*.28,.94,.18),
                (side*.29,.72,.196),(side*.297,.50,.191),(side*.365,.40,.045),
                (side*.348,.39,-.15),(backx,.395,-.36)]
        ribbon(('Left' if side<0 else 'Right')+' | continuous seated shoulder strap',points,.060,leather,pack)
        # Pack anchor reinforcing tabs: broad enough to carry the strap load.
        ribbon(('Left' if side<0 else 'Right')+' | upper sewn anchor',[(backx,.94,-.49),(backx,.84,-.50)],.095,canvas,pack,.018)
    ribbon('Closure | one broad leather tongue',[(-.23,1.016,-.573),(-.23,.94,-.64),(-.23,.77,-.647),(-.23,.61,-.632),(-.23,.42,-.58)],.065,leather,pack,.016)
    buckle('Closure | seated bronze buckle',-.23,.72,-.664,bronze,pack,.094,.10)
    # Welted side seam follows each panel join; the broad face stays quiet.
    for side in (-1,1):
        pts=[]
        for x,y,z,rx,rz in packrings[1:-1]:
            a=math.pi*.76 if side<0 else math.pi*.24
            ca=math.copysign(abs(math.cos(a))**.58,math.cos(a));sa=math.copysign(abs(math.sin(a))**.58,math.sin(a))
            fold=1+.022*math.exp(-((y-.52)/.18)**2)*math.sin(a*4+2.4*y)+.01*math.sin(a*3+.6*(len(pts)+1))
            pts.append((x+rx*ca*fold,y,z-rz*sa*fold-.004))
        tube('Pack | sewn side panel welt',pts,.005,hem,pack,6)
    # Binding at the worn bottom seam follows the compressed bag perimeter.
    # Bottom seam is part of the loft construction; a decorative ring would cut
    # through its squared corners, so it is deliberately not added separately.
    # Non-destructive source evaluation uses the delivery triangulation, including
    # crown caps, so source shading is representative of the export normals.
    for col in (hat,pack):
        for o in col.objects:
            mod=o.modifiers.new('Final runtime triangulation | non-destructive','TRIANGULATE')
            mod.quad_method='BEAUTY';mod.ngon_method='BEAUTY'
    stage=collection('STUDIO | excluded from export')
    world=bpy.data.worlds.new('Hunter neutral world');world.use_nodes=True;scene.world=world
    world.node_tree.nodes['Background'].inputs['Color'].default_value=(.15,.17,.19,1);world.node_tree.nodes['Background'].inputs['Strength'].default_value=.4
    for name,loc,power,size in [('Soft neutral key',(-3,-4,5),420,4),('Rear construction fill',(2,3,3),300,3),('Front fill',(3,-2,2),180,3)]:
        d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size
        o=bpy.data.objects.new(name,d);stage.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,.8))-o.location).to_track_quat('-Z','Y').to_euler()
    cd=bpy.data.cameras.new('Hunter review');cam=bpy.data.objects.new('Hunter review',cd);stage.objects.link(cam);scene.camera=cam;cd.type='ORTHO'
    cam.location=(4,-7,3.4);cam.rotation_euler=(Vector((-.04,.1,.8))-cam.location).to_track_quat('-Z','Y').to_euler();cd.ortho_scale=1.95
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16;scene.cycles.use_denoising=True
    scene.render.threads_mode='FIXED';scene.render.threads=16
    scene.render.resolution_x=scene.render.resolution_y=700;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
    scene.view_settings.view_transform='AgX';scene.render.film_transparent=False
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE))

def render_views(prefix='source'):
    scene=bpy.context.scene;cam=scene.camera
    views=[('front',(0,-8,.82)),('side',(8,0,.82)),('back',(0,8,.82)),('three-quarter',(4,-7,3.4)),('underside',(3,-6,-1.0))]
    center=Vector((-.04,.1,.8));cam.data.ortho_scale=1.95
    for name,loc in views:
        cam.location=loc;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(REVIEW/(prefix+'-'+name+'.png'));bpy.ops.render.render(write_still=True)
    # Direct camera-scale evidence: no raster image editing or invented detail.
    cam.location=(0,-8,.8);cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.0
    scene.render.resolution_x=scene.render.resolution_y=96
    scene.render.filepath=str(REVIEW/(prefix+'-native-scale.png'));bpy.ops.render.render(write_still=True)
    scene.render.resolution_x=scene.render.resolution_y=700
    clay=bpy.data.materials.get('STUDIO | neutral clay') or material('STUDIO | neutral clay',(160,160,160),.8)
    layer=scene.view_layers[0];layer.material_override=clay
    for name,loc in [('clay-front',(0,-8,.82)),('clay-back',(0,8,.82)),('clay-interface',(4,6,2.6))]:
        cam.location=loc;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.95
        scene.render.filepath=str(REVIEW/(prefix+'-'+name+'.png'));bpy.ops.render.render(write_still=True)
    layer.material_override=None
    wire=bpy.data.materials.get('STUDIO | wire inspection')
    if not wire:
        wire=material('STUDIO | wire inspection',(170,170,170),.8)
        nodes=wire.node_tree.nodes;w=nodes.new('ShaderNodeWireframe');w.use_pixel_size=True;w.inputs['Size'].default_value=.65
        mix=nodes.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.45,.45,.45,1);mix.inputs[2].default_value=(.012,.018,.022,1)
        wire.node_tree.links.new(w.outputs['Fac'],mix.inputs[0]);wire.node_tree.links.new(mix.outputs[0],nodes.get('Principled BSDF').inputs['Base Color'])
    layer.material_override=wire;cam.location=(4,-7,3.4);cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath=str(REVIEW/(prefix+'-wire.png'));bpy.ops.render.render(write_still=True);layer.material_override=None

def fmt(v):
    s=f'{v:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
def deliver():
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE));scene=bpy.context.scene
    assert scene['protected_seed_source_sha256']==digest(OUT/'life-props.blend')
    if '--audit-helper' in ARGS:
        helper=Path(ARGS[ARGS.index('--audit-helper')+1])
        spec=importlib.util.spec_from_file_location('asset_audit',str(helper))
        audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
        audits={name:audit.audit_collection('ASSET | '+name,max_triangles=12000,require_uv=False) for name in ('hunter-hat','hunter-pack')}
        (OUT/'hunter-attire-source-audit.json').write_text(json.dumps(audits,indent=2)+'\n')
        assert all(not report['errors'] for report in audits.values()),audits
    dep=bpy.context.evaluated_depsgraph_get();delivery=collection('DELIVERY | evaluated closed meshes');copies=[]
    lines=['/* Generated by blender_hunter_assets.py; reviewed source only. */','#ifndef HUNTER_ASSETS_H','#define HUNTER_ASSETS_H','#include "foundry_assets.h"']
    manifest={'blender_version':bpy.app.version_string,'axes':'X right,Y up,Z front','pivot':'same ground-center as unchanged player seed','assets':{},'source_sha256':digest(SOURCE),'script_sha256':digest(__file__),'status':'work_in_progress','texture_bytes':0,
              'protected_references':{path:digest(ROOT/path) for path in ['assets/blender/life-props.blend','src/generated/life_assets.h']},
              'reference_images':['public/art/references/vault-mouth/04-hunter-camp.png','public/art/references/vault-mouth/10-pack-dead-lamp.png','public/art/references/vault-mouth/08-hunter-cairn.png']}
    unique=set();total=0
    for slug,symbol in [('hunter-hat','FOUNDRY_HUNTER_HAT'),('hunter-pack','FOUNDRY_HUNTER_PACK')]:
        col=bpy.data.collections['ASSET | '+slug];groups={};lo=[1e9]*3;hi=[-1e9]*3
        for src in col.objects:
            me=bpy.data.meshes.new_from_object(src.evaluated_get(dep),depsgraph=dep)
            bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
            bmesh.ops.triangulate(bm,faces=list(bm.faces));dead=[f for f in bm.faces if f.calc_area()<1e-10]
            if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
            bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();me.calc_loop_triangles()
            o=bpy.data.objects.new(slug+' | '+src.name,me);delivery.objects.link(o);copies.append(o)
            for tri in me.loop_triangles:
                m=me.materials[tri.material_index];unique.add(m.name);g=groups.setdefault(m.name,{'mat':m,'p':[],'n':[],'i':[],'lookup':{}})
                for vi,li in zip(tri.vertices,tri.loops):
                    p=game(me.vertices[vi].co);n=game(me.corner_normals[li].vector);assert all(math.isfinite(a) for a in (*p,*n));assert abs(Vector(n).length-1)<.002
                    key=tuple(round(a,6) for a in (*p,*n));idx=g['lookup'].get(key)
                    if idx is None:
                        idx=len(g['p'])//3;g['lookup'][key]=idx;g['p'].extend(p);g['n'].extend(n)
                        for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                    g['i'].append(idx)
        desc=[];triangles=0
        for j,g in enumerate(groups.values()):
            pre=slug.replace('-','_')+'_'+str(j);nv=len(g['p'])//3;assert nv<65536
            quantized=[Vector(tuple(round(v,6) for v in g['p'][k:k+3])) for k in range(0,len(g['p']),3)]
            for k in range(0,len(g['i']),3):
                a,b,c=[quantized[i] for i in g['i'][k:k+3]]
                assert (b-a).cross(c-a).length*.5>=1e-10,(slug,j,'degenerate after C decimal quantization')
            for suffix,ctype in [('p','float'),('n','float'),('i','unsigned short')]:
                lines.append('static const '+ctype+' '+pre+'_'+suffix+'[] = {')
                for k in range(0,len(g[suffix]),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in g[suffix][k:k+12])+',')
                lines.append('};')
            n=g['mat'].node_tree.nodes.get('Principled BSDF');rgba=[round(srgb(c)*255) for c in n.inputs['Base Color'].default_value[:3]]+[255]
            desc.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(g['i'])),'{'+','.join(map(str,rgba))+'}',fmt(n.inputs['Metallic'].default_value),fmt(n.inputs['Roughness'].default_value),'0.0f'])+'}')
            triangles+=len(g['i'])//3
        lines.append('static const FoundryMeshData '+symbol+'_MESHES[] = {'+','.join(desc)+'};')
        lines.append('static const FoundryAssetData '+symbol+' = {'+symbol+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(hi[k]-lo[k]) for k in range(3))+'}};')
        manifest['assets'][slug]={'symbol':symbol,'triangles':triangles,'materials':list(groups),'bounds_min':lo,'bounds_max':hi};total+=triangles
    assert total<=12000 and len(unique)<=6,(total,unique)
    lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n')
    for o in scene.objects:o.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0]
    bpy.ops.export_scene.gltf(filepath=str(GLB),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    manifest.update(total_triangles=total,unique_material_count=len(unique),header_sha256=digest(HEADER),glb_sha256=digest(GLB),glb_bytes=GLB.stat().st_size)
    (OUT/'hunter-attire-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    # Remove source asset geometry from the clean import scene, retain only studio/reference.
    for name in ['ASSET | hunter-hat','ASSET | hunter-pack','DELIVERY | evaluated closed meshes']:
        c=bpy.data.collections.get(name)
        for o in list(c.objects):bpy.data.objects.remove(o,do_unlink=True)
        bpy.data.collections.remove(c)
    before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(GLB));imported=set(bpy.data.objects)-before
    verification={'source_reopened':True,'glb_reimported':True,'meshes':0,'triangles':0,'degenerate_triangles':0,'invalid_normals':0,'boundary_edges':0,'nonmanifold_edges':0,'external_images':[]}
    for o in imported:
        if o.type!='MESH':continue
        verification['meshes']+=1;me=o.data;me.calc_loop_triangles();verification['triangles']+=len(me.loop_triangles)
        for t in me.loop_triangles:
            ps=[me.vertices[i].co for i in t.vertices]
            verification['degenerate_triangles']+=int((ps[1]-ps[0]).cross(ps[2]-ps[0]).length*.5<1e-10)
        for n in me.corner_normals:verification['invalid_normals']+=int(not all(math.isfinite(x) for x in n.vector) or abs(n.vector.length-1)>.002)
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6)
        verification['boundary_edges']+=sum(e.is_boundary for e in bm.edges);verification['nonmanifold_edges']+=sum(not e.is_manifold for e in bm.edges);bm.free()
    assert verification['triangles']==total and verification['degenerate_triangles']==verification['invalid_normals']==0,verification
    assert verification['boundary_edges']==verification['nonmanifold_edges']==0,verification
    verification['source_sha256']=digest(SOURCE);verification['glb_sha256']=digest(GLB)
    verification['protected_references_unchanged']=all(digest(ROOT/path)==value for path,value in manifest['protected_references'].items())
    (OUT/'hunter-attire-verification.json').write_text(json.dumps(verification,indent=2)+'\n')
    render_views('reimport')
    print('HUNTER_DELIVERY '+json.dumps(manifest))

if MODE=='study':
    build();render_views('source');print('HUNTER_STUDY_READY '+str(SOURCE))
else:deliver()
