"""Original Celestial Foundry props. Run with Blender --background --factory-startup.
Units: meters. Source Z-up / front -Y; glTF conversion supplies Y-up / front +Z.
"""
import bpy, math, json, os, sys
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets' / 'blender'
REVIEW = OUT / 'review'
MODELS = ROOT / 'public' / 'models'
ARCHIVE = ROOT / '.local' / 'exploratory-assets'
for p in (OUT, REVIEW, MODELS, ARCHIVE): p.mkdir(parents=True, exist_ok=True)
TAU = math.tau

# This process is isolated factory startup; no user file is loaded or changed.
scene = bpy.data.scenes.new('Celestial_Foundry_Source')
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
MATS = {}

def material(name, color, metallic=0.0, roughness=.5, emission=None):
    m = bpy.data.materials.new(name); m.use_nodes = True
    n = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    n.inputs['Base Color'].default_value = (*color, 1)
    n.inputs['Metallic'].default_value = metallic
    n.inputs['Roughness'].default_value = roughness
    if emission:
        n.inputs['Emission Color'].default_value = (*color,1)
        n.inputs['Emission Strength'].default_value = emission
    m.diffuse_color = (*color,1)
    MATS[name] = m
    return m

bronze = material('01 | cast bronze', (.31,.145,.047), .82,.31)
brass = material('02 | rubbed raised brass', (.59,.33,.095), .86,.27)
oxide = material('03 | recessed verdigris', (.035,.20,.17), .5,.62)
stone = material('04 | blue-black basalt', (.027,.045,.062), .08,.72)
dark = material('05 | bronze cavity and engraving', (.05,.031,.019), .72,.47)
cyan = material('06 | celestial enamel', (.12,.68,.72), .2,.24, .65)
amber = material('07 | consecrated amber', (.95,.33,.047), .15,.31,.85)
clay = material('REVIEW | neutral clay', (.4,.43,.45), 0,.6)

current = None
roles = {}
def collection(name):
    c = bpy.data.collections.new(name); scene.collection.children.link(c); return c

def bind(o, name, mat, role='Fixed_Frame'):
    o.name = name
    for c in list(o.users_collection): c.objects.unlink(o)
    current.objects.link(o)
    if mat: o.data.materials.append(mat)
    o['assembly'] = role
    roles.setdefault(current.name,{}).setdefault(role,[]).append(o)
    return o

def finish(o, bevel=0.0, smooth=True):
    if smooth and o.type == 'MESH':
        for p in o.data.polygons: p.use_smooth = True
    if bevel:
        m=o.modifiers.new('Manufactured edge radius','BEVEL'); m.width=bevel; m.segments=3
    return o

def mesh(name, verts, faces, mat, role='Fixed_Frame', bevel=0):
    data=bpy.data.meshes.new(name); data.from_pydata(verts,[],faces); data.update()
    return finish(bind(bpy.data.objects.new(name,data),name,mat,role),bevel)

def cube(name, loc, scale, mat, bevel=.04, role='Fixed_Frame'):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o=bind(bpy.context.object,name,mat,role); o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,bevel,False)

def cylinder(name, loc, radius, depth, mat, vertices=64, axis=(0,0,1), role='Fixed_Frame', bevel=.018):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    o=bind(bpy.context.object,name,mat,role)
    o.rotation_quaternion = Vector((0,0,1)).rotation_difference(Vector(axis))
    o.rotation_mode='QUATERNION'
    o.rotation_quaternion = Vector((0,0,1)).rotation_difference(Vector(axis))
    return finish(o,bevel)

def rod(name,a,b,r,mat,role='Fixed_Frame'):
    a,b=Vector(a),Vector(b)
    return cylinder(name,(a+b)*.5,r,(b-a).length,mat,32,(b-a).normalized(),role,.01)

def sphere(name,loc,scale,mat,role='Fixed_Frame'):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=1, location=loc)
    o=bind(bpy.context.object,name,mat,role); o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o)

def ring(name, center, radius, width, depth, mat, role='Fixed_Frame', rotation=None, segments=128):
    # Solid annulus with a rectangular manufactured section, softened by an editable bevel.
    verts=[]; faces=[]
    for i in range(segments):
        t=TAU*i/segments
        for r,y in ((radius+width/2,-depth/2),(radius+width/2,depth/2),(radius-width/2,depth/2),(radius-width/2,-depth/2)):
            verts.append((r*math.cos(t),y,r*math.sin(t)))
    for i in range(segments):
        for j in range(4): faces.append((i*4+j,((i+1)%segments)*4+j,((i+1)%segments)*4+(j+1)%4,i*4+(j+1)%4))
    o=mesh(name,verts,faces,mat,role,min(width*.19,.025)); o.location=center
    if rotation: o.rotation_euler=rotation
    return o

def lathe(name,profile,mat,role='Fixed_Frame',segments=96):
    verts=[]; faces=[]
    for r,z in profile:
        for i in range(segments):
            t=TAU*i/segments; verts.append((r*math.cos(t),r*math.sin(t),z))
    for j in range(len(profile)-1):
        for i in range(segments): faces.append((j*segments+i,j*segments+(i+1)%segments,(j+1)*segments+(i+1)%segments,(j+1)*segments+i))
    # The closed radial section models wall thickness and underside instead of a single surface.
    for i in range(segments): faces.append(((len(profile)-1)*segments+i,(len(profile)-1)*segments+(i+1)%segments,(i+1)%segments,i))
    return mesh(name,verts,faces,mat,role)

def tapered_sweep(name,pts,radii,mat,role='Fixed_Frame',sides=12):
    verts=[];faces=[]
    for j,p in enumerate(pts):
        p=Vector(p); before=Vector(pts[max(0,j-1)]); after=Vector(pts[min(len(pts)-1,j+1)])
        tangent=(after-before).normalized(); side=Vector((0,1,0)); cross=tangent.cross(side).normalized()
        for i in range(sides):
            a=TAU*i/sides; v=p+side*math.cos(a)*radii[j]*.75+cross*math.sin(a)*radii[j]
            verts.append(v)
    for j in range(len(pts)-1):
        for i in range(sides): faces.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
    faces.append(tuple(reversed(range(sides))))
    faces.append(tuple((len(pts)-1)*sides+i for i in range(sides)))
    return mesh(name,verts,faces,mat,role)

def bezier(a,b,c,d,n=30):
    a,b,c,d=map(Vector,(a,b,c,d))
    return [((1-t)**3*a+3*(1-t)**2*t*b+3*(1-t)*t*t*c+t**3*d) for t in [i/(n-1) for i in range(n)]]

def octagonal_plinth(radius,height=0.6):
    for name,r,z,h,mat in [('Lower basalt step',radius,.12,.24,stone),('Bronze footing inset',radius*.94,.26,.055,bronze),('Carved basalt drum',radius*.88,.42,.30,stone),('Chamfered upper cap',radius*.92,.60,.15,stone),('Top bronze lip',radius*.84,.685,.034,bronze)]:
        o=cylinder(name,(0,0,z),r,h,mat,8,bevel=.04);o.scale.y=.70
    for i in range(8):
        t=TAU*(i+.5)/8
        # Quiet shallow metal seams seat into the drum.
        x=radius*.823*math.cos(t); y=radius*.823*.70*math.sin(t)
        o=cube('Basalt inlaid seam', (x,y,.425),(.035,.02,.19),oxide,.006)
        o.rotation_euler.z=t-math.pi/2

ORRERY=collection('ASSET | Orrery');current=ORRERY
octagonal_plinth(2.1)
C=Vector((0,0,3.38))
for sign in (-1,1):
    cube('Foot plate', (sign*1.05,0,.765),(.82,.74,.13),bronze,.045)
    pts=bezier((sign*.88,0,.80),(sign*2.0,0,1.30),(sign*2.39,0,2.25),(sign*2.00,0,3.38))
    tapered_sweep('Swept load-bearing fork',pts,[.22-.075*i/29 for i in range(30)],bronze)
    # Rear buttress joins the arm midway, avoiding a paper-thin front-only construction.
    pts=bezier((sign*.85,.40,.8),(sign*1.4,.7,1.10),(sign*2.0,.35,1.85),(sign*2.11,0,2.42))
    tapered_sweep('Rear fork buttress',pts,[.13-.035*i/29 for i in range(30)],dark)
    cylinder('Outer trunnion housing',(sign*1.98,0,3.38),.22,.40,brass,64,(1,0,0))
    cylinder('Outer axle dark seat',(sign*2.20,0,3.38),.145,.035,dark,48,(1,0,0))
    cylinder('Outer bearing cap',(sign*2.24,0,3.38),.104,.045,bronze,48,(1,0,0))
    for y in (-.24,.24):
        cylinder('Foot recessed bolt',(sign*1.08,y,.855),.061,.025,brass,8)

ring('Calendar frame',C,1.89,.205,.235,bronze)
ring('Calendar oxide channel',C+Vector((0,-.125,0)),1.89,.076,.008,oxide)
ring('Calendar outer rolled edge',C,2.015,.043,.26,brass)
ring('Calendar inner rolled edge',C,1.770,.038,.24,brass)
for i in range(48):
    t=TAU*i/48; radius=1.89
    major=i%4==0
    o=cube('Calendar graduation %02d'%i,(radius*math.cos(t),-.143,3.38+radius*math.sin(t)),(.090 if major else .038,.022,.012),brass,.003)
    o.rotation_euler.y=-t
for i in range(8):
    t=TAU*i/8
    sphere('Blue enamel compass pin',(2.015*math.cos(t),-.146,3.38+2.015*math.sin(t)),(.045,.025,.045),cyan)

# Gimbal transforms express actual orthogonal pivot constraints.
middle_rotation=Matrix.Rotation(math.radians(24),4,'X')
inner_rotation=middle_rotation @ Matrix.Rotation(math.radians(-37),4,'Z')
middle=ring('Middle meridian',C,1.515,.112,.13,brass,'Gimbal_Middle')
middle.rotation_euler=middle_rotation.to_euler()
middle_inner=ring('Middle dark inset',C,1.515,.036,.145,oxide,'Gimbal_Middle')
middle_inner.rotation_euler=middle_rotation.to_euler()
inner=ring('Inner ecliptic',C,1.16,.118,.13,bronze,'Gimbal_Inner')
inner.rotation_euler=inner_rotation.to_euler()
inner_lip=ring('Inner raised lip',C,1.09,.026,.147,brass,'Gimbal_Inner')
inner_lip.rotation_euler=inner_rotation.to_euler()
for sign in (-1,1):
    rod('Middle gimbal axle',C+Vector((sign*1.51,0,0)),C+Vector((sign*1.79,0,0)),.086,dark)
    cylinder('Middle pivot boss',C+Vector((sign*1.515,0,0)),.145,.18,bronze,48,(1,0,0))
    axis=middle_rotation.to_3x3()@Vector((0,0,sign))
    rod('Inner pivot spindle',C+axis*1.10,C+axis*1.515,.065,brass,'Gimbal_Middle')
    cylinder('Inner pivot collar',C+axis*1.30,.11,.11,dark,48,axis,'Gimbal_Middle')
rod('Central spindle',C+Vector((0,0,-.96)),C+Vector((0,0,.96)),.028,brass,'Gimbal_Inner')
sphere('Core_Star',C,(.32,.26,.32),amber,'Core_Star')
for i in range(8):
    t=TAU*i/8
    radius=.68 if i%2==0 else .52
    # Cast star rays: broad root, taper, genuine front/back thickness.
    v=[(.12,-.04,-.065),(.12,-.04,.065),(radius,0,0),(.12,.04,-.065),(.12,.04,.065)]
    f=[(0,1,2),(3,2,4),(0,2,3),(1,4,2),(0,3,4,1)]
    o=mesh('Cored star ray',v,f,brass,'Core_Star',.012);o.location=C;o.rotation_euler.y=-t
for i in range(3):
    angle=[.58,2.70,4.20][i]; r=[1.52,1.16,1.16][i]
    R=middle_rotation if i==0 else inner_rotation
    position=C+R.to_3x3()@Vector((r*math.cos(angle),0,r*math.sin(angle)))
    sphere('Mounted orbit bead',position,(.13,.13,.13),cyan if i!=1 else amber,'Gimbal_Middle' if i==0 else 'Gimbal_Inner')
# A shallow star relief centered in the plinth face.
for i in range(4):
    t=TAU*i/4
    o=cube('Plinth votive sigil',(0,-1.28,.43),(.15,.025,.022),brass,.005);o.rotation_euler.y=t+math.pi/4

BELL=collection('ARCHIVE | Unintegrated bell study');current=BELL
octagonal_plinth(1.42)
# Tapered shrine uprights and arched canopy are substantial from the rear.
for sign in (-1,1):
    cube('Upright foot',(sign*.98,0,.78),(.47,.59,.16),bronze,.045,'Bell_Frame')
    pts=bezier((sign*.98,0,.82),(sign*1.12,.02,1.50),(sign*1.16,0,2.43),(sign*.87,0,2.91))
    tapered_sweep('Bell shrine tapered arch',pts,[.16-.02*i/29 for i in range(30)],bronze,'Bell_Frame')
    rod('Bell suspension axle',(sign*.93,0,2.68),(sign*.22,0,2.68),.081,brass,'Bell_Frame')
    cylinder('Suspension bearing',(sign*.97,0,2.68),.16,.22,dark,48,(1,0,0),'Bell_Frame')
    cylinder('Suspension pin cap',(sign*1.10,0,2.68),.12,.07,brass,48,(1,0,0),'Bell_Frame')
pts=bezier((-.87,0,2.91),(-.60,0,3.29),(.60,0,3.29),(.87,0,2.91))
tapered_sweep('Crowned load-bearing arch',pts,[.14]*30,bronze,'Bell_Frame')
ring('Crown votive aperture',(0,0,3.22),.18,.051,.16,brass,'Bell_Frame',segments=64)
sphere('Crown enamel star',(0,-.11,3.22),(.05,.025,.05),cyan,'Bell_Frame')

# Closed cross section: crown, flared wall, rolled lip and visible cavity thickness.
profile=[(.0,2.60),(.14,2.60),(.22,2.55),(.25,2.48),(.28,2.39),(.35,2.29),(.40,2.12),(.45,1.91),(.54,1.69),(.68,1.46),(.77,1.36),(.78,1.29),(.76,1.23),(.68,1.22),(.66,1.31),(.58,1.44),(.48,1.64),(.40,1.87),(.35,2.08),(.30,2.25),(.24,2.38),(.14,2.46),(0,2.46)]
shell=lathe('Bell cast hollow shell',profile,bronze,'Bell_Shell')
# Lathed belts track actual bell body and do not float off its surface.
lathe('Bell lip bronze bead',[(.77,1.27),(.806,1.285),(.812,1.32),(.786,1.36),(.755,1.36),(.748,1.29)],brass,'Bell_Shell')
lathe('Bell shoulder inscription belt',[(.28,2.38),(.292,2.385),(.314,2.34),(.331,2.29),(.316,2.288)],oxide,'Bell_Shell')
lathe('Bell upper rolled collar',[(.252,2.46),(.28,2.44),(.284,2.42),(.26,2.42)],brass,'Bell_Shell')
ring('Bell crown suspension eye',(0,0,2.635),.105,.057,.19,bronze,'Bell_Shell',segments=48)
rod('Clapper stem',(0,0,2.44),(0,0,1.28),.035,dark,'Clapper')
sphere('Clapper striker',(0,0,1.37),(.115,.115,.16),dark,'Clapper')
for i in range(12):
    t=TAU*i/12
    # Distinct incised tally marks, kept subordinate to the main casting.
    x=.422*math.cos(t);y=.422*math.sin(t)
    o=cube('Bell inscription tally',(x,y,2.065),(.011,.016,.077),brass,.003,'Bell_Shell');o.rotation_euler.z=t

# A votive bowl is physically seated on the front edge of the platform.
bowl=lathe('Votive offering bowl',[(.0,.72),(.22,.72),(.28,.77),(.31,.88),(.31,.91),(.27,.92),(.24,.80),(0,.78)],stone,'Bell_Frame',64)
bowl.location.y=-.67
cylinder('Votive amber pool',(0,-.67,.80),.21,.015,amber,64,role='Bell_Frame',bevel=.007)

# Asset staging, explicitly excluded from every export.
STAGE=collection('PRESENTATION | Excluded from export');current=STAGE
floor=cube('Review ground',(0,0,-.085),(200,200,.1),material('REVIEW | Ground',(.018,.026,.038),0,.78),0)
world=bpy.data.worlds.new('Review world');world.use_nodes=True;scene.world=world
bg=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.11,.15,.19,1);bg.inputs['Strength'].default_value=.32

def light(name,loc,energy,color,size):
    d=bpy.data.lights.new(name,'AREA');d.energy=energy;d.color=color;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(name,d);STAGE.objects.link(o);o.location=loc
    o.rotation_euler=(Vector((0,0,2.5))-o.location).to_track_quat('-Z','Y').to_euler();return o

light('Large warm key',(-5,-6,8),1700,(1,.79,.56),5)
light('Broad cool fill',(5,-4,5),1350,(.42,.68,1),5)
light('Amber rim',(2,4,7),2100,(1,.52,.21),4)
camdata=bpy.data.cameras.new('Asset review camera');cam=bpy.data.objects.new('Asset review camera',camdata);STAGE.objects.link(cam);scene.camera=cam
camdata.type='ORTHO';camdata.lens=55
scene.render.resolution_x=1000;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
try:scene.render.engine='CYCLES'
except TypeError:pass
if scene.render.engine=='CYCLES':
    scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.film_transparent=False

def show(asset):
    for c in (ORRERY,BELL):
        c.hide_render=c!=asset;c.hide_viewport=c!=asset

def render(asset,name,direction,clay_override=False,scale=None,resolution=1000,target_dir=REVIEW):
    show(asset)
    center=Vector((0,0,2.65 if asset==ORRERY else 1.65))
    cam.location=center+Vector(direction).normalized()*15
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    camdata.ortho_scale=scale or (6.2 if asset==ORRERY else 3.85)
    scene.render.resolution_x=scene.render.resolution_y=resolution
    scene.view_layers[0].material_override=clay if clay_override else None
    scene.render.filepath=str(target_dir/name)
    bpy.ops.render.render(write_still=True)
    scene.view_layers[0].material_override=None

def evaluated_metrics(asset):
    deps=bpy.context.evaluated_depsgraph_get();tri=0;verts=0;bounds=[]
    for o in asset.objects:
        if o.type!='MESH':continue
        eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();tri+=len(me.loop_triangles);verts+=len(me.vertices)
        bounds += [o.matrix_world@Vector(b) for b in eo.bound_box];eo.to_mesh_clear()
    lo=[min(v[i] for v in bounds) for i in range(3)];hi=[max(v[i] for v in bounds) for i in range(3)]
    return {'evaluated_triangles':tri,'evaluated_vertices':verts,'source_mesh_objects':len(asset.objects),'source_bounds_min':lo,'source_bounds_max':hi,'gltf_dimensions_xyz':[hi[0]-lo[0],hi[2]-lo[2],hi[1]-lo[1]],'gltf_bounds_min':[lo[0],lo[2],-hi[1]],'gltf_bounds_max':[hi[0],hi[2],-lo[1]]}

def export_asset(asset,filename,target_dir=MODELS):
    show(asset);bpy.context.view_layer.update()
    delivery=collection('DELIVERY temporary '+asset.name)
    copies=[];deps=bpy.context.evaluated_depsgraph_get()
    for role,objects in roles[asset.name].items():
        batch=[]
        for src in objects:
            if src.type!='MESH':continue
            me=bpy.data.meshes.new_from_object(src.evaluated_get(deps),depsgraph=deps)
            o=bpy.data.objects.new('Export '+src.name,me);delivery.objects.link(o);o.matrix_world=src.matrix_world.copy();batch.append(o)
        if not batch:continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in batch:o.select_set(True)
        bpy.context.view_layer.objects.active=batch[0]
        bpy.ops.object.join();joined=batch[0];joined.name=role
        scene.cursor.location=(0,0,3.38) if asset==ORRERY and role!='Fixed_Frame' else ((0,0,2.44) if role=='Clapper' else (0,0,0))
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        copies.append(joined)
    bpy.ops.object.select_all(action='DESELECT')
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0]
    kwargs={'filepath':str(target_dir/filename),'use_selection':True,'export_format':'GLB','export_apply':True,'export_yup':True,'export_cameras':False,'export_lights':False,'export_extras':True,'export_animations':False}
    bpy.ops.export_scene.gltf(**kwargs)
    for o in copies:bpy.data.objects.remove(o,do_unlink=True)
    bpy.data.collections.remove(delivery)

manifest={'blender_version':bpy.app.version_string,'coordinate_system':'GLB X right, Y up, front +Z; source Z up/front -Y','units':'meters','pivot':'ground centered','external_dependencies':[],'materials':[{ 'name':m.name,'metallic':next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED').inputs['Metallic'].default_value,'roughness':next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED').inputs['Roughness'].default_value} for m in (bronze,brass,oxide,stone,dark,cyan,amber)],'assets':{}}
for asset,name in ((ORRERY,'orrery.glb'),(BELL,'bell-altar.glb')):
    show(asset);bpy.context.view_layer.update()
    manifest['assets'][name]=evaluated_metrics(asset)
    manifest['assets'][name]['nodes']=list(roles[asset.name])
    target_dir=ARCHIVE if asset==BELL else MODELS
    export_asset(asset,name,target_dir)
    manifest['assets'][name]['bytes']=(target_dir/name).stat().st_size
(ARCHIVE/'manifest.json').write_text(json.dumps(manifest,indent=2))
show(ORRERY);scene.cursor.location=(0,0,0)
cam.location=(7,-11,7);cam.rotation_euler=(Vector((0,0,2.6))-cam.location).to_track_quat('-Z','Y').to_euler();camdata.ortho_scale=6.3
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'celestial-foundry.blend'))
for asset,prefix in ((ORRERY,'orrery'),(BELL,'bell')):
    render(asset,prefix+'-beauty.png',(5,-10,4),target_dir=ARCHIVE)
    review_target=ARCHIVE if asset==BELL else REVIEW
    render(asset,prefix+'-front-clay.png',(0,-1,0),True,resolution=768,target_dir=review_target)
    render(asset,prefix+'-back-clay.png',(0,1,0),True,resolution=768,target_dir=review_target)
    render(asset,prefix+'-side-clay.png',(1,0,0),True,resolution=768,target_dir=review_target)
show(ORRERY)
print('ASSET_DELIVERY_COMPLETE '+json.dumps(manifest))
