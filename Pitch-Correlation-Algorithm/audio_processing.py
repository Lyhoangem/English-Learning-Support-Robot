import librosa
import numpy as np
import parselmouth
import warnings
import soundfile as sf
from pathlib import Path

def trim_silence_and_save(audio_path, top_db=30, output_path=None):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        y, sr = librosa.load(audio_path, sr=None)
        
    y_trimmed, _ = librosa.effects.trim(y, top_db=top_db)
    if not len(y_trimmed) or not np.any(y_trimmed):
        raise ValueError(f'Empty or silent audio: {audio_path}')
    source = Path(audio_path)
    trimmed_path = str(output_path or source.with_name(source.stem + '_trimmed.wav'))
    sf.write(trimmed_path, y_trimmed, sr)
    
    return trimmed_path, y_trimmed, sr

def extract_features_praat(audio_path, verbose=False):
    snd = parselmouth.Sound(audio_path)
    pitch_object = snd.to_pitch()
    pitch = np.nan_to_num(pitch_object.selected_array['frequency'])

    time_stamps = pitch_object.xs()
    if verbose: print(f"\n[KIỂM CHỨNG BƯỚC NHẢY] Các mốc thời gian của {audio_path} - {len(pitch)} cao độ:\n{time_stamps[:len(pitch)]}\n")
    return pitch, time_stamps

def get_local_chunk(raw_array, w_start, w_end, total_dur):
    N = len(raw_array)
    if total_dur == 0: return np.array([])
    i1 = int((w_start / total_dur) * N)
    i2 = int((w_end / total_dur) * N)
    if i1 >= i2 or i1 >= N: return np.array([])
    return raw_array[i1:i2]

def equalize_local_chunk(user_chunk, ref_chunk):
    if len(user_chunk) == 0 or len(ref_chunk) == 0: return np.zeros_like(ref_chunk)
    if len(user_chunk) == len(ref_chunk): return user_chunk

    x_ref = np.linspace(0, 1, len(ref_chunk))
    x_user = np.linspace(0, 1, len(user_chunk))
    
    return np.interp(x_ref, x_user, user_chunk)

def dynamic_sliding_window_pearson(aligned_user, aligned_ref, num_words):
    N = len(aligned_ref)
    if num_words <= 0 or N == 0: return np.array([]), np.array([]), np.array([])
    
    W = max(6, int((2 * N) / (num_words + 2)))
    if W % 2 != 0: W -= 1 
    
    S = max(1, W // 2)
    num_slides = num_words + 1
    
    running_r, window_starts, window_ends = [], [], []
    
    for i in range(num_slides):
        start_idx = i * S
        end_idx = min(start_idx + W, N) if i < num_slides - 1 else N
        
        u_win = aligned_user[start_idx : end_idx]
        r_win = aligned_ref[start_idx : end_idx]
        
        valid = (u_win > 0) & (r_win > 0)
        
        if np.sum(valid) >= 3 and np.std(u_win[valid]) > 0 and np.std(r_win[valid]) > 0:
            r = np.corrcoef(u_win[valid], r_win[valid])[0, 1]
        else:
            r = np.nan
            
        running_r.append(r)
        window_starts.append(start_idx)
        window_ends.append(end_idx)
        
    return np.array(running_r), np.array(window_starts), np.array(window_ends)
