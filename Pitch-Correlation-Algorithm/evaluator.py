from pathlib import Path
import re
import warnings
import librosa
import soundfile as sf
import whisper
import argparse
import json
from pathlib import Path


def normalize_word(word):
    return re.sub(r'[^\w]', '', word).lower()


def load_stt_model():
    return whisper.load_model('base', download_root=str(Path(__file__).parent / '.cache' / 'whisper'))


def get_user_timestamps(model, audio_path):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        audio, sr = sf.read(audio_path, dtype='float32')
        if audio.ndim > 1:
            audio = audio.mean(axis=1)
        if sr != 16000:
            audio = librosa.resample(audio, orig_sr=sr, target_sr=16000)
        result = model.transcribe(audio, word_timestamps=True, fp16=False,
                                  language='en', verbose=None)
    return [dict(word=normalize_word(word['word']), start=word['start'], end=word['end'])
            for segment in result.get('segments', []) for word in segment.get('words', [])]


def run(audio_processing):
    parser = argparse.ArgumentParser(description='Evaluate pronunciation using existing audio files.')
    parser.add_argument('audio', nargs='*', type=Path, help='WAV files to transcribe')
    parser.add_argument('--output', type=Path, help='Optional JSON output path')
    args = parser.parse_args()
    model = load_stt_model()
    results = []
    for path in args.audio:
        trimmed, _, _ = audio_processing.trim_silence_and_save(str(path))
        pitch, _ = audio_processing.extract_features_praat(trimmed)
        timestamps = get_user_timestamps(model, trimmed)
        results.append({'file': str(path), 'trimmed_file': trimmed,
                        'pitch_frames': len(pitch), 'recognized_words': timestamps})
        print(f'{path}: {len(timestamps)} recognized words, {len(pitch)} pitch frames')
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    elif not args.audio:
        parser.print_help()
