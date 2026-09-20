"""Terrain-only presentation meshes derived from the existing immutable tile masks.
Blender Z-up authoring; C/GLB X-right,Y-up,+Z-front. No collision data is written.
"""
import bpy, bmesh, math, re, json, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review';MODELS=ROOT/'public'/'models'
HEADER=ROOT/'src'/'generated'/'terrain_assets.h'
text=(ROOT/'src'/'room.c').read_text();mapblock=text.split('static const char *MAPS')[1].split('u8  tiles')[0]
rows=re.findall(r'"([#.*~,bofmsP\-]+)"',mapblock)
assert len(rows)==44 and all(len(row)==40 for row in rows),[len(row) for row in rows]
maps=[rows[:22],rows[22:]]
scene=bpy.data.scenes.new('TERRAIN | two existing rooms');bpy.context.window.scene=scene
for s in list(bpy.data.scenes):
    if s!=scene:bpy.data.scenes.remove(s)
scene.unit_settings.system='METRIC'
def material(name,rgb,roughness=.75,metallic=0):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    n.inputs['Base Color'].default_value=(*rgb,1);n.inputs['Roughness'].default_value=roughness;n.inputs['Metallic'].default_value=metallic;m.diffuse_color=(*rgb,1)
    return m
rock=material('Terrain | blue slate body',(.078,.095,.105),.88)
rocklit=material('Terrain | slate cleavage planes',(.091,.108,.118),.80)
rockdark=material('Terrain | deep shale seams',(.057,.071,.078),.94)
mineral=material('Terrain | quiet oxidised sediment',(.089,.117,.103),.82,.08)
mortar=material('Terrain | deep dressed joints',(.040,.059,.061),.93)
ashlar=[material('Terrain | limestone tone '+str(i),c,.73+.04*i) for i,c in enumerate([(.15,.178,.18),(.127,.157,.163),(.176,.194,.184)])]
trim=material('Terrain | worn structural cornice',(.193,.215,.207),.66)
oxide=material('Terrain | patinated inset metal',(.070,.134,.109),.61,.5)
MATS=[rock,rocklit,rockdark,mineral,mortar,*ashlar,trim,oxide]
current=None
def mesh(name,verts,faces,mat,indices=None,smooth=False):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update()
    bm=bmesh.new();bm.from_mesh(me)
    if name.startswith('Continuous room'):bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new(name,me);current.objects.link(o)
    if isinstance(mat,list):
        for m in mat:me.materials.append(m)
        for p,idx in zip(me.polygons,indices):p.material_index=idx
    else:me.materials.append(mat)
    for p in me.polygons:p.use_smooth=smooth or (name.startswith('Continuous room') and p.material_index<4)
    return o
def xyz(p):return (p[0],-p[2],p[1])
def box(name,x0,x1,y0,y1,z0,z1,mat,bevel=0):
    # All footprint bounds remain inside original solid tile rectangles.
    verts=[xyz((x,y,z)) for z in (z0,z1) for y in (y0,y1) for x in (x0,x1)]
    faces=[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]
    o=mesh(name,verts,faces,mat)
    if bevel:
        mod=o.modifiers.new('Worn cut edges','BEVEL');mod.width=bevel;mod.segments=1
        mod.affect='EDGES'
        normals=o.modifiers.new('Planar masonry highlights','WEIGHTED_NORMAL');normals.keep_sharp=True
    return o
def hashed(x,y):return (math.sin(x*127.1+y*311.7)*43758.5453123)%1
def solid(room,x,y):return 0<=x<40 and 0<=y<22 and maps[room][y][x] in '#*'
def city(room,x,y):return room==1 or (26<=x<=39 and 5<=y<=12) or (18<=x<=39 and 13<=y<=21)
def column(room,x,y):return 18<=x<=20 and (13<=y<=19 if room==0 else 12<=y<=20)
def raw(room,x,y):return solid(room,x,y) and not city(room,x,y)

def relief(x,y):
    # Stretched, folded fracture cells produce broken slate plates rather than
    # repeated horizontal stripes or isotropic cloudy noise.
    qx=x/1.48+.13*math.sin(y*.59);qy=y/.96+x*.13+.13*math.sin(x*.72)
    ix,iy=math.floor(qx),math.floor(qy);dist=[]
    for dy in (-1,0,1):
        for dx in (-1,0,1):
            cx=ix+dx;cy=iy+dy
            px=cx+.20+.60*hashed(cx,cy);py=cy+.19+.62*hashed(cx+18,cy-7)
            d=(qx-px)**2+(qy-py)**2
            dist.append((d,hashed(cx+8,cy+11)))
    dist.sort();edge=math.sqrt(dist[1][0])-math.sqrt(dist[0][0])
    cleft=max(0,1-edge/.23)
    face=-.045-.18*cleft-.034*min(dist[0][0],1)-.025*dist[0][1]
    return face,dist[0][1]

def raw_front(room,x,y,tx,ty):
    d,u=relief(x,y)
    top=22-ty;bottom=top-1
    # Exact collider-aligned exposed lip at the play plane; all relief is behind it.
    distances=[]
    if not solid(room,tx,ty-1):distances.append(top-y)
    if not solid(room,tx,ty+1):distances.append(y-bottom)
    if not solid(room,tx-1,ty):distances.append(x-tx)
    if not solid(room,tx+1,ty):distances.append(tx+1-x)
    if distances:d*=min(1,max(0,min(distances)/.19))
    return d,u

def build_mass(room):
    v=[];f=[];mi=[]
    def quad(points,idx):
        n=len(v);v.extend(xyz(p) for p in points);f.append(tuple(n+i for i in range(len(points))));mi.append(idx)
    # Front sheets tessellate only occupied cells; no extrapolated decoration
    # bridges holes or converts a one-way platform into a solid visual wall.
    for ty in range(22):
        for tx in range(40):
            if not solid(room,tx,ty):continue
            top=22-ty;bottom=top-1;c=city(room,tx,ty)
            if c:
                quad([(tx,bottom,-.36),(tx+1,bottom,-.36),(tx+1,top,-.36),(tx,top,-.36)],4)
            else:
                n=5
                for j in range(n):
                    for i in range(n):
                        points=[]
                        for a,b in ((0,0),(1,0),(1,1),(0,1)):
                            x=tx+(i+a)/n;y=bottom+(j+b)/n
                            if 0<i+a<n:x+=(hashed(round(x*5),round(y*5))-.5)*.13
                            if 0<j+b<n:y+=(hashed(round(x*5)+3,round(y*5)+7)-.5)*.13
                            z,_=raw_front(room,x,y,tx,ty);points.append((x,y,z))
                        _,phase=relief(tx+(i+.5)/n,bottom+(j+.5)/n)
                        band=1 if phase<.22 else (2 if phase>.83 else 0)
                        if phase>.70 and phase<.76:band=3
                        quad(points,band)
            front=-.36 if c else 0;depth=-2.8 if not c else -2.55;idx=4 if c else 2
            # Boundary walls exist exactly where the collision mask has an edge.
            if not solid(room,tx,ty-1):quad([(tx,top,front),(tx+1,top,front),(tx+1,top,depth),(tx,top,depth)],8 if c else 1)
            if not solid(room,tx,ty+1):quad([(tx,bottom,depth),(tx+1,bottom,depth),(tx+1,bottom,front),(tx,bottom,front)],idx)
            if not solid(room,tx-1,ty):quad([(tx,bottom,front),(tx,top,front),(tx,top,depth),(tx,bottom,depth)],idx)
            if not solid(room,tx+1,ty):quad([(tx+1,bottom,depth),(tx+1,top,depth),(tx+1,top,front),(tx+1,bottom,front)],idx)
            quad([(tx,bottom,depth),(tx,top,depth),(tx+1,top,depth),(tx+1,bottom,depth)],idx)
    return mesh('Continuous room mass and cleavage surfaces',v,f,MATS,mi,False)

def city_runs(room,ty,exclude_column=True):
    xs=[x for x in range(40) if solid(room,x,ty) and city(room,x,ty) and (not exclude_column or not column(room,x,ty))]
    runs=[]
    for x in xs:
        if not runs or x!=runs[-1][1]:runs.append([x,x+1])
        else:runs[-1][1]=x+1
    return runs

def masonry(room):
    # Staggered long ashlar blocks with two deliberate course sizes. Recessed
    # joints read as architecture instead of every collision tile becoming a cube.
    for ty in range(22):
        top=22-ty;bottom=top-1
        courses=1 if ty%4 in (0,1) else 2
        for x0,x1 in city_runs(room,ty):
            for k in range(courses):
                low=bottom+k/courses;high=bottom+(k+1)/courses
                x=x0;span=1.45+hashed(ty+k,room)*1.25
                offset=(ty*.89+k*1.17)%span
                breaks=[x0]+[b for b in [x0-offset+j*span for j in range(1,30)] if x0+.18<b<x1-.18]+[x1]
                for j,(a,b) in enumerate(zip(breaks,breaks[1:])):
                    front=-.015-.035*hashed(a+room,ty+k)
                    inset=.024
                    box('Ashlar course %02d block %02d'%(ty,j),a+inset,b-inset,low+inset,high-inset,-.38,front,ashlar[(ty+j+room)%3],.032 if courses==1 else .024)
            # Exposed top courses receive one broad cap, not a row of lumpy blocks.
            start=x0
            for x in range(x0,x1+1):
                exposed=x<x1 and not solid(room,x,ty-1)
                if x==start and exposed:continue
                prev=not solid(room,x-1,ty-1) if x>start else False
                if prev and (not exposed or x==x1):
                    box('Continuous standing cap',start,x,top-.13,top,-.62,0,trim,.0)
                    box('Recessed cornice undercut',start+.015,x-.015,top-.23,top-.17,-.31,-.09,mortar,.010)
                if not prev or not exposed:start=x
            # Boundary jambs have an inset vertical quoin reveal at exposed sides.
            for x,side in ((x0,-1),(x1,1)):
                nx=x-1 if side<0 else x
                if not solid(room,nx,ty):
                    a=x+.055 if side<0 else x-.14
                    box('End quoin bevel',a,a+.085,bottom+.08,top-.08,-.35,-.003,trim,.018)

def column_shaft(room):
    low,high=(2,9) if room==0 else (1,10)
    verts=[];faces=[];ns=36;vs=12
    for j in range(vs+1):
        y=low+(high-low)*j/vs
        for i in range(ns+1):
            x=18+3*i/ns
            # Continuous engaged shaft, five shallow flutes; none protrudes +Z.
            flute=(.5+.5*math.cos((x-18)*math.tau*5/3))**2
            z=-.027-.18*flute
            verts.append(xyz((x,y,z)))
    for j in range(vs):
        for i in range(ns):
            q=j*(ns+1)+i;faces.append((q,q+1,q+ns+2,q+ns+1))
    mesh('Engaged five-flute structural pier',verts,faces,ashlar[1],smooth=True)
    for y,h in ((low,.18),(low+.18,.13),(high-.18,.18),(high-.34,.10)):
        box('Pier capital or footing',18,21,y,y+h,-.56,0,trim,.025 if y!=high-.18 else 0)
    for x in (18.12,20.80):box('Pier edge return',x,x+.08,low+.3,high-.35,-.30,-.035,ashlar[2],.015)

collections=[]
for room in range(2):
    current=bpy.data.collections.new('TERRAIN | '+('Vault Mouth' if room==0 else 'Drowned Quarter'));scene.collection.children.link(current);collections.append(current)
    build_mass(room);masonry(room);column_shaft(room)

# Reusable material-split runtime arrays. No streaming or external textures.
lines=['/* Generated from src/room.c by scripts/blender_terrain_assets.py. */','#ifndef TERRAIN_ASSETS_H','#define TERRAIN_ASSETS_H','#include "foundry_assets.h"']
def fmt(x):
    s=f'{x:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
def array(name,ctype,values):
    lines.append('static const '+ctype+' '+name+'[] = {')
    for i in range(0,len(values),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in values[i:i+12])+',')
    lines.append('};')
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
manifest={'source':'src/room.c MAPS; city presentation follows root IsCity','room_source_sha256':hashlib.sha256((ROOT/'src'/'room.c').read_bytes()).hexdigest(),'mask_source':maps,'scope':'solid tiles only; one-way shelves, actors, plants, water, lights and props excluded','assets':{}}
total=0
for room,col in enumerate(collections):
    slug='vault-terrain' if room==0 else 'drowned-terrain';ident='FOUNDRY_VAULT_TERRAIN' if room==0 else 'FOUNDRY_DROWNED_TERRAIN'
    scene.view_layers[0].update();deps=bpy.context.evaluated_depsgraph_get();groups={};copies=[];lo=[1e8]*3;hi=[-1e8]*3
    delivery=bpy.data.collections.new('DELIVERY temporary');scene.collection.children.link(delivery)
    for src in col.objects:
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.triangulate(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();me.calc_loop_triangles()
        o=bpy.data.objects.new(src.name+' runtime',me);delivery.objects.link(o);copies.append(o)
        for t in me.loop_triangles:
            if (me.vertices[t.vertices[1]].co-me.vertices[t.vertices[0]].co).cross(me.vertices[t.vertices[2]].co-me.vertices[t.vertices[0]].co).length<1e-10:continue
            m=me.materials[t.material_index];g=groups.setdefault(m.name,{'mat':m,'positions':[],'normals':[],'indices':[],'map':{}})
            for vi,li in zip(t.vertices,t.loops):
                p=me.vertices[vi].co;n=me.corner_normals[li].vector;p=(p.x,p.z,-p.y);n=(n.x,n.z,-n.y)
                key=tuple(round(v,6) for v in (*p,*n));idx=g['map'].get(key)
                if idx is None:
                    idx=len(g['positions'])//3;g['map'][key]=idx;g['positions'].extend(p);g['normals'].extend(n)
                    for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
                g['indices'].append(idx)
    descriptors=[];triangles=0
    for k,g in enumerate(groups.values()):
        pre=slug.replace('-','_')+'_'+str(k);nv=len(g['positions'])//3;assert nv<65536
        for suf,ctype,key in (('p','float','positions'),('n','float','normals'),('i','unsigned short','indices')):array(pre+'_'+suf,ctype,g[key])
        n=next(n for n in g['mat'].node_tree.nodes if n.type=='BSDF_PRINCIPLED');rgba=[round(max(0,min(1,srgb(c)))*255) for c in n.inputs['Base Color'].default_value[:3]]+[255]
        descriptors.append('{'+','.join([pre+'_p',pre+'_n',pre+'_i',str(nv),str(len(g['indices'])),'{'+','.join(map(str,rgba))+'}',fmt(n.inputs['Metallic'].default_value),fmt(n.inputs['Roughness'].default_value),'0.0f'])+'}')
        triangles+=len(g['indices'])//3
    lines.append('static const FoundryMeshData '+ident+'_MESHES[] = {'+','.join(descriptors)+'};')
    lines.append('static const FoundryAssetData '+ident+' = {'+ident+'_MESHES,'+str(len(groups))+',{40.0f,22.0f,'+fmt(hi[2]-lo[2])+'}};')
    for o in scene.objects:o.select_set(False)
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name=slug
    bpy.ops.export_scene.gltf(filepath=str(MODELS/(slug+'.glb')),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)
    # Verify every delivered vertex belongs to the closed solid-cell footprint.
    violations=[]
    for g in groups.values():
        p=g['positions']
        for i in range(0,len(p),3):
            x,y,z=p[i:i+3]
            candidates=[(math.floor(x+dx),math.floor(22-y+dy)) for dx in (-1e-5,1e-5) for dy in (-1e-5,1e-5)]
            if z>.00001 or not any(solid(room,tx,ty) for tx,ty in candidates):violations.append([x,y,z])
    assert not violations,(slug,violations[:10])
    manifest['assets'][slug]={'triangles':triangles,'material_meshes':len(groups),'source_objects':len(col.objects),'bounds_min':lo,'bounds_max':hi,'solid_tile_count':sum(c in '#*' for row in maps[room] for c in row),'mask_vertex_violations':len(violations),'material_names':list(groups)};total+=triangles
lines.append('#endif');HEADER.write_text('\n'.join(lines)+'\n')
manifest['total_triangles']=total;assert total<60000,total
(OUT/'terrain-manifest.json').write_text(json.dumps(manifest,indent=2))

# Neutral stage is excluded from geometry export. Render complete-room and detail views.
stage=bpy.data.collections.new('PRESENTATION | terrain review');scene.collection.children.link(stage)
world=bpy.data.worlds.new('Terrain neutral atmosphere');world.use_nodes=True;scene.world=world
bg=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.055,.075,.084,1);bg.inputs['Strength'].default_value=.35
def light(name,loc,power,color,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.color=color;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(name,d);stage.objects.link(o);o.location=loc;o.rotation_euler=(Vector((20,0,11))-o.location).to_track_quat('-Z','Y').to_euler()
light('Terrain broad key',(-3,-14,31),18000,(.84,.96,1),22)
light('Terrain green reflected fill',(38,-8,15),10000,(.62,.89,.76),18)
light('Terrain grazing top',(22,4,27),14000,(.92,.90,.78),12)
cd=bpy.data.cameras.new('Terrain review');cam=bpy.data.objects.new('Terrain review',cd);stage.objects.link(cam);scene.camera=cam;cd.type='ORTHO';cd.ortho_scale=41
scene.render.engine='CYCLES';scene.cycles.samples=20;scene.cycles.use_denoising=True;scene.render.image_settings.file_format='PNG'
scene.render.resolution_x=1440;scene.render.resolution_y=810;scene.render.resolution_percentage=100
cam.location=(20,-48,14);cam.rotation_euler=(Vector((20,0,11))-cam.location).to_track_quat('-Z','Y').to_euler()
for c in collections:c.hide_render=c!=collections[0];c.hide_viewport=c!=collections[0]
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'terrain.blend'))
for room,col in enumerate(collections):
    for c in collections:c.hide_render=c!=col;c.hide_viewport=c!=col
    slug='vault-terrain' if room==0 else 'drowned-terrain'
    scene.render.filepath=str(REVIEW/(slug+'-full.png'));bpy.ops.render.render(write_still=True)
    oldloc=cam.location.copy();oldrot=cam.rotation_euler.copy();oldscale=cd.ortho_scale
    center=Vector((4,-.8,5)) if room==0 else Vector((19,-.8,6));cam.location=center+Vector((3,-15,4));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cd.ortho_scale=11
    scene.render.filepath=str(REVIEW/(slug+'-detail.png'));bpy.ops.render.render(write_still=True)
    cam.location=oldloc;cam.rotation_euler=oldrot;cd.ortho_scale=oldscale
print('TERRAIN_COMPLETE '+json.dumps(manifest))
