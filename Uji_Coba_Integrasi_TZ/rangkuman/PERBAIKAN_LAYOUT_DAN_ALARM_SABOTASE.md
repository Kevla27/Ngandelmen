# Perbaikan Layout Layar OLED 128x32 & Indikator Alarm Sabotase (SPLN Gambar 4)

Dokumen ini menjelaskan akar masalah mengapa alarm sebelumnya tidak terlihat di layar OLED SSD1306, rincian perbaikan tata letak (*layout*) sesuai **Standar Layar Meter PLN (SPLN / Pusertif Gambar 4)**, serta penambahan indikator visual alarm sabotase (*tamper*) berdaya kontras tinggi.

---

## 1. Akar Masalah: Mengapa Alarm Sabotase Tidak Terlihat Sama Sekali?

Setelah dilakukan pelacakan menyeluruh pada *codebase*, ditemukan **2 penyebab utama**:

1. **Variabel Status Alarm Terputus dari Task Display:**
   * Tombol *user* **PC13** pada board Nucleo-U575ZI-Q memang memicu interupsi `HAL_GPIO_EXTI_Falling_Callback()`, namun hanya mengirim pesan ke *tamper queue*.
   * Di sisi lain, task penampil layar `task_oled128x32_carousel` pada [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c) menjalankan *loop* sendiri dengan variabel `disp_ctx.alarm_icon_active` yang **selalu bernilai `false` secara permanen**.
   * Akibatnya, bagaimanapun tombol ditekan, status alarm pada display **tidak pernah berubah menjadi aktif**.

2. **Desain Visual Sebelumnya Terlalu Kecil (Tenggelam):**
   * Sebelumnya, indikator alarm hanya berupa penggantian 1 karakter kecil `!` menggantikan `OK` (`ctx->alarm_icon_active ? "!" : "OK"`).
   * Pada layar mini OLED 0.91/0.96 inch (128x32 pixel), karakter `!` hanya berukuran lebar 1 pixel dengan tinggi 5 pixel font 6x8, sehingga secara fisik hampir mustahil dilihat mata manusia saat digabung dengan teks padat lainnya.

---

## 2. Solusi & Desain Tampilan Baru (Sesuai SPLN Gambar 4)

Berdasarkan spesifikasi resmi **SPLN Gambar 4**:
* `NNN` = Respon Alarm (posisi pojok kiri atas).
* `[B]` = Status Baterai.
* `<-` = Indikasi energi aktif terbalik.
* `🖐️ / !` = Indikasi Ketidaknormalan (Abnormality / Tamper).
* `XXX` = Tampilan Fasa Tegangan (`L123`).
* `YYY` = Tampilan Fasa Arus (`I123`).
* `Kode` = Kode Register OBIS (posisi pojok kanan atas).
* `zz` = Indeks urutan tampilan (*scrolling* carousel).
* `Angka Utama` = Karakter numerik besar 7-segment / tebal (**`Font_11x18`**).
* `Satuan` = Satuan besaran listrik (`V`, `A`, `kW`, `kWh`, `Hz`, dll.) atau kode respon sabotase (`E01`).

---

## 3. Komparasi Visual: Kondisi Normal vs Kondisi Alarm Sabotase

### A. Kondisi NORMAL (Tidak Ada Sabotase)
* **Baris 1 ($Y = 0..8$):** Teks putih normal pada latar hitam:
  `OK` di $X=2$, `[B]` di $X=26$, `L123` di $X=46$, `I123` di $X=72$, dan Kode OBIS (misal `32.07`) di $X=98$.
* **Garis Pemisah ($Y = 10$):** Garis titik-titik putus halus (*subtle dotted line*).
* **Baris 2 ($Y = 13..31$):**
  * Sisi Kiri ($X = 1, Y = 22$): Urutan scroll `zz` (misal `02`).
  * Tengah ($Y = 13..31$): Angka besar `230.0` tebal (**Font 11x18**).
  * Sisi Kanan ($X = 120, Y = 22$): Satuan `V`.

```text
+-------------------------------------------------------------+
| OK    [B]   L123    I123                              32.07 | <- Baris 1 (Font 6x8)
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - | <- Garis putus-putus (Y=10)
|                                                             |
| 02               2 3 0 . 0                                V | <- zz | Nilai Besar (11x18) | Satuan
+-------------------------------------------------------------+
```

---

### B. Kondisi ALARM SABOTASE AKTIF (Tombol PC13 Ditekan)
Saat terdeteksi pembukaan tutup meter (*case open* / tombol PC13 ditekan):
1. **Banner Inverted Menyala Terang (*Glowing Inverted Badge*):**
   * Di pojok kiri atas ($X = 0..25, Y = 0..8$), digambar kotak putih solid dengan teks hitam pekat:
     **`[!ALM]`** (menggabungkan tanda seru ketidaknormalan `!` dan respon alarm `ALM`).
   * Pada layar OLED hitam, kotak putih ini menyala terang (*high-contrast*) seperti lampu indikator bahaya fisik.
2. **Garis Pemisah Menjadi Garis Peringatan Padat (*Solid Alert Bar*):**
   * Garis pada $Y = 10$ berubah seketika dari putus-putus menjadi **garis putih solid tebal tanpa putus** membentang dari pixel 0 hingga 127.
3. **Respon Instan (< 100 ms) ke Halaman Khusus Sabotase:**
   * Begitu tombol PC13 ditekan, layar tidak menunggu delay 2 detik, melainkan **langsung melompat ke halaman darurat**:
     * `zz`: **`AL`**
     * Nilai Utama Tengah: **`SABOTASE`** (Font 7x10 tebal di tengah)
     * Kode Error Kanan: **`E01`** (Error 01: Tutup meter dibuka / *case open*)
     * Kode OBIS Kanan Atas: **`96.50`** (OBIS Tamper / Sabotase)
4. **Proteksi Banner Menyeluruh:**
   * Saat carousel berputar ke halaman parameter lain (Tegangan, Arus, Daya, Frekuensi, Total Energi), banner menyala **`[!ALM]`** dan **garis padat peringatan** tetap terkunci aktif di bagian atas agar teknisi tetap menyadari adanya status sabotase.
5. **Pemulihan Mudah (*Toggle Bench Test*):**
   * Menekan tombol PC13 kembali akan mengembalikan status menjadi **NORMAL** seketika, mengembalikan banner ke `OK`, dan menghapus halaman sabotase dari antrean carousel.

```text
+-------------------------------------------------------------+
| ##### [B]   L123    I123                              96.50 | <- [!ALM] INVERTED BADGE (Putih Terang)
| !ALM#                                                       |
|=============================================================| <- GARIS TEBAL SOLID (Alert Line)
|                                                             |
| AL               S A B O T A S E                        E01 | <- zz="AL" | "SABOTASE" | Error "E01"
+-------------------------------------------------------------+
```

---

## 4. Berkas yang Diubah & Rincian Implementasi

| No | Berkas | Perubahan |
|---|---|---|
| 1 | [`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian\Uji_Coba_Integrasi\firmware\app\display\inc\display.h) | Menambahkan enum `DISP_PAGE_TAMPER_ALARM`, serta field `is_alarm_active`, `alarm_code[6]`, dan `obis_code[10]` pada `display_spln_frame_t`. |
| 2 | [`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian\Uji_Coba_Integrasi\firmware\app\display\src\display.c) | Mengimplementasikan rendering halaman `DISP_PAGE_TAMPER_ALARM` (`AL`, `SABOTASE`, `E01`, OBIS `96.50`), serta otomatis melewati (*skip*) halaman alarm saat kondisi normal. |
| 3 | [`firmware/app/fsm/inc/rtos_tasks.h`](file:///g:/PPPP/Day_Sekian\Uji_Coba_Integrasi\firmware\app\fsm\inc\rtos_tasks.h) | Menambahkan fungsi aman interupsi `rtos_queue_send_tamper_event_from_isr()`. |
| 4 | [`firmware/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/Day_Sekian\Uji_Coba_Integrasi\firmware\app\fsm\src\rtos_tasks.c) | Implementasi `rtos_queue_send_tamper_event_from_isr()` menggunakan FreeRTOS `xQueueSendFromISR` dan menambahkan monitor log darurat pada `task_tamper_emergency_entry`. |
| 5 | [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian\Uji_Coba_Integrasi\Core\Src\main.c) | Menghubungkan tombol PC13 EXTI falling interrupt dengan *debounce* 250ms, *toggle* `g_tamper_alarm_active`, memicu refresh layar instan (< 100 ms), serta merender inverted badge `[!ALM]` dan garis solid di OLED. |

---

## 5. Hasil Verifikasi Pengujian

1. **Unit Test Layar SPLN & Alarm Sabotase (`tests/test_spln_display_tamper.exe`):**
   ```text
   ====================================================================
     TEST VERIFIKASI TATA LETAK SPLN GAMBAR 4 & ALARM SABOTASE OLED     
   ====================================================================
   [UJI 1] Verifikasi Tampilan Normal (alarm_icon_active = false)...
     > Page 01 (IDPEL): OK | zz: 01 | Val: 530000000001 | OBIS: 96.01 [PASS]
     > Page 02 (Volt R): OK | zz: 02 | Val: 230.0 V | OBIS: 32.07 [PASS]
     > Carousel Skip Test (Normal tidak menampilkan halaman sabotase): [PASS]
   
   [UJI 2] Verifikasi Tampilan Alarm Sabotase (alarm_icon_active = true)...
     > Banner Alarm Tersemat di Page 02: Respon Alarm = '!ALM' [PASS]
     > Halaman Sabotase E01: Header: '!ALM' | zz: 'AL' | Teks: 'SABOTASE' | Kode: 'E01' | OBIS: '96.50' [PASS]
     > Carousel Inclusion Test (Halaman sabotase masuk dalam siklus carousel): [PASS]
     > Carousel Loop Completion Test (Kembali ke Page 01): [PASS]
   
   [UJI 3] Verifikasi Pemulihan Sabotase (Kembali Normal)...
     > Status Layar Berhasil Pulih ke OK [PASS]
   
   HASIL: 100% SELURUH SPESIFIKASI LAYAR ALARM SPLN GAMBAR 4 LULUS!
   ```

2. **Kompilasi Firmware Target STM32U575 (`ninja -C build/Debug`):**
   ```text
   [8/8] Linking C executable Uji_Coba_Integrasi.elf
   Memory region         Used Size  Region Size  %age Used
                RAM:       80368 B       768 KB     10.22%
                ROM:       70680 B         2 MB      3.37%
              SRAM4:           0 B        16 KB      0.00%
   Build: 0 Error, 0 Warning (100% Bersih)
   ```

