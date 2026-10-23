"""Generate Kokoro audio for the selected Excel sentences."""
import argparse
import json
import os
from pathlib import Path
import numpy as np
import soundfile as sf
from sentence_data import DEFAULT_WORKBOOK, load_sentences

US_MALE_VOICES = ('adam', 'echo', 'eric', 'fenrir', 'liam', 'michael', 'onyx', 'puck', 'santa')
VOICE_IDS = {'sarah': 'af_sarah', **{name: f'am_{name}' for name in US_MALE_VOICES}}

def generate_audio(sentences, output_dir, voices=('michael',), speed=1.0, overwrite=False):
    os.environ.setdefault('HF_HOME', str(Path(__file__).parent / '.cache' / 'huggingface'))
    os.environ.setdefault('TORCH_HOME', str(Path(__file__).parent / '.cache' / 'torch'))
    from kokoro import KPipeline
    pipeline = None
    voice_ids = VOICE_IDS
    for sentence in sentences:
        for voice in voices:
            path = Path(output_dir) / voice / f"{sentence['id']}.wav"
            metadata = path.with_suffix('.json')
            expected = dict(sentence=sentence['sentence'], voice=voice_ids[voice], speed=speed)
            if not overwrite and path.exists() and metadata.exists() and all(json.loads(metadata.read_text(encoding='utf-8')).get(k) == v for k, v in expected.items()):
                continue
            if pipeline is None:
                pipeline = KPipeline(lang_code='a', repo_id='hexgrad/Kokoro-82M')
            chunks = []
            for _, _, audio in pipeline(sentence['sentence'], voice=voice_ids[voice], speed=speed):
                if hasattr(audio, 'detach'):
                    audio = audio.detach().cpu().numpy()
                chunks.append(np.asarray(audio))
            if not chunks or not sum(chunk.size for chunk in chunks):
                raise RuntimeError(f'No audio for {voice}/{sentence["id"]}')
            path.parent.mkdir(parents=True, exist_ok=True)
            sf.write(path, np.concatenate(chunks), 24000)
            metadata.write_text(json.dumps(expected, ensure_ascii=False, indent=2), encoding='utf-8')
            print(f"Generated {voice}/{sentence['id']}: {sentence['sentence']}", flush=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workbook', type=Path, default=DEFAULT_WORKBOOK)
    parser.add_argument('--output-dir', type=Path, default=Path(__file__).parent / 'audio')
    parser.add_argument('--limit', type=int, default=100, help='0 = all colored sentences')
    parser.add_argument('--voices', nargs='+', choices=list(VOICE_IDS), default=['michael'])
    parser.add_argument('--speed', type=float, default=1.0)
    parser.add_argument('--overwrite', action='store_true')
    args = parser.parse_args()
    if args.limit < 0 or args.speed <= 0:
        parser.error('limit must be >= 0 and speed must be > 0')
    generate_audio(load_sentences(args.workbook, args.limit), args.output_dir, args.voices, args.speed, args.overwrite)

if __name__ == '__main__':
    main()
