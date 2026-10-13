# Panduan Arsitektur & Pengawas Hardware Watchdog (IWDG)
## Multi-Task Heartbeat Supervisor Pattern (FreeRTOS & STM32U575)
**Kepatuhan Standar: SPLN D3.006: 2021 Tabel 4 (Alarm Internal Meter - ERR04 MCU Failure)**

---

## 1. Latar Belakang & Masalah Klasik Watchdog pada RTOS

Pada sistem mikrokontroler tanpa RTOS (*bare-metal*), implementasi *Watchdog Timer* sangat sederhana:
```c
while (1) {
    proses_metrologi();
    proses_tampilan();
    HAL_IWDG_Refresh(&hiwdg); // Feed the dog
}
```
Jika perulangan utama macet, IWDG akan kehabisan waktu (*timeout*) dan mikrokontroler me-reset diri secara otomatis.

### Bahaya Fatal pada Sistem Multi-Tasking FreeRTOS:
Dalam smart meter 3-fasa dengan FreeRTOS, ada beberapa task yang berjalan secara independen dengan prioritas berbeda:
- **TamperTask** (Priority 4 - High)
- **ProfileTask / Metrologi** (Priority 3 - Medium)
- **DLMSTask** (Priority 2 - Normal)
- **OLEDTask / UI** (Priority 1 - Low)
- **ADE9000 IRQ Worker** (Priority 5 - Real-time)

> [!CAUTION]
> **Anti-Pattern Fatal:** Jika pengembang membuat task watchdog tersendiri yang hanya berisi:
> ```c
> void task_watchdog_feeder(void *pv) {
>     for(;;) {
>         HAL_IWDG_Refresh(&hiwdg);
>         vTaskDelay(pdMS_TO_TICKS(1000));
>     }
> }
> ```
> Jika `ProfileTask` (pencatat kWh) atau `DLMSTask` mengalami *deadlock* atau perulangan tak terbatas (*infinite loop*), **Task Feeder ini tetap me-refresh IWDG!** Akibatnya: meteran macet di lapangan, kWh berhenti tercatat, tetapi hardware watchdog tidak pernah mereset MCU (*False Refresh*).

Sebaliknya, jika setiap task me-refresh hardware IWDG secara sembarangan, task yang berjalan paling cepat akan terus me-refresh IWDG tanpa peduli task lain yang macet.

---

## 2. Solusi: Multi-Task Heartbeat Supervisor Pattern

Untuk menjamin kepatuhan terhadap **SPLN D3.006: 2021 Tabel 4 (ERR04: MCU Failure / Tidak Berfungsi Normal)**, firmware menerapkan arsitektur **Pengawas Terpadu (*Watchdog Supervisor*)**:

```
+-----------------------------------------------------------------------------------+
|                     ARSITEKTUR MULTI-TASK WATCHDOG SUPERVISOR                     |
+-----------------------------------------------------------------------------------+

   [Task Tamper]      --> bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT)    ---+
   [Task Metrologi]   --> bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT)  ---+
   [Task DLMS]        --> bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT)       ---+--> [Heartbeat Bitmask]
   [Task OLED UI]     --> bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT)         ---+      (Mask: 0x1F)
   [Task ADE9000 IRQ] --> bsp_watchdog_kick_task(WATCHDOG_TASK_ADE9000_BIT)   ---+
                                                                                     |
                                                                                     v
                                                                 +------------------------------+
                                                                 |  Watchdog Supervisor Step    |
                                                                 |  (Evaluasi Setiap 1.0 Detik) |
                                                                 +------------------------------+
                                                                                 |
                                                  +------------------------------+------------------------------+
                                                  | (Semua 5 Task Melapor)                                      | (Ada Task Macet)
                                                  v                                                             v
                                      +-----------------------+                                     +-----------------------+
                                      | Hardware IWDG Reload  |                                     | TAHAN REFRESH IWDG!   |
                                      | IWDG->KR = 0xAAAA     |                                     | (Biarkan Hardware IWDG|
                                      | Reset Heartbeat Mask  |                                     | Hitung Mundur Habis)  |
                                      +-----------------------+                                     +-----------------------+
                                                  |                                                             |
                                                  v                                                             v
                                            Sistem Normal                                                 HARD MCU RESET
                                                                                                                |
                                                                                                                v
                                                                                                    Boot: Deteksi RCC_RSR
                                                                                                    Catat ERR04 & TAMP_BKP3R
```

---

## 3. Alokasi Bitmask Task & Register Backup

### 3.1. Bitmask Task
Setiap task kritis FreeRTOS diberikan satu bit unik pada mask pengawas:

| Nama Simbolik | Nilai Bit | Deskripsi Task | Prioritas FreeRTOS |
| :--- | :---: | :--- | :---: |
| `WATCHDOG_TASK_TAMPER_BIT` | `0x01` (`1 << 0`) | Task Darurat Sabotase & Relay | Priority 4 |
| `WATCHDOG_TASK_METROLOGY_BIT`| `0x02` (`1 << 1`) | Task Sampling ADE9000 & Profil Beban | Priority 3 |
| `WATCHDOG_TASK_DLMS_BIT` | `0x04` (`1 << 2`) | Server Protokol DLMS/COSEM HDLC | Priority 2 |
| `WATCHDOG_TASK_UI_BIT` | `0x08` (`1 << 3`) | Pengelola Carousel Layar OLED/LCD | Priority 1 |
| `WATCHDOG_TASK_ADE9000_BIT` | `0x10` (`1 << 4`) | Worker Task Interupsi IRQ0/IRQ1 | Priority 5 |

Mask gabungan wajib: `WATCHDOG_DEFAULT_MONITORED_MASK = 0x1F` (5 task wajib aktif).

### 3.2. Retensi Register Backup (TAMP_BKP3R)
Jika terjadi hardware reset akibat watchdog:
1. Register `RCC->RSR` (Reset Status Register) pada STM32U575 mendeteksi bit `RCC_RSR_IWDGRSTF`.
2. Register baterai `TAMP_BKP3R` membaca nilai akumulator reset sebelumnya, menambahkan `+1`, dan menyimpannya kembali di domain $V_{BAT}$.
3. Bendera reset dibersihkan melalui `__HAL_RCC_CLEAR_RESET_FLAGS()`.
4. Kejadian ini dicatat sebagai alarm internal **ERR04** pada display dan kamus OBIS DLMS.

---

## 4. Konfigurasi Hardware IWDG STM32U575

Driver mengendalikan register periferal IWDG secara langsung (*bare-metal registers*) untuk efisiensi maksimal dan tidak bergantung pada library HAL berukuran besar:

- **Sumber Clock:** LSI (*Low-Speed Internal Oscillator*) $32\text{ kHz}$.
- **Prescaler:** Divider `/64` $\rightarrow$ Frekuensi pencacah $= 500\text{ Hz}$ ($1\text{ tick} = 2\text{ ms}$).
- **Reload Register (`RLR`):** $3.000\text{ ms} / 2\text{ ms} = 1.500$ (nilai 12-bit, batas maksimal 4.095).
- **Timeout Periode:** Tepat **3,0 detik**.
- **Supervisor Task Interval:** Setiap **1,0 detik**.
  Artinya, jika satu task macet, supervisor memiliki toleransi hingga $3\times$ siklus gagal sebelum hardware IWDG memicu reset paksa.

---

## 5. Antarmuka Uji Coba & Konsol UART (CLI)

Pada terminal serial 115200 bps, pengguna dapat memeriksa kesehatan pengawas dan mensimulasikan kegagalan task:

1. **Tekan `'w'` (Status Telemetri Watchdog):**
```text
[WATCHDOG] Telemetri Pengawas Multi-Task:
           Status Init    : AKTIF
           Total Reset    : 0 kali
           Monitored Mask : 0x1F
           Heartbeat Mask : 0x1F
           Missing Mask   : 0x00
           Timeout IWDG   : 3000 ms
```

2. **Tekan `'W'` (Simulasi Freeze Task):**
```text
[SIMULASI] Bekukan DLMSTask untuk menguji Watchdog Supervisor Reset!
[SIMULASI WATCHDOG] Task bit 0x04 dibekukan/simulasi hang!
[WATCHDOG SUPERVISOR] PERINGATAN: Task macet terdeteksi (Missing Mask: 0x04)! Menolak refresh IWDG.
[WATCHDOG SUPERVISOR] PERINGATAN: Task macet terdeteksi (Missing Mask: 0x04)! Menolak refresh IWDG.
[WATCHDOG SUPERVISOR] PERINGATAN: Task macet terdeteksi (Missing Mask: 0x04)! Menolak refresh IWDG.
--> HARDWARE RESET TRIGGERED OLEH IWDG SETELAH 3.0 DETIK!
```

---

## 6. Verifikasi Suite Pengujian CTest (`test_bsp_watchdog.c`)

Suite pengujian unit otomatis telah ditambahkan ke sistem build host:
- `test_watchdog_init_and_default_state`: Validasi parameter default timeout & monitored mask.
- `test_watchdog_normal_heartbeat_and_refresh`: Validasi multi-task heartbeat & reload timer.
- `test_watchdog_task_hang_detection_and_refusal`: Validasi deteksi task macet & penolakan refresh IWDG hingga countdown habis.
- `test_watchdog_boot_cause_and_backup_register_counter`: Validasi deteksi penyebab boot & retensi register TAMP_BKP3R.
- `test_watchdog_custom_monitored_mask`: Validasi pengawasan dinamis subset task.
- `test_watchdog_null_safety`: Validasi pointer guard.

**Hasil Eksekusi CTest:**
```text
test_bsp_watchdog ................ Passed (100% Lulus, 24/24 Test Suites Passed)
```

