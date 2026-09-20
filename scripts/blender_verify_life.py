"""Clean model reimports and actual runtime representation reviews for small assets."""
import bpy,json,hashlib,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets'/'blender';manifest=json.loads((OUT/'life-manifest.json').read_text())
source=bpy.data.scenes['LIFE | original compact forms'];stage=bpy.data.collections['PRESENTATION | small asset studio']
report={'source_reopened':True,'assets':{}}
for slug,data in manifest['assets'].items():
    sc=bpy.data.scenes.new('Reimport '+slug);bpy.context.window.scene=sc;sc.world=source.world;sc.collection.children.link(stage)
    path=ROOT/'public'/'models'/(slug+'.glb');bpy.ops.import_scene.gltf(filepath=str(path))
    obs=[o for o in sc.objects if o.type=='MESH' and o.name!='Studio ground'];lo=[1e9]*3;hi=[-1e9]*3;tris=0;degen=0;invalid=0;mats=[]
    for o in obs:
        me=o.data;me.calc_loop_triangles();tris+=len(me.loop_triangles)
        for t in me.loop_triangles:
            a,b,c=[me.vertices[i].co for i in t.vertices]
            if (b-a).cross(c-a).length<1e-10:degen+=1
        for v in me.vertices:
            p=o.matrix_world@v.co;p=(p.x,p.z,-p.y)
            for k in range(3):lo[k]=min(lo[k],p[k]);hi[k]=max(hi[k],p[k])
            if v.normal.length<.95 or not all(math.isfinite(vv) for vv in (*v.co,*v.normal)):invalid+=1
        for m in me.materials:
            n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
            assert abs(n.inputs['Alpha'].default_value-1)<1e-6
            assert not any(n.type=='TEX_IMAGE' for n in m.node_tree.nodes)
            mats.append(m.name)
    dims=[hi[k]-lo[k] for k in range(3)]
    assert tris==data['triangles'];assert degen==0,(slug,degen);assert invalid==0,(slug,invalid)
    assert max(abs(a-b) for a,b in zip(dims,data['dimensions']))<.00001
    report['assets'][slug]={'triangles':tris,'degenerate_triangles':degen,'invalid_normals':invalid,'dimensions':dims,'alpha':'opaque','external_textures':[],'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'materials':mats}
    cam=source.camera;sc.camera=cam;center=Vector(((lo[0]+hi[0])/2,-(lo[2]+hi[2])/2,(lo[1]+hi[1])/2))
    cam.location=center+Vector((3,-8,3));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=max(dims)*1.35
    bpy.data.objects['Studio ground'].hide_render=lo[1]<-.02
    sc.render.engine='CYCLES';sc.cycles.samples=20;sc.cycles.use_denoising=True;sc.render.resolution_x=sc.render.resolution_y=768;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG'
    sc.render.filepath=str(OUT/'review'/(slug+'-runtime-reimport.png'));bpy.ops.render.render(write_still=True)
(OUT/'life-verification.json').write_text(json.dumps(report,indent=2))
print('LIFE_ASSETS_VERIFIED '+json.dumps(report))
