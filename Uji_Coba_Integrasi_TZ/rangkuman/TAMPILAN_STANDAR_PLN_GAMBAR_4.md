# Penerapan Tata Letak Layar OLED 128x32 Sesuai Standar Layar Meter PLN (Gambar 4)

Dokumen ini mencatat penyesuaian antarmuka tampilan grafis pada layar **OLED SSD1306 128x32** milik **E3** agar mengemulasikan dan memenuhi spesifikasi resmi **Standar Layar Tampilan Smart Meter PLN (SPLN / Pusertif Gambar 4)**.

---

## 1. Analisis Spesifikasi Standar (Gambar 4)

Berdasarkan dokumen teknis yang diberikan:
* **Baris Pertama (Simbol & Kode):**
  * `NNN`: Respon alarm (`OK ` jika normal, `!` jika ada sabotase/tamper).
  * `[B]`: Kotak status baterai RTC.
  * `<-`: Arah energi aktif terbalik (*reverse power*).
  * `L123`: Indikator fasa tegangan (L1, L2, L3).
  * `I123`: Indikator fasa arus (I1, I2, I3).
  * `Kode`: Kode register OBIS penagihan (misal `32.07`, `01.08`).
* **Baris Kedua (Teks & Angka Utama):**
  * `zz`: Indeks urutan *scrolling* carousel (`01`, `02`, ..., `14`) di sisi kiri.
  * Angka Utama: Karakter numerik besar (tinggi 18 pixel tebal, mirip 7-segment meteran).
  * Satuan: Satuan unit teknik di sisi kanan (`V`, `A`, `kW`, `kWh`, `Hz`, dll.).

---

## 2. Pemetaan Koordinat Pixel pada Layar OLED 128x32

```
    0                    Lebar Pixel = 128 px                      127
  0 +----------------------------------------------------------------+
    | OK[B] L123 I123 32.07                                          | <- Baris 1: Font_6x8 (Y: 0..8)
 10 | - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  | <- Garis pemisah putus-putus
    |                                                                |
    | 02               2 3 0 . 0                                   V | <- Baris 2: zz (Font_6x8)
    |                                                                |             Nilai (Font_11x18)
 31 +----------------------------------------------------------------+             Satuan (Font_6x8)
```

---

## 3. Berkas yang Diubah

1. **[`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/inc/display.h):**
   * Menambahkan tipe data struktur standar:
     ```c
     typedef struct {
         char header_symbols[24];  /* Baris 1: Simbol & Kode OBIS (Font 6x8) */
         char scroll_index_zz[6];  /* Baris 2 Kiri: zz urutan scrolling */
         char main_value[16];      /* Baris 2 Tengah: Nilai angka utama besar */
         char unit[10];            /* Baris 2 Kanan: Satuan */
         bool is_large_font;       /* true jika menggunakan Font 11x18 */
     } display_spln_frame_t;

     void display_render_spln_frame(const display_context_t *ctx, display_spln_frame_t *frame);
     ```

2. **[`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display.c):**
   * Mengimplementasikan fungsi `display_render_spln_frame()` yang memetakan ke-14 halaman carousel ke indeks `zz`, kode OBIS standar, nilai pengukuran dari E1, dan satuan resminya.

3. **[`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c):**
   * Memperbarui task FreeRTOS `task_oled128x32_carousel` untuk merender:
     * Baris 1: `header_symbols` dengan `Font_6x8` di $Y=0$.
     * Garis pemisah titik-titik halus di $Y=10$.
     * Baris 2 Kiri: `scroll_index_zz` dengan `Font_6x8` di $Y=22$.
     * Baris 2 Tengah: `main_value` dengan **`Font_11x18`** di $Y=13$ (tinggi 18 pixel tebal).
     * Baris 2 Kanan: `unit` dengan `Font_6x8` di $Y=22$.

---

## 4. Daftar 14 Halaman Carousel Tampilan Baru

| zz | Halaman Parameter | Baris 1 (Simbol & Kode) | Baris 2 (zz, Nilai Besar, Satuan) |
|---|---|---|---|
| **01** | ID Pelanggan | `OK[B] L123 I123 96.01` | `01  530000000001` |
| **02** | Tegangan Fasa R / L1 | `OK[B] L123 I123 32.07` | `02    230.0   V` |
| **03** | Tegangan Fasa S / L2 | `OK[B] L123 I123 52.07` | `03    230.0   V` |
| **04** | Tegangan Fasa T / L3 | `OK[B] L123 I123 72.07` | `04    230.0   V` |
| **05** | Arus Fasa R / L1 | `OK[B] L123 I123 31.07` | `05    11.95   A` |
| **06** | Arus Fasa S / L2 | `OK[B] L123 I123 51.07` | `06    11.95   A` |
| **07** | Arus Fasa T / L3 | `OK[B] L123 I123 71.07` | `07    11.95   A` |
| **08** | Arus Netral N | `OK[B] L123 I123 91.07` | `08     2.39   A` |
| **09** | Daya Aktif Total | `OK[B] L123 I123 01.07` | `09    6.898  kW` |
| **10** | Daya Reaktif Total | `OK[B] L123 I123 03.07` | `10    0.000  kvar` |
| **11** | Daya Semu Total | `OK[B] L123 I123 09.07` | `11    6.898  kVA` |
| **12** | Faktor Daya Total | `OK[B] L123 I123 13.07` | `12    1.000  PF` |
| **13** | Frekuensi Jaringan | `OK[B] L123 I123 14.07` | `13    50.00  Hz` |
| **14** | Total Energi Aktif | `OK[B] L123 I123 01.08` | `14   125.43  kWh` |

*Catatan: Jika terjadi event sabotase (tombol PC13 ditekan), simbol `OK` di kiri atas otomatis berubah menjadi tanda `!` (indikasi ketidaknormalan / respon alarm `NNN`).*

---

## 5. Hasil Verifikasi Kompilasi Target

```text
[1/2] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/firmware/app/display/src/display.c.obj
[2/2] Linking C executable Uji_Coba_Integrasi.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       80352 B       768 KB     10.22%
             ROM:       69400 B         2 MB      3.31%
           SRAM4:           0 B        16 KB      0.00%
Build: 0 Error, 0 Warning (100% Bersih)
```

