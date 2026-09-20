"""Editable, packed Rock Surface material; no map edits or runtime outputs.
Build: Blender -b -t 16 --python-exit-code 1 --python this_file.py
Reopen/verify: same invocation with -- --verify
"""
from pathlib import Path
import bpy, math, json, hashlib, struct, sys
import numpy as np
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/blender';REVIEW=OUT/'review/scanned-stone';TEX=ROOT/'public/materials/vendor/rock-surface'
SOURCE=OUT/'scanned-stone.blend';REPORT=OUT/'scanned-stone-verification.json'
MAT='Rock Surface | original CC0 scan | 2m UV tile'
URL='https://polyhaven.com/a/rock_surface'
OLD=OUT/'material-library.blend'
REVIEW.mkdir(parents=True,exist_ok=True)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def node(nodes,kind,name,x,y):
    n=nodes.new(kind);n.name=n.label=name;n.location=(x,y);return n
def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
def plain(name,color,roughness=.8):
    m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=roughness;return m

def build():
    provenance=json.loads((TEX/'provenance.json').read_text())
    bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene
    scene.name='Scanned stone | editable 2m material review'
    scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
    scene['preserved_v13_material_library_sha256']=sha(OLD)
    scene['asset_source']=URL;scene['asset_author']='Amal Kumar';scene['license']='CC0'
    scene['physical_tile_width_m']=2.0
    mat=bpy.data.materials.new(MAT);mat.use_nodes=True;mat.use_fake_user=True;mat.asset_mark()
    mat.asset_data.author='Amal Kumar (source scan); shader assembly in this repository'
    mat.asset_data.description='CC0 Rock Surface. Original packed 16-bit maps, normalized +Y tangent normals, measured 2m UV tile. No AO or active displacement.'
    mat['source_url']=URL;mat['license']='CC0';mat['tile_width_m']=2.;mat['normal_convention']='OpenGL +Y, explicit normalize before tangent-space conversion'
    nodes=mat.node_tree.nodes;nodes.clear();links=mat.node_tree.links
    out=node(nodes,'ShaderNodeOutputMaterial','Material output | surface only',900,160)
    bsdf=node(nodes,'ShaderNodeBsdfPrincipled','Stone | no metallic coating',620,180)
    bsdf.inputs['Metallic'].default_value=0;bsdf.inputs['IOR'].default_value=1.48
    links.new(bsdf.outputs['BSDF'],out.inputs['Surface'])
    uv=node(nodes,'ShaderNodeUVMap','UVMap | one UV square = 2m',-1200,100);uv.uv_map='UVMap'
    textures={}
    for k,(name,space) in enumerate([('Diffuse','sRGB'),('nor_gl','Non-Color'),('Rough','Non-Color'),('Displacement','Non-Color')]):
        path=TEX/(name+'.png');assert sha(path)==provenance['original_maps'][name]['sha256']
        image=bpy.data.images.load(str(path),check_existing=False);image.name='Rock Surface | '+name+' | original 16-bit'
        image.colorspace_settings.name=space;image.use_fake_user=True;image.pack()
        image.filepath='//../../public/materials/vendor/rock-surface/'+name+'.png'
        t=node(nodes,'ShaderNodeTexImage',name+' | original packed16 | '+space,-950,480-k*300)
        t.image=image;t.extension='REPEAT';t.interpolation='Linear';links.new(uv.outputs['UV'],t.inputs['Vector']);textures[name]=t
    links.new(textures['Diffuse'].outputs['Color'],bsdf.inputs['Base Color'])
    links.new(textures['Rough'].outputs['Color'],bsdf.inputs['Roughness'])
    # The vendor's filtered normals are not uniformly unit length. Normalize the
    # raw linear XYZ vector without changing handedness or modifying source maps.
    previous=textures['nor_gl'].outputs['Color']
    for k,(name,operation,value) in enumerate([('Decode RGB x2','MULTIPLY',(2,2,2)),('Decode XYZ -1','ADD',(-1,-1,-1)),
                                            ('Normalize source XYZ','NORMALIZE',None),('Encode half','MULTIPLY',(.5,.5,.5)),('Encode RGB +half','ADD',(.5,.5,.5))]):
        n=node(nodes,'ShaderNodeVectorMath',name,-650+k*220,-100);n.operation=operation
        if value is not None:n.inputs[1].default_value=value
        links.new(previous,n.inputs[0]);previous=n.outputs['Vector']
    normal=node(nodes,'ShaderNodeNormalMap','OpenGL +Y tangent normal | no green flip',400,-190)
    normal.space='TANGENT';normal.uv_map='UVMap';normal.inputs['Strength'].default_value=1
    links.new(previous,normal.inputs['Color']);links.new(normal.outputs['Normal'],bsdf.inputs['Normal'])
    disp=node(nodes,'ShaderNodeDisplacement','OPTIONAL height | OFF, amplitude uncalibrated',-500,-560)
    disp.inputs['Midlevel'].default_value=.5;disp.inputs['Scale'].default_value=0
    links.new(textures['Displacement'].outputs['Color'],disp.inputs['Height'])
    # The height map is preserved for authoring; it is not layered over the normal
    # or sent to surface displacement without a measured amplitude and geometry test.
    clay=plain('REVIEW | neutral geometric clay',(.24,.24,.24),.85);clay.use_fake_user=True
    normalclay=mat.copy();normalclay.name='REVIEW | normal-only neutral clay'
    if normalclay.asset_data:normalclay.asset_clear()
    normalclay.use_fake_user=True
    p=normalclay.node_tree.nodes['Stone | no metallic coating']
    for sock in ('Base Color','Roughness'):
        for link in list(p.inputs[sock].links):normalclay.node_tree.links.remove(link)
    p.inputs['Base Color'].default_value=(.24,.24,.24,1);p.inputs['Roughness'].default_value=.85
    # Measured studio slab, not a new game asset. Planar faces use 2m per UV tile.
    v=[(x,y,z) for y in (-.09,.09) for z in (0,2) for x in (-1,1)]
    faces=[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]
    me=bpy.data.meshes.new('2m slab review mesh');me.from_pydata(v,[],faces);me.update();uvs=me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        axis=max(range(3),key=lambda a:abs(poly.normal[a]));dims=[a for a in range(3) if a!=axis]
        for li in poly.loop_indices:
            co=me.vertices[me.loops[li].vertex_index].co;uvs.data[li].uv=(co[dims[0]]/2+.5,co[dims[1]]/2)
    slab=bpy.data.objects.new('REVIEW | slab 2m x 2m, UV0..1',me);scene.collection.objects.link(slab);slab.location.x=-1.4;me.materials.append(mat)
    bevel=slab.modifiers.new('Studio edge radius 15mm','BEVEL');bevel.width=.015;bevel.segments=2
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64,ring_count=32,radius=.72,location=(1.15,0,.74))
    sphere=bpy.context.object;sphere.name='REVIEW | sphere, 2m equatorial tile scale';sphere.data.materials.append(mat);sphere.rotation_euler.z=-math.pi/2
    for p in sphere.data.polygons:p.use_smooth=True
    for uvp in sphere.data.uv_layers.active.data:uvp.uv.x*=math.tau*.72/2;uvp.uv.y*=math.pi*.72/2
    bpy.ops.mesh.primitive_plane_add(size=6,location=(0,0,.01));repeat=bpy.context.object;repeat.name='REVIEW | measured 6m plane, 3x3 repeat'
    repeat.data.materials.append(mat)
    for p in repeat.data.uv_layers.active.data:p.uv*=3
    repeat.hide_render=True;repeat.hide_viewport=True
    world=bpy.data.worlds.new('Scanned stone neutral studio');world.use_nodes=True;scene.world=world
    world.node_tree.nodes['Background'].inputs['Color'].default_value=(.08,.08,.08,1);world.node_tree.nodes['Background'].inputs['Strength'].default_value=.16
    light=bpy.data.lights.new('STUDIO | movable opposite key','AREA');light.energy=650;light.size=3
    lo=bpy.data.objects.new(light.name,light);scene.collection.objects.link(lo);lo.location=(-3,-4,4);aim(lo,(0,0,1))
    cd=bpy.data.cameras.new('STUDIO | matched review camera');cam=bpy.data.objects.new(cd.name,cd);scene.collection.objects.link(cam);scene.camera=cam
    cd.type='ORTHO';cd.ortho_scale=5.1;cam.location=(0,-7,1.4);aim(cam,(0,0,1))
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=20;scene.cycles.use_denoising=True
    scene.render.threads_mode='FIXED';scene.render.threads=16;scene.render.resolution_x=840;scene.render.resolution_y=500
    scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.view_settings.view_transform='AgX'
    text=bpy.data.texts.new('README | measured material, source and usage')
    text.write('Rock Surface by Amal Kumar, '+URL+'; CC0. Original 1024-square 16-bit PNGs packed unchanged.\n'
               'One UV tile spans 2 metres on a planar surface. The 2m slab and 6m repeat plane are measured. Sphere latitude UVs compress toward poles and have a hidden rear seam; they test response, not distortion-free texel density.\n'
               'Diffuse sRGB. nor_gl, Rough and Displacement Non-Color. Normal RGB decodes to XYZ, normalizes, re-encodes and enters tangent Normal Map without Y inversion.\n'
               'No AO, color painting, light bake or active displacement. Height is retained in an OFF node with zero scale: absolute amplitude is uncalibrated.\n'
               'IOR1.48 is an editable presentation assumption, not a scanned measurement. All primitives are studio references, not game mesh assets. The runtime separately uses palette modulation and technical map conversion.\n')
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE))

def views(prefix):
    scene=bpy.context.scene;cam=scene.camera;key=bpy.data.objects['STUDIO | movable opposite key']
    slab=bpy.data.objects['REVIEW | slab 2m x 2m, UV0..1'];sphere=bpy.data.objects['REVIEW | sphere, 2m equatorial tile scale']
    repeat=bpy.data.objects['REVIEW | measured 6m plane, 3x3 repeat'];layer=scene.view_layers[0]
    def save(name):scene.render.filepath=str(REVIEW/(prefix+'-'+name+'.png'));bpy.ops.render.render(write_still=True)
    for side in (-1,1):
        key.location=(side*3,-4,4);aim(key,(0,0,1));save('material-left' if side<0 else 'material-right')
        layer.material_override=bpy.data.materials['REVIEW | normal-only neutral clay'];save('normal-clay-left' if side<0 else 'normal-clay-right');layer.material_override=None
    layer.material_override=bpy.data.materials['REVIEW | neutral geometric clay'];save('geometry-clay');layer.material_override=None
    cam.location=(3,-7,3.2);aim(cam,(0,0,1));save('material-oblique')
    slab.hide_render=sphere.hide_render=True;repeat.hide_render=False;repeat.hide_viewport=False
    cam.location=(0,-.001,8);aim(cam,(0,0,0));cam.data.ortho_scale=6.25;scene.render.resolution_x=scene.render.resolution_y=720
    for side in (-1,1):
        key.location=(side*4,-3,6);key.data.energy=1150;key.data.size=5;aim(key,(0,0,0));save('repeat-3x3-left' if side<0 else 'repeat-3x3-right')
    layer.material_override=None

def verify():
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE));scene=bpy.context.scene;mat=bpy.data.materials[MAT]
    assert scene['preserved_v13_material_library_sha256']==sha(OLD)
    p=json.loads((TEX/'provenance.json').read_text());report={'status':'pass','scope':'Editable packed material and source preservation; not AAA or complete game acceptance',
        'source_url':URL,'author':'Amal Kumar','license':'CC0','physical_tile_width_m':2,'source_sha256':sha(SOURCE),'blender':bpy.app.version_string,
        'preserved_v13_library_sha256':sha(OLD),'images':{},'shader':{},'limits':['sphere UV compression/pole distortion is expected; rear seam is not a texture-tile seam','height amplitude uncalibrated and displacement disabled','actual game palette/sampling is verified separately by root']}
    for name in ('Diffuse','nor_gl','Rough','Displacement'):
        image=bpy.data.images['Rock Surface | '+name+' | original 16-bit'];path=TEX/(name+'.png');raw=path.read_bytes()
        width,height,bits,color=struct.unpack('>IIBB',raw[16:26]);packed=bytes(image.packed_file.data)
        assert hashlib.sha256(packed).hexdigest()==sha(path)==p['original_maps'][name]['sha256']
        space='sRGB' if name=='Diffuse' else 'Non-Color';assert image.colorspace_settings.name==space
        assert list(image.size)==[width,height] and bits==16
        a=np.empty(len(image.pixels),dtype=np.float32);image.pixels.foreach_get(a);a=a.reshape(height,width,-1)
        a=a[:,:,:3] if a.shape[2]>=3 else np.repeat(a[:,:,:1],3,axis=2)
        assert np.isfinite(a).all()
        stats={'width':width,'height':height,'bits':bits,'png_color_type':color,'colorspace':space,'packed_original_bytes':len(packed),'sha256':sha(path),'channel_min':a.min(axis=(0,1)).tolist(),'channel_max':a.max(axis=(0,1)).tolist()}
        if name=='nor_gl':
            xyz=a*2-1;length=np.linalg.norm(xyz,axis=2);assert length.min()>.25 and xyz[:,:,2].min()>0
            stats['decoded_length_min']=float(length.min());stats['decoded_length_max']=float(length.max());normalized=xyz/length[:,:,None]
            stats['normalized_length_error_max']=float(np.abs(np.linalg.norm(normalized,axis=2)-1).max())
        stats['wrap_delta_x_mean']=float(np.abs(a[:,0]-a[:,-1]).mean());stats['interior_delta_x_mean']=float(np.abs(a[:,1:]-a[:,:-1]).mean())
        stats['wrap_delta_y_mean']=float(np.abs(a[0]-a[-1]).mean());stats['interior_delta_y_mean']=float(np.abs(a[1:]-a[:-1]).mean())
        report['images'][name]=stats
    nodes=mat.node_tree.nodes
    expected={'Decode RGB x2':'MULTIPLY','Decode XYZ -1':'ADD','Normalize source XYZ':'NORMALIZE','Encode half':'MULTIPLY','Encode RGB +half':'ADD'}
    for name,op in expected.items():assert nodes[name].operation==op and len(nodes[name].inputs[0].links)==1
    for name,value in [('Decode RGB x2',(2,2,2)),('Decode XYZ -1',(-1,-1,-1)),('Encode half',(.5,.5,.5)),('Encode RGB +half',(.5,.5,.5))]:
        assert tuple(nodes[name].inputs[1].default_value)==value
    bsdf=nodes['Stone | no metallic coating']
    assert bsdf.inputs['Base Color'].links[0].from_node.name.startswith('Diffuse | original packed16')
    assert bsdf.inputs['Roughness'].links[0].from_node.name.startswith('Rough | original packed16')
    assert nodes['OpenGL +Y tangent normal | no green flip'].space=='TANGENT'
    assert nodes['OpenGL +Y tangent normal | no green flip'].inputs['Strength'].default_value==1
    out=nodes['Material output | surface only'];assert not out.inputs['Displacement'].links
    assert nodes['OPTIONAL height | OFF, amplitude uncalibrated'].inputs['Scale'].default_value==0
    assert not any(n.type=='AMBIENT_OCCLUSION' for n in nodes)
    report['shader']={'normal_decode':'RGB*2-1 -> normalize -> XYZ*.5+.5 -> tangent Normal Map','green_flip':False,'normal_strength':1,'ao_node':False,'active_displacement':False,'diffuse_to_basecolor':True,'roughness_to_roughness':True}
    report['shader_links']=[{'from':l.from_node.name,'output':l.from_socket.name,'to':l.to_node.name,'input':l.to_socket.name} for l in mat.node_tree.links]
    REPORT.write_text(json.dumps(report,indent=2)+'\n');views('reopened');print('SCANNED_STONE_VERIFIED '+str(REPORT))

if '--verify' in sys.argv:verify()
else:build();views('source');print('SCANNED_STONE_SOURCE '+str(SOURCE))
