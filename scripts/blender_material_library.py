"""Editable, periodic PBR surface library and portable CPU Cycles bakes.

Build: blender -b --factory-startup --disable-autoexec -t 16 --python-exit-code 1
       --python scripts/blender_material_library.py
Verify: append -- --verify to reopen source and reimport only the saved PNG maps.
The maps contain surface material, never directional illumination or brick joints.
"""
from pathlib import Path
import bpy, math, json, hashlib, sys, struct
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets' / 'blender'
REVIEW = OUT / 'review' / 'materials'
TEX = ROOT / 'public' / 'materials'
SIZE = 1024
TILE_METERS = 2.0
ASSETS = ('city-stone', 'vault-basalt', 'aged-timber')
for directory in (OUT, REVIEW, TEX):
    directory.mkdir(parents=True, exist_ok=True)


def lin(c):
    c = c / 255
    return c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4


def rgba(rgb):
    return tuple(lin(c) for c in rgb) + (1,)


class Graph:
    def __init__(self, name):
        self.mat = bpy.data.materials.new(name)
        self.mat.use_nodes = True
        self.mat.use_fake_user = True
        self.mat.asset_mark()
        self.nodes = self.mat.node_tree.nodes
        self.nodes.clear()
        self.links = self.mat.node_tree.links
        self.output = self.node('ShaderNodeOutputMaterial', 'Material output')
        self.bsdf = self.node('ShaderNodeBsdfPrincipled', 'Nonmetal surface response')
        self.bsdf.inputs['Metallic'].default_value = 0
        self.bsdf.inputs['IOR'].default_value = 1.48
        self.links.new(self.bsdf.outputs['BSDF'], self.output.inputs['Surface'])
        self.uv = self.node('ShaderNodeTexCoord', 'Measured 2m square UV tile').outputs['UV']
        sep = self.node('ShaderNodeSeparateXYZ', 'Surface U / V')
        self.links.new(self.uv, sep.inputs[0])
        self.u, self.v = sep.outputs['X'], sep.outputs['Y']
        self.ux = self.calc('MULTIPLY', self.u, math.tau)
        self.vy = self.calc('MULTIPLY', self.v, math.tau)
        self.cosu, self.sinu = self.calc('COSINE', self.ux), self.calc('SINE', self.ux)
        self.cosv, self.sinv = self.calc('COSINE', self.vy), self.calc('SINE', self.vy)

    def node(self, kind, label):
        node = self.nodes.new(kind)
        node.name = label
        node.label = label
        return node

    def connect(self, source, socket):
        if isinstance(source, (int, float)):
            socket.default_value = source
        elif isinstance(source, tuple):
            socket.default_value = source
        else:
            self.links.new(source, socket)

    def calc(self, op, a, b=0, name=None):
        node = self.node('ShaderNodeMath', name or op.lower())
        node.operation = op
        self.connect(a, node.inputs[0])
        self.connect(b, node.inputs[1])
        return node.outputs[0]

    def periodic(self, name, ru, rv, detail=2, rough=.58, phase=0):
        """4D torus embedding makes both value and derivatives wrap smoothly."""
        vec = self.node('ShaderNodeCombineXYZ', name + ' periodic torus')
        for source, radius, socket in ((self.cosu, ru, 0), (self.sinu, ru, 1), (self.cosv, rv, 2)):
            self.links.new(self.calc('MULTIPLY', source, radius), vec.inputs[socket])
        noise = self.node('ShaderNodeTexNoise', name)
        noise.noise_dimensions = '4D'
        self.links.new(vec.outputs[0], noise.inputs['Vector'])
        self.links.new(self.calc('ADD', self.calc('MULTIPLY', self.sinv, rv), phase), noise.inputs['W'])
        noise.inputs['Scale'].default_value = 1
        noise.inputs['Detail'].default_value = detail
        noise.inputs['Roughness'].default_value = rough
        return noise.outputs['Fac']

    def cleavage(self, name, ru, rv, phase):
        """Angular cellular fracture boundaries, periodically embedded in 4D."""
        vec = self.node('ShaderNodeCombineXYZ', name + ' periodic torus')
        for source, radius, socket in ((self.cosu, ru, 0), (self.sinu, ru, 1), (self.cosv, rv, 2)):
            self.links.new(self.calc('MULTIPLY', source, radius), vec.inputs[socket])
        node = self.node('ShaderNodeTexVoronoi', name)
        node.voronoi_dimensions = '4D'
        node.feature = 'DISTANCE_TO_EDGE'
        self.links.new(vec.outputs[0], node.inputs['Vector'])
        self.links.new(self.calc('ADD', self.calc('MULTIPLY', self.sinv, rv), phase), node.inputs['W'])
        node.inputs['Scale'].default_value = 1
        return node.outputs['Distance']

    def ramp(self, fac, stops, name):
        node = self.node('ShaderNodeValToRGB', name)
        cr = node.color_ramp
        cr.interpolation = 'EASE'
        while len(cr.elements) > 2:
            cr.elements.remove(cr.elements[-1])
        for i, (pos, color) in enumerate(stops):
            el = cr.elements[i] if i < 2 else cr.elements.new(pos)
            el.position = pos
            el.color = (color, color, color, 1) if isinstance(color, (float, int)) else color
        self.links.new(fac, node.inputs['Fac'])
        return node.outputs['Color']

    def mix(self, a, b, fac, name):
        node = self.node('ShaderNodeMixRGB', name)
        node.blend_type = 'MIX'
        self.connect(fac, node.inputs[0])
        self.connect(a, node.inputs[1])
        self.connect(b, node.inputs[2])
        return node.outputs[0]

    def finish(self, color, roughness, height):
        bump = self.node('ShaderNodeBump', 'Physical millimeter surface relief')
        bump.inputs['Strength'].default_value = 1
        bump.inputs['Distance'].default_value = 1
        self.links.new(height, bump.inputs['Height'])
        self.links.new(bump.outputs['Normal'], self.bsdf.inputs['Normal'])
        self.links.new(color, self.bsdf.inputs['Base Color'])
        self.links.new(roughness, self.bsdf.inputs['Roughness'])
        self.color, self.roughness, self.height = color, roughness, height
        self.mat['tile_meters'] = TILE_METERS
        self.mat['color_role'] = 'Intrinsic surface color; no light, AO, edge or waterline baked in'
        self.mat['normal_convention'] = 'Tangent-space OpenGL +Y'
        # Layout stays editable and grouped by conceptual function through names.
        for i, n in enumerate(self.nodes):
            n.location = ((i % 8) * 230, -(i // 8) * 240)
        self.output.location = (2050, 160)
        self.bsdf.location = (1800, 160)
        return self


def make_procedural(slug):
    g = Graph('SOURCE | ' + slug)
    macro = g.periodic('Broad geological variation' if slug != 'aged-timber' else 'Weathered long fiber zones', .75, .95, 2, phase=2.8)
    if slug == 'city-stone':
        grain = g.periodic('Fine dressed mineral aggregate', 26, 28, 2, phase=1.7)
        pits = g.periodic('Sparse surface spall field', 7, 8.5, 2, phase=8.1)
        broken = g.ramp(pits, [(0, 1), (.27, .9), (.37, 0), (1, 0)], 'Sparse shallow spalls')
        fleck = g.ramp(grain, [(0, 0), (.63, 0), (.78, .24), (1, .38)], 'Quiet pale aggregate')
        color = g.ramp(macro, [(.22, rgba((81, 94, 99))), (.5, rgba((105, 118, 124))), (.79, rgba((133, 143, 141)))], 'Blue gray dressed stone body')
        color = g.mix(color, rgba((143, 139, 121)), g.calc('MULTIPLY', broken, .22), 'Exposed warm mineral in sparse wear')
        color = g.mix(color, rgba((160, 167, 159)), fleck, 'Scattered pale grain')
        rough = g.calc('SUBTRACT', g.calc('ADD', .77, g.calc('MULTIPLY', macro, .13)), g.calc('MULTIPLY', grain, .06), 'Matte dressed roughness')
        height = g.calc('ADD', g.calc('MULTIPLY', grain, .00065), g.calc('MULTIPLY', broken, -.0018), 'Submillimeter aggregate with sparse 1.8mm pits')
        height = g.calc('ADD', height, g.calc('MULTIPLY', macro, .0013))
        cleft = g.cleavage('Interrupted geological hairlines', 1.9, 2.6, 3.7)
        fissure = g.ramp(cleft, [(0, 1), (.010, .75), (.042, 0), (1, 0)], 'Shallow fracture profile')
        mask = g.ramp(pits, [(0, 0), (.53, 0), (.66, 1), (1, 1)], 'Sparse fracture survival')
        height = g.calc('SUBTRACT', height, g.calc('MULTIPLY', g.calc('MULTIPLY', fissure, mask), .0023), 'Sparse 2.3mm hairlines, no albedo shadow')
    elif slug == 'vault-basalt':
        grain = g.periodic('Fine basalt aggregate', 30, 33, 2.2, phase=4.1)
        fracture = g.periodic('Broad weathered cleavage', 4.2, 6.8, 2, phase=9.6)
        pore = g.periodic('Sparse basalt vesicles', 15, 17, 1.4, phase=1.3)
        cavities = g.ramp(pore, [(0, 1), (.26, .72), (.35, 0), (1, 0)], 'Sparse vesicle hollows')
        mineral = g.ramp(fracture, [(0, 0), (.69, 0), (.77, .44), (1, .64)], 'Limited iron bearing residue')
        color = g.ramp(macro, [(0, rgba((43, 47, 51))), (.5, rgba((60, 66, 69))), (1, rgba((75, 78, 79)))], 'Charcoal basalt body')
        color = g.mix(color, rgba((98, 81, 56)), mineral, 'Warm trace mineral without illumination')
        rough = g.calc('ADD', .65, g.calc('MULTIPLY', fracture, .24), 'Varied dry basalt roughness')
        height = g.calc('ADD', g.calc('MULTIPLY', fracture, .0055), g.calc('MULTIPLY', grain, .0009), 'Cleavage plus mineral grain')
        height = g.calc('ADD', height, g.calc('MULTIPLY', cavities, -.0025), 'Sparse deeper surface vesicles')
        cleft = g.cleavage('Angular broken basalt cleavage boundaries', 3.8, 4.7, 8.2)
        fissure = g.ramp(cleft, [(0, 1), (.012, .78), (.060, 0), (1, 0)], 'Shallow broken angular fracture profile')
        mask = g.ramp(fracture, [(0, 0), (.58, 0), (.73, 1), (1, 1)], 'Sparse fracture exposure, quiet remaining planes')
        height = g.calc('SUBTRACT', height, g.calc('MULTIPLY', g.calc('MULTIPLY', fissure, mask), .0028), 'Sparse shallow angular surface break, no dark painted cracks')
    else:
        fiber = g.periodic('Long wavy growth fibers', .24, 13, 2, phase=3.2)
        fine = g.periodic('Fine longitudinal pores', .52, 45, 1.6, phase=8.6)
        broken = g.periodic('Intermittent fiber splits', 2.4, 20, 1.2, phase=5.3)
        dark = g.ramp(fiber, [(0, 1), (.30, .78), (.46, .12), (.67, 0), (1, 0)], 'Dark earlywood fibers')
        split = g.ramp(broken, [(0, 1), (.23, .75), (.31, 0), (1, 0)], 'Sparse drying checks')
        color = g.ramp(macro, [(0, rgba((79, 64, 47))), (.5, rgba((108, 86, 61))), (1, rgba((128, 106, 78)))], 'Aged warm wood body')
        color = g.mix(color, rgba((53, 41, 30)), g.calc('MULTIPLY', dark, .52), 'Growth color follows fibers')
        color = g.mix(color, rgba((49, 39, 31)), g.calc('MULTIPLY', split, .33), 'Split fiber intrinsic color')
        rough = g.calc('ADD', .69, g.calc('MULTIPLY', fiber, .19), 'Dry matte timber response')
        height = g.calc('ADD', g.calc('MULTIPLY', fiber, .0021), g.calc('MULTIPLY', fine, .00045), 'Longitudinal growth relief')
        height = g.calc('ADD', height, g.calc('MULTIPLY', split, -.002), 'Sparse checking depth')
    return g.finish(color, rough, height)


def plane(name, size, mat, uv_repeat=1):
    mesh = bpy.data.meshes.new(name)
    s = size / 2
    mesh.from_pydata([(-s, -s, 0), (s, -s, 0), (s, s, 0), (-s, s, 0)], [], [(0, 1, 2), (0, 2, 3)])
    uv = mesh.uv_layers.new(name='Measured UV')
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            uv.data[li].uv = ((co.x / size + .5) * uv_repeat, (co.y / size + .5) * uv_repeat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(mat)
    return obj


def activate(obj):
    for o in bpy.context.scene.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def configure(scene):
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.render.threads_mode = 'FIXED'
    scene.render.threads = 16
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1056
    scene.render.resolution_y = 816
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.render.image_settings.color_depth = '8'
    scene.view_settings.view_transform = 'AgX'
    scene.render.film_transparent = False


def bake(g, obj, slug):
    scene = bpy.context.scene
    scene.render.bake.use_selected_to_active = False
    scene.render.bake.margin = 0
    scene.render.bake.normal_space = 'TANGENT'
    scene.render.bake.normal_r = 'POS_X'
    scene.render.bake.normal_g = 'POS_Y'
    scene.render.bake.normal_b = 'POS_Z'
    scene.cycles.samples = 1
    activate(obj)
    out = TEX / slug
    out.mkdir(parents=True, exist_ok=True)
    target = g.node('ShaderNodeTexImage', 'BAKE TARGET | portable PNG')
    emission = g.node('ShaderNodeEmission', 'Unlit color and scalar bake adapter')
    for role in ('basecolor', 'normal', 'roughness'):
        image = bpy.data.images.new(slug + ' ' + role, width=SIZE, height=SIZE, alpha=False, float_buffer=False)
        image.colorspace_settings.name = 'sRGB' if role == 'basecolor' else 'Non-Color'
        target.image = image
        for node in g.nodes:
            node.select = False
        target.select = True
        g.nodes.active = target
        if role == 'normal':
            g.links.new(g.bsdf.outputs['BSDF'], g.output.inputs['Surface'])
            bpy.ops.object.bake(type='NORMAL')
        else:
            g.links.new(g.color if role == 'basecolor' else g.roughness, emission.inputs['Color'])
            g.links.new(emission.outputs[0], g.output.inputs['Surface'])
            bpy.ops.object.bake(type='EMIT')
        image.filepath_raw = str(out / (role + '.png'))
        image.file_format = 'PNG'
        image.save()
        image.pack()
        print('MATERIAL_MAP_SAVED', slug, role, flush=True)
    g.links.new(g.bsdf.outputs['BSDF'], g.output.inputs['Surface'])
    g.nodes.remove(emission)
    g.nodes.remove(target)
    scene.cycles.samples = 24


def reimport_material(slug):
    g = Graph('PNG REIMPORT | ' + slug)
    nodes = {}
    for role in ('basecolor', 'normal', 'roughness'):
        image = bpy.data.images.load(str(TEX / slug / (role + '.png')), check_existing=False)
        image.colorspace_settings.name = 'sRGB' if role == 'basecolor' else 'Non-Color'
        image.pack()
        node = g.node('ShaderNodeTexImage', role + ' | ' + image.colorspace_settings.name)
        node.image = image
        node.extension = 'REPEAT'
        node.interpolation = 'Linear'
        g.links.new(g.uv, node.inputs['Vector'])
        nodes[role] = node
    g.links.new(nodes['basecolor'].outputs['Color'], g.bsdf.inputs['Base Color'])
    g.links.new(nodes['roughness'].outputs['Color'], g.bsdf.inputs['Roughness'])
    normal = g.node('ShaderNodeNormalMap', 'Tangent +Y normal, strength 1')
    normal.space = 'TANGENT'
    normal.inputs['Strength'].default_value = 1
    g.links.new(nodes['normal'].outputs['Color'], normal.inputs['Color'])
    g.links.new(normal.outputs[0], g.bsdf.inputs['Normal'])
    g.mat['tile_meters'] = TILE_METERS
    g.mat['normal_convention'] = 'Tangent-space OpenGL +Y'
    return g.mat


def stage(scene):
    world = bpy.data.worlds.new('Neutral studio world')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (.16, .16, .16, 1)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = .5
    scene.world = world
    cam = bpy.data.objects.new('Material inspection camera', bpy.data.cameras.new('Material inspection camera'))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 3.15
    cam.location = (1.6, -2.25, 4.2)
    cam.rotation_euler = (Vector((0, 0, -.03)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    lights = []
    for name, loc, power, size in (('Broad neutral key', (-2.8, -1.7, 4.4), 450, 3.0), ('Grazing softbox', (2.6, .8, 1.0), 140, 1.8)):
        data = bpy.data.lights.new(name, 'AREA')
        data.energy = power
        data.shape = 'DISK'
        data.size = size
        obj = bpy.data.objects.new(name, data)
        scene.collection.objects.link(obj)
        obj.location = loc
        obj.rotation_euler = (-obj.location).to_track_quat('-Z', 'Y').to_euler()
        lights.append(obj)
    return cam, lights


def slab(obj):
    solid = obj.modifiers.new('Actual 10cm thick inspection slab', 'SOLIDIFY')
    solid.thickness = .10
    bevel = obj.modifiers.new('Actual 8mm inspection edge radius', 'BEVEL')
    bevel.width = .008
    bevel.segments = 3
    bevel.affect = 'EDGES'


def render(scene, filename):
    if '--stats-only' in sys.argv:
        return
    scene.render.filepath = str(REVIEW / (filename + '.png'))
    bpy.ops.render.render(write_still=True)
    print('MATERIAL_REVIEW_SAVED', filename, flush=True)


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.name = 'Material library | editable source'
    configure(scene)
    graphs = {}
    objects = []
    for slug in ASSETS:
        g = make_procedural(slug)
        graphs[slug] = g
        obj = plane('SOURCE SLAB | ' + slug, TILE_METERS, g.mat)
        bake(g, obj, slug)
        slab(obj)
        obj.hide_render = True
        objects.append(obj)
    cam, lights = stage(scene)
    for slug, obj in zip(ASSETS, objects):
        obj.hide_render = False
        render(scene, slug + '-source')
        obj.hide_render = True
        reimport_material(slug)
    for i, obj in enumerate(objects):
        obj.location.x = (i - 1) * 2.25
        obj.hide_render = False
    cam.location = (3.6, -6.0, 8.5)
    cam.rotation_euler = (-cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = 8
    for image in bpy.data.images:
        if image.filepath:
            image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'material-library.blend'))
    manifest = {
        'blender_version': bpy.app.version_string,
        'source': 'assets/blender/material-library.blend',
        'resolution': [SIZE, SIZE], 'png_channel_bits': 8, 'tile_meters': [TILE_METERS, TILE_METERS],
        'provisional_game_mapping': {'meters_per_game_unit': .4, 'game_units_per_tile': 5, 'status': 'assumption; runtime may set mapping'},
        'runtime_metallic': 0,
        'texture_budget': {'portable_maps': 9, 'rgb8_uncompressed_bytes_without_mips': SIZE * SIZE * 3 * 9, 'rgba8_gpu_bytes_with_full_mips_approx': SIZE * SIZE * 4 * 9 * 4 // 3},
        'texture_roles': {'basecolor': 'sRGB intrinsic albedo, no directional light or AO', 'normal': 'Non-Color tangent-space +Y OpenGL; RGB=XYZ', 'roughness': 'Non-Color scalar in all RGB channels'},
        'normal_bake': {'space': 'TANGENT', 'swizzle': ['POS_X', 'POS_Y', 'POS_Z'], 'margin': 0, 'target': 'triangulated 2m UV square with +Z normal and +U/+V tangents', 'source': 'editable periodic Bump shader', 'strength': 1},
        'periodicity': 'UV mapped to 4D torus; smooth periodic value and derivative in U and V',
        'resource_limit': {'engine': 'Cycles CPU', 'threads': 16, 'logical_cpus': 24, 'gpu': 'not used', 'review_samples': 24},
        'materials': {},
        'scope_limits': ['Texture surface only; edge chips, masonry joints and deep fractures remain geometry responsibilities.', 'No local contact dirt, waterline, ambient occlusion, emissive ore or directional lighting baked into albedo.', 'No runtime shader integration included; target-camera response remains unverified.'],
        'overall_state': 'work_in_progress',
    }
    refs = {'city-stone': ['drowned-quarter/20-carved-cornice.png', 'drowned-quarter/27-underwater-material.png'], 'vault-basalt': ['vault-mouth/27-raw-rock-seam.png'], 'aged-timber': ['vault-mouth/11-hunters-rope.png']}
    for slug in ASSETS:
        manifest['materials'][slug] = {'source_material': graphs[slug].mat.name, 'references': ['public/art/references/' + r for r in refs[slug]], 'grain_axis': '+U longitudinal' if slug == 'aged-timber' else 'isotropic with broad variation', 'maps': {role: 'public/materials/' + slug + '/' + role + '.png' for role in ('basecolor', 'normal', 'roughness')}}
    (OUT / 'material-library-manifest.json').write_text(json.dumps(manifest, indent=2))
    print('MATERIAL_LIBRARY_SOURCE_COMPLETE', flush=True)


def verify():
    source_path = OUT / 'material-library.blend'
    bpy.ops.wm.open_mainfile(filepath=str(source_path))
    assert all(bpy.data.materials.get('SOURCE | ' + slug) for slug in ASSETS)
    for slug in ASSETS:
        portable = bpy.data.materials.get('PNG REIMPORT | ' + slug)
        assert portable is not None and portable.use_fake_user
        portable_images = [n.image for n in portable.node_tree.nodes if n.type == 'TEX_IMAGE']
        assert len(portable_images) == 3 and all(i is not None and i.packed_file for i in portable_images)
    saved_count = sum(len(bpy.data.materials['SOURCE | ' + s].node_tree.nodes) for s in ASSETS)
    # A clean scene and fresh PNG datablocks ensure the review uses portable maps.
    scene = bpy.data.scenes.new('Verification | clean PNG shader reimport')
    bpy.context.window.scene = scene
    configure(scene)
    cam, lights = stage(scene)
    report = {'source_reopened': True, 'source_sha256': hashlib.sha256(source_path.read_bytes()).hexdigest(), 'editable_source_nodes': saved_count, 'materials': {}, 'scope': 'Portable PNG/source/tiling evidence. Does not certify AAA or the game shader.'}
    for slug in ASSETS:
        mat = reimport_material(slug)
        obj = plane('REIMPORT SLAB | ' + slug, TILE_METERS, mat)
        slab(obj)
        render(scene, slug + '-reimport')
        # Opposing neutral key makes bumps/indentations change highlights while
        # intrinsic color remains attached. This checks normal orientation.
        for light in lights:
            light.location.x *= -1
            light.rotation_euler = (-light.location).to_track_quat('-Z', 'Y').to_euler()
        render(scene, slug + '-opposed-light')
        for light in lights:
            light.location.x *= -1
            light.rotation_euler = (-light.location).to_track_quat('-Z', 'Y').to_euler()
        obj.hide_render = True
        tiled = plane('REIMPORT 3x3 repeat | ' + slug, TILE_METERS * 3, mat, 3)
        cam.location = (0, 0, 8)
        cam.rotation_euler = (0, 0, 0)
        cam.data.ortho_scale = 8.2
        render(scene, slug + '-tile-3x3')
        # True unlit 3x3 albedo evidence excludes stage-light gradients.
        nodes = mat.node_tree.nodes
        links = mat.node_tree.links
        output = next(n for n in nodes if n.type == 'OUTPUT_MATERIAL')
        bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
        base = next(n for n in nodes if n.type == 'TEX_IMAGE' and n.name.startswith('basecolor'))
        emission = nodes.new('ShaderNodeEmission')
        links.new(base.outputs['Color'], emission.inputs['Color'])
        links.new(emission.outputs[0], output.inputs['Surface'])
        scene.view_settings.view_transform = 'Standard'
        render(scene, slug + '-albedo-3x3')
        scene.view_settings.view_transform = 'AgX'
        links.new(bsdf.outputs[0], output.inputs['Surface'])
        nodes.remove(emission)
        tiled.hide_render = True
        cam.location = (1.6, -2.25, 4.2)
        cam.rotation_euler = (Vector((0, 0, -.03)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
        cam.data.ortho_scale = 3.15
        maps = {}
        for role in ('basecolor', 'normal', 'roughness'):
            path = TEX / slug / (role + '.png')
            image = bpy.data.images.load(str(path), check_existing=False)
            image.colorspace_settings.name = 'sRGB' if role == 'basecolor' else 'Non-Color'
            pixels = np.array(image.pixels[:], dtype=np.float32).reshape(SIZE, SIZE, 4)[..., :3]
            assert np.isfinite(pixels).all()
            edge = np.concatenate((np.abs(pixels[:, 0] - pixels[:, -1]).ravel(), np.abs(pixels[0] - pixels[-1]).ravel()))
            near = np.concatenate((np.abs(pixels[:, 1] - pixels[:, 0]).ravel(), np.abs(pixels[1] - pixels[0]).ravel()))
            data = {'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'bytes': path.stat().st_size, 'colorspace': image.colorspace_settings.name, 'size': list(image.size), 'linear_min': float(pixels.min()), 'linear_max': float(pixels.max()), 'wrap_edge_mean_delta': float(edge.mean()), 'near_edge_mean_delta': float(near.mean())}
            png = path.read_bytes()
            width, height, bits, color_type = struct.unpack('>IIBB', png[16:26])
            data.update({'png_channel_bits': bits, 'png_color_type': color_type, 'channels': 3 if color_type == 2 else 4 if color_type == 6 else 1})
            assert (width, height, bits, color_type) == (SIZE, SIZE, 8, 2)
            if role == 'basecolor':
                # Blender's byte-backed image.pixels exposes encoded channel
                # values; the shader performs the sRGB decode at sampling time.
                encoded = pixels
                decoded = np.where(encoded <= .04045, encoded / 12.92, np.power((encoded + .055) / 1.055, 2.4))
                data.update({'linear_min': float(decoded.min()), 'linear_max': float(decoded.max()), 'mean_linear_rgb': [float(v) for v in decoded.mean(axis=(0, 1), dtype=np.float64)], 'mean_srgb_rgb_0_255': [float(v) for v in (encoded.mean(axis=(0, 1), dtype=np.float64) * 255)], 'runtime_note': 'Absolute intrinsic albedo. Do not multiply by another dark base color without family mean normalization.'})
            if role == 'normal':
                normal = pixels * 2 - 1
                lengths = np.linalg.norm(normal, axis=-1)
                data.update({'mean_vector_length': float(lengths.mean()), 'max_vector_length_error': float(np.abs(lengths - 1).max()), 'minimum_z': float(normal[..., 2].min()), 'channel_convention': '+Y OpenGL tangent'})
                assert normal[..., 2].min() > .1 and np.abs(lengths - 1).max() < .025
            if role == 'roughness':
                assert np.abs(pixels[..., 0] - pixels[..., 1]).max() < .005
            maps[role] = data
        report['materials'][slug] = maps
    report['technical_status'] = 'pass'
    (OUT / 'material-library-verification.json').write_text(json.dumps(report, indent=2))
    print('MATERIAL_LIBRARY_VERIFIED ' + json.dumps(report), flush=True)


def repair_package():
    """Persist unused reusable material assets without rebaking portable maps."""
    bpy.ops.wm.open_mainfile(filepath=str(OUT / 'material-library.blend'))
    for slug in ASSETS:
        source = bpy.data.materials['SOURCE | ' + slug]
        source.use_fake_user = True
        source.asset_mark()
        old = bpy.data.materials.get('PNG REIMPORT | ' + slug)
        if old:
            bpy.data.materials.remove(old, do_unlink=True)
        reimport_material(slug)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'material-library.blend'))


if __name__ == '__main__':
    if '--verify' in sys.argv:
        verify()
    elif '--repair-package' in sys.argv:
        repair_package()
    else:
        build()
