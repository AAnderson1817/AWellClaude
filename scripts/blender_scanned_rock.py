"""One bounded Poly Haven rock fit in the existing terrain asset pipeline.

The vendor scan is immutable. The fitted front retains its triangle density;
only the upper contact transition is deformed. All nonpatch C data is preserved.
"""
import bpy, bmesh, math, json, re, sys, hashlib, importlib.util, copy
from pathlib import Path
from mathutils import Vector, Quaternion

SOURCE_SHA256='e6de5faaad702bc3be9930049851f91d444edb230adaba93b3272a753f1b8e92'
ALIGNMENT=(.9444789886474609,.3275698721408844,0,.025640230625867844)

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def inside_patch(points):
    return all(-1e-6<=x<=5.000001 and 12.999999<=y<=16.000001 for x,y,z in points)

def protected(points):
    return all(abs(y-16)<1e-6 for x,y,z in points) or all(z< -2.7 for x,y,z in points)

def replaceable(points):return inside_patch(points) and not protected(points)

def make_mesh(name,verts,faces,material,collection):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update();me.materials.append(material)
    ob=bpy.data.objects.new(name,me);collection.objects.link(ob);return ob

def fit_scan(root,scene,collection,material):
    vendor=root/'assets/vendor/polyhaven/rock-face-01/rock_face_01_1k.blend'
    assert digest(vendor)==SOURCE_SHA256,'Vendor source has changed'
    source=bpy.data.collections.new('SOURCE | Poly Haven Rock Face01 and undeformed crop')
    scene.collection.children.link(source);source.hide_render=True;source.hide_viewport=True
    with bpy.data.libraries.load(str(vendor),link=False) as (available,loaded):
        assert 'rock_face_01' in available.objects
        loaded.objects=['rock_face_01']
    original=loaded.objects[0];source.objects.link(original)
    original.name='SOURCE | untouched Rock Face01 by Dario Barresi'
    # Resolve and pack source dependencies. The on-disk vendor bytes remain original.
    for mat in original.data.materials:
        if not mat or not mat.use_nodes:continue
        for node in mat.node_tree.nodes:
            if node.type=='TEX_IMAGE' and node.image:
                path=vendor.parent/'textures'/Path(node.image.filepath.replace('\\','/')).name
                if path.is_file():node.image.filepath=str(path);node.image.pack()
    q=Quaternion(ALIGNMENT);me=original.data.copy()
    ob=bpy.data.objects.new('Scanned rock | above Vault door front',me);collection.objects.link(ob)
    for vertex in me.vertices:
        p=q@vertex.co;vertex.co=(p.x+2.5,p.y+.59,p.z+10.6)
    bm=bmesh.new();bm.from_mesh(me)
    for co,no in [((0,0,0),(-1,0,0)),((5,0,0),(1,0,0)),((0,0,13),(0,0,-1)),((0,0,15.87),(0,0,1))]:
        bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),plane_co=co,plane_no=no,dist=1e-6,clear_outer=True,clear_inner=False)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-7)
    bmesh.ops.triangulate(bm,faces=list(bm.faces))
    tiny=[f for f in bm.faces if f.calc_area()<1e-10]
    if tiny:bmesh.ops.delete(bm,geom=tiny,context='FACES')
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();me.update()
    me.normals_split_custom_set([(0,0,0)]*len(me.loops))
    undeformed=ob.copy();undeformed.data=me.copy();source.objects.link(undeformed)
    undeformed.name='SOURCE | rigid aligned crop before local contact fitting'
    me.calc_loop_triangles();front_triangles=len(me.loop_triangles);shifts=[]
    for vertex in me.vertices:
        t=max(0,min(1,(vertex.co.z-15.15)/.72));s=t*t*(3-2*t);before=vertex.co.y
        vertex.co.y=.035+(1-.94*s)*(before-.035)
        if vertex.co.z>15.85:vertex.co.y=max(vertex.co.y,.0842)
        if s:shifts.append(abs(vertex.co.y-before))
    me.materials.clear();me.materials.append(material)
    bm=bmesh.new();bm.from_mesh(me);bm.verts.ensure_lookup_table();rv=[];rf=[];closure_counts={}
    for label,axis,value,expected in [('left',0,0,Vector((-1,0,0))),('right',0,5,Vector((1,0,0))),('bottom',2,13,Vector((0,0,-1)))]:
        edges=[e for e in bm.edges if e.is_boundary and all(abs(v.co[axis]-value)<1e-5 for v in e.verts)]
        adj={}
        for edge in edges:
            a,b=edge.verts;adj.setdefault(a.index,[]).append(b.index);adj.setdefault(b.index,[]).append(a.index)
        ends=[i for i,neighbors in adj.items() if len(neighbors)==1]
        assert len(ends)==2,(label,'one simple boundary chain required',len(ends))
        chain=[];cur=ends[0];last=None
        while True:
            chain.append(cur);nxt=[i for i in adj[cur] if i!=last]
            if not nxt:break
            last,cur=cur,nxt[0]
        assert len(chain)==len(adj),(label,'disconnected boundary')
        points=[bm.verts[i].co.copy() for i in chain]
        points.extend([Vector((points[-1].x,2.96,points[-1].z)),Vector((points[0].x,2.96,points[0].z))])
        normal=sum((p.cross(points[(i+1)%len(points)]) for i,p in enumerate(points)),Vector((0,0,0)))
        if normal.dot(expected)<0:points.reverse()
        start=len(rv);rv.extend(tuple(p) for p in points);rf.append(tuple(range(start,start+len(points))));closure_counts[label]=len(points)
    bm.free()
    returns=make_mesh('Scanned rock | concave cut returns',rv,rf,material,collection)
    back=make_mesh('Scanned rock | rear closure',[(0,2.96,13),(5,2.96,13),(5,2.96,15.87),(0,2.96,15.87)],[(3,2,1,0)],material,collection)
    contact=make_mesh('Scanned rock | exact upper contact arris',[(0,.0832,15.87),(5,.0832,15.87),(5,2.96,15.87),(0,2.96,15.87),(0,0,16),(5,0,16),(5,2.96,16),(0,2.96,16)],[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],material,collection)
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-6)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    if sum(f.normal.y*f.calc_area() for f in bm.faces)>0:
        bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.normal_update()
    bm.to_mesh(me);bm.free();me.update()
    # Explicit area-weighted corner normals within a 50-degree hemisphere.
    # This replaces the scan's stale imported normals after rigid/local fitting.
    incident={};face_data={}
    for polygon in me.polygons:
        a,b,c=[me.vertices[i].co for i in polygon.vertices[:3]];cross=(b-a).cross(c-a)
        normal=cross.normalized();weight=cross.length;face_data[polygon.index]=(normal,weight)
        for i in polygon.vertices:incident.setdefault(i,[]).append((normal,weight))
    corner=[None]*len(me.loops);cutoff=math.cos(math.radians(50))
    for polygon in me.polygons:
        n,area=face_data[polygon.index]
        for li in polygon.loop_indices:
            total=Vector((0,0,0))
            for other,weight in incident[me.loops[li].vertex_index]:
                if other.dot(n)>=cutoff:total+=other*weight
            corner[li]=total.normalized()
    me.normals_split_custom_set(corner)
    objects=[ob,returns,back,contact]
    return objects,{'source':'Rock Face01 by Dario Barresi / Poly Haven / CC0','source_url':'https://polyhaven.com/a/rock_face_01','license_url':'https://polyhaven.com/license','vendor_blend_sha256':SOURCE_SHA256,'source_triangles':20174,'cropped_front_triangles':front_triangles,'uniform_scale':1,'rigid_quaternion_wxyz':list(ALIGNMENT),'translation_blender':[2.5,.59,10.6],'deformation':{'world_y_region':[15.15,16],'scan_transition_end':15.87,'maximum_depth_shift':max(shifts),'mean_affected_depth_shift':sum(shifts)/len(shifts),'affected_vertices':len(shifts),'lower_2_15_tiles_unchanged':True},'closure_polygon_vertices':closure_counts,'source_normals':'Recomputed from actual fitted planes; area weighted within50degrees. Open front explicitly faces Blender -Y / game +Z.','assembly':'Scanned front, simple concave side/bottom returns and exact contact wedge seat into retained production rear/top; not a collision mesh or a claimed single welded manifold.','runtime_materials':'Existing basalt substrate2/palette/PBR maps; vendor textures are portable source reference only.'}

def triangles(objects):
    result=[]
    for ob in objects:
        me=ob.data;me.calc_loop_triangles()
        for tri in me.loop_triangles:
            p=[me.vertices[i].co for i in tri.vertices]
            if (p[1]-p[0]).cross(p[2]-p[0]).length<1e-10:continue
            result.append(([(v.x,v.z,-v.y) for v in p],[(me.corner_normals[i].vector.x,me.corner_normals[i].vector.z,-me.corner_normals[i].vector.y) for i in tri.loops]))
    return result

def export_arrays_glb(scene,asset,names,models):
    delivery=bpy.data.collections.new('DELIVERY | scanned rock verification');scene.collection.children.link(delivery);copies=[]
    for index,m in enumerate(asset.meshes):
        # Share equal position/normal corners in the portable mesh. The embedded
        # arrays intentionally retain old storage plus appended triangles; leaving
        # each new corner isolated makes Blender derive zero geometric normals on
        # very acute cap triangles despite valid stored shading normals.
        unique={};positions=[];normals=[];indices=[]
        for old_index in m.indices:
            p=m.positions[old_index];n=m.normals[old_index];key=tuple((*p,*n))
            vertex=unique.get(key)
            if vertex is None:
                vertex=len(positions);unique[key]=vertex;positions.append((p[0],-p[2],p[1]));normals.append((n[0],-n[2],n[1]))
            indices.append(vertex)
        me=bpy.data.meshes.new('Vault final material '+str(index));me.from_pydata(positions,[],[tuple(indices[i:i+3]) for i in range(0,len(indices),3)]);me.update()
        me.materials.append(bpy.data.materials[names[index]])
        for polygon in me.polygons:polygon.use_smooth=True
        me.normals_split_custom_set_from_vertices(normals)
        ob=bpy.data.objects.new('Vault final material '+str(index),me);delivery.objects.link(ob);copies.append(ob)
    bpy.ops.object.select_all(action='DESELECT')
    for ob in copies:ob.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name='vault-terrain'
    bpy.ops.export_scene.gltf(filepath=str(models/'vault-terrain.glb'),use_selection=True,export_format='GLB',export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    bpy.data.objects.remove(joined,do_unlink=True);bpy.data.collections.remove(delivery)

def apply(root,scene,collection,material,header,out,models,review,manifest):
    spec=importlib.util.spec_from_file_location('scan_architecture_audit',root/'tools/check-architecture.py')
    audit=importlib.util.module_from_spec(spec);sys.modules[spec.name]=audit;spec.loader.exec_module(audit)
    before_digest=digest(header);before_text=header.read_text();assets=audit.read_assets(header);baseline=copy.deepcopy(assets)
    original_objects=list(collection.objects)
    patch,metadata=fit_scan(root,scene,collection,material)
    patch_triangles=triangles(patch);asset=assets['FOUNDRY_VAULT_TERRAIN'];removed=0
    for mesh in asset.meshes:
        kept=[]
        for i in range(0,len(mesh.indices),3):
            ids=mesh.indices[i:i+3]
            if replaceable([mesh.positions[j] for j in ids]):removed+=1
            else:kept.extend(ids)
        mesh.indices=kept
    additions=[]
    for points,normals in patch_triangles:
        if protected(points):continue
        points=[tuple(round(v,8) for v in p) for p in points]
        normals=[tuple(v/math.sqrt(sum(c*c for c in n)) for v in n) for n in normals]
        additions.append((points,normals))
    mesh=asset.meshes[0]
    for points,normals in additions:
        i=len(mesh.positions);mesh.positions.extend(points);mesh.normals.extend(normals);mesh.indices.extend((i,i+1,i+2))
    assert len(mesh.positions)<65536
    def fmt(v):
        s=f'{v:.8f}'.rstrip('0').rstrip('.');return (s if '.' in s else s+'.0')+'f'
    text=before_text
    # Retain original vertex/normal strings, other meshes and Drowned block.
    for suffix,values in [('p',[v for points,normals in additions for p in points for v in p]),('n',[v for points,normals in additions for n in normals for v in n])]:
        pattern=r'(static const float vault_terrain_0_'+suffix+r'\[\] = \{\n)(.*?)(\n\};)'
        match=re.search(pattern,text,re.S);assert match
        appended='\n'.join(','.join(fmt(v) for v in values[i:i+12])+',' for i in range(0,len(values),12))
        text=text[:match.start()]+match.group(1)+match.group(2)+'\n'+appended+match.group(3)+text[match.end():]
    for index,(old,new) in enumerate(zip(baseline['FOUNDRY_VAULT_TERRAIN'].meshes,asset.meshes)):
        if old.indices==new.indices:continue
        pattern=r'(static const unsigned short vault_terrain_'+str(index)+r'_i\[\] = \{\n)(.*?)(\n\};)'
        match=re.search(pattern,text,re.S);assert match
        data='\n'.join(','.join(map(str,new.indices[i:i+12]))+',' for i in range(0,len(new.indices),12))
        text=text[:match.start()]+match.group(1)+data+match.group(3)+text[match.end():]
        prefix=f'vault_terrain_{index}_p,vault_terrain_{index}_n,vault_terrain_{index}_i,'
        old_descriptor=prefix+f'{len(old.positions)},{len(old.indices)},'
        assert text.count(old_descriptor)==1
        text=text.replace(old_descriptor,prefix+f'{len(new.positions)},{len(new.indices)},')
    assert before_text.split('static const float drowned_terrain_0_p')[1]==text.split('static const float drowned_terrain_0_p')[1]
    header.write_text(text)
    # Editable source keeps every original object and all outside faces.
    source_removed=0
    for ob in original_objects:
        if ob.type!='MESH':continue
        me=ob.data;bm=bmesh.new();bm.from_mesh(me)
        faces=[f for f in bm.faces if replaceable([(v.co.x,v.co.z,-v.co.y) for v in f.verts])]
        if faces:
            source_removed+=len(faces);bmesh.ops.delete(bm,geom=faces,context='FACES_ONLY');bm.to_mesh(me);me.update()
        bm.free()
        if not ob.data.polygons:bpy.data.objects.remove(ob,do_unlink=True)
    # Original top/rear remain; omit duplicates from the fitted source objects.
    for ob in patch:
        bm=bmesh.new();bm.from_mesh(ob.data)
        faces=[f for f in bm.faces if protected([(v.co.x,v.co.z,-v.co.y) for v in f.verts])]
        if faces:bmesh.ops.delete(bm,geom=faces,context='FACES_ONLY');bm.to_mesh(ob.data);ob.data.update()
        bm.free()
        if not ob.data.polygons:bpy.data.objects.remove(ob,do_unlink=True)
    # Rebuild the Vault GLB from the exact final arrays. Drowned export is untouched.
    names=manifest['assets']['vault-terrain']['material_names']
    export_arrays_glb(scene,asset,names,models)
    metadata.update({'removed_old_triangles':removed,'added_triangles':len(additions),'removed_editable_source_polygons':source_removed,'baseline_header_sha256':before_digest,'final_header_sha256':digest(header),'drowned_header_block_byte_identical':True,'protected_top_rear_retained':True,'lod_reduction':False})
    (review/'scanned-rock-provenance.json').write_text(json.dumps(metadata,indent=2))
    manifest['scanned_rock_patch']=metadata
    manifest['architecture_revision']='v16: one scanned-rock fit above Vault door; other v14 architecture unchanged'
    manifest['assets']['vault-terrain']['triangles']=sum(len(m.indices)//3 for m in asset.meshes)
    manifest['assets']['vault-terrain']['source_objects']=len(collection.objects)
    manifest['total_triangles']=sum(a['triangles'] for a in manifest['assets'].values())
    manifest['delivery_budget']['total_triangles']=110000
    manifest['delivery_budget']['note']='The approved full-resolution patch adds7,928 triangles; runtime performance acceptance remains a separate gate.'
    assert manifest['total_triangles']<110000
    return metadata

if __name__=='__main__' and '--export-current-vault' in sys.argv:
    root=Path(__file__).resolve().parents[1]
    spec=importlib.util.spec_from_file_location('scan_architecture_audit',root/'tools/check-architecture.py')
    audit=importlib.util.module_from_spec(spec);sys.modules[spec.name]=audit;spec.loader.exec_module(audit)
    asset=audit.read_assets(root/'src/generated/terrain_assets.h')['FOUNDRY_VAULT_TERRAIN']
    manifest=json.loads((root/'assets/blender/terrain-manifest.json').read_text())
    scene=bpy.data.scenes['TERRAIN | two existing rooms'];bpy.context.window.scene=scene
    export_arrays_glb(scene,asset,manifest['assets']['vault-terrain']['material_names'],root/'public/models')
