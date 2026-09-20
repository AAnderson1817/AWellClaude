"""Editable broad-plane sculpt for the Drowned Quarter carved witness.

The construction is intentionally explicit at nose and lips. Source cages and
Boolean cutters remain editable; the evaluated union is reduced only for the
separate delivery representation. Runtime axes: X right, Y up, Z front.
"""
import bpy,bmesh,math
from mathutils import Vector
from mathutils.bvhtree import BVHTree

def build_sculpt(col,clay):
    def xyz(p):return(p[0],-p[2],p[1])
    def lerp_profile(t,pts):
        if t<=pts[0][0]:return pts[0][1]
        for (a,av),(b,bv) in zip(pts,pts[1:]):
            if a<=t<=b:return av+(bv-av)*(t-a)/(b-a)
        return pts[-1][1]
    def smooth(a,b,t):
        u=max(0,min(1,(t-a)/(b-a)));return u*u*(3-2*u)
    def dome(x,y,cx,cy,rx,ry,power=1):
        r=((x-cx)/rx)**2+((y-cy)/ry)**2
        return max(0,1-r)**power
    def halfwidth(y):
        return lerp_profile(y,[(.025,3.14),(.25,3.36),(.94,3.54),(1.6,3.67),(2.65,3.7385),(3.65,3.66),(4.28,3.48),(4.975,3.10)])
    def sculpt(x,y):
        ax=abs(x);w=halfwidth(y)
        # Broad carved skull and jaw, with low architectural burial margins.
        z=lerp_profile(y,[(.025,-.15),(.30,.14),(.7,.33),(1.3,.31),(2.1,.37),(2.8,.43),(3.5,.32),(4.1,.37),(4.65,.27),(4.975,.02)])
        z*=1-.80*(ax/w)**3
        # Broad cheek planes transition into temples without enclosed triangular
        # relief badges. Quiet hollows are subordinate to that continuous mass.
        for side in (-1,1):
            xx=x*side
            ridge=math.exp(-((xx-2.07)/.88)**4-((y-2.64)/.94)**2)
            z+=.27*ridge
            hollow=math.exp(-((xx-1.82)/.65)**2-((y-1.65)/.87)**2)
            z-=.075*hollow
        z+=.22*dome(x,y,0,.61,1.90,.66,1.6)
        # Brow roof has an upper stone plane and a steep carved return into the eye.
        dx=ax-1.50
        if abs(dx)<1.20:
            brow_y=3.55+.59*math.sqrt(max(0,1-(dx/1.20)**2))
            d=y-brow_y
            roof=lerp_profile(d,[(-.25,0),(-.10,.32),(.015,.47),(.12,.43),(.35,0)])
            z+=roof*smooth(0,.23,1-abs(dx)/1.2)
        # True broad eye wells, with fixed opal centres protected at local z=.165.
        e=(dx/1.08)**2+((y-3.5)/.44)**2
        if e<1.40:
            floor=-.06+.135*min(1,e)
            z=floor+(z-floor)*smooth(.72,1.40,e)
        # Nose and mouth now have explicit independent lofted control cages below.
        # This main skull surface only carries their embedded construction seat.
        # Buried stone margins step back behind the facial mass, with several large
        # authored break planes. No all-over noise or decorative surface cuts.
        outer=smooth(.77,.97,ax/w)
        returns=lerp_profile(y,[(.025,.18),(.75,.31),(1.43,.17),(2.18,.32),(3.05,.36),(3.49,.19),(4.18,.29),(4.975,.17)]) if x<0 else lerp_profile(y,[(.025,.24),(.63,.16),(1.22,.31),(2.03,.15),(2.87,.34),(3.65,.27),(4.18,.17),(4.975,.25)])
        z=z*(1-outer)+returns*outer
        for cx,cy,rx,ry,d in [(-3.07,3.80,.60,.51,.09),(3.12,2.03,.56,.69,.10),(2.16,4.64,.43,.26,.08)]:
            z-=d*max(0,1-abs(x-cx)/rx-abs(y-cy)/ry)
        return max(-.235,min(1.130,z))
    def mesh(name,v,f,mat):
        me=bpy.data.meshes.new(name);me.from_pydata([xyz(p) for p in v],[],f);me.update()
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
        o=bpy.data.objects.new(name,me);col.objects.link(o);me.materials.append(mat)
        for p in me.polygons:p.use_smooth=True
        return o
    NX,NY=112,100;v=[];f=[]
    for j in range(NY+1):
        ybase=.025+4.95*j/NY;w=halfwidth(ybase)
        for i in range(NX+1):
            x=w*(2*i/NX-1)
            # Unequal broad broken returns. Large surviving stone fractures shape
            # the contour; these are not a repeated noisy perimeter treatment.
            bottom=lerp_profile(x,[(-3.74,.49),(-3.0,.32),(-2.7,.18),(-2.1,.025),(-1.55,.05),(-.65,.11),(.3,.025),(1.2,.12),(1.7,.03),(2.45,.28),(3.2,.44),(3.74,.53)])
            top=lerp_profile(x,[(-3.74,4.1),(-3.08,4.26),(-2.74,4.65),(-2.18,4.67),(-1.65,4.91),(-.85,4.96),(.25,4.89),(.95,4.975),(1.55,4.88),(2.10,4.91),(2.68,4.53),(3.1,4.40),(3.74,4.10)])
            y=ybase+(bottom-.025)*(1-smooth(.025,.75,ybase))-(4.975-top)*smooth(4.15,4.975,ybase)
            v.append((x,y,sculpt(x,y)))
    s=NX+1
    for j in range(NY):
        for i in range(NX):
            q=j*s+i;f.append((q,q+1,q+s+1,q+s))
    boundary=list(range(s))+[j*s+NX for j in range(1,NY+1)]+list(range(NY*s+NX-1,NY*s-1,-1))+[j*s for j in range(NY-1,0,-1)]
    back=len(v);v += [(v[q][0],v[q][1],-.26) for q in boundary]
    for i,q in enumerate(boundary):f.append((q,back+i,back+(i+1)%len(boundary),boundary[(i+1)%len(boundary)]))
    f.append(tuple(reversed(range(back,len(v)))))
    body=mesh('Single continuous facial stone volume',v,f,clay);body.data.polygons[-1].use_smooth=False
    def loft(name,rows,kind):
        # Cross sections are deliberately authored. Subdivision rounds the broad
        # planes; there is no sampled displacement formula deciding nose/lip shape.
        vs=[];fs=[];count=17
        for yy,width,profile in rows:
            for k in range(count):
                u=-1+2*k/(count-1);x=width*u
                if kind=='nose':
                    y=yy;z=lerp_profile(abs(u),profile)
                else:
                    y=1.40-.17*(abs(x)/1.63)**1.7+yy
                    z=profile-(profile-.31)*smooth(.57,1,abs(u))
                vs.append((x,y,z))
        for j in range(len(rows)-1):
            for k in range(count-1):
                q=j*count+k;fs.append((q,q+1,q+1+count,q+count))
        edge=list(range(count))+[j*count+count-1 for j in range(1,len(rows))]+list(range((len(rows)-1)*count+count-2,(len(rows)-1)*count-1,-1))+[j*count for j in range(len(rows)-2,0,-1)]
        back=len(vs);vs += [(vs[q][0],vs[q][1],.14) for q in edge]
        for j,q in enumerate(edge):fs.append((q,back+j,back+(j+1)%len(edge),edge[(j+1)%len(edge)]))
        fs.append(tuple(reversed(range(back,len(vs)))))
        o=mesh(name,vs,fs,clay);sub=o.modifiers.new('Broad carved plane transitions','SUBSURF');sub.levels=2;sub.render_levels=2
        return o
    nose=loft('CONTROL | bridge central tip and alar loft',[
        (4.16,.49,[(0,.48),(.40,.44),(.72,.37),(1,.28)]),
        (3.75,.42,[(0,.73),(.4,.71),(.72,.53),(1,.28)]),
        (3.25,.51,[(0,.94),(.4,.91),(.72,.70),(1,.30)]),
        (2.91,.68,[(0,1.07),(.4,1.01),(.72,.80),(1,.34)]),
        (2.70,.88,[(0,1.13),(.4,1.06),(.70,.93),(1,.39)]),
        (2.52,1.08,[(0,1.12),(.35,1.02),(.65,1.00),(.85,.75),(1,.40)]),
        (2.32,1.07,[(0,.98),(.35,.83),(.65,.91),(.85,.71),(1,.40)]),
        (2.15,.90,[(0,.72),(.35,.56),(.65,.66),(.85,.51),(1,.36)]),
        (2.05,.67,[(0,.48),(.35,.45),(.65,.42),(1,.31)])],'nose')
    for side in (-1,1):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,radius=1,location=xyz((side*.63,2.215,.76)))
        cutter=bpy.context.object;cutter.name='CONTROL | nostril cavity '+str(side);cutter.scale=(.27,.40,.11)
        for c in list(cutter.users_collection):c.objects.unlink(cutter)
        col.objects.link(cutter);cutter.hide_render=True
        mod=nose.modifiers.new('Deep actual nostril subtraction '+str(side),'BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
    upper=loft('CONTROL | maxilla and upper lip patch',[(.76,1.02,.48),(.57,1.29,.61),(.38,1.47,.74),(.21,1.54,.82),(.085,1.51,.81),(.032,1.46,.63),(.014,1.38,.34)],'mouth')
    lower=loft('CONTROL | lower lip continuous chin patch',[(-.014,1.38,.34),(-.065,1.48,.67),(-.17,1.55,.79),(-.32,1.55,.74),(-.48,1.46,.60),(-.69,1.22,.46)],'mouth')
    # The evaluated joined sculpt is a real union of the intersecting solid control
    # cages. Keep all input lofts and nostril cutters editable in the saved source.
    bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();bm=bmesh.new()
    for o in (body,nose,upper,lower):
        me=bpy.data.meshes.new_from_object(o.evaluated_get(deps),depsgraph=deps);bm.from_mesh(me);bpy.data.meshes.remove(me);o.hide_render=True
    me=bpy.data.meshes.new('Continuous joined sculpt');bm.to_mesh(me);bm.free()
    sculpted=bpy.data.objects.new('Carved mineral head | joined editable lofts',me);col.objects.link(sculpted);me.materials.append(clay)
    remesh=sculpted.modifiers.new('Union joined anatomical cages','REMESH');remesh.mode='VOXEL';remesh.voxel_size=.026;remesh.use_smooth_shade=True
    smoothmod=sculpted.modifiers.new('One restrained sculpture relaxation','SMOOTH');smoothmod.factor=.32;smoothmod.iterations=2
    # One deliberate recessed downturned crease replaces voxel-sized accidental
    # bridges inside the joined lips. The closed elliptical cutter tapers at ends.
    cv=[];cf=[];rings,segments=56,24
    for i in range(rings+1):
        u=-.999+1.998*i/rings;x=1.44*u;radius=math.sqrt(1-u*u)
        y=1.40-.17*(abs(x)/1.63)**1.7
        for k in range(segments):
            a=math.tau*k/segments;cv.append((x,y+.041*radius*math.sin(a),.58+.32*radius*math.cos(a)))
    for i in range(rings):
        for k in range(segments):cf.append((i*segments+k,i*segments+(k+1)%segments,(i+1)*segments+(k+1)%segments,(i+1)*segments+k))
    cf += [tuple(reversed(range(segments))),tuple(rings*segments+k for k in range(segments))]
    mouthcut=mesh('CONTROL | single recessed mouth crease',cv,cf,clay);mouthcut.hide_render=True
    mod=sculpted.modifiers.new('Single continuous mouth cut','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=mouthcut
    # A few large break planes interrupt exposed architectural returns. Their
    # recesses remove material from the original stone; they are not applied
    # decorative shards or a uniform noise displacement over the face.
    for label,outline,floor in [
        ('left lower return',[(-4.05,.71),(-3.20,1.18),(-4.05,1.58)],.055),
        ('right temple edge',[(4.05,2.60),(3.40,2.95),(4.05,3.43)],.08),
        ('upper left fracture',[(-3.53,4.90),(-2.62,4.56),(-2.26,5.30)],.11),
    ]:
        verts=[(x,y,floor) for x,y in outline]+[(x,y,1.4) for x,y in outline]
        faces=[(2,1,0),(3,4,5),(0,1,4,3),(1,2,5,4),(2,0,3,5)]
        cutter=mesh('CONTROL | architectural break '+label,verts,faces,clay)
        cutter.hide_render=True
        mod=sculpted.modifiers.new('Broad broken return '+label,'BOOLEAN')
        mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
    # Retain source controls while exporting only the evaluated joined surface.
    sculpted['delivery_head_triangle_target']=25000
    sculpted['source_role']='Evaluated union of editable skull, nasal and lip cages'
    bpy.context.view_layer.update()
    deps=bpy.context.evaluated_depsgraph_get()
    evaluated=bpy.data.meshes.new_from_object(sculpted.evaluated_get(deps),depsgraph=deps)
    evaluated.calc_loop_triangles()
    tree=BVHTree.FromPolygons([v.co.copy() for v in evaluated.vertices],[tuple(t.vertices) for t in evaluated.loop_triangles],all_triangles=True)
    bpy.data.meshes.remove(evaluated)
    def surface_z(x,y):
        hit,normal,index,distance=tree.ray_cast(Vector((x,-10,y)),Vector((0,1,0)),20)
        if hit is None:raise ValueError(('Ornament misses carved surface',x,y))
        return -hit.y
    return sculpted,surface_z
