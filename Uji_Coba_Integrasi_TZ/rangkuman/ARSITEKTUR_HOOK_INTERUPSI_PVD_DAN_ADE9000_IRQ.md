# Arsitektur Hook Interupsi Kritis: PVD (Last-Gasp Sequence) & ADE9000 Hardware IRQ (Instant Tamper)

**Dokumen Rekayasa Sistem E3**  
**Peran:** E3 Lead (*Application, Display SPLN D3.006 Gambar 4, Tamper Event Management & Safety*)  
**Kolaborator:** E1 (*Metrology & AFE*), E2 (*DLMS/COSEM Protocol*), E4 (*Hardware & PCB Layout*)  
**Target Hardware:** STM32U575VGT6 (Arm Cortex-M33, TrustZone) + Analog Devices ADE9000  
**Standar Acuan:** SPLN D3.006: 2021 (Tabel 6 & Butir 6.8), IEC 62053-22 Class 0.5S/0.2S, 26-Week Plan Rev D  
**Tanggal Rilis:** Oktober 2026  

---

## 1. Latar Belakang & Motivasi Arsitektur

Pada tahap pengembangan awal (*Weeks 1–7: Blind Development*), subsistem metrologi dan pembacaan sensor beroperasi menggunakan metode **Polling Periodik 1 Detik (1 Hz)** melalui `task_metrology_profiling`. Pendekatan polling periodik ini memadai untuk menghitung profil beban 15-menit dan rata-rata tegangan/arus RMS.

Namun, untuk mencapai kepatuhan penuh terhadap standar operasional meter industri (*industrial-grade polyphase AMI meter*), terdapat **dua celah rekayasa kritis** yang wajib ditangani menggunakan arsitektur berbasis interupsi perangkat keras (*hardware interrupts*):

1. **Integritas Data Saat Mati Lampu Mendadak (*Power-Fail Resilience*):**  
   Ketika tegangan jala-jala padam tiba-tiba, mikrokontroler akan mengalami *Brown-Out Reset* (BOR) dalam hitungan milidetik. Tanpa peringatan dini (*early warning*), penulisan data penagihan energi (kWh) dan log sabotase yang sedang berlangsung di memori Flash/FRAM dapat terpotong, berisiko menyebabkan kerusakan partisi memori (*flash memory corruption*) atau hilangnya transaksi terakhir.
2. **Latensi Deteksi Sabotase Fisik (*Sub-Cycle Fraud Detection*):**  
   Kecurangan seperti pembalikan arah arus (*reverse power*) atau pelepasan kawat netral (*neutral missing*) dapat merugikan utilitas secara instan. Menunggu siklus polling 1 detik terlalu lambat. AFE ADE9000 memiliki mesin deteksi analog yang mampu mendeteksi anomali dalam 1 siklus gelombang listrik ($20\text{ ms}$). Mesin ini memerlukan jalur interupsi langsung ke CPU.

---

## 2. Hook 1: PVD (*Programmable Voltage Detector*) — Protokol "Napas Terakhir" (*Last-Gasp Sequence*)

### 2.1. Konsep Fisik & Ambang Batas Tegangan STM32U575

Modul **PWR PVD** pada STM32U575 memantau rel tegangan catu daya $V_{DD}$ secara kontinu terhadap tingkat ambang batas (*threshold level*) yang dapat diprogram. 

```
Tegangan Vdd
 3.3 V ───┐ 
          │  [Listrik PLN Padam / Blackout Terjadi]
          │ 
 2.8 V ───┼────────────────────────────────────────► [ INTERUPSI PVD AKTIF (EXTI 16) ]
          │                                            │
          │   JENDELA WAKTU "LAST-GASP" (~10 - 30 ms) │  1. Matikan beban boros (OLED, Buzzer)
          │   Energi disuplai oleh sisa muatan         │  2. Flush Tamper Log FIFO ke Flash
          │   kapasitor/superkapasitor SMPS            │  3. Simpan state Relai & Billing Register
          │                                            │  4. Kunci memori non-volatile
          │                                            ▼
 1.9 V ───┼────────────────────────────────────────► [ HARDWARE BOR (Brown-Out Reset) ]
          │                                            CPU mati total secara aman & terkendali
 0.0 V ───┴────────────────────────────────────────► Mati Total
```

* **Brown-Out Reset (BOR):** Bekerja pada level $\approx 1.9\text{ V}$. BOR adalah sakelar pemutus keras; CPU langsung terhenti seketika tanpa sempat mengeksekusi instruksi penyimpanan.
* **PVD Threshold:** Dikonfigurasi pada **Level 5 (~2.8 V)** atau **Level 6 (~2.9 V)**. PVD memberikan jeda waktu kritis sebesar **$10\text{ ms} - 30\text{ ms}$** sebelum tegangan menyentuh ambang BOR.

---

### 2.2. Diagram Alur Eksekusi Protokol Last-Gasp (E3)

```
[ Tegangan Vdd anjlok di bawah 2.8 V ]
                  │
                  ▼ (EXTI Line 16)
         PVD_PVM_IRQHandler()
                  │
                  ▼
         HAL_PWR_PVDCallback()
                  │
  ┌───────────────┴────────────────────────┐
  ▼                                        ▼
[ 1. De-energisasi Beban ]        [ 2. Emergency Memory Flush ]
  • Matikan DC-DC OLED Display       • nvram_save_tamper_log_snapshot(&g_tamper_ctx)
  • Matikan GPIO Buzzer PB7          • Simpan status FSM Relai Beban
  • Matikan LED Indikator            • Simpan akumulator kWh/kVARh terakhir
  (Menghemat sisa arus kapasitor)    (Atomic Write ke Flash Bank 2 / FRAM)
                  │                                        │
                  └──────────────────┬─────────────────────┘
                                     │
                                     ▼
                     [ 3. Lock & Low-Power Standby ]
                       • Kunci Flash Write Protection
                       • Masuk ke while(1) menunggu BOR
```

---

### 2.3. Perancangan Antarmuka Perangkat Lunak (C API)

```c
/**
 * @file bsp_power_fail.h
 * @brief Antarmuka Penanganan Deteksi Penurunan Catu Daya (PVD Last-Gasp)
 */

#ifndef BSP_POWER_FAIL_H
#define BSP_POWER_FAIL_H

#include <stdbool.h>

/**
 * @brief Inisialisasi modul PVD STM32U575 pada level 2.8V dan registrasi interrupt EXTI16
 */
void bsp_pvd_init(void);

/**
 * @brief Callback darurat saat tegangan catu anjlok melewati ambang batas PVD
 */
void bsp_pvd_power_fail_emergency_handler(void);

#endif /* BSP_POWER_FAIL_H */
```

---

## 3. Hook 2: Interupsi Hardware ADE9000 (`IRQ0` & `IRQ1` via GPIO EXTI)

### 3.1. Anatomi Pin & Register Status ADE9000

IC Metrologi Analog Devices ADE9000 memiliki dua pin interupsi perangkat keras fisik (Kemasan 40-lead LFCSP):
* **Pin `IRQ0` (Pin 31, Active-Low Open-Drain):** Khusus untuk kejadian terkait perhitungan daya, energi, dan pembalikan arah arus (*reverse power*).
* **Pin `IRQ1` (Pin 32, Active-Low Open-Drain):** Khusus untuk kejadian terkait kualitas daya, hilang tegangan fase (*sag/swell*), dan anomali kawat netral (*missing neutral*).

Keduanya membutuhkan resistor *pull-up* eksternal $10\text{ k}\Omega$ ke rel $3.3\text{ V}$ dan dihubungkan ke pin input GPIO STM32U575 (misal `PB0` dan `PB1`).

```
┌─────────────────────────────────┐                 ┌─────────────────────────────────┐
│       ANALOG DEVICES ADE9000    │                 │      STM32U575VGT6 (E3 Core)    │
│                                 │                 │                                 │
│  [STATUS0: REVAP, REVBP, REVCP] │                 │                                 │
│  Pin 31 (IRQ0) ─────────────────┼──[ Pull-Up ]───►│ PB0 (EXTI Line 0 - Falling Edge)│
│                                 │      10 kΩ      │                                 │
│  [STATUS1: MISNEUT, ISUM, SAG]  │                 │                                 │
│  Pin 32 (IRQ1) ─────────────────┼──[ Pull-Up ]───►│ PB1 (EXTI Line 1 - Falling Edge)│
│                                 │      10 kΩ      │                                 │
│  SPI Bus (SCLK, MOSI, MISO, CS) ◄─────────────────► SPI1 / SPI2 Bus Master          │
└─────────────────────────────────┘                 └─────────────────────────────────┘
```

---

### 3.2. Pemetaan Deteksi Cepat Sabotase SPLN D3.006: 2021

| Register ADE9000 | Bit Flag | Waktu Respon | Jenis Sabotase Terdeteksi | Kode SPLN Tabel 6 | Respon Firmware E3 |
|---|---|---|---|---|---|
| **`STATUS0`** | `REVAP`, `REVBP`, `REVCP` | $\le 20\text{ ms}$ (1 siklus) | Aliran Daya Aktif Terbalik (*Reverse Power*) | **`ERR26`** (`REVERSE`) | Kirim `DLMS_TAMPER_REVERSE_CURRENT` ke Queue, refresh OLED ke `REVERSE ERR26`, Bunyikan Buzzer. |
| **`STATUS1`** | `MISNEUT` / `ISUM` | $\le 20\text{ ms}$ | Kawat Netral Hilang / Pembumian Arus | **`ERR23`** (`PERIKSA`) | Kirim `DLMS_TAMPER_MISSING_NEUTRAL` ke Queue, refresh OLED ke `PERIKSA ERR23`, Bunyikan Buzzer. |
| **`STATUS1`** | `SAG` / `VPEAK` | $\le 10\text{ ms}$ | Hilang Tegangan 1/2 Fase (*Phase Loss*) | **`ERR24`** (`PERIKSA`) | Kirim `DLMS_TAMPER_MISSING_VOLTAGE` ke Queue, Putus Relai Beban (*Trip*). |

---

### 3.3. Diagram Alur Eksekusi Interupsi ADE9000 ke RTOS Queue

```
[ Anomali Arus Balik / Netral terjadi pada Beban Listrik ]
                          │
                          ▼ (Deteksi DSP Analog ADE9000 < 20 ms)
[ Pin IRQ0 / IRQ1 ditarik ke LOW ]
                          │
                          ▼ (Falling Edge Interrupt)
[ EXTI0_IRQHandler() / EXTI1_IRQHandler() ]
                          │
                          ▼ (Konteks Interrupt ISR)
[ Baca Register STATUS0 / STATUS1 via Transaksi Cepat SPI ]
                          │
                          ▼
  ┌───────────────────────┴───────────────────────┐
  │ Bit REVAP/BP/CP = 1 ?                          │ Bit MISNEUT/ISUM = 1 ?
  ▼ (Ya: Reverse Power)                           ▼ (Ya: Missing Neutral)
tamper_event_msg_t msg = {                      tamper_event_msg_t msg = {
  .tamper_code = DLMS_TAMPER_REVERSE_CURRENT,     .tamper_code = DLMS_TAMPER_MISSING_NEUTRAL,
  .is_active = true,                              .is_active = true,
  .timestamp = now_sec                            .timestamp = now_sec
};                                              };
  │                                               │
  └───────────────────────┬───────────────────────┘
                          │
                          ▼
        rtos_queue_send_tamper_event_from_isr(&msg)
                          │
                          ▼ (Tulis balik 1 ke bit STATUS ADE9000 untuk Clear Flag)
        ADE9000_SPI_WriteRegister(ADE9000_STATUS0, bit_mask)
                          │
                          ▼ (FreeRTOS Task: task_dlms_entry & display_task)
      • Rekaman masuk ke Tamper Log FIFO (OBIS 0.0.99.98.0.255)
      • Counter bertambah pada OBIS 0.0.96.20.27.255 (Reverse) / 0.0.96.20.24.255 (Netral)
      • OLED seketika berganti ke layar peringatan alarm solid
      • Buzzer PB7 berbunyi dengan pola pulsa 200 ms
```

---

## 4. Matriks Perbandingan Karakteristik Kedua Hook

| Parameter Kinerja | Hook 1: PVD (*Last-Gasp Sequence*) | Hook 2: ADE9000 Hardware IRQ |
|---|---|---|
| **Asal Pemicu** | Internal mikrokontroler (Blok Power PWR STM32) | Eksternal IC AFE Metrologi (Analog DSP Engine) |
| **Kanal Interupsi** | EXTI Line 16 (`PVD_PVM_IRQn`) | EXTI Line 0 / 1 (`EXTI0_IRQn` / `EXTI1_IRQn`) |
| **Kondisi Pemicu** | Tegangan rel $V_{DD}$ turun di bawah ambang batas ($\le 2.8\text{ V}$) | Pin `IRQ0`/`IRQ1` ditarik LOW oleh ADE9000 saat anomali daya |
| **Tujuan Utama** | Mencegah kerusakan partisi Flash & kehilangan data penagihan | Deteksi seketika ($\le 20\text{ ms}$) sabotase pencurian listrik |
| **Beban Komputasi** | Sangat singkat: Flush buffer RAM ke Flash, lalu *halt* | Transaksi SPI singkat membaca status, kirim pesan ke RTOS queue |
| **Dampak ke Layar/Buzzer**| Layar OLED & Buzzer langsung dimatikan (*save energy*) | Layar OLED menampilkan alarm `!ALM`, Buzzer aktif berirama 200 ms |

---

## 5. Rencana Penerapan Bertahap (26-Week Plan Alignment)

Sesuai dengan roadmap **26-Week Development Plan Rev D**:

1. **Tahap Pra-Week 8 (Saat ini):**
   * Menyusun arsitektur perangkat lunak, deklarasi fungsi API (*header*), dan modul *stub/mock* driver untuk PVD dan ADE9000 IRQ.
   * Memvalidasi bahwa pemanggilan fungsi penyimpanan darurat `nvram_save_tamper_log_snapshot()` aman dieksekusi secara atomik.
2. **Tahap Week 8–9 (Kedatangan Board Fisik Spin 1):**
   * Mengonfigurasi modul PVD pada peripheral HAL STM32U575 di Non-Secure World.
   * Memetakan pin fisik `IRQ0` dan `IRQ1` ADE9000 pada skematik KiCad buatan E4 ke pin GPIO EXTI STM32.
3. **Tahap Weeks 13–14 (Phase 3: Hardware Tamper Integration):**
   * Pengujian dinamis injeksi sinyal arus balik (*reverse current*) dan simulasi pemutusan netral pada rig uji bangku (*bench test*).
   * Verifikasi pemenuhan standar SPLN D3.006: 2021 Tabel 6 dengan respon alarm instan $\le 5\text{ detik}$.

