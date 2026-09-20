"""Terrain-only presentation meshes derived from the existing immutable tile masks.
Blender Z-up authoring; C/GLB X-right,Y-up,+Z-front. No collision data is written.
"""
import bpy, bmesh, math, re, json, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'blender';REVIEW=OUT/'review'/'architecture-v12';MODELS=ROOT/'public'/'models'
REVIEW.mkdir(parents=True,exist_ok=True)
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
bronze=material('Terrain | repaired cast bronze',(.16,.118,.060),.56,.78)
well=material('Terrain | unlit sealed interiors',(.012,.020,.022),.94)
MATS=[rock,rocklit,rockdark,mineral,mortar,*ashlar,trim,oxide,bronze,well]
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

# Recesses are modeled in the original solid facade, not painted over its front.
# The solid rear remains. They are occupied architectural relief, not new exits.
DOORS=[(6.29,7.73,10.16,12.76),(31.27,32.71,10.16,12.76)]
WINDOWS=[(4.18,4.78,7.14,8.82),(6.33,6.93,7.14,8.82),
         (31.38,31.98,7.14,8.82),(33.62,34.22,7.14,8.82)]
OPENINGS=DOORS+WINDOWS

def subtract_rect(rect,cut):
    a,b,c,d=rect;x0,x1,y0,y1=cut
    l,r=max(a,x0),min(b,x1);bot,top=max(c,y0),min(d,y1)
    if l>=r or bot>=top:return [rect]
    result=[]
    if a<l:result.append((a,l,c,d))
    if r<b:result.append((r,b,c,d))
    if c<bot:result.append((l,r,c,bot))
    if top<d:result.append((l,r,top,d))
    return result

def masonry_rects(room,rect):
    result=[rect]
    if room==1:
        for hole in OPENINGS:result=[p for r in result for p in subtract_rect(r,hole)]
    return result

def cut_stone(name,x0,x1,y0,y1,z0,z1,mat,seed=0):
    # A deliberately chipped eight-corner face with cut depth and softened edges.
    # Its envelope stays inside supplied dimensions; no random raised noise.
    w=x1-x0;h=y1-y0;c=min(w,h)*(.09+.035*hashed(seed,17))
    points=[(x0+c,y0),(x1-c*.7,y0),(x1,y0+c*.75),(x1,y1-c),
            (x1-c*.8,y1),(x0+c*.6,y1),(x0,y1-c*.85),(x0,y0+c)]
    verts=[xyz((x,y,z)) for z in (z0,z1) for x,y in points];n=len(points)
    faces=[tuple(reversed(range(n))),tuple(n+i for i in range(n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    o=mesh(name,verts,faces,mat)
    m=o.modifiers.new('Worn stone arris','BEVEL');m.width=min(.018,min(w,h)*.055);m.segments=2
    n=o.modifiers.new('Stable broad cut planes','WEIGHTED_NORMAL');n.keep_sharp=True
    return o

def shape(name,outline,front,back,mat,bevel=.012):
    clean=[]
    for p in outline:
        if not clean or math.dist(p,clean[-1])>1e-7:clean.append(p)
    if len(clean)>2 and math.dist(clean[0],clean[-1])<1e-7:clean.pop()
    outline=clean
    n=len(outline);v=[xyz((x,y,z)) for z in (back,front) for x,y in outline]
    f=[tuple(reversed(range(n))),tuple(n+i for i in range(n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    o=mesh(name,v,f,mat)
    if bevel:
        mod=o.modifiers.new('Dressed profile edge','BEVEL');mod.width=bevel;mod.segments=2
        mod=o.modifiers.new('Profile broad normals','WEIGHTED_NORMAL');mod.keep_sharp=True
    return o

def sweep(name,points,r,mat,sides=6):
    v=[];f=[]
    for i,p in enumerate(points):
        p=Vector(p);t=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(i-1,0)])).normalized()
        side=t.cross(Vector((0,0,1))).normalized()
        if side.length<.01:side=Vector((1,0,0))
        up=t.cross(side).normalized()
        for k in range(sides):
            a=math.tau*k/sides;v.append(xyz(p+r*(side*math.cos(a)+up*math.sin(a))))
    for i in range(len(points)-1):
        for k in range(sides):f.append((i*sides+k,i*sides+(k+1)%sides,(i+1)*sides+(k+1)%sides,(i+1)*sides+k))
    f += [tuple(reversed(range(sides))),tuple((len(points)-1)*sides+k for k in range(sides))]
    return mesh(name,v,f,mat,smooth=True)

def arch_sector(name,cx,cy,r0,r1,a0,a1,front,back,mat,segments=8):
    outline=[]
    for i in range(segments+1):
        a=a0+(a1-a0)*i/segments;outline.append((cx+r1*math.cos(a),cy+r1*math.sin(a)))
    for i in range(segments+1):
        a=a1-(a1-a0)*i/segments;outline.append((cx+r0*math.cos(a),cy+r0*math.sin(a)))
    return shape(name,outline,front,back,mat,.012)

def lathe(name,profile,cx,cz,mat,sides=64,flutes=0):
    v=[];f=[]
    for radius,y in profile:
        for i in range(sides):
            a=math.tau*i/sides
            r=radius-(.024*(.5+.5*math.cos(a*12)) if flutes else 0)
            v.append(xyz((cx+r*math.cos(a),y,cz+r*math.sin(a))))
    for j in range(len(profile)-1):
        for i in range(sides):f.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
    f+=[tuple(reversed(range(sides))),tuple((len(profile)-1)*sides+i for i in range(sides))]
    o=mesh(name,v,f,mat,smooth=True)
    # End faces are stone bedding planes; smoothing across them makes each
    # shaft course read as a rounded cushion instead of a cut cylinder.
    for p in o.data.polygons:
        if len(p.vertices)>4:p.use_smooth=False
    return o

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
                front=-2.50 if column(room,tx,ty) else -.36
                for x0,x1,y0,y1 in masonry_rects(room,(tx,tx+1,bottom,top)):
                    quad([(x0,y0,front),(x1,y0,front),(x1,y1,front),(x0,y1,front)],4)
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
            if not solid(room,tx,ty-1) and not column(room,tx,ty):quad([(tx,top,front),(tx+1,top,front),(tx+1,top,depth),(tx,top,depth)],8 if c else 1)
            if not solid(room,tx,ty+1) and not column(room,tx,ty):quad([(tx,bottom,depth),(tx+1,bottom,depth),(tx+1,bottom,front),(tx,bottom,front)],idx)
            side_bottom,side_top=bottom,top
            if column(room,tx,ty):
                low,high=(2,9) if room==0 else (1,10)
                side_bottom=max(bottom,low+.11);side_top=min(top,high-.25)
            if not solid(room,tx-1,ty):quad([(tx,side_bottom,front),(tx,side_top,front),(tx,side_top,depth),(tx,side_bottom,depth)],idx)
            if not solid(room,tx+1,ty):quad([(tx+1,side_bottom,depth),(tx+1,side_top,depth),(tx+1,side_top,front),(tx+1,side_bottom,front)],idx)
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
    chipped=0
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
                    for ra,rb,rc,rd in masonry_rects(room,(a+inset,b-inset,low+inset,high-inset)):
                        if room==1 and 7<=ty<=14 and x0 in (3,31) and rb-ra>.5 and rd-rc>.3 and hashed(a+9,ty)>.65 and chipped<12:
                            cut_stone('Flood-worn ashlar course %02d block %02d'%(ty,j),ra,rb,rc,rd,-.38,front,ashlar[(ty+j+room)%3],ty+j*13);chipped+=1
                        else:
                            box('Ashlar course %02d block %02d'%(ty,j),ra,rb,rc,rd,-.38,front,ashlar[(ty+j+room)%3],.032 if courses==1 else .024)
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

def facade_portal(rect,index,window=False):
    x0,x1,y0,y1=rect;cx=(x0+x1)/2;r=(x1-x0)/2
    spring=y1-r;rear=-1.22 if not window else -.92
    # The planar front was actually cut out. A closed, unlit masonry well and
    # a substantial inner reveal now occupy that cavity behind the play plane.
    box('Sealed rear of drowned '+('slot' if window else 'doorway'),x0,x1,y0,y1,rear-.12,rear,well,.0)
    box('Doorway inner left return',x0,x0+.09,y0,spring,rear,-.14,mortar,.018)
    box('Doorway inner right return',x1-.09,x1,y0,spring,rear,-.14,mortar,.018)
    box('Doorway threshold interior',x0,x1,y0,y0+.10,rear,-.10,mortar,.018)
    # Curved masonry fills the rectangular cut's upper spandrels, while the
    # genuine opening below is an arch with a deep radial return.
    for side in (-1,1):
        xx=x0 if side<0 else x1
        points=[(xx,spring),(xx,y1),(cx,y1)]
        aa=[math.pi/2+(math.pi/2)*i/20 for i in range(21)] if side<0 else [math.pi/2-(math.pi/2)*i/20 for i in range(21)]
        points += [(cx+r*math.cos(a),spring+r*math.sin(a)) for a in aa]
        shape('Integrated doorway arch spandrel',points,-.06,-.40,ashlar[(index+1)%3],.0)
    arch_sector('Deep radial arch return',cx,spring,r-.095,r+.02,0,math.pi,-.13,rear,mortar,32)
    # Individual keyed voussoirs give the opening structural construction.
    count=7 if not window else 5;thick=.17 if not window else .13
    for k in range(count):
        gap=.018
        arch_sector('Drowned keyed arch block %d'%k,cx,spring,r-.015,r+thick,k*math.pi/count+gap,(k+1)*math.pi/count-gap,-.010,-.36,trim if k==count//2 else ashlar[(index+k)%3],5)
    # Two recessed jamb courses and their seated lower blocks.
    n=4 if not window else 3
    for side in (-1,1):
        xa,xb=(x0-thick,x0+.03) if side<0 else (x1-.03,x1+thick)
        for k in range(n):
            ya=y0+(spring-y0)*k/n+.018;yb=y0+(spring-y0)*(k+1)/n-.018
            cut_stone('Worn dressed jamb',xa,xb,ya,yb,-.40,-.004,ashlar[(index+k)%3],index*13+k)
        cut_stone('Jamb foot seat',xa-.025,xb+.025,y0-.06,y0+.16,-.42,-.002,trim,index+side)
        sweep('Recessed bronze reveal',[(x0+.065 if side<0 else x1-.065,y0+.12,-.14),
              (x0+.065 if side<0 else x1-.065,spring-.03,-.14)],.019 if not window else .012,oxide)
    # A coved sill terminates inside the same solid mask. It is not a platform.
    cut_stone('Quiet recessed sill',x0-.17,x1+.17,y0-.10,y0+.02,-.48,-.022,trim,index)
    if window:
        # Obvious fixed bars make these slots part of the flooded house facade.
        for d in (-.12,.12):
            top=spring+math.sqrt(max(0,(r-.06)**2-d*d))
            sweep('Fixed drowned slot grille',[(cx+d,y0+.13,-.34),(cx+d,top-.055,-.34)],.017,oxide)
        cy=y0+(y1-y0)*.47
        pts=[(cx+.15*math.cos(math.tau*k/32),cy+.15*math.sin(math.tau*k/32),-.315) for k in range(33)]
        sweep('Slot orbital grille joint',pts,.018,bronze)
    else:
        # Broad stone lintel is supported by shaped, seated corbels. All bounds
        # stay inside the island and behind the original collision plane.
        y=y1+.24
        for side in (-1,1):
            xx=cx+side*(r+.01);w=.14
            outline=[(xx-w,y),(xx+w,y),(xx+w,y-.26),(xx+.10,y-.46),(xx+.07,y-.70),
                     (xx-.09,y-.70),(xx-.13,y-.44),(xx-w,y-.25)]
            shape('Scroll corbel carrying doorway cornice',outline,-.024,-.43,trim,.019)
            a=[(xx+side*.09,y-.10,-.014),(xx+side*.075,y-.28,-.014),(xx,y-.48,-.014)]
            sweep('Inset corbel meridian',a,.008,oxide)
        for ya,yb,margin,z in ((y-.05,y+.075,.34,-.006),(y+.08,y+.20,.40,-.010),
                               (y+.205,y+.27,.44,-.014)):
            left,right=(3.015,7.985) if index==0 else (31.015,35.985)
            cut_stone('Supported doorway cornice',max(left,cx-r-margin),min(right,cx+r+margin),ya,yb,-.51,z,trim,index+int(ya*11))
        # A modest repaired bronze strap binds the broken lintel. It has real
        # turned returns and dark fixing pins, never an emissive icon.
        bx=cx+(.38 if index==0 else -.36)
        cut_stone('Drowned lintel bronze repair strap',bx-.042,bx+.042,y-.04,y+.255,-.043,-.002,bronze,index)
        for by in (y+.015,y+.21):
            sweep('Seated lintel pin',[(bx,by,-.036),(bx,by,-.001)],.024,oxide,8)

def drowned_facades():
    for i,rect in enumerate(DOORS):facade_portal(rect,i)
    for i,rect in enumerate(WINDOWS):facade_portal(rect,i,True)
    # Horizontal bed-joint repairs and sparse irregular tide staining are placed
    # within the island faces. Their shapes follow construction and water contact.
    for left,right in ((3,8),(31,36)):
        for i in range(4):
            x=left+.42+i*1.19
            y=14.99-.035*hashed(i,left)
            outline=[(x,y+.018),(x+.12,y+.027),(x+.37,y+.016),(x+.48,y-.010),
                (x+.39,y-.037),(x+.32,y-.026),(x+.26,y-.070),(x+.20,y-.033),
                (x+.09,y-.046),(x+.035,y-.030)]
            shape('Irregular mineral tide trace',outline,-.018,-.047,mineral,.0)
        # Two long dressed string courses bind each house across its full width.
        for y in (9.25,13.70):
            for a,b in ((left+.05,left+2.10),(left+2.13,right-.05)):
                cut_stone('Facade structural string course',a,b,y,y+.11,-.37,-.016,ashlar[2],int(a+y))

def column_shaft(room):
    low,high=(2,9) if room==0 else (1,10)
    cx,cz=19.5,-1.50
    # Full volumetric engaged column, seated in the original three-tile pier.
    # A quiet entasis and separate cut courses replace the formerly flat stripes.
    shaft_low=low+.64;shaft_high=high-1.23;course=(shaft_high-shaft_low)/4
    for j in range(4):
        a=shaft_low+j*course+.012;b=shaft_low+(j+1)*course-.012
        profile=[]
        heights=[a,a+.022]+[a+(b-a)*k/6 for k in range(1,6)]+[b-.022,b]
        for k,y in enumerate(heights):
            t=(y-shaft_low)/(shaft_high-shaft_low)
            radius=1.14+.075*math.sin(math.pi*t)-.035*t
            if k in (0,len(heights)-1):radius-=.012
            profile.append((radius,y))
        lathe('Engaged stone shaft course',profile,cx,cz,ashlar[(j+room)%3],64,1)
    # Pedestal and base have actual torus/cove profiles; no floating slabs.
    lathe('Carved column base',[(1.44,low+.06),(1.44,low+.17),(1.35,low+.22),(1.28,low+.27),
        (1.25,low+.33),(1.23,low+.45),(1.20,low+.57),(1.16,low+.65)],cx,cz,trim)
    # Bell capital is one continuous stone carving that flares under the abacus.
    cap=[(1.13,high-1.25),(1.14,high-1.10),(1.18,high-.93),(1.24,high-.74),
         (1.33,high-.54),(1.41,high-.40),(1.46,high-.34)]
    lathe('Continuous flared bell capital',cap,cx,cz,trim)
    # Bronze binding collars explicitly belong to metallurgy and to construction.
    for y,r in ((low+.52,1.23),(shaft_low+course*2,1.235),(high-1.21,1.19)):
        lathe('Cast structural binding collar',[(r-.025,y-.065),(r,y-.047),(r,y+.053),(r-.025,y+.070)],cx,cz,bronze)
        lathe('Patina recessed collar joint',[(r+.006,y-.013),(r+.007,y+.009)],cx,cz,oxide)
    # Seven pointed arcade ribs are seated on the capital, following its changing
    # radius in 3D. They are not line decals pasted across an empty front plane.
    def cap_r(y):
        for (r0,y0),(r1,y1) in zip(cap,cap[1:]):
            if y0<=y<=y1:return r0+(r1-r0)*(y-y0)/(y1-y0)
        return cap[-1][0]
    for k in range(7):
        angle=.09+(math.pi-.18)*(k+.5)/7
        points=[]
        for j in range(25):
            u=j/24;y=high-1.08+.66*math.sin(math.pi*u)
            a=angle+(u-.5)*.34;r=cap_r(y)+.016
            points.append((cx+r*math.cos(a),y,cz+r*math.sin(a)))
        sweep('Pointed capital orbital arcade',points,.015,oxide,6)
    # The exact y=high/z=0 collision lip is retained as a substantial abacus.
    box('Exact collider-aligned capital abacus',18,21,high-.17,high,-2.96,0,trim,0)
    box('Capital recessed shadow joint',18.035,20.965,high-.25,high-.18,-2.90,-.045,mortar,.008)
    box('Original flat footing envelope',18,21,low,low+.11,-2.96,0,trim,0)
    # Orbital records are restrained narrow physical inlays hugging the shaft.
    for j in range(2):
        yc=shaft_low+course*(.75+j*1.9);pts=[]
        for k in range(33):
            a=.25+(math.pi-.5)*k/32;x=cx+1.185*math.cos(a);y=yc+.26*math.cos(a)
            z=cz+math.sqrt(max(0,1.185**2-(x-cx)**2))+.011;pts.append((x,y,z))
        sweep('Shaft inclined orbital inlay',pts,.012,oxide)

collections=[]
for room in range(2):
    current=bpy.data.collections.new('TERRAIN | '+('Vault Mouth' if room==0 else 'Drowned Quarter'));scene.collection.children.link(current);collections.append(current)
    build_mass(room);masonry(room);column_shaft(room)
    if room==1:drowned_facades()

# Reusable material-split runtime arrays. No streaming or external textures.
lines=['/* Generated from src/room.c by scripts/blender_terrain_assets.py. */','#ifndef TERRAIN_ASSETS_H','#define TERRAIN_ASSETS_H','#include "foundry_assets.h"']
def fmt(x):
    s=f'{x:.6f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
def array(name,ctype,values):
    lines.append('static const '+ctype+' '+name+'[] = {')
    for i in range(0,len(values),12):lines.append(','.join(fmt(v) if ctype=='float' else str(v) for v in values[i:i+12])+',')
    lines.append('};')
def srgb(c):return 12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055
manifest={'source':'src/room.c MAPS; city presentation follows root IsCity','room_source_sha256':hashlib.sha256((ROOT/'src'/'room.c').read_bytes()).hexdigest(),'mask_source':maps,'scope':'solid tiles only; one-way shelves, actors, plants, water, lights and props excluded',
          'architecture_revision':'recessed drowned house openings; volumetric capitals and supported trim',
          'door_recesses_xy':DOORS,'window_recesses_xy':WINDOWS,
          'recess_contract':'front mass and ashlar are removed inside wells; sealed rear retains full solid coverage; no collision changes',
          'references':['public/art/references/drowned-quarter/14-drowned-doorway.png','public/art/references/drowned-quarter/20-carved-cornice.png','public/art/references/drowned-quarter/13-sunken-column.png','public/art/references/vault-mouth/16-city-column.png'],
          'delivery_budget':{'total_triangles':100000,'material_meshes_per_room':12,'front_z_max':0},'assets':{}}
total=0
for room,col in enumerate(collections):
    slug='vault-terrain' if room==0 else 'drowned-terrain';ident='FOUNDRY_VAULT_TERRAIN' if room==0 else 'FOUNDRY_DROWNED_TERRAIN'
    scene.view_layers[0].update();deps=bpy.context.evaluated_depsgraph_get();groups={};copies=[];lo=[1e8]*3;hi=[-1e8]*3
    delivery=bpy.data.collections.new('DELIVERY temporary');scene.collection.children.link(delivery)
    for src in col.objects:
        me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
        # Quantize the actual delivery geometry before welding. This prevents
        # submicron bevel slivers from collapsing only when written to C.
        for v in me.vertices:v.co=tuple(round(c,6) for c in v.co)
        bm=bmesh.new();bm.from_mesh(me)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00002)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=.00002)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        tiny=[f for f in bm.faces if f.calc_area()<1e-6 or min(e.calc_length() for e in f.edges)<1e-5]
        if tiny:bmesh.ops.delete(bm,geom=tiny,context='FACES_ONLY')
        unused=[v for v in bm.verts if not v.link_faces]
        if unused:bmesh.ops.delete(bm,geom=unused,context='VERTS')
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(me);bm.free();me.update();me.calc_loop_triangles()
        # Evaluated bevel custom normals can contain zero or inverted corner
        # normals at a removed sliver. Repair those on the shared delivery mesh
        # so clean GLB and embedded C use the same geometric fallback.
        normals=[n.vector.copy() for n in me.corner_normals]
        for p in me.polygons:
            for li in p.loop_indices:
                n=normals[li]
                normals[li]=(p.normal.copy() if n.length<.9 or n.dot(p.normal)<.02 else n.normalized())
        me.normals_split_custom_set(normals);me.update();me.calc_loop_triangles()
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
    assert len(groups)<=12,(slug,'renderer capacity',len(groups))
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
manifest['total_triangles']=total;assert total<100000,total
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
    scene.render.resolution_x=1000;scene.render.resolution_y=1100
    center=Vector((19.5,1.25,5.5));cam.location=center+Vector((3,-20,2));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cd.ortho_scale=11
    scene.render.filepath=str(REVIEW/(slug+'-detail.png'));bpy.ops.render.render(write_still=True)
    if room==1:
        scene.render.resolution_x=1440;scene.render.resolution_y=1100
        center=Vector((5.5,.65,11));cam.location=center+Vector((2,-20,2));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cd.ortho_scale=11.8
        scene.render.filepath=str(REVIEW/'drowned-facade-detail.png');bpy.ops.render.render(write_still=True)
    scene.render.resolution_x=1440;scene.render.resolution_y=810
    cam.location=oldloc;cam.rotation_euler=oldrot;cd.ortho_scale=oldscale
print('TERRAIN_COMPLETE '+json.dumps(manifest))
