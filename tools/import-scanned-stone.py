#!/usr/bin/env python3
"""Import the pinned CC0 Rock Surface maps without overwriting their originals.

Input files are the unmodified 1K PNG downloads listed in provenance.json.
This offline conversion prepares RGB8 inputs for the embedded material packer.
It performs no repainting, baked lighting, sharpening or green-channel reversal.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import struct

import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
FILES={
    'Diffuse':('diff','3581fe9abecdf2e2afc0b7d64a7af4f05b4dc01f3058ac02b90f39977466df5c'),
    'nor_gl':('nor_gl','a9e14af4267b6c512987575bd1120f5fba5d6b1a178e4e58d3bab44be8895ffa'),
    'Rough':('rough','01061a50b9f356580966f4d2c124c6c688bd11c27fd945be00e78c9712056e09'),
    'Displacement':('disp','c00bf9a4041c723f9ef7f4417f12e74e11bb37407b4f8f4efdc5fe8ac4c9e0f4'),
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'public/materials/vendor/rock-surface')
    args=parser.parse_args()
    vendor=ROOT/'public/materials/vendor/rock-surface'
    output=ROOT/'public/materials/scanned-stone'
    sources={}
    for name,(suffix,expected) in FILES.items():
        path=args.source/(name+'.png')
        if digest(path)!=expected:
            raise ValueError(f'Unrecognized source bytes: {path}')
        data=path.read_bytes()
        width,height,bits,color,*_=struct.unpack('>IIBBBBB',data[16:29])
        if (width,height,bits)!=(1024,1024,16):
            raise ValueError(f'Expected original RGB/scalar 16-bit 1K PNG: {path}')
        sources[name]={'file':name+'.png','sha256':expected,'bytes':len(data),
                       'width':width,'height':height,'bits':bits,'color_type':color,
                       'url':f'https://dl.polyhaven.org/file/ph-assets/Textures/png/1k/rock_surface/rock_surface_{suffix}_1k.png'}
    vendor.mkdir(parents=True,exist_ok=True)
    output.mkdir(parents=True,exist_ok=True)
    for name in FILES:
        src=args.source/(name+'.png');dst=vendor/(name+'.png')
        if src.resolve()!=dst.resolve():shutil.copyfile(src,dst)
    # Pillow exposes the high byte of RGB16 samples; explicitly retain that
    # conversion contract instead of implying a sixteen-bit output or roundtrip.
    Image.open(vendor/'Diffuse.png').convert('RGB').save(output/'basecolor.png')
    vectors=np.asarray(Image.open(vendor/'nor_gl.png').convert('RGB'),dtype=np.float64)/127.5-1
    lengths=np.linalg.norm(vectors,axis=2,keepdims=True)
    if lengths.min()<=.25 or vectors[:,:,2].min()<=0:
        raise ValueError('Unexpected zero, reversed or invalid source normal')
    # The vendor's reduced-resolution normals are filtered but not all unit
    # length. Restore vector directions before quantizing and later mip filtering.
    normal=np.rint(np.clip((vectors/lengths+1)*127.5,0,255)).astype(np.uint8)
    Image.fromarray(normal).save(output/'normal.png')
    rough=np.asarray(Image.open(vendor/'Rough.png'),dtype=np.float64)
    Image.fromarray(np.rint(rough/257).astype(np.uint8)).save(output/'roughness.png')
    report={'asset':'Rock Surface','author':'Amal Kumar',
            'source':'https://polyhaven.com/a/rock_surface','license':'CC0',
            'license_url':'https://polyhaven.com/license','api_credit':'Powered by Poly Haven',
            'imported_utc':datetime.now(timezone.utc).isoformat(),
            'physical_width_m':2,'game_repeat_world_units':5,
            'scale_note':'Game art convention is 0.4 m per world unit; not a measured character height.',
            'original_maps':sources,
            'conversion':{'basecolor':'RGB16 high-byte extraction to RGB8; source sRGB unchanged',
                          'normal':'RGB16 high-byte decode; normalize each nonzero +Y vector; nearest RGB8; no green flip',
                          'roughness':'linear R16 / 257 rounded to nearest R8',
                          'displacement':'original preserved for editable material source; no runtime displacement',
                          'source_normal_length_min':float(lengths.min()),'source_normal_length_max':float(lengths.max())},
            'converted_sha256':{name:digest(output/(name+'.png')) for name in ('basecolor','normal','roughness')},
            'source_preservation':all(digest(vendor/(name+'.png'))==expected for name,(_,expected) in FILES.items())}
    (vendor/'provenance.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('PASS: four pinned vendor maps preserved; three technical RGB8/R8 conversions written')

if __name__=='__main__':main()
