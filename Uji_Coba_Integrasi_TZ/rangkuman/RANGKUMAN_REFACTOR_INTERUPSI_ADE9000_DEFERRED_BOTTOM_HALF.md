# Rangkuman Refactor Interupsi ADE9000: Arsitektur Deferred Bottom-Half & SPI Mutex

**Tanggal:** 13 Oktober 2026  
**Peran:** E3 Lead (*Application, Display, Tamper Management, Storage & Safety*)  
**Status Target:** Dual-Target Lolos 100% (MinGW Host CTest 20/20 & ARM Cortex-M33 TrustZone 0 Error / 0 Warning)

---

## 1. Latar Belakang & Akar Masalah (Mengapa Di-refactor?)

Pada implementasi interupsi ADE9000 sebelumnya:
* Handler interupsi `bsp_ade9000_irq0_handler()` dan `bsp_ade9000_irq1_handler()` dipanggil langsung oleh ISR EXTI perangkat keras (`EXTI0_IRQHandler` dan `EXTI1_IRQHandler`).
* Di dalam ISR tersebut, kode mengeksekusi transaksi SPI langsung (`ADE9000_SPI_ReadRegister` dan `ADE9000_SPI_WriteRegister`) serta panggilan `printf()`.

### Risiko Fatal di Perangkat Keras Fisik (Spin 1):
1. **SPI Bus Collision & HardFault:** Jika Task Metrologi (`task_metrology_profiling_entry` / `metrology_adapter_sample`) sedang membaca 15 register ADE9000 pada bus SPI1, lalu terjadi interupsi EXTI0/EXTI1, ISR akan menyela dan menarik pin Chip Select (CS) SPI secara paksa di tengah transmisi byte. Hal ini mengakibatkan register SPI mengalami *data corruption*, *deadlock* flag `BSY`, atau CPU *HardFault*.
2. **ISR Latency & Lock Inversion:** Eksekusi SPI dan `printf` di dalam ISR memakan waktu beberapa ratus mikrosekon hingga milidetik, melanggar kaidah *hard real-time* embedded systems.

---

## 2. Solusi Arsitektur: Top-Half / Bottom-Half Architecture

Sistem interupsi ADE9000 kini telah direfaktor menjadi arsitektur bertingkat (**Top-Half ISR** dan **Bottom-Half Worker Task**):

```
┌────────────────────────────────────────────────────────────────────────┐
│                   TOP-HALF (Hardware EXTI ISR Context)                 │
├────────────────────────────────────────────────────────────────────────┤
│ 1. Catat counter interupsi (s_irq0_count++ / s_irq1_count++)           │
│ 2. Set bit pending secara atomik (s_irq_pending_flags |= FLAG)         │
│ 3. Pemicu FreeRTOS Task Notification:                                  │
│    vTaskNotifyGiveFromISR(s_ade9000_worker_task_handle, &pxWoken);     │
│ 4. portYIELD_FROM_ISR(pxWoken);                                        │
│                                                                        │
│ ⚡ WAKTU EKSEKUSI: < 1 µs | ZERO SPI BUS TRAFFIC | ZERO PRINTF          │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ (Context Switch via FreeRTOS)
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│               BOTTOM-HALF (ADE9000 IRQ Worker Task Context)            │
├────────────────────────────────────────────────────────────────────────┤
│ 1. Bangun dari ulTaskNotifyTake(pdTRUE, portMAX_DELAY)                 │
│ 2. Ambil SPI Mutex: bsp_ade9000_spi_lock()                             │
│ 3. Baca register ADE9000 via SPI (STATUS0 / STATUS1)                   │
│ 4. Acknowledge Write-1-to-Clear via SPI                                │
│ 5. Lepas SPI Mutex: bsp_ade9000_spi_unlock()                           │
│ 6. Evaluasi Tamper Event SPLN (Reverse, Urutan Fase, Netral, Sag)      │
│ 7. Kirim ke antrean RTOS: rtos_queue_send_tamper_event(&msg)           │
│ 8. Pemicu Refresh OLED & Cetak Log Diagnostik (Aman di Task Context)   │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Sinkronisasi Antar Task via SPI Mutex

Untuk menjamin tidak ada tabrakan bus SPI antara **Task Metrologi (1 Hz)** dan **ADE9000 Bottom-Half Worker Task**, disediakan sepasang fungsi sinkronisasi:

* [`bsp_ade9000_spi_lock()`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/src/bsp_ade9000_irq.c): Mengambil FreeRTOS Mutex `s_spi_mutex` dengan mekanisme *Priority Inheritance*.
* [`bsp_ade9000_spi_unlock()`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/src/bsp_ade9000_irq.c): Melepaskan FreeRTOS Mutex.

### Proteksi pada Modul Metrologi:
Fungsi [`metrology_adapter_sample()`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/E1/Adapter/metrology_adapter.c) yang membaca 15 register ADE9000 kini diproteksi penuh:
```c
/* 1. Ambil SPI Mutex untuk mencegah tabrakan dengan interupsi ADE9000 Bottom-Half */
bsp_ade9000_spi_lock();

/* Baca register RMS Tegangan, Arus, Daya, Frekuensi, dan Energi via SPI */
uint32_t airms = ADE9000_SPI_ReadRegister(ADE9000_AIRMS);
...
uint32_t cwatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_LO);

/* Lepaskan SPI Mutex segera setelah transaksi SPI selesai */
bsp_ade9000_spi_unlock();
```

Jika interupsi perangkat keras terjadi di tengah-tengah pembacaan register ke-5 oleh Task Metrologi:
1. Top-Half ISR berjalan sekejap (< 1 µs) hanya untuk mencatat pending flag dan memberi sinyal task notification.
2. ISR selesai dan CPU kembali ke Task Metrologi untuk menuntaskan pembacaan register SPI hingga selesai.
3. Setelah Task Metrologi memanggil `bsp_ade9000_spi_unlock()`, Bottom-Half Worker Task langsung mengambil alih giliran, membaca register status, dan meng-acknowledge interupsi tanpa ada konflik sama sekali.

---

## 4. Berkas yang Dimodifikasi

| No | Berkas | Perubahan yang Dilakukan |
| :---: | :--- | :--- |
| 1 | [`firmware/drivers/bsp/inc/bsp_ade9000_irq.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/inc/bsp_ade9000_irq.h) | Menambahkan definisi `ADE9000_IRQ_FLAG_IRQ0`/`IRQ1`, prototipe Top-Half `bsp_ade9000_irq0_isr()` & `irq1_isr()`, prototipe Bottom-Half `bsp_ade9000_irq_bottom_half_process()`, prototipe worker task `bsp_ade9000_irq_start_worker_task()`, serta API `bsp_ade9000_spi_lock()`/`unlock()`. |
| 2 | [`firmware/drivers/bsp/src/bsp_ade9000_irq.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/src/bsp_ade9000_irq.c) | Mengimplementasikan pemisahan Top-Half dan Bottom-Half. Menghapus transaksi SPI dan `printf` dari ISR. Mengintegrasikan Task Notification FreeRTOS dan SPI Mutex. |
| 3 | [`E1/Adapter/metrology_adapter.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/E1/Adapter/metrology_adapter.c) | Membungkus urutan pembacaan 15 register metrologi dengan `bsp_ade9000_spi_lock()` dan `bsp_ade9000_spi_unlock()`. |
| 4 | [`firmware/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/src/rtos_tasks.c) | Menghubungkan peluncuran worker task `bsp_ade9000_irq_start_worker_task()` ke dalam `rtos_start_all_tasks()`. |
| 5 | [`firmware/tests/drivers/test_bsp_ade9000_irq.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/tests/drivers/test_bsp_ade9000_irq.c) | Menambahkan Section 7 untuk menguji mode eksekusi deferred, verifikasi pending flags, proteksi SPI mutex, serta Write-1-to-Clear. |
| 6 | [`firmware/drivers/bsp/inc/bsp_pin_mapping.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/inc/bsp_pin_mapping.h) | Memperbaiki ketidaksengajaan karakter typo `mni` pada baris 1. |

---

## 5. Hasil Verifikasi Pengujian

### A. Pengujian Host PC (MinGW GCC & CTest)
```text
Test project G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/build
 1/20 Test  #1: test_dlms_association ............   Passed    0.03 sec
 2/20 Test  #2: test_dlms_apdu ...................   Passed    0.02 sec
 3/20 Test  #3: test_dlms_integration ............   Passed    0.03 sec
 4/20 Test  #4: test_dlms_tamper_log .............   Passed    0.03 sec
 5/20 Test  #5: test_dlms_hdlc ...................   Passed    0.03 sec
 6/20 Test  #6: test_dlms_server .................   Passed    0.04 sec
 7/20 Test  #7: test_dlms_task ...................   Passed    0.03 sec
 8/20 Test  #8: test_dlms_nvm ....................   Passed    0.02 sec
 9/20 Test  #9: test_bsp_lpuart ..................   Passed    0.03 sec
10/20 Test #10: test_bsp_pvd .....................   Passed    0.03 sec
11/20 Test #11: test_bsp_ade9000_irq .............   Passed    0.03 sec  <-- TERUJI DEFERRED HANDLING
12/20 Test #12: test_e3_app ......................   Passed    0.03 sec
13/20 Test #13: test_fsm_statechart ..............   Passed    0.03 sec
14/20 Test #14: test_load_profile ................   Passed    0.03 sec
15/20 Test #15: test_load_profile_dlms_adapter ...   Passed    0.04 sec
16/20 Test #16: test_nvram_storage ...............   Passed    0.03 sec
17/20 Test #17: test_rtos_tasks ..................   Passed    0.03 sec
18/20 Test #18: test_power_relay .................   Passed    0.03 sec
19/20 Test #19: test_spln_display_tamper .........   Passed    0.03 sec
20/20 Test #20: test_e1_to_e2_integration ........   Passed    0.03 sec

100% tests passed, 0 tests failed out of 20 (Total Test time = 0.62 sec)
```

### B. Kompilasi Silang ARM Cortex-M33 TrustZone (STM32U575)
* **Secure World:**
  * RAM Terpakai: **1.608 B / 256 KB (0,61%)**
  * ROM Terpakai: **19.024 B / 504 KB (3,69%)**
  * Status: **0 Error, 0 Warning** (`Uji_Coba_Integrasi_TZ_S.hex` berhasil dibuat)
* **Non-Secure World:**
  * RAM Terpakai: **81.224 B / 512 KB (15,49%)**
  * ROM Terpakai: **95.392 B / 1.024 KB (9,10%)**
  * Status: **0 Error, 0 Warning** (`Uji_Coba_Integrasi_TZ_NS.hex` berhasil dibuat)

