import os
import sys
import re
import glob
import json
import numpy as np
import pandas as pd
import soundfile as sf
from kokoro import KPipeline

os.environ["PYTHONIOENCODING"] = "utf-8"
if sys.stdout.encoding != "utf-8":
    sys.stdout.reconfigure(encoding="utf-8")
if sys.stderr.encoding != "utf-8":
    sys.stderr.reconfigure(encoding="utf-8")
# ==============================================================================
# CẤU HÌNH HỆ THỐNG
# ==============================================================================
INPUT_DIR = "data"       # Thư mục chứa 10 file .xlsx
OUTPUT_DIR = "output"    # Thư mục đầu ra chứa Audio và JSON
SAMPLE_RATE = 24000      # Tần số lấy mẫu chuẩn của Kokoro (24kHz)
SAVE_INDIVIDUAL_SENTENCES = True  # True: lưu thêm từng câu lẻ theo số thứ tự (001.wav...)

# 10 giọng đọc chuẩn của thư viện Kokoro (Kokoro-82M):
# - 9 giọng nam Mỹ (am_adam, am_echo, am_eric, am_fenrir, am_liam, am_michael, am_onyx, am_puck, am_santa)
# - 1 giọng nữ Mỹ (af_sarah)
VOICE_MAP = {
    "adam": "am_adam",
    "echo": "am_echo",
    "eric": "am_eric",
    "fenrir": "am_fenrir",
    "liam": "am_liam",
    "michael": "am_michael",
    "onyx": "am_onyx",
    "puck": "am_puck",
    "santa": "am_santa",
    "sarah": "af_sarah",
}

EXCEL_FILES = [
    "Set 100_Art_Band_2_3_revised.xlsx",
    "Set 100_Culture_Society_Band_2_3_revised.xlsx",
    "Set 100_Education_Band_2_3_revised.xlsx",
    "Set 100_Environment_Band_2_3_revised.xlsx",
    "Set 100_Feelings_Band_2_3_revised.xlsx",
    "Set 100_Medical_Band_2_3_revised.xlsx",
    "Set 100_Nature_Band_2_3_revised.xlsx",
    "Set 100_Science_Technology_Band_2_3_revised.xlsx",
    "Set 100_Space_Band_2_3_revised.xlsx",
    "Set 100_Sports_Band_2_3_revised.xlsx",
]


def extract_topic_name(filename: str) -> str:
    """Trích xuất tên chủ đề từ tên file .xlsx (VD: Set 100_Art_Band_2_3_revised.xlsx -> Art)."""
    base = os.path.basename(filename)
    match = re.search(r"Set 100_(.+?)_Band_2_3_revised\.xlsx", base, re.IGNORECASE)
    if match:
        return match.group(1)
    return os.path.splitext(base)[0]


def parse_excel_file(filepath: str) -> list:
    """Đọc file Excel và trả về danh sách các câu kèm thông tin vị trí từ nhấn."""
    xls = pd.ExcelFile(filepath)
    df = pd.read_excel(filepath, sheet_name=xls.sheet_names[0])

    records = []
    for _, row in df.iterrows():
        if pd.isna(row.get("Ví dụ mẫu siêu đơn giản (Band 2.0-3.0)")):
            continue

        stt = int(row["STT"])
        sentence = str(row["Ví dụ mẫu siêu đơn giản (Band 2.0-3.0)"]).strip()
        total_stressed = int(row["Tổng số từ nhấn"]) if pd.notna(row.get("Tổng số từ nhấn")) else 0

        # Lấy vị trí từ nhấn (hỗ trợ cả dạng số liền 1234 lẫn chuỗi phân tách dấu phẩy 1,2,4,6)
        raw_pos = str(row.get("Vị trí nhấn", "")).strip()
        positions = [int(d) for d in re.findall(r"\d", raw_pos)]

        # Lấy danh sách từ được nhấn tương ứng
        words_in_sentence = sentence.split()
        stressed_words = []
        for pos in positions:
            val = row.get(pos, row.get(str(pos), None))
            if pd.notna(val):
                word_clean = str(val).strip().rstrip(".,!?;:")
            elif 1 <= pos <= len(words_in_sentence):
                word_clean = words_in_sentence[pos - 1].rstrip(".,!?;:")
            else:
                word_clean = ""
            stressed_words.append(word_clean)

        records.append({
            "stt": stt,
            "sentence": sentence,
            "tong_so_tu_nhan": total_stressed,
            "cac_tu_duoc_nhan": stressed_words,
            "vi_tri_nhan": positions,
        })

    return records


def synthesize_sentence(pipeline: KPipeline, text: str, voice_id: str) -> np.ndarray:
    """Sinh audio waveform cho 1 câu sử dụng mô hình Kokoro."""
    generator = pipeline(text, voice=voice_id, speed=1.0, split_pattern=r"\n+")
    audio_segments = []
    for _, _, audio in generator:
        if audio is not None:
            if hasattr(audio, "cpu"):
                audio = audio.cpu().numpy()
            audio_segments.append(audio)

    if not audio_segments:
        return np.zeros(0, dtype=np.float32)
    return np.concatenate(audio_segments)


def run_pipeline():
    audio_root = os.path.join(OUTPUT_DIR, "Audio")
    json_root = os.path.join(OUTPUT_DIR, "JSON")
    os.makedirs(audio_root, exist_ok=True)
    os.makedirs(json_root, exist_ok=True)

    print("=== BƯỚC 1: Đọc 10 file Excel và xuất file JSON ===")
    all_topics_data = {}
    for fname in EXCEL_FILES:
        fpath = os.path.join(INPUT_DIR, fname)
        if not os.path.exists(fpath):
            print(f"[CẢNH BÁO] Không tìm thấy file: {fpath}")
            continue

        topic = extract_topic_name(fname)
        records = parse_excel_file(fpath)
        all_topics_data[topic] = records

        # Lưu file JSON riêng cho từng chủ đề
        topic_json_path = os.path.join(json_root, f"{topic}.json")
        with open(topic_json_path, "w", encoding="utf-8") as f:
            json.dump(records, f, ensure_ascii=False, indent=2)
        print(f"  -> Đã tạo JSON: {topic_json_path} ({len(records)} câu)")

    # Lưu file JSON tổng hợp
    master_json_path = os.path.join(json_root, "all_sentences.json")
    with open(master_json_path, "w", encoding="utf-8") as f:
        json.dump(all_topics_data, f, ensure_ascii=False, indent=2)
    print(f"  -> Đã tạo file JSON tổng hợp: {master_json_path}\n")

    print("=== BƯỚC 2: Khởi tạo mô hình Kokoro TTS Pipeline ===")
    pipeline = KPipeline(lang_code="a")  # 'a' đại diện cho American English
    silence_padding = np.zeros(int(SAMPLE_RATE * 0.4), dtype=np.float32)  # Khoảng lặng 0.4s giữa các câu

    print("=== BƯỚC 3: Tiến hành sinh Audio cho 10 giọng đọc x 10 chủ đề ===")
    for voice_short_name, kokoro_voice_id in VOICE_MAP.items():
        voice_dir = os.path.join(audio_root, voice_short_name)
        os.makedirs(voice_dir, exist_ok=True)
        print(f"\n[Voice: {voice_short_name} -> {kokoro_voice_id}]")

        for topic, records in all_topics_data.items():
            topic_dir = os.path.join(voice_dir, topic)
            if SAVE_INDIVIDUAL_SENTENCES:
                os.makedirs(topic_dir, exist_ok=True)

            combined_topic_audio = []

            for item in records:
                stt = item["stt"]
                sentence = item["sentence"]

                audio_np = synthesize_sentence(pipeline, sentence, kokoro_voice_id)
                if len(audio_np) == 0:
                    continue

                # Lưu từng file audio lẻ theo câu nếu bật cờ cấu hình
                if SAVE_INDIVIDUAL_SENTENCES:
                    sentence_wav_path = os.path.join(topic_dir, f"{stt:03d}.wav")
                    sf.write(sentence_wav_path, audio_np, SAMPLE_RATE)

                combined_topic_audio.append(audio_np)
                combined_topic_audio.append(silence_padding)

            # Gộp toàn bộ câu của chủ đề thành 1 file wav duy nhất
            if combined_topic_audio:
                merged_audio = np.concatenate(combined_topic_audio)
                topic_wav_path = os.path.join(voice_dir, f"{topic}.wav")
                sf.write(topic_wav_path, merged_audio, SAMPLE_RATE)
                print(f"  + Hoàn tất chủ đề '{topic}': {topic_wav_path}")

    print("\n🎉 Hoàn tất toàn bộ quá trình xử lý! Kiểm tra thư mục 'output' để xem kết quả.")


if __name__ == "__main__":
    run_pipeline()