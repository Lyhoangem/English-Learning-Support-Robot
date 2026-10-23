# So sánh 100 câu với các giọng US nam

Chạy `python main.py`. Mặc định dùng 100 câu tô màu trong workbook ở `data/`.
Sarah là giọng tham chiếu. Giọng nam: Adam, Echo, Eric, Fenrir, Liam,
Michael, Onyx, Puck, Santa.

`mean_r` của từng câu là trung bình cửa sổ Pearson hợp lệ: r <= 0 thành 0,
NaN bị loại; không có dữ liệu hợp lệ thì null. `sentence_counts_mean_r_gte`
đếm cộng dồn số câu đạt >= 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3.
Câu lỗi/null không được tính đạt ngưỡng. Không tính tổng hoặc trung bình r toàn bộ câu.

Đường dẫn từ gốc project:

- `audio/<voice>/<topic>/<STT>.wav`: audio và JSON metadata bên cạnh.
- `images/<voice>/<topic>/<STT>.png`: biểu đồ Sarah so với giọng nam.
- `output/highlighted/trimmed_audio/<voice>/<topic>/<STT>.wav`: audio trim.
- `output/highlighted/<voice>/report.json`: kết quả từng câu.
- `output/highlighted/<voice>/summary.json`: thống kê ngưỡng.
- `output/highlighted/<voice>/errors.json`: câu lỗi.
- `output/highlighted/summary.json`: tổng hợp các giọng.
- `output/highlighted/sentences.json`: câu và vị trí nhấn từ Excel.

Praat và Whisper base cùng đọc WAV đã trim (top_db=30). SequenceMatcher
căn chỉnh từ nhận diện với từ Excel. Không gán timestamp giả cho từ không khớp.
Vị trí nhấn dùng để hiển thị trên biểu đồ.

```powershell
python -m pip install -r requirements.txt
python main.py
python main.py --voices puck santa --skip-generation --resume
python main.py --audio-dir audio --images-dir images
```

`--resume` dùng lại câu có báo cáo thành công và có ảnh, xử lý lại câu lỗi/thiếu ảnh.
`--skip-generation` yêu cầu audio và metadata đã tồn tại.
`--audio-dir`, `--images-dir` đổi thư mục audio/ảnh; `--output-dir` đổi thư mục JSON.
`--limit 0` chọn tất cả câu tô màu. `--verbose` in mốc pitch.
Model lưu trong `.cache`. Báo cáo cập nhật sau mỗi câu; mã thoát 1 nếu có lỗi.
Chạy riêng một số giọng vẫn giữ thống kê các giọng khác.
