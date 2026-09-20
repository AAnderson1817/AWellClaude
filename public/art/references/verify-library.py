"""Validate the two 30-image libraries and make review thumbnails, never edit originals."""
from pathlib import Path
import hashlib
import json
import textwrap
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
REVIEW = ROOT / 'review'
REVIEW.mkdir(exist_ok=True)
all_hashes = []
report = {'scope': '60 generated references; contact sheets are display thumbnails only and are not counted', 'rooms': []}
font_path = Path('C:/Windows/Fonts/segoeui.ttf')
font = ImageFont.truetype(str(font_path), 15)
head = ImageFont.truetype(str(font_path), 25)
small = ImageFont.truetype(str(font_path), 14)

for folder in ['vault-mouth', 'drowned-quarter']:
    path = ROOT / folder / 'manifest.json'
    manifest = json.loads(path.read_text(encoding='utf-8'))
    images = manifest['images']
    assert len(images) == 30, (folder, len(images))
    assert sorted(e['id'] for e in images) == list(range(1, 31))
    assert len(list((ROOT / folder).glob('*.png'))) == 30
    sheet = Image.new('RGB', (1984, 1260), '#0b1214')
    draw = ImageDraw.Draw(sheet)
    draw.text((20, 15), manifest['environment'] + ' | 30 individual references', font=head, fill='#d9e7df')
    draw.text((20, 48), 'Review thumbnails only. Concepts are not runtime screenshots. Read manifest review notes before using details.', font=small, fill='#8fa7a0')
    room_report = {'environment': manifest['environment'], 'count': len(images), 'files': []}
    for index, entry in enumerate(sorted(images, key=lambda e: e['id'])):
        original = ROOT / folder / entry['filename']
        assert original.is_file(), original
        digest = hashlib.sha256(original.read_bytes()).hexdigest()
        with Image.open(original) as im:
            im.load()
            width, height = im.size
            assert width >= 700 and height >= 700, (original, im.size)
            assert getattr(im, 'n_frames', 1) == 1
            metadata = {'width': width, 'height': height, 'bytes': original.stat().st_size, 'sha256': digest}
            entry.update(metadata)
            room_report['files'].append({'id': entry['id'], 'filename': entry['filename'], **metadata})
            thumb = im.convert('RGB')
            thumb.thumbnail((308, 182), Image.Resampling.LANCZOS)
        all_hashes.append(digest)
        x = 20 + (index % 6) * 326
        y = 82 + (index // 6) * 232
        draw.rectangle((x, y, x + 307, y + 181), fill='#101d21')
        sheet.paste(thumb, (x + (308 - thumb.width) // 2, y + (182 - thumb.height) // 2))
        label = f"{entry['id']:02d}  {entry['title']}"
        for line, text in enumerate(textwrap.wrap(label, width=39)[:2]):
            draw.text((x, y + 187 + line * 18), text, font=font, fill='#d9e7df')
    manifest['count'] = len(images)
    manifest['status'] = 'complete'
    manifest['validation'] = 'All 30 final PNGs decoded; IDs 1-30 unique; SHA256 and dimensions recorded. Individual artistic caveats are in review fields.'
    path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    sheet_path = REVIEW / f'{folder}-contact-sheet.jpg'
    sheet.save(sheet_path, quality=92, subsampling=0)
    room_report['contactSheet'] = str(sheet_path.relative_to(ROOT)).replace('\\', '/')
    report['rooms'].append(room_report)

assert len(all_hashes) == 60
assert len(set(all_hashes)) == 60, 'Duplicate final file bytes'
report['totalImages'] = 60
report['uniqueSHA256'] = 60
report['decodedPNGs'] = 60
(REVIEW / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps({'totalImages': 60, 'uniqueSHA256': 60, 'decodedPNGs': 60, 'rooms': [{'name': r['environment'], 'count': r['count'], 'contactSheet': r['contactSheet']} for r in report['rooms']]}))
