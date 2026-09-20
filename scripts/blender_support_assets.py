"""Authored one-way shelves, corbels and receiving architecture for both rooms.

Run in a fresh Blender background session. --verify reopens the source and clean
GLB reimports. All authoring coordinates are fixed-camera stage coordinates;
P() maps these to real 3D positions while preserving the original landing line.
"""
import bpy, bmesh, math, re, json, hashlib, sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review'/'supports';MODELS=ROOT/'public'/'models';HEADER=ROOT/'src'/'generated'/'support_assets.h'
for p in (OUT,REVIEW,MODELS,HEADER.parent):p.mkdir(parents=True,exist_ok=True)
SOURCE=(ROOT/'src'/'room.c').read_text()
ROWS=re.findall(r'"([#.*~,bofmsP\-]+)"',SOURCE.split('static const char *MAPS')[1].split('u8  tiles')[0])
assert len(ROWS)==44 and all(len(r)==40 for r in ROWS)
MAPS=[ROWS[:22],ROWS[22:]]
RUNS=[{'room':r,'row':y,'x0':m.start(),'x1':m.end(),'top':22-y,
       'kind':'grate' if (r==0 and y==20) or (r==1 and y==1) else 'stone' if r==1 or (m.start()>=26 and 5<=y<=12) or (m.start()>=18 and y>=13) else 'timber'}
      for r in range(2) for y,line in enumerate(MAPS[r]) for m in re.finditer(r'-+',line)]
ASSETS=[('vault-supports','FOUNDRY_VAULT_SUPPORTS'),('drowned-supports','FOUNDRY_DROWNED_SUPPORTS')]
def P(x,y,z):return (20+(x-20)*(52-z)/52,11+(y-11)*(52-z)/52,z)
def xyz(p):return (p[0],-p[2],p[1])
def linear(c):
    c/=255;return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
def material(name,rgb,rough=.85,metal=0,role='stone'):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');c=tuple(linear(v) for v in rgb)
    n.inputs['Base Color'].default_value=(*c,1);n.inputs['Roughness'].default_value=rough;n.inputs['Metallic'].default_value=metal;m.diffuse_color=(*c,1);m['runtime_family']=role;return m
def mesh(name,verts,faces,mat,col,smooth=False,indices=None):
    me=bpy.data.meshes.new(name);me.from_pydata([xyz(P(*p)) for p in verts],[],faces);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new(name,me);col.objects.link(o)
    for m in mat if isinstance(mat,list) else [mat]:me.materials.append(m)
    for i,p in enumerate(me.polygons):p.use_smooth=smooth;p.material_index=indices[i] if indices else 0
    return o
def box(name,x0,x1,y0,y1,z0,z1,mat,col):
    return mesh(name,[(x,y,z) for z in (z0,z1) for y in (y0,y1) for x in (x0,x1)],[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)],mat,col)
def extrude_x(name,x0,x1,profile,mats,col,indices=None,steps=1,grain=False):
    v=[];f=[];mi=[];n=len(profile)
    for k in range(steps+1):
        x=x0+(x1-x0)*k/steps
        for j,(y,z) in enumerate(profile):
            # Worn sidegrain stays below the untouched true top/front lip.
            wear=.004*math.sin(x*14+j*2)+.003*math.sin(x*29-j) if grain and j not in (0,n-1) else 0
            v.append((x,y-abs(wear),z))
    for k in range(steps):
        for j in range(n):f.append((k*n+j,k*n+(j+1)%n,(k+1)*n+(j+1)%n,(k+1)*n+j));mi.append(indices[j] if indices else 0)
    f.append(tuple(reversed(range(n))));mi.append(0)
    f.append(tuple(steps*n+j for j in range(n)));mi.append(0)
    return mesh(name,v,f,mats,col,False,mi)
def tube(name,points,r,mat,col,sides=8):
    v=[];f=[]
    for i,p in enumerate(points):
        p=Vector(p);t=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])).normalized()
        ref=Vector((0,0,1));a=t.cross(ref).normalized()
        if a.length<.1:a=Vector((1,0,0))
        b=t.cross(a).normalized()
        for k in range(sides):v.append(tuple(p+r*(a*math.cos(k*math.tau/sides)+b*math.sin(k*math.tau/sides))))
    for j in range(len(points)-1):
        for k in range(sides):f.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
    f.append(tuple(reversed(range(sides))));f.append(tuple((len(points)-1)*sides+k for k in range(sides)))
    return mesh(name,v,f,mat,col,True)
def disk(name,x,y,z,r,depth,mat,col,sides=16):
    v=[(x+r*math.cos(i*math.tau/sides),y+r*math.sin(i*math.tau/sides),zz) for zz in (z-depth,z) for i in range(sides)]
    f=[tuple(reversed(range(sides))),tuple(sides+i for i in range(sides))]+[(i,(i+1)%sides,(i+1)%sides+sides,i+sides) for i in range(sides)]
    return mesh(name,v,f,mat,col)
def receivers(room,col,rockmat,stonemat):
    """Narrow load-bearing spines, never broad plaques behind each shelf.

    The locked-camera native review rejected the earlier wide wall bays because
    they covered the city. These ribs occupy only actual anchor axes and join
    the existing ceiling/floor silhouettes. Shared axes are merged before build.
    """
    for kind,mat in [('timber',rockmat),('stone',stonemat)]:
        axes={}
        for r in RUNS:
            if r['room']!=room or r['kind']!=kind:continue
            low,high=(r['top']-1.6,22.25) if r['row']<=8 else (-.35,r['top']-.37)
            aa=[r['x0']+.64,r['x1']-.64] if r['x1']-r['x0']>3 else [(r['x0']+r['x1'])/2]
            for ax in aa:axes.setdefault(round(ax,3),[]).append((low,high))
        for ax,intervals in sorted(axes.items()):
            merged=[]
            for low,high in sorted(intervals):
                if merged and low<=merged[-1][1]+2.5:merged[-1]=(merged[-1][0],max(high,merged[-1][1]))
                else:merged.append((low,high))
            for low,high in merged:
                # A chamfered octagonal rib presents narrow lit returns and a
                # shallow face channel, not a flat rectangular backdrop.
                profile=[(-.16,-5.055),(-.07,-5.095),(.07,-5.095),(.16,-5.055),(.23,-5.17),(.23,-5.74),(.13,-5.88),(-.13,-5.88),(-.23,-5.74),(-.23,-5.17)]
                steps=max(1,math.ceil((high-low)/.7));v=[];f=[];n=len(profile)
                for j in range(steps+1):
                    t=j/steps;y=low+(high-low)*t
                    irregular=.022*math.sin(y*4.1+ax) if kind=='timber' else 0
                    for xx,z in profile:v.append((ax+xx+irregular,y,z-.018*(.5+.5*math.sin(y*3.7+ax))))
                for j in range(steps):
                    for k in range(n):f.append((j*n+k,j*n+(k+1)%n,(j+1)*n+(k+1)%n,(j+1)*n+k))
                f.extend([tuple(reversed(range(n))),tuple(steps*n+k for k in range(n))])
                ob=mesh(f'R{room} | {kind} receiving spine {ax:.2f}',v,f,mat,col,False)
                ob['geometry_role']='narrow rear spine on anchor axis; roof/floor connected; not collision'
def area(scene,col,name,loc,power,size,color=(.84,.94,1)):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.size=size;d.color=color;o=bpy.data.objects.new(name,d);col.objects.link(o);o.location=loc;o.rotation_euler=(Vector((20,0,11))-o.location).to_track_quat('-Z','Y').to_euler();return o
def camera(scene,col):
    d=bpy.data.cameras.new('Support review camera');o=bpy.data.objects.new('Support review camera',d);col.objects.link(o);scene.camera=o;return o
def view(cam,center,offset,scale):
    center=Vector(xyz(center));cam.location=center+Vector(offset);cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=scale
def settings(scene):
    scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=1280;scene.render.resolution_y=720;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.view_settings.view_transform='AgX'
def render(scene,name):scene.render.filepath=str(REVIEW/(name+'.png'));bpy.ops.render.render(write_still=True)

def build():
    scene=bpy.data.scenes.new('SUPPORTS | authored existing one-way runs');bpy.context.window.scene=scene
    for s in list(bpy.data.scenes):
        if s!=scene:bpy.data.scenes.remove(s)
    cols=[]
    for slug,_ in ASSETS:c=bpy.data.collections.new('ASSET | '+slug);scene.collection.children.link(c);cols.append(c)
    wood=material('Support | split aged oak',(105,89,66),.90,role='wood')
    wooddark=material('Support | worn timber underside',(75,64,50),.97,role='wood')
    endgrain=material('Support | fresh old endgrain',(120,104,78),.88,role='wood')
    grain=material('Support | incised endgrain and growth lines',(62,56,43),.99,role='wood')
    iron=material('Support | hand-forged iron straps',(53,68,65),.56,.78,'metal')
    grateiron=material('Support | rubbed shaft grate iron',(102,120,105),.50,.76,'metal')
    stone=material('Support | dressed stone',(87,104,109),.80)
    cap=material('Support | rubbed stone arris',(109,123,121),.72)
    recess=material('Support | stone recess',(59,76,81),.91)
    bronze=material('Support | restrained bronze keys',(118,100,69),.55,.73,'metal')
    rearrock=material('Support | recessed rock receiver',(43,55,60),.98,role='rear_rock')
    rearstone=material('Support | recessed masonry receiver',(47,64,69),.96,role='rear_stone')
    for rid,run in enumerate(RUNS):
        room=run['room'];col=cols[room];x0,x1,top=run['x0'],run['x1'],run['top'];length=x1-x0;kind=run['kind'];prefix=f'R{room} y{run["row"]} x{x0}-{x1} | '
        # Every shelf uses the same exact physical top line in the gameplay plane.
        # No raised hardware, bevel or seam crosses this immutable line.
        if kind=='grate':
            box(prefix+'unbroken grounded front rail',x0,x1,top-.067,top,-.08,0,grateiron,col)
            box(prefix+'rear bearing rail',x0,x1,top-.15,top,-2.3,-2.12,iron,col)
            for k in range(int(length*4)+1):
                x=x0+.055+k*(length-.11)/(length*4)
                box(prefix+'seated crossbar',x-.029,x+.029,top-.255,top-.035,-2.3,-.014,grateiron,col)
            for side in (x0+.075,x1-.075):
                box(prefix+'side bearing into existing rock',side-.075,side+.075,top-.26,top-.10,-2.65,-.16,bronze,col)
                for zz in (-.28,-1.88):
                    disk(prefix+'flush side pin',side,top-.08,zz,.029,.018,bronze,col,12)
            run['rear_receiver']='existing solid abutments at either end; no shaft-blocking pier'
            continue
        anchors=[x0+.64,x1-.64]
        if length<=3:anchors=[(x0+x1)/2]
        # Receiver joins an existing continuous floor or roof at a separate depth
        # plane. The silhouette has no extra top landing in the play layer.
        upper=run['row']<=8
        run['rear_receiver']='ceiling' if upper else 'floor';run['receiver_front_z']=-5.05;run['receiver_back_z']=-7.0
        if kind=='timber':
            # Three thick long planks have real edge chamfers, underside relief,
            # end faces and fasteners; front top edge remains exact and continuous.
            for board in range(3):
                za=-board*.54;zb=za-.525
                profile=[(top,za),(top-.19,za),(top-.245,za-.035),(top-.27,zb+.055),(top-.22,zb),(top-.022,zb),(top,zb+.025)]
                extrude_x(prefix+f'hewn plank {board}',x0,x1,profile,[wood,wooddark,endgrain],col,[0,1,1,1,0,0,2],max(4,int(length*6)),True)
                for side in (x0-.003,x1+.003):
                    for rr in (.032,.057,.08):
                        tube(prefix+'visible cut endgrain',[(side,top-.13+math.cos(k*math.tau/32)*rr,za-.26+math.sin(k*math.tau/32)*rr*1.6) for k in range(33)],.0028,grain,col,5)
            # Substantial bearers extend from the wall socket to the front plank.
            for ix,ax in enumerate(anchors):
                profile=[(top-.28,-.19),(top-.47,-.23),(top-.55,-1.55),(top-.49,-5.09),(top-.28,-5.09)]
                extrude_x(prefix+'transverse bearer',ax-.11,ax+.11,profile,[wooddark,wood],col,[1,0,0,0,1])
                box(prefix+'forged wall socket plate',ax-.18,ax+.18,top-1.48,top-.36,-5.33,-5.035,iron,col)
                # Curved wrought brace rises continuously to its under-beam seat.
                points=[]
                for k in range(13):
                    t=k/12;points.append((ax,top-1.36+.91*math.sin(t*math.pi/2),-5.09+4.55*t))
                tube(prefix+'pegged curved iron brace',points,.044,iron,col,10)
                for py in (top-.62,top-1.23):disk(prefix+'socket forged rivet',ax,py,-5.015,.055,.037,bronze,col)
                # Flush pegs on the visible fascia: exposed dowel endgrain is a
                # construction cue even when longitudinal board ends are edge-on.
                disk(prefix+'oak endgrain peg',ax,top-.111,-.004,.061,.040,endgrain,col,20)
                for rr in (.019,.039):
                    tube(prefix+'peg growth ring',[(ax+math.cos(k*math.tau/24)*rr,top-.111+math.sin(k*math.tau/24)*rr,-.006) for k in range(25)],.004,grain,col,5)
                # Strap returns around the front/down face rather than floating.
                box(prefix+'seated iron face strap',ax-.085,ax+.085,top-.22,top-.037,-.018,-.009,iron,col)
                disk(prefix+'strap nail',ax,top-.065,-.001,.022,.008,bronze,col,12)
            # Sparse front growth ridges are actual shallow relief in the timber.
            for j in range(3):
                pts=[(x0+.06+(length-.12)*i/34,top-.064-j*.047+.003*math.sin(i*.44+j),-.0035) for i in range(35)]
                tube(prefix+'longitudinal split grain',pts,.0033,grain,col,5)
        else:
            # Continuous calibrated cap above individual ogee blocks. The top lip
            # is not segmented by cosmetic mortar gaps.
            extrude_x(prefix+'continuous landing cap',x0,x1,[(top,0),(top-.115,0),(top-.15,-.042),(top-.15,-1.52),(top,-1.52)], [cap,stone],col,[0,0,1,1,0])
            for k in range(length):
                profile=[(top-.14,-.025),(top-.205,-.043),(top-.245,-.100),(top-.282,-.19),(top-.31,-.26),(top-.37,-.29),(top-.405,-.25),(top-.46,-.34),(top-.49,-.42),(top-.51,-1.52),(top-.14,-1.52)]
                extrude_x(prefix+'carved ogee block',x0+k+.013,x0+k+1-.013,profile,[stone,cap,recess],col,[0,0,0,0,1,1,0,2,2,2,0])
                # Metal cramp is embedded below the lip, never a raised bar.
                if k%3==1:box(prefix+'flush bronze assembly cramp',x0+k+.44,x0+k+.51,top-.112,top-.036,-.010,-.001,bronze,col)
            for ax in anchors:
                # A massed stone springer. Each side follows a quarter ogee from
                # the projecting crown to a deep seated tail in the receiver.
                prof=[(top-.50,-.49),(top-.56,-.48),(top-.70,-.54),(top-.91,-.72),(top-1.13,-1.07),(top-1.35,-1.65),(top-1.49,-2.4),(top-1.54,-5.17),(top-.50,-5.17)]
                extrude_x(prefix+'springing stone corbel',ax-.235,ax+.235,prof,[stone,recess],col,[0,0,0,0,0,0,1,1,0])
                # Thinner neighboring roll follows the load path; different depth
                # and curvature make it true carving rather than a painted mark.
                pts=[]
                for k in range(17):
                    t=k/16;pts.append((ax-.15,top-.60-.82*math.sin(t*math.pi/2),-.51-4.63*t))
                tube(prefix+'carved corbel roll',pts,.029,cap,col,10)
                # A restrained round bronze seat echoes other city castings.
                disk(prefix+'seated orbital pin',ax,top-.725,-.49,.069,.025,bronze,col,24)
                disk(prefix+'recessed orbital pin center',ax,top-.725,-.46,.033,.010,recess,col,20)
    for room,col in enumerate(cols):receivers(room,col,rearrock,rearstone)
    manifest=export(scene,cols)
    stage=bpy.data.collections.new('PRESENTATION | excluded from runtime exports');scene.collection.children.link(stage)
    w=bpy.data.worlds.new('Support review world');w.use_nodes=True;scene.world=w;bg=next(n for n in w.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.075,.10,.12,1);bg.inputs['Strength'].default_value=.5
    area(scene,stage,'Large cool daylight',(8,-18,31),6500,18)
    area(scene,stage,'Gentle warm fill',(34,-8,16),2600,14,(1,.82,.64))
    cam=camera(scene,stage);settings(scene)
    for room in range(2):
        for j,col in enumerate(cols):col.hide_render=j!=room;col.hide_viewport=j!=room
        cam.data.type='PERSP';cam.data.angle=math.radians(24.415162)*1280/720 # Set sensor fitting below for exact vertical field.
        cam.data.sensor_fit='VERTICAL';cam.data.angle_y=math.radians(24.415162)
        cam.location=(20,-52,11);cam.rotation_euler=(Vector((20,0,11))-cam.location).to_track_quat('-Z','Y').to_euler()
        render(scene,ASSETS[room][0]+'-stage')
    cols[0].hide_render=False;cols[0].hide_viewport=False;cols[1].hide_render=True;cols[1].hide_viewport=True
    view(cam,(9,15.2,-2),(5,-11,4.2),7.3);render(scene,'timber-construction')
    view(cam,(29.4,16.1,-2.0),(5,-12,4.3),7.1);render(scene,'stone-construction')
    view(cam,(23.7,1.55,-1),(4,-11,4.0),7.2);render(scene,'grate-construction')
    # Return source to a whole Vault view instead of a closeup of a hidden asset.
    view(cam,(20,11,-2),(0,-55,0),42)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'supports.blend'))
    print('SUPPORT_SOURCE_COMPLETE '+json.dumps(manifest))

def export(scene,cols):
    lines=['/* Generated by scripts/blender_support_assets.py. */','#ifndef SUPPORT_ASSETS_H','#define SUPPORT_ASSETS_H','#include "foundry_assets.h"']
    def fmt(v):
        s=f'{v:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
    def arr(name,ctype,vs):
        lines.append('static const '+ctype+' '+name+'[] = {')
        for i in range(0,len(vs),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in vs[i:i+12])+',')
        lines.append('};')
    manifest={'source':'src/room.c MAPS','source_sha256':hashlib.sha256(SOURCE.encode()).hexdigest(),'source_blend':'supports.blend','blender_version':bpy.app.version_string,'runtime_axes':'X right,Y up,Z front','runtime_transform':'identity; absolute world coordinates','projection_contract':{'camera':[20,11,52],'look_at':[20,11,0],'vertical_fov_degrees':24.415162,'forced_perspective':'world=(20+(screenX-20)*(52-z)/52,11+(screenY-11)*(52-z)/52,z)','front_lip_z':0,'front_lip_top':'22-mapRow','front_lip_span':'exact [x0,x1] from immutable one-way run','rear_receiver_front_z':-5.05,'rear_receiver_back_z':-7.0,'brackets_and_corbels':'z<=-.35 except front fascia; no added collision or gameplay surfaces'},'runs':RUNS,'assets':{}}
    for room,((slug,symbol),col) in enumerate(zip(ASSETS,cols)):
        bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();groups={};copies=[];lo=[1e9]*3;hi=[-1e9]*3
        delivery=bpy.data.collections.new('DELIVERY TEMP '+slug);scene.collection.children.link(delivery)
        for src in col.objects:
            me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps);bm=bmesh.new();bm.from_mesh(me);bmesh.ops.triangulate(bm,faces=list(bm.faces));bad=[f for f in bm.faces if f.calc_area()<1e-10]
            if bad:bmesh.ops.delete(bm,geom=bad,context='FACES')
            bm.to_mesh(me);bm.free();me.calc_loop_triangles();ob=bpy.data.objects.new(src.name+' delivery',me);delivery.objects.link(ob);copies.append(ob)
            for tri in me.loop_triangles:
                mat=me.materials[tri.material_index];g=groups.setdefault(mat.name,{'mat':mat,'p':[],'n':[],'i':[],'keys':{}})
                for vi,li in zip(tri.vertices,tri.loops):
                    p=me.vertices[vi].co;n=me.corner_normals[li].vector;p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y);key=tuple(round(v,6) for v in (*p,*n));idx=g['keys'].get(key)
                    if idx is None:
                        idx=len(g['p'])//3;g['keys'][key]=idx;g['p'].extend(p);g['n'].extend(n)
                        for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                    g['i'].append(idx)
        desc=[];tris=0;roles=[]
        for j,g in enumerate(groups.values()):
            pre=slug.replace('-','_')+'_'+str(j);nv=len(g['p'])//3;assert nv<65536
            for suffix,ctype in [('p','float'),('n','float'),('i','unsigned short')]:arr(pre+'_'+suffix,ctype,g[suffix])
            n=next(n for n in g['mat'].node_tree.nodes if n.type=='BSDF_PRINCIPLED');rgba=[round(srgb(c)*255) for c in n.inputs['Base Color'].default_value[:3]]+[255]
            desc.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(g['i'])),'{'+','.join(map(str,rgba))+'}',fmt(n.inputs['Metallic'].default_value),fmt(n.inputs['Roughness'].default_value),'0.0f'])+'}');tris+=len(g['i'])//3;roles.append('endgrain' if 'endgrain' in g['mat'].name else g['mat']['runtime_family'])
        lines.append('static const FoundryMeshData '+symbol+'_MESHES[] = {'+','.join(desc)+'};')
        lines.append('static const FoundryAssetData '+symbol+' = {'+symbol+'_MESHES,'+str(len(groups))+',{'+','.join(fmt(hi[i]-lo[i]) for i in range(3))+'}};')
        # Additional generated family codes let renderer apply material-specific
        # shading without guessing from RGB. 0 none/metal,1stone,3wood;
        # 5 rear rock and 6 rear masonry use recessed receiver atmosphere.
        # 7 preserves modeled endgrain without a longitudinal surface map.
        lines.append('static const unsigned char '+symbol+'_SUBSTRATES[] = {'+','.join(str({'wood':3,'stone':1,'rear_rock':5,'rear_stone':6,'metal':0,'endgrain':7}[r]) for r in roles)+'};')
        for ob in scene.objects:ob.select_set(False)
        for ob in copies:ob.select_set(True)
        bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
        bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
        bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
        manifest['assets'][slug]={'symbol':symbol,'triangles':tris,'material_meshes':len(groups),'materials':list(groups),'material_families':roles,'bounds_min':lo,'bounds_max':hi,'dimensions':[hi[i]-lo[i] for i in range(3)],'source_objects':len(col.objects),'one_way_runs':sum(1 for r in RUNS if r['room']==room)}
    lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n');manifest['total_triangles']=sum(v['triangles'] for v in manifest['assets'].values())
    manifest['projection_contract'].update({'rear_receiver_front_z':-5.055,'rear_receiver_back_z':-5.898,'receiver_form':'0.46-tile narrow chamfered spines on actual anchor axes; no broad wall bays','brackets_and_corbels':'z<=-.45 below near fascia; no added collision or gameplay surfaces'})
    manifest['substrate_codes']={'metal':0,'stone':1,'wood':3,'rear_rock':5,'rear_stone':6,'endgrain':7}
    for r in manifest['runs']:
        if r['kind']!='grate':r['receiver_front_z']=-5.055;r['receiver_back_z']=-5.898
    (OUT/'supports-manifest.json').write_text(json.dumps(manifest,indent=2));return manifest

def verify():
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'supports.blend'));source=bpy.context.scene;stage=bpy.data.collections['PRESENTATION | excluded from runtime exports'];manifest=json.loads((OUT/'supports-manifest.json').read_text());report={'source_reopened':True,'blender_version':bpy.app.version_string,'assets':{}}
    for room,(slug,symbol) in enumerate(ASSETS):
        sc=bpy.data.scenes.new('Clean reimport '+slug);bpy.context.window.scene=sc;sc.world=source.world;sc.collection.children.link(stage);path=MODELS/(slug+'.glb');bpy.ops.import_scene.gltf(filepath=str(path))
        obs=[o for o in sc.objects if o.type=='MESH'];points=[];tris=bad=normals=0
        for ob in obs:
            me=ob.data;me.calc_loop_triangles();tris+=len(me.loop_triangles);points.extend(ob.matrix_world@v.co for v in me.vertices)
            for t in me.loop_triangles:
                a,b,c=[me.vertices[i].co for i in t.vertices]
                if (b-a).cross(c-a).length<1e-10:bad+=1
            normals+=sum(not all(math.isfinite(v) for v in (*p.co,*p.normal)) or p.normal.length<.95 for p in me.vertices)
        lo=[min(p[i] for p in points) for i in range(3)];hi=[max(p[i] for p in points) for i in range(3)];dims=[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]];expected=manifest['assets'][slug]
        assert tris==expected['triangles'],(slug,tris,expected['triangles']);assert bad==0,(slug,bad);assert normals==0,(slug,normals)
        assert max(abs(a-b) for a,b in zip(dims,expected['dimensions']))<.001,(slug,dims)
        report['assets'][slug]={'triangles':tris,'degenerate_triangles':bad,'invalid_normals':normals,'dimensions':dims,'checks':'pass','sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size}
        settings(sc);cam=next(o for o in stage.objects if o.type=='CAMERA');sc.camera=cam;cam.data.type='PERSP';cam.data.sensor_fit='VERTICAL';cam.data.angle_y=math.radians(24.415162);cam.location=(20,-52,11);cam.rotation_euler=(Vector((20,0,11))-cam.location).to_track_quat('-Z','Y').to_euler();render(sc,slug+'-reimport')
        if room==0:
            view(cam,(9,15.2,-2),(5,-11,4.2),7.3);render(sc,'timber-reimport-detail')
            view(cam,(29.4,16.1,-2.0),(5,-12,4.3),7.1);render(sc,'stone-reimport-detail')
    report['source_sha256']=hashlib.sha256((OUT/'supports.blend').read_bytes()).hexdigest();report['scope']='Source/GLB technical delivery only; map projection gate is independently verified by another script; runtime visual review remains required.'
    (OUT/'supports-verification.json').write_text(json.dumps(report,indent=2));print('SUPPORT_REIMPORT_VERIFIED '+json.dumps(report))

if '--verify' in sys.argv:verify()
else:build()
