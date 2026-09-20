#!/usr/bin/env python3
"""Check delivered architecture against gameplay and the actual fixed camera.

Reads the C arrays consumed by raylib, not Blender's scene or generator manifest.
This tests technical alignment and selected affordance risks, never AAA quality.
Python standard library only. See docs/architecture-contract.md for scope/limits.
"""
import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
RW, RH, TILE_PIXELS = 40, 22, 8
FRONT_Z = -.10
REAR_STANDOFF = -.45
LIP_TOLERANCE = .5 / TILE_PIXELS
SUPPORT_FASCIA = .55
PROJECTED_TRIM = .5  # offscreen receiver termination; front contacts stay strict
RASTER = 16  # half an original pixel; contact probes are more closely spaced
FLOAT = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?[fF]?"


def number(value):
    return float(value.rstrip('fF'))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


@dataclass
class Mesh:
    positions: list
    normals: list
    indices: list
    color: list
    metallic: float
    roughness: float
    emission: float


@dataclass
class Asset:
    name: str
    meshes: list
    size: tuple
    substrates: list | None = None


@dataclass
class Camera:
    x: float = 20
    y: float = 11
    z: float = 52
    fov: float = 24.415162

    def project(self, point):
        x, y, z = point
        scale = self.z / (self.z-z)
        return self.x+(x-self.x)*scale, self.y+(y-self.y)*scale


def read_camera(source):
    found = re.search(r"camera\s*=\s*\(Camera3D\)\s*\{\{([^}]+)\},\{([^}]+)\},\{([^}]+)\},\s*("+FLOAT+r"),\s*CAMERA_PERSPECTIVE\s*\}", source)
    if not found:
        raise ValueError("Cannot identify the actual fixed perspective camera; update the projection audit explicitly")
    eye, target, up = [tuple(number(v.strip()) for v in found[i].split(',')) for i in (1, 2, 3)]
    if target != (eye[0], eye[1], 0.) or up != (0.,1.,0.) or eye != (20.,11.,52.):
        raise ValueError(f"Camera changed from the calibrated axis-aligned stage: {eye}, {target}, {up}")
    camera = Camera(*eye, number(found[4]))
    height = 2*camera.z*math.tan(math.radians(camera.fov)/2)
    if abs(height-22.5) > .0001:
        raise ValueError(f"Camera height {height} no longer maps to the original 180-pixel frame")
    dw = int(re.search(r"#define DW (\d+)", source)[1])
    dh = int(re.search(r"#define DH (\d+)", source)[1])
    if abs(dw/dh-16/9) > .000001:
        raise ValueError("Render-target aspect no longer matches the 40 by 22.5 stage")
    return camera


def read_maps(path):
    source = path.read_text(encoding='utf-8')
    match = re.search(r"static const char \*MAPS\[ROOM_COUNT\]\[RH\]\s*=\s*\{(.*?)\n\};",source,re.S)
    if not match:
        raise ValueError("Missing original MAPS declaration")
    rows = re.findall(r'"([^"\n]+)"',match[1])
    if len(rows) != 2*RH or any(len(row) != RW for row in rows):
        raise ValueError("MAPS dimensions changed; do not silently reinterpret collision")
    return [rows[:RH],rows[RH:]]


def read_assets(path):
    source = path.read_text(encoding='utf-8')
    arrays = {}
    for kind,name,body in re.findall(r"static const (float|unsigned short) (\w+)\[\]\s*=\s*\{(.*?)\};",source,re.S):
        values = [v.strip() for v in body.split(',') if v.strip()]
        arrays[name] = [number(v) for v in values] if kind=='float' else [int(v) for v in values]
    groups = {}
    entry = re.compile(r"\{(\w+),(\w+),(\w+),(\d+),(\d+),\{([^}]+)\},("+FLOAT+r"),("+FLOAT+r"),("+FLOAT+r")\}")
    for name,body in re.findall(r"static const FoundryMeshData (\w+)\[\]\s*=\s*\{(.*?)\};",source,re.S):
        body = re.sub(r"\s+","",body)
        meshes=[]
        for m in entry.finditer(body):
            p,n,indices = [arrays[m[i]] for i in (1,2,3)]
            if len(p)!=int(m[4])*3 or len(n)!=len(p) or len(indices)!=int(m[5]):
                raise ValueError(f"{name}: declared array counts do not match delivered storage")
            meshes.append(Mesh(list(zip(p[::3],p[1::3],p[2::3])),list(zip(n[::3],n[1::3],n[2::3])),indices,
                               [int(v) for v in m[6].split(',')],*[number(m[i]) for i in (7,8,9)]))
        if not meshes or ''.join(entry.sub('',body).split(',')):
            raise ValueError(f"{name}: unsupported/unparsed mesh initializer")
        groups[name]=meshes
    assets={}
    substrates={name:[int(v.strip()) for v in body.split(',') if v.strip()]
                for name,body in re.findall(r"static const unsigned char (\w+)_SUBSTRATES\[\]\s*=\s*\{([^}]+)\};",source)}
    for name,group,count,size in re.findall(r"static const FoundryAssetData (\w+)\s*=\s*\{(\w+),(\d+),\{([^}]+)\}\};",source):
        if len(groups[group]) != int(count):
            raise ValueError(f"{name}: mesh count does not match")
        assets[name]=Asset(name,groups[group],tuple(number(v.strip()) for v in size.split(',')),substrates.get(name))
    if not assets:
        raise ValueError(f"No delivered FoundryAssetData found in {path}")
    return assets


def cross(a,b,c):
    u=tuple(b[i]-a[i] for i in range(3));v=tuple(c[i]-a[i] for i in range(3))
    return u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]


def clip_z(polygon,zmin):
    out=[]
    for a,b in zip(polygon,polygon[1:]+polygon[:1]):
        inside_a,inside_b=a[2]>=zmin,b[2]>=zmin
        if inside_a: out.append(a)
        if inside_a != inside_b:
            t=(zmin-a[2])/(b[2]-a[2])
            out.append(tuple(a[i]+t*(b[i]-a[i]) for i in range(3)))
    return out


def projected(triangles,camera,zmin=None,orthographic=False):
    out=[]
    for tri in triangles:
        polygon=clip_z(list(tri),zmin) if zmin is not None else tri
        points=[p[:2] if orthographic else camera.project(p) for p in polygon]
        out.extend((points[0],points[i],points[i+1]) for i in range(1,len(points)-1))
    return out


def rasterize(triangles,scale=RASTER):
    """Scan-convert delivered triangles at pixel centers; no generator masks used."""
    width,height=RW*scale,RH*scale
    mask=bytearray(width*height)
    for triangle in triangles:
        p=[(x*scale,(RH-y)*scale) for x,y in triangle]
        low=max(0,math.ceil(min(v[1] for v in p)-.5))
        high=min(height-1,math.floor(max(v[1] for v in p)-.5))
        for row in range(low,high+1):
            yy=row+.5;hits=[]
            for a,b in zip(p,p[1:]+p[:1]):
                if min(a[1],b[1])<=yy<max(a[1],b[1]):
                    hits.append(a[0]+(yy-a[1])*(b[0]-a[0])/(b[1]-a[1]))
            if len(hits)<2: continue
            start=max(0,math.ceil(min(hits)-.5-1e-9));end=min(width-1,math.floor(max(hits)-.5+1e-9))
            if end>=start:
                at=row*width+start; mask[at:at+end-start+1]=b'\1'*(end-start+1)
    return mask


def expected_mask(rows,role,scale):
    width=RW*scale;mask=bytearray(width*RH*scale)
    for ty,row in enumerate(rows):
        for tx,tile in enumerate(row):
            solid=tile in '#*'
            if not solid and not(role=='supports' and tile=='-'): continue
            for sub in range(scale):
                if not solid and (sub+.5)/scale>SUPPORT_FASCIA: continue
                start=(ty*scale+sub)*width+tx*scale
                mask[start:start+scale]=b'\1'*scale
    return mask


def inflate(mask,scale,radius):
    width,height=RW*scale,RH*scale;out=bytearray(len(mask))
    for y in range(height):
        for x in range(width):
            if not mask[y*width+x]: continue
            for yy in range(max(0,y-radius),min(height,y+radius+1)):
                a=yy*width+max(0,x-radius);b=yy*width+min(width,x+radius+1)
                out[a:b]=b'\1'*(b-a)
    return out


def runs(rows,role):
    out=[]
    for y in range(1,RH):
        active=lambda x: rows[y][x]=='-' if role=='supports' else rows[y][x] in '#*' and rows[y-1][x] not in '#*'
        x=0
        while x<RW:
            if not active(x): x+=1;continue
            start=x
            while x<RW and active(x): x+=1
            out.append((y,start,x))
    return out


class TriangleQuery:
    def __init__(self,triangles):
        self.bins={}
        for tri in triangles:
            a,b,c=tri;area=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
            if abs(area)<1e-12: continue
            for x in range(math.floor(min(p[0] for p in tri)),math.floor(max(p[0] for p in tri))+1):
                for y in range(math.floor(min(p[1] for p in tri)),math.floor(max(p[1] for p in tri))+1):
                    self.bins.setdefault((x,y),[]).append((tri,area))

    def hit(self,x,y):
        for (a,b,c),area in self.bins.get((math.floor(x),math.floor(y)),[]):
            u=((b[0]-x)*(c[1]-y)-(b[1]-y)*(c[0]-x))/area
            v=((c[0]-x)*(a[1]-y)-(c[1]-y)*(a[0]-x))/area
            if u>=-1e-7 and v>=-1e-7 and u+v<=1+1e-7:return True
        return False


def audit(asset,rows,role,camera):
    errors=[];triangles=[];positions=[]
    metrics={'vertices':0,'triangles':0,'materials':len(asset.meshes),'invalid_normals':0,'degenerate_triangles':0,'reversed_normal_triangles':0}
    if len(asset.meshes)>12:errors.append('More than 12 material meshes; exceeds the agreed runtime architecture slots')
    if role=='supports':
        metrics['substrates']=asset.substrates
        if asset.substrates is None or len(asset.substrates)!=len(asset.meshes):
            errors.append('Support substrate storage must match the material mesh count')
        elif any(code not in (0,1,2,3,5,6) for code in asset.substrates):
            errors.append('Support substrate code is not an approved foreground/rear architecture family')
    for index,mesh in enumerate(asset.meshes):
        positions.extend(mesh.positions);metrics['vertices']+=len(mesh.positions)
        if len(mesh.positions)>65535 or len(mesh.indices)%3 or any(i<0 or i>=len(mesh.positions) for i in mesh.indices):
            errors.append(f'mesh {index}: invalid 16-bit index/triangle storage');continue
        if any(not math.isfinite(v) for p in mesh.positions+mesh.normals for v in p):
            errors.append(f'mesh {index}: nonfinite geometry');continue
        if len(mesh.color)!=4 or any(v<0 or v>255 for v in mesh.color) or mesh.color[3]!=255:
            errors.append(f'mesh {index}: architecture must have valid opaque RGBA')
        if not (0<=mesh.metallic<=1 and 0<=mesh.roughness<=1 and mesh.emission==0):
            errors.append(f'mesh {index}: invalid/nonzero-emission architecture material')
        metrics['invalid_normals']+=sum(abs(math.sqrt(sum(v*v for v in n))-1)>.02 for n in mesh.normals)
        for j in range(0,len(mesh.indices),3):
            ids=mesh.indices[j:j+3];tri=tuple(mesh.positions[i] for i in ids);triangles.append(tri)
            normal=cross(*tri);area2=math.sqrt(sum(v*v for v in normal))
            if area2<1e-10:metrics['degenerate_triangles']+=1;continue
            average=[sum(mesh.normals[i][k] for i in ids)/3 for k in range(3)]
            if sum(normal[k]*average[k] for k in range(3))/area2<-.05:metrics['reversed_normal_triangles']+=1
    if not positions or any(not math.isfinite(v) for p in positions for v in p):
        return {'asset':asset.name,'role':role,'status':'fail','errors':errors+['Missing/nonfinite positions']}
    metrics['triangles']=len(triangles)
    bounds=[[min(p[i] for p in positions),max(p[i] for p in positions)] for i in range(3)]
    metrics['world_bounds']=bounds
    metrics['projected_bounds']=[[min(camera.project(p)[i] for p in positions),max(camera.project(p)[i] for p in positions)] for i in range(2)]
    if bounds[2][1]>.00001:errors.append(f'Architecture protrudes in front of z0: {bounds[2][1]}')
    if bounds[2][0]<-8:errors.append(f'Architecture exceeds the agreed near-scene depth of -8: {bounds[2][0]}')
    if any(low<-PROJECTED_TRIM-.00001 or high>limit+PROJECTED_TRIM+.00001 for (low,high),limit in zip(metrics['projected_bounds'],(RW,RH))):
        errors.append(f'Projected architecture exceeds the stage trim allowance of {PROJECTED_TRIM} tiles')
    if any(abs((high-low)-size)>.001 for (low,high),size in zip(bounds,asset.size)):
        errors.append('Declared asset extents differ from actual delivered bounds')
    for key in ('invalid_normals','degenerate_triangles','reversed_normal_triangles'):
        if metrics[key]:errors.append(f'{key}: {metrics[key]}')
    front=projected(triangles,camera,FRONT_Z);query=TriangleQuery(front)
    contacts=[];contact_samples=0
    for row,x0,x1 in runs(rows,role):
        top=RH-row
        for n in range((x1-x0)*32):
            x=x0+(n+.5)/32
            if min(x-x0,x1-x)<LIP_TOLERANCE:continue
            contact_samples+=1
            below=query.hit(x,top-LIP_TOLERANCE)
            above=query.hit(x,top+LIP_TOLERANCE)
            if not below or above:contacts.append({'row':row,'x':x,'missing_below':not below,'occludes_above':above})
    metrics['contact_samples']=contact_samples;metrics['contact_failures']=len(contacts)
    if contacts:errors.append(f'{len(contacts)} map-derived contact probes do not match the visible front lip')
    expected=expected_mask(rows,role,RASTER);allowed=inflate(expected,RASTER,math.ceil(LIP_TOLERANCE*RASTER))
    front_mask=rasterize(front)
    leaks=[i for i,(a,e) in enumerate(zip(front_mask,allowed)) if a and not e]
    metrics['front_band_leak_samples']=len(leaks)
    if leaks:errors.append(f'{len(leaks)} front-band samples occupy unauthored air/water beyond tolerance')
    if role=='supports':
        near=rasterize(projected(triangles,camera,REAR_STANDOFF+.00001))
        unrecessed=sum(a and not e for a,e in zip(near,allowed))
        metrics['unrecessed_offmask_samples']=unrecessed
        if unrecessed:errors.append(f'{unrecessed} off-envelope support samples are closer than the -.45 rear stand-off')
    else:
        orthographic=rasterize(projected(triangles,camera,orthographic=True),8)
        expected_ortho=expected_mask(rows,'terrain',8)
        mismatch=sum(a!=b for a,b in zip(orthographic,expected_ortho))
        metrics['orthographic_mask_samples']=len(orthographic);metrics['orthographic_mask_mismatches']=mismatch
        if mismatch:errors.append(f'{mismatch} solid-mask sample mismatches in delivered terrain')
    # Whole-depth projection is diagnostic: legitimate rear faces can extend above
    # a lower landing under perspective. Never label all such pixels collisions.
    whole=rasterize(projected(triangles,camera),8);whole_allowed=expected_mask(rows,role,8)
    metrics['whole_depth_projected_offmask_samples_diagnostic']=sum(a and not e for a,e in zip(whole,whole_allowed))
    metrics['full_stage_samples_diagnostic']=len(whole)
    metrics['open_stage_samples_diagnostic']=sum(not e for e in whole_allowed)
    metrics['open_stage_covered_fraction_diagnostic']=round(metrics['whole_depth_projected_offmask_samples_diagnostic']/max(1,metrics['open_stage_samples_diagnostic']),6)
    return {'asset':asset.name,'role':role,'status':'fail' if errors else 'pass','errors':errors,'metrics':metrics,
            'contact_examples':contacts[:40], 'front_leak_examples':[[round((i%(RW*RASTER)+.5)/RASTER,4),round((i//(RW*RASTER)+.5)/RASTER,4)] for i in leaks[:40]]}


def binding_status(source,name):
    for call in re.findall(r"\bAsset\s*\((.*?)\);",source,re.S):
        if name in call and re.search(r",\s*\(Vector3\)\s*\{\s*0\s*,\s*0\s*,\s*0\s*\}\s*,\s*1\s*$",call):return 'identity draw found'
    # Supports select material substrate per mesh, so their identity draw is a
    # direct loop. Recognize this explicit source binding, not any nearby Draw.
    compact=re.sub(r'\s+','',source)
    selection='constFoundryAssetData*supports=roomIdx==0?&FOUNDRY_VAULT_SUPPORTS:&FOUNDRY_DROWNED_SUPPORTS;'
    loop='for(inti=0;i<supports->mesh_count;i++){'
    draw='Draw(&supportModels[roomIdx][i],(Vector3){0,0,0},(Vector3){1,1,1},(Vector3){0,1,0},0,c,m->metallic,m->roughness,m->emission);'
    if name.endswith('_SUPPORTS') and selection in compact and loop in compact:
        body=compact.split(loop,1)[1].split('substrate=0;',1)[0]
        load=f'LoadAsset(&{name},supportModels[{0 if "VAULT" in name else 1}]);'
        if load in compact and draw in body and 'constFoundryMeshData*m=&supports->meshes[i];' in body:
            return 'identity per-material support draw found'
    return 'unverified: no recognized literal identity asset draw found; inspect integration'


def self_test():
    """Adversarial geometry fixtures show which real failure classes are caught."""
    import copy
    rows=['.'*RW for _ in range(RH)];rows[10]='.'*10+'##'+'.'*28
    mesh=Mesh([(10,11,0),(12,11,0),(12,12,0),(10,12,0)],[(0,0,1)]*4,[0,1,2,0,2,3],[100,110,120,255],0,.8,0)
    base=Asset('hand_authored_two_tile_front',[mesh],(2,1,0));camera=Camera()
    assert audit(base,rows,'terrain',camera)['status']=='pass'
    cases={}
    for label,mutate in (
        ('shifted contact',lambda a:setattr(a.meshes[0],'positions',[(x,y-.2,z) for x,y,z in a.meshes[0].positions])),
        ('receded contact',lambda a:setattr(a.meshes[0],'positions',[(x,y,-2) for x,y,z in a.meshes[0].positions])),
        ('front intrusion',lambda a:setattr(a.meshes[0],'positions',[(x,y,.04) for x,y,z in a.meshes[0].positions])),
        ('inverted normals',lambda a:setattr(a.meshes[0],'normals',[(0,0,-1)]*4)),
        ('non-emissive rule',lambda a:setattr(a.meshes[0],'emission',.2)),
        ('invalid index',lambda a:setattr(a.meshes[0],'indices',[0,1,99])),
    ):
        asset=copy.deepcopy(base);mutate(asset);result=audit(asset,rows,'terrain',camera)
        assert result['status']=='fail',label
        cases[label]=result['errors']
    support_rows=['.'*RW for _ in range(RH)];support_rows[10]='.'*10+'--'+'.'*28
    lip=copy.deepcopy(mesh);lip.positions=[(10,11.5,0),(12,11.5,0),(12,12,0),(10,12,0)]
    rear=copy.deepcopy(mesh);rear.positions=[(10.8,10,-.8),(11.2,10,-.8),(11.2,11,-.8),(10.8,11,-.8)]
    support=Asset('hand_authored_shelf_and_rear',[lip,rear],(2,2,.8),[1,6])
    assert audit(support,support_rows,'supports',camera)['status']=='pass'
    rear.positions=[(x,y,-.2) for x,y,z in rear.positions];support.size=(2,2,.2)
    result=audit(support,support_rows,'supports',camera)
    assert any('rear stand-off' in error for error in result['errors'])
    cases['unrecessed rear receiver']=result['errors']
    return {'status':'passed','valid_fixture':'passed','valid_recessed_support':'passed','rejected_mutations':cases}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--terrain',type=Path,default=ROOT/'src/generated/terrain_assets.h')
    parser.add_argument('--supports',type=Path,default=ROOT/'src/generated/support_assets.h')
    parser.add_argument('--terrain-only',action='store_true',help='Bounded historical terrain audit; explicitly omit new support assets')
    parser.add_argument('--require-integrated',action='store_true')
    parser.add_argument('--self-test',action='store_true')
    parser.add_argument('--report',type=Path,default=ROOT/'build/architecture-check.json')
    args=parser.parse_args();room_path=ROOT/'src/room.c';depth_path=ROOT/'src/depth.c'
    report={'schema_version':1,'verified_utc':datetime.now(timezone.utc).isoformat(),'scope':'Delivered arrays, calibrated camera, collision contacts and recessed architecture. Not artistic/AAA acceptance.',
            'tolerances':{'front_z_min':FRONT_Z,'rear_standoff':REAR_STANDOFF,'landing_original_pixels':LIP_TOLERANCE*TILE_PIXELS,'raster_original_pixels':TILE_PIXELS/RASTER,'support_front_fascia_tiles':SUPPORT_FASCIA,'projected_stage_trim_tiles':PROJECTED_TRIM},
            'sources':{'checker_sha256':digest(Path(__file__)),'room_c_sha256':digest(room_path),'depth_c_sha256':digest(depth_path)},'assets':[]}
    missing=[]
    try:
        rows=read_maps(room_path);source=depth_path.read_text(encoding='utf-8');camera=read_camera(source)
        report['camera']=vars(camera)
        if args.self_test:report['adversarial_fixtures']=self_test()
        for role,path,suffix in [('terrain',args.terrain,'TERRAIN')]+([] if args.terrain_only else [('supports',args.supports,'SUPPORTS')]):
            if not path.exists():missing.append(str(path));continue
            report['sources'][str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)]=digest(path)
            assets=read_assets(path)
            for room,name in enumerate(('VAULT','DROWNED')):
                symbol=f'FOUNDRY_{name}_{suffix}'
                if symbol not in assets:raise ValueError(f'Missing required delivered asset: {symbol}')
                result=audit(assets[symbol],rows[room],role,camera)
                result['room']=room;result['binding']=binding_status(source,symbol)
                if args.require_integrated and result['binding'].startswith('unverified'):
                    result['errors'].append(result['binding']);result['status']='fail'
                report['assets'].append(result)
                print(f"{result['status'].upper()} {symbol}: "+'; '.join(result['errors'] or ['contacts, bounds, normals, attributes and scoped masks']))
        if missing:report['missing_deliveries']=missing
        report['status']='incomplete' if missing else ('fail' if any(a['status']=='fail' for a in report['assets']) else 'pass')
        report['integration_scope']='required' if args.require_integrated else 'reported only; asset audit is not renderer acceptance'
        report['omitted_supports']=args.terrain_only
    except (ValueError,KeyError,AssertionError) as error:
        report['status']='fail';report['fatal_error']=str(error);print(f'FAIL {error}',file=sys.stderr)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f"{report['status'].upper()}: {args.report}")
    return 0 if report['status']=='pass' else 1


if __name__=='__main__':
    sys.exit(main())
