"""Matched neutral views of saved editable terrain and the reopened portable GLB."""
import bpy,json,hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/blender/review/terrain-v16'
source_collection=bpy.data.collections['TERRAIN | Vault Mouth']
world=bpy.data.worlds.new('Patch review neutral world');world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.14,.16,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.45
clay=bpy.data.materials.new('REVIEW | neutral clay');clay.use_nodes=True
clay.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.32,.32,.32,1)
clay.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.72
center=Vector((2.5,.8,14.5));files={}
for label in ('source','reimport'):
    scene=bpy.data.scenes.new('Matched scan patch '+label);bpy.context.window.scene=scene
    scene.world=world;scene.view_layers[0].material_override=clay
    if label=='source':
        scene.collection.children.link(source_collection);source_collection.hide_render=False;source_collection.hide_viewport=False
    else:bpy.ops.import_scene.gltf(filepath=str(ROOT/'public/models/vault-terrain.glb'))
    for name,pos,energy,size in [('Key',(-3,-5,20),700,4),('Fill',(7,-2,17),220,5)]:
        d=bpy.data.lights.new(label+name,'AREA');d.energy=energy;d.shape='DISK';d.size=size
        ob=bpy.data.objects.new(label+name,d);scene.collection.objects.link(ob);ob.location=pos;ob.rotation_euler=(center-ob.location).to_track_quat('-Z','Y').to_euler()
    d=bpy.data.cameras.new('Patch Camera');cam=bpy.data.objects.new('Patch Camera',d);scene.collection.objects.link(cam);scene.camera=cam;d.type='ORTHO';d.ortho_scale=6.2
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=20;scene.cycles.use_denoising=True
    scene.render.threads_mode='FIXED';scene.render.threads=8;scene.render.resolution_x=960;scene.render.resolution_y=576;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.view_settings.view_transform='AgX'
    for view,pos in [('front',(2.5,-15,14.5)),('grazing',(8,-12,16.5))]:
        cam.location=pos;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
        path=OUT/(label+'-scanned-patch-'+view+'.png');scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
        files[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
(OUT/'matched-render-manifest.json').write_text(json.dumps({'scope':'Neutral actual saved terrain source and clean production GLB import. Read-only review; no source save.','source_blend_sha256':hashlib.sha256((ROOT/'assets/blender/terrain.blend').read_bytes()).hexdigest(),'glb_sha256':hashlib.sha256((ROOT/'public/models/vault-terrain.glb').read_bytes()).hexdigest(),'files':files},indent=2))
