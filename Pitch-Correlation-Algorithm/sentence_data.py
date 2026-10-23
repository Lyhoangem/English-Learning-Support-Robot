import re
from pathlib import Path
from openpyxl import load_workbook
from xml.etree import ElementTree
import colorsys

DEFAULT_WORKBOOK = Path(__file__).parent / 'data' / 'Những Câu Dài Nhất.xlsx'

def normalize_word(word):
    return re.sub(r'[^\w]', '', word).lower()

def parse_positions(value, count, word_count):
    text = str(int(value)) if isinstance(value, (int, float)) else str(value).strip()
    if re.search(r'[,;\s]', text):
        positions = [int(x) for x in re.split(r'[,;\s]+', text) if x]
    else:
        solutions = []
        def split(rest, chosen):
            if not rest:
                if len(chosen) == count:
                    solutions.append(chosen)
                return
            if len(chosen) >= count or rest.startswith('0'):
                return
            for size in range(1, min(len(rest), len(str(word_count))) + 1):
                number = int(rest[:size])
                if 1 <= number <= word_count and (not chosen or number > chosen[-1]):
                    split(rest[size:], chosen + [number])
        split(text, [])
        if len(solutions) != 1:
            raise ValueError(f'Ambiguous/invalid stress positions: {text!r}')
        positions = solutions[0]
    if len(positions) != count or len(set(positions)) != count or any(p < 1 or p > word_count for p in positions):
        raise ValueError(f'Invalid stress count/positions: {value!r}')
    return positions

def selected_color(cell, book):
    if cell.fill.patternType != 'solid':
        return None
    color = cell.fill.fgColor
    if color.type == 'rgb':
        rgb = color.rgb[-6:]
    elif color.type == 'theme':
        scheme = ElementTree.fromstring(book.loaded_theme).find('.//{*}clrScheme')
        entry = list(scheme)[color.theme][0]
        rgb = entry.get('val') or entry.get('lastClr')
    else:
        return None
    red, green, blue = [int(rgb[i:i+2], 16) / 255 for i in (0, 2, 4)]
    hue, saturation, _ = colorsys.rgb_to_hsv(red, green, blue)
    if saturation < 0.2:
        return None
    if hue < 0.04 or hue > 0.85:
        return 'pink'
    if 0.04 <= hue <= 0.13:
        return 'orange'
    return None


def load_sentences(workbook=DEFAULT_WORKBOOK, limit=100):
    book = load_workbook(workbook, read_only=True, data_only=True)
    sentences, topic = [], None
    try:
        for row_number, cells in enumerate(book.active.iter_rows(), 1):
            row = [cell.value for cell in cells]
            if len(row) < 5 or not isinstance(row[0], (int, float)) or not isinstance(row[2], str):
                continue
            topic = row[1] or topic
            color = selected_color(cells[2], book)
            if color is None:
                continue
            sentence = row[2].strip()
            words = sentence.split()
            # Some rows repeat the header or omit stress metadata.
            if row[3] is None or row[4] is None:
                raise ValueError(f'Missing stress metadata at Excel row {row_number}')
            try:
                count = int(row[3])
                positions = parse_positions(row[4], count, len(words))
            except (ValueError, TypeError) as exc:
                raise ValueError(f'Excel row {row_number}: {exc}') from exc
            sentences.append(dict(id=f'{topic}/{int(row[0]):03d}', color=color,
                                  source_row=row_number, source_id=int(row[0]), topic=topic,
                                  sentence=sentence, stress_count=count, stress_positions=positions,
                                  words=[dict(word=w, is_stressed=i in positions) for i, w in enumerate(words, 1)]))
    finally:
        book.close()
    if len({s['id'] for s in sentences}) != len(sentences):
        raise ValueError('Duplicate STT within a topic; output names would collide')
    if limit and len(sentences) < limit:
        raise ValueError(f'Only {len(sentences)} sentences; requested {limit}')
    if limit:
        sentences = sentences[:limit]
    return sentences
