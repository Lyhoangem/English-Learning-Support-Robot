import os
import sys
from matplotlib import text
import re
import librosa
import time
import numpy as np
import soundfile as sf
from faster_whisper import WhisperModel

os.environ["PYTHONIOENCODING"] = "utf-8"
if hasattr(sys.stdout, "reconfigure") and sys.stdout.encoding != "utf-8":
    sys.stdout.reconfigure(encoding="utf-8")
# ==============================================================================
# CẤU HÌNH
# ==============================================================================
WAV_FILE_PATH = "output/Audio/adam/Art/050.wav"  # Đường dẫn file .wav cần kiểm tra
TXT_FILE_PATH = "sentences.txt"                   # File txt chứa danh sách câu

def normalize_text(text: str) -> str:
    """Chuẩn hóa câu để tra cứu chính xác tuyệt đối (bỏ dấu câu, đưa về chữ thường)."""
    text = text.lower().strip()
    # Loại bỏ dấu câu (như ., ! ? , v.v.)
    text = re.sub(r"[^a-z0-9\s]", "", text)
    # Thu gọn khoảng trắng thừa nếu có
    return " ".join(text.split())

def hash_func(key_string: str) -> int:
    """Hàm băm tính toán vị trí trong mảng 1 chiều (hash table)."""
    hash_value = 0 
    for char in key_string:
        hash_value = (hash_value * 31 + ord(char)) % 1000007
    return hash_value

def load_sentence(txt_path: str) -> list:
    """Đọc file txt và lưu vào mảng 1 chiều (hash table) để tra cứu O(1)."""
    hash_table = [None] * 1000007
    
    if not os.path.exists(txt_path):
        print(f"Lỗi: Không tìm thấy file {txt_path}!")
        return hash_table

    with open(txt_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split("|")
            if len(parts) >= 3:
                topic = parts[0].strip()
                stt = int(parts[1].strip())
                sentence = parts[2].strip()
                
                # Chuẩn hóa câu để tra cứu chính xác tuyệt đối
                norm_key = normalize_text(sentence)
                index = hash_func(norm_key)
                
                # Thêm vào ngăn tương ứng trong bảng băm
                if hash_table[index] is None:
                    hash_table[index] = []

                hash_table[index] = {
                    "topic": topic,
                    "stt": stt,
                    "sentence": sentence,
                    "norm_key": norm_key
                }
    return hash_table

def search_sentence(hash_table: list, transcribed_text: str):
    """Tìm kiếm câu trong bảng băm."""
    norm_key = normalize_text(transcribed_text)
    index = hash_func(norm_key)
    if not hash_table[index]:
        return None
    return hash_table[index]

def load_audio_16khz(wav_path: str) -> np.ndarray:
    audio, sr = sf.read(wav_path, dtype="float32")
    if audio.ndim > 1:
        audio = audio.mean(axis=1)
    if sr != 16000:
        audio = librosa.resample(audio, orig_sr=sr, target_sr=16000)
    return audio

def recognize_audio(wav_path: str):
    if not os.path.exists(wav_path):
        print(f"Không tìm thấy file audio: {wav_path}")
        return

    t0 = time.perf_counter()
    
    # 1. Tải dữ liệu từ file txt
    database = load_sentence(TXT_FILE_PATH)
    if not database:
        return

    # 2. Khởi tạo mô hình Whisper (chạy offline, tự động tải model base.en lần đầu)
    model = WhisperModel("base.en", device="cpu", compute_type="int8", download_root="models")

    # 3. Đọc audio và nhận dạng text
    audio_16k = load_audio_16khz(wav_path)
    segments, _ = model.transcribe(
        audio_16k,
        language="en",
        beam_size=1,
        temperature=0.0,
        without_timestamps=True,
        vad_filter=False
    )
    transcribed_text = normalize_text(" ".join([segment.text for segment in segments]))
    # 4. Tra cứu chính xác tuyệt đối trong bảng băm O(1)
    result = search_sentence(database, transcribed_text)
    elapsed = time.perf_counter() - t0

    # 5. In kết quả ra ô output
    if result:
        print(f"topic: {result['topic']}")
        print(f"stt: {result['stt']}")
        print(f"sentence: {result['sentence']}")
    else:
        print(f"Không tìm thấy khớp chính xác cho văn bản nhận dạng: '{transcribed_text}'")
    
    print(f"(Thời gian xử lý: {elapsed:.3f}s)")

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else WAV_FILE_PATH
    recognize_audio(target)