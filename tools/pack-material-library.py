#!/usr/bin/env python3
"""Package authored material maps for the embedded renderer, without art changes.

Source RGB8 PNGs remain untouched. Base color is box-filtered in linear light;
OpenGL +Y normals are vector-filtered and renormalized; scalar roughness is
box-filtered in linear data space and stored in the detail image alpha channel.
Requires Pillow and numpy. This is technical packaging, not artistic acceptance.
"""
import argparse
from datetime import datetime, timezone
import hashlib
from io import BytesIO
import json
from pathlib import Path
import struct
import sys
import zlib

import numpy as np
import PIL
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
MATERIALS = ('city-stone', 'vault-basalt', 'aged-timber')
SIZE = 512


def sha(data):
    return hashlib.sha256(data).hexdigest()


def relative(path):
    return path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else str(path)


def png_info(data):
    if data[:8] != b'\x89PNG\r\n\x1a\n' or data[12:16] != b'IHDR':
        raise ValueError('Expected a PNG with IHDR')
    w, h, bits, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', data[16:29])
    return {'width': w, 'height': h, 'channel_bits': bits, 'png_color_type': color,
            'compression': compression, 'filter_method': filtering, 'interlace': interlace}


def load_source(path, role):
    data = path.read_bytes()
    info = png_info(data)
    with Image.open(BytesIO(data)) as image:
        image.load()
        mode = image.mode
        if info['channel_bits'] != 8 or mode not in (('L', 'RGB') if role == 'roughness' else ('RGB',)):
            raise ValueError(f'{path}: {role} requires RGB8 (L8 is also allowed for roughness), found {mode}/{info["channel_bits"]}')
        pixels = np.asarray(image).copy()
    width, height = info['width'], info['height']
    if width != height or width < SIZE or width % SIZE:
        raise ValueError(f'{path}: expected square dimensions that are an integer multiple of {SIZE}')
    if role == 'roughness' and pixels.ndim == 3:
        if not (np.array_equal(pixels[..., 0], pixels[..., 1]) and np.array_equal(pixels[..., 0], pixels[..., 2])):
            raise ValueError(f'{path}: roughness RGB channels differ; refusing to invent a conversion')
        pixels = pixels[..., 0]
    return pixels, {**info, 'path': relative(path), 'mode': mode, 'sha256': sha(data), 'file_bytes': len(data)}


def box_reduce(values, size=SIZE):
    height, width = values.shape[:2]
    if height != width or height % size or height < size:
        raise ValueError('Box reduction requires an integer square reduction')
    factor = height // size
    return values.reshape(size, factor, size, factor, *values.shape[2:]).mean(axis=(1, 3), dtype=np.float64)


def srgb_to_linear(values):
    return np.where(values <= .04045, values / 12.92, ((values + .055) / 1.055) ** 2.4)


def linear_to_srgb(values):
    return np.where(values <= .0031308, values * 12.92, 1.055 * np.maximum(values, 0) ** (1 / 2.4) - .055)


def byte_values(values):
    return np.rint(np.clip(values, 0, 1) * 255).astype(np.uint8)


def normal_stats(encoded):
    vectors = encoded.astype(np.float64) / 127.5 - 1
    lengths = np.linalg.norm(vectors, axis=2)
    errors = np.abs(lengths - 1)
    return {'length_min': float(lengths.min()), 'length_max': float(lengths.max()),
            'length_error_mean': float(errors.mean()), 'length_error_max': float(errors.max()),
            'length_error_p99': float(np.quantile(errors, .99)),
            'negative_z_texels': int(np.count_nonzero(vectors[..., 2] < 0))}


def filter_normal(encoded, size=SIZE):
    vectors = encoded.astype(np.float64) / 127.5 - 1
    lengths = np.linalg.norm(vectors, axis=2, keepdims=True)
    if np.any(lengths < 1e-10) or np.max(np.abs(lengths - 1)) > .02:
        raise ValueError('Source normal contains invalid vectors beyond RGB8 quantization tolerance')
    filtered = box_reduce(vectors / lengths, size)
    lengths = np.linalg.norm(filtered, axis=2, keepdims=True)
    if np.any(lengths < 1e-10):
        raise ValueError('Filtered normal vectors cancel; cannot choose a replacement direction')
    normalized = filtered / lengths
    encoded_result = byte_values(normalized * .5 + .5)
    stats = normal_stats(encoded_result)
    if stats['length_error_max'] > .007:
        raise ValueError('Runtime normal exceeds expected RGB8 normalization error')
    stats['float_length_error_max_before_quantization'] = float(np.max(np.abs(np.linalg.norm(normalized, axis=2) - 1)))
    stats['shortest_average_vector_before_normalization'] = float(lengths.min())
    return encoded_result, stats


def encode_png(pixels, mode):
    buffer = BytesIO()
    Image.fromarray(pixels, mode=mode).save(buffer, format='PNG', compress_level=9, optimize=False)
    data = buffer.getvalue()
    with Image.open(BytesIO(data)) as decoded:
        if decoded.mode != mode or not np.array_equal(np.asarray(decoded), pixels):
            raise ValueError('PNG round trip changed packaged pixels')
    return data


def memory_estimate():
    levels = []
    size = SIZE
    while size:
        levels.append({'size': [size, size], 'texels': size * size})
        size //= 2
    texels = sum(level['texels'] for level in levels)
    exact = texels * (3 + 4) * len(MATERIALS)
    expanded = texels * (4 + 4) * len(MATERIALS)
    return {'assumption': 'Uncompressed full mip chain, RGB8 base plus RGBA8 detail; driver may expand RGB8 to RGBA8.',
            'levels_per_texture': len(levels), 'mip_levels': levels,
            'rgb8_base_rgba8_detail_bytes_all_materials': exact,
            'rgb8_base_rgba8_detail_mib_all_materials': exact / 1048576,
            'both_rgba8_bytes_all_materials': expanded,
            'both_rgba8_mib_all_materials': expanded / 1048576,
            'exclusions': 'Driver alignment, upload staging, source PNG memory and sampling overhead are not included.'}


def header_for(packages):
    parts = ['/* Generated by tools/pack-material-library.py. Authored originals are preserved. */\n',
             '#ifndef FOUNDRY_MATERIAL_TEXTURES_H\n#define FOUNDRY_MATERIAL_TEXTURES_H\n',
             'typedef struct FoundrySurfaceImages { const unsigned char *base; int base_size; const unsigned char *detail; int detail_size; float mean[3]; } FoundrySurfaceImages;\n']
    entries = []
    for slug, base, detail, means in packages:
        stem = 'foundry_surface_' + slug.replace('-', '_')
        for suffix, data in (('base', base), ('detail', detail)):
            parts.append(f'static const unsigned char {stem}_{suffix}[] = {{\n')
            parts.extend(','.join(f'0x{value:02x}' for value in data[i:i+24]) + ',\n' for i in range(0, len(data), 24))
            parts.append('};\n')
        mean = ','.join(f'{value:.9f}f' for value in means)
        entries.append(f'    {{{stem}_base,(int)sizeof({stem}_base),{stem}_detail,(int)sizeof({stem}_detail),{{{mean}}}}},\n')
    parts.append('/* Order: city stone, vault basalt, aged timber. Means are encoded runtime RGB / 255. */\n')
    parts.append('static const FoundrySurfaceImages FOUNDRY_SURFACE_IMAGES[3] = {\n')
    parts.extend(entries)
    parts.append('};\n#endif\n')
    return ''.join(parts).encode('ascii')


def self_test():
    # A black/white pair has linear mean .5, hence encoded sRGB 188, not 128.
    checker = np.array([[0, 255], [255, 0]], dtype=np.uint8)
    reduced = byte_values(linear_to_srgb(box_reduce(srgb_to_linear(checker / 255), 1)))
    assert int(reduced[0, 0]) == 188
    # Opposing slopes average toward +Z; a +Y slope must keep its handedness.
    vectors = np.array([[[.6, 0, .8], [-.6, 0, .8]], [[.6, 0, .8], [-.6, 0, .8]]])
    filtered, _ = filter_normal(byte_values(vectors * .5 + .5), 1)
    assert np.max(np.abs(filtered[0, 0].astype(int) - [128, 128, 255])) <= 1
    slope = np.broadcast_to([0, .6, .8], (2, 2, 3))
    filtered, _ = filter_normal(byte_values(slope * .5 + .5), 1)
    assert filtered[0, 0, 1] > 200 and filtered[0, 0, 2] > 225
    rough = byte_values(box_reduce(np.array([[0., .2], [.6, 1.]]), 1))
    assert int(rough[0, 0]) == 115
    detail = np.concatenate((filtered, rough[..., None]), axis=2)
    data = encode_png(detail, 'RGBA')
    assert png_info(data)['channel_bits'] == 8 and png_info(data)['png_color_type'] == 6
    return {'status': 'pass', 'probes': ['linear-light color averaging', 'opposing normal slope cancellation',
                                      '+Y convention preserved', 'linear scalar roughness', 'lossless detail PNG channel packing']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'public/materials')
    parser.add_argument('--runtime', type=Path, default=ROOT / 'public/materials/runtime')
    parser.add_argument('--header', type=Path, default=ROOT / 'src/generated/material_textures.h')
    parser.add_argument('--report', type=Path, default=ROOT / 'docs/evidence/materials/runtime-package.json')
    parser.add_argument('--check', action='store_true', help='Validate exact current output bytes and recorded source hashes without writing')
    parser.add_argument('--self-test', action='store_true', help='Also run independent synthetic filtering probes')
    args = parser.parse_args()
    for field in ('source', 'runtime', 'header', 'report'):
        setattr(args, field, getattr(args, field).resolve())
    report = {'schema_version': 1, 'generated_utc': datetime.now(timezone.utc).isoformat(),
              'scope': 'Technical map conversion only; source art preserved. No artistic acceptance.',
              'tool_sha256': sha(Path(__file__).read_bytes()), 'runtime_resolution': [SIZE, SIZE],
              'material_order': list(MATERIALS),
              'versions': {'python': sys.version.split()[0], 'Pillow': PIL.__version__, 'numpy': np.__version__, 'zlib': zlib.ZLIB_RUNTIME_VERSION},
              'filtering': {'basecolor': 'sRGB decode -> linear-light area box -> sRGB encode -> nearest RGB8',
                            'normal': 'RGB8 to XYZ, normalize each source vector, area box, renormalize, nearest RGB8; no green flip (+Y)',
                            'roughness': 'linear scalar area box -> nearest R8 stored in detail alpha',
                            'runtime_sampling': 'Sample detail as linear data and normalize decoded normal after bilinear/mip sampling. Base means are encoded RGB means, not linear luminance.',
                            'mips': 'Only level zero is packaged; runtime generates mips. GPU normal mip averages require renormalization after sampling.'},
              'materials': [], 'gpu_memory_estimate': memory_estimate()}
    if args.self_test:
        report['synthetic_checks'] = self_test()
    outputs = {}; packages = []; source_hashes = {}
    for slug in MATERIALS:
        source = {}; meta = {}
        for role in ('basecolor', 'normal', 'roughness'):
            path = args.source / slug / (role + '.png')
            source[role], meta[role] = load_source(path, role)
            source_hashes[path] = meta[role]['sha256']
        if len({array.shape[:2] for array in source.values()}) != 1:
            raise ValueError(f'{slug}: map resolutions differ')
        base = byte_values(linear_to_srgb(box_reduce(srgb_to_linear(source['basecolor'].astype(np.float64) / 255))))
        normals, normal_measurements = filter_normal(source['normal'])
        rough = byte_values(box_reduce(source['roughness'].astype(np.float64) / 255))
        detail = np.concatenate((normals, rough[..., None]), axis=2)
        base_data, detail_data = encode_png(base, 'RGB'), encode_png(detail, 'RGBA')
        base_path, detail_path = [args.runtime / slug / (name + '.png') for name in ('basecolor', 'detail')]
        outputs[base_path] = base_data; outputs[detail_path] = detail_data
        means = base.mean(axis=(0, 1), dtype=np.float64) / 255
        packages.append((slug, base_data, detail_data, means))
        report['materials'].append({'name': slug, 'sources': meta,
            'basecolor': {**png_info(base_data), 'path': relative(base_path), 'file_bytes': len(base_data), 'sha256': sha(base_data),
                          'encoded_rgb_mean': means.tolist()},
            'detail': {**png_info(detail_data), 'path': relative(detail_path), 'file_bytes': len(detail_data), 'sha256': sha(detail_data),
                       'channels': 'RGB: OpenGL +Y tangent normal, A: linear roughness'},
            'normal_source': normal_stats(source['normal']), 'normal_runtime': normal_measurements,
            'roughness_source_range': [int(source['roughness'].min()), int(source['roughness'].max())],
            'roughness_runtime_range': [int(rough.min()), int(rough.max())],
            'roughness_runtime_normalized_range': [float(rough.min() / 255), float(rough.max() / 255)],
            'png_round_trip': 'exact pixel equality'})
    header = header_for(packages)
    outputs[args.header] = header
    if any(path in source_hashes for path in [*outputs, args.report]):
        raise ValueError('Output paths overlap authored source maps; originals must remain untouched')
    report['header'] = {'path': relative(args.header), 'sha256': sha(header), 'file_bytes': len(header),
                        'png_payload_bytes': sum(len(base) + len(detail) for _, base, detail, _ in packages),
                        'size_fields': '(int)sizeof corresponding static unsigned char PNG arrays'}
    changed = [relative(path) for path, value in source_hashes.items() if sha(path.read_bytes()) != value]
    if changed:
        raise ValueError(f'Sources changed during packing; await stable bake and retry: {changed}')
    report['source_preservation'] = 'All nine input byte hashes unchanged after processing; no source writes.'
    report['status'] = 'pass'
    if args.check:
        errors = [relative(path) for path, data in outputs.items() if not path.exists() or path.read_bytes() != data]
        if not args.report.exists():
            errors.append(relative(args.report))
        else:
            prior = json.loads(args.report.read_text(encoding='utf-8'))
            for field in ('tool_sha256', 'materials', 'header', 'gpu_memory_estimate'):
                if prior.get(field) != report[field]:
                    errors.append(f'report {field}')
        if errors:
            raise ValueError(f'Stale or changed package: {errors}')
        print('PASS: all runtime PNGs, header bytes, source hashes and report measurements match; no files written')
    else:
        outputs[args.report] = (json.dumps(report, indent=2) + '\n').encode('utf-8')
        for path, data in outputs.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            temporary = path.with_suffix(path.suffix + '.tmp')
            temporary.write_bytes(data)
            temporary.replace(path)
        print(f'PASS: {len(MATERIALS)} material packages, {report["header"]["png_payload_bytes"]:,} embedded PNG bytes')
        print(f'Report: {relative(args.report)}')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError, AssertionError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        sys.exit(1)
