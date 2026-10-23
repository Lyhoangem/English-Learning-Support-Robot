import matplotlib.pyplot as plt
import numpy as np
import difflib
from sentence_data import normalize_word
from audio_processing import get_local_chunk, equalize_local_chunk, dynamic_sliding_window_pearson, summarize_pearson

def plot_analysis(y_ref, sr_ref, y_user, sr_user, raw_ref_pitch, raw_user_pitch, ref_words_json, user_words_info, output_file="michael.png", sentence=None, verbose=False, user_label='Michael'):
    ref_audio_dur = len(y_ref) / sr_ref
    user_audio_dur = len(y_user) / sr_user
    
    N_ref = len(raw_ref_pitch)
    stitched_user_pitch = np.zeros(N_ref)
    
    ref_text = [normalize_word(w['word']) for w in ref_words_json]
    user_text = [w['word'] for w in user_words_info]
    matcher = difflib.SequenceMatcher(None, ref_text, user_text)
    
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            for k, l in zip(range(i1, i2), range(j1, j2)):
                r_word = ref_words_json[k]
                u_word = user_words_info[l]
                
                if 'start_time' not in r_word: continue
                
                r_pitch_chunk = get_local_chunk(raw_ref_pitch, r_word['start_time'], r_word['end_time'], ref_audio_dur)
                u_pitch_chunk = get_local_chunk(raw_user_pitch, u_word['start'], u_word['end'], user_audio_dur)
                aligned_u_pitch = equalize_local_chunk(u_pitch_chunk, r_pitch_chunk)
                
                start_idx = int((r_word['start_time'] / ref_audio_dur) * N_ref)
                max_idx = min(start_idx + len(aligned_u_pitch), N_ref)
                fill_len = max_idx - start_idx
                
                if fill_len > 0:
                    stitched_user_pitch[start_idx:max_idx] = aligned_u_pitch[:fill_len]

    # CHỈ GỌI HÀM CỬA SỔ TRƯỢT 1 LẦN VÀ TÁI SỬ DỤNG
    running_r, w_starts, w_ends = dynamic_sliding_window_pearson(stitched_user_pitch, raw_ref_pitch, num_words=len(ref_words_json))
    metrics = summarize_pearson(running_r)
    metrics['pitch_point_counts'] = {
        'sarah_before_resize': len(raw_ref_pitch),
        'voice_before_resize': len(raw_user_pitch),
        'sarah_after_resize': len(raw_ref_pitch),
        'voice_after_resize': len(stitched_user_pitch),
    }
    
    if len(running_r) > 0:
        t_starts = (w_starts / N_ref) * ref_audio_dur
        t_ends = (w_ends / N_ref) * ref_audio_dur
        
    # VẼ BIỂU ĐỒ
    fig, axs = plt.subplots(4, 1, figsize=(15, 20))
    
    axs[0].fill_between(np.linspace(0, ref_audio_dur, len(y_ref)), y_ref, color='black', alpha=0.8)
    axs[0].twinx().plot(np.linspace(0, ref_audio_dur, N_ref), np.where(raw_ref_pitch==0, np.nan, raw_ref_pitch), color='blue', linewidth=2.5)
    axs[0].set_title(f'1. Sóng âm và Cao độ - Sarah | {len(raw_ref_pitch)} điểm cao độ')
    
    axs[1].fill_between(np.linspace(0, user_audio_dur, len(y_user)), y_user, color='dimgray', alpha=0.8)
    axs[1].twinx().plot(np.linspace(0, user_audio_dur, len(raw_user_pitch)), np.where(raw_user_pitch==0, np.nan, raw_user_pitch), color='orange', linewidth=2.5)
    axs[1].set_title(f'2. Sóng âm và Cao độ - {user_label} | {len(raw_user_pitch)} điểm cao độ')
    
    norm_x = np.linspace(0, ref_audio_dur, N_ref) 
    
    axs[2].plot(norm_x, np.where(raw_ref_pitch > 0, raw_ref_pitch, np.nan), color='blue', label='Mẫu (Hz)')
    axs[2].plot(norm_x, np.where(stitched_user_pitch > 0, stitched_user_pitch, np.nan), color='green', label='Người đọc đã Resize (Hz)')
    axs[2].set_title(f'3. Đối chiếu CAO ĐỘ (Đã đồng bộ độ dài) | Sarah: {N_ref} điểm | {user_label}: {len(stitched_user_pitch)} điểm')
    axs[2].set_ylabel("Tần số (Hz)")
    axs[2].legend(loc='upper right')
    
    if len(running_r) > 0:
        axs[3].axhline(0.3, color='red', linestyle='--', linewidth=1.5, label='Ngưỡng lỗi (r < 0.3)')
        added_valid_label = False
        added_nan_label = False
        
        for s, e, r in zip(t_starts, t_ends, running_r):
            if not np.isnan(r):
                label = 'Hệ số Pearson' if not added_valid_label else ""
                axs[3].plot([s, e], [r, r], color='purple', linewidth=3, label=label)
                added_valid_label = True
                if r < 0.3:
                    axs[3].fill_between([s, e], r, 0.3, color='red', alpha=0.2)
            else:
                label = 'NaN (Khoảng lặng)' if not added_nan_label else ""
                axs[3].plot([s, e], [0, 0], color='gray', linewidth=3, linestyle=':', label=label)
                axs[3].text(s + (e-s)/2, 0.08, "NaN", color='gray', ha='center', fontsize=9, fontweight='bold')
                added_nan_label = True
                
    axs[3].set_title('4. KIỂM CHỨNG CHÉO: Cửa sổ trượt (Minh họa theo khối thời gian)')
    axs[3].set_ylim(-1.1, 1.1)
    axs[3].legend(loc='upper right')
    
    for k, w in enumerate(ref_words_json):
        if 'start_time' not in w: continue
        mid_x = w['start_time'] + (w['end_time'] - w['start_time']) / 2
        
        for ax in [axs[0], axs[2], axs[3]]: 
            ax.axvline(x=w['start_time'], color='gray', linestyle=':', alpha=0.5)
            ax.axvline(x=w['end_time'], color='gray', linestyle=':', alpha=0.5)
            
        for ax in [axs[2], axs[3]]:
            y_pos_word = ax.get_ylim()[0] + (ax.get_ylim()[1] - ax.get_ylim()[0]) * 0.1
            ax.text(mid_x, y_pos_word, w['word'].upper(), ha='center', fontsize=9, rotation=25, fontweight='bold' if w.get('is_stressed') else 'normal', color='crimson' if w.get('is_stressed') else 'black')
            
    axs[0].set_xlim(0, ref_audio_dur)
    axs[1].set_xlim(0, user_audio_dur)
    for ax in axs[2:]:
        ax.set_xlim(0, ref_audio_dur)
    if sentence:
        positions = [str(i) for i, word in enumerate(ref_words_json, 1) if word.get('is_stressed')]
        fig.suptitle(sentence + f'\nSarah vs {user_label} | {len(positions)} từ nhấn: {", ".join(positions)} (đỏ đậm)', fontsize=16)
            
    plt.tight_layout(rect=(0, 0, 1, 0.95) if sentence else (0, 0, 1, 1))
    plt.savefig(output_file, dpi=300)
    plt.close(fig)
    return metrics
