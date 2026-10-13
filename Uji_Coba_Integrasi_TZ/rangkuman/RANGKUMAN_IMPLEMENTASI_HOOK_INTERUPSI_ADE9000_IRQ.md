# Rangkuman Implementasi: Hook Interupsi 2 ADE9000 Hardware IRQ (PB0 & PB1 via EXTI)

**Tanggal:** 13 Oktober 2026  
**Peran:** E3 Lead (*Application, Display SPLN D3.006, Tamper Event Management & Safety*)  
**Target Hardware:** STM32U575VGT6 + Analog Devices ADE9000 (ARM Cortex-M33 TrustZone)  
**Status Verifikasi:** 100% LULUS (20/20 CTest Suites, 0 Errors, 0 Warnings pada Kompilasi Cross ARM GCC)

---

## 1. Ikhtisar Pekerjaan yang Dilakukan

Setelah penyelesaian Hook Interupsi 1 (PVD Last-Gasp), fokus utama sesi ini adalah merealisasikan **Hook Interupsi 2: ADE9000 Hardware IRQ** (`IRQ0` & `IRQ1`) untuk menangkap anomali jaringan listrik dan sabotase pencurian energi berkecepatan tinggi ($\le 20\text{ ms}$) sesuai standar **SPLN D3.006: 2021 Tabel 6**:

1. **Pembangunan Driver BSP Interupsi ADE9000 (`bsp_ade9000_irq.h` & `bsp_ade9000_irq.c`):**
   * **Kanal IRQ0 (Pin 31 ADE9000 $\rightarrow$ PB0 STM32 / EXTI Line 0):**
     - Membaca register `STATUS0` (Address `0x402`).
     - Deteksi **Aliran Daya Aktif Terbalik (*Reverse Power*)** (`REVAP | REVBP | REVCP`) $\rightarrow$ SPLN Tabel 6 No 8 (**`REVERSE ERR26`** / `DLMS_TAMPER_REVERSE_CURRENT`).
     - Deteksi **Urutan Fase Terbalik (*Wrong Phase Sequence*)** (`PWRORD`) $\rightarrow$ SPLN Tabel 6 No 3 (**`PERIKSA ERR21`** / `DLMS_TAMPER_WRONG_PHASE_SEQ`).
     - Acknowledge interupsi secara perangkat keras melalui mekanisme *Write-1-to-Clear* pada `STATUS0`.
   * **Kanal IRQ1 (Pin 32 ADE9000 $\rightarrow$ PB1 STM32 / EXTI Line 1):**
     - Membaca register `STATUS1` (Address `0x403`).
     - Deteksi **Kawat Netral Hilang / Pembumian Arus** (`MISNEUT | ISUM`) $\rightarrow$ SPLN Tabel 6 No 5 (**`PERIKSA ERR23`** / `DLMS_TAMPER_MISSING_NEUTRAL`).
     - Deteksi **Hilang Tegangan 1/2 Fase (*Voltage Sag / Phase Loss*)** (`SAG | SAGA | SAGB | SAGC`) $\rightarrow$ SPLN Tabel 6 No 6 (**`PERIKSA ERR24`** / `DLMS_TAMPER_MISSING_VOLTAGE`).
     - Acknowledge interupsi secara perangkat keras melalui mekanisme *Write-1-to-Clear* pada `STATUS1`.
   * **Integrasi Non-Blocking ke FreeRTOS Queue:**
     - Mengirimkan event secara aman dari konteks interrupt menggunakan `rtos_queue_send_tamper_event_from_isr()`.
     - Memicu refresh layar seketika via `display_task_trigger_instant_refresh(true)`.

2. **Perluasan Model Virtual Emulasi ADE9000 (`E1/Mock/Mock.h` & `E1/Mock/Mock.c`):**
   * Menambahkan register `STATUS0`, `STATUS1`, `MASK0`, dan `MASK1` ke dalam struct `ADE9000_Mock_t`.
   * Mengimplementasikan logika *Write-1-to-Clear* pada `ADE9000_Mock_WriteRegister()` sehingga perilaku simulasi register status persis identik dengan silikon fisik ADE9000.
   * Menambahkan API setter simulasi: `ADE9000_Mock_SetSTATUS0()`, `ADE9000_Mock_SetSTATUS1()`, `ADE9000_Mock_SetMASK0()`, dan `ADE9000_Mock_SetMASK1()`.

3. **Konfigurasi Jalur Interupsi ARM Cortex-M33 TrustZone:**
   * **Secure World (`Secure/Core/Src/main.c`):**
     - Menambahkan perutean interupsi ke Non-Secure World:
       - `NVIC_SetTargetState(EXTI0_IRQn);`
       - `NVIC_SetTargetState(EXTI1_IRQn);`
   * **Non-Secure World Interrupt Vectors (`NonSecure/Core/Src/stm32u5xx_it.c`):**
     - Mengimplementasikan `EXTI0_IRQHandler()` $\rightarrow$ `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0)`.
     - Mengimplementasikan `EXTI1_IRQHandler()` $\rightarrow$ `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1)`.
   * **Callback HAL:**
     - Menghubungkan `HAL_GPIO_EXTI_Falling_Callback()` ke `bsp_ade9000_irq0_handler()` dan `bsp_ade9000_irq1_handler()`.
   * **Prioritas NVIC:**
     - Dikonfigurasi pada Prioritas 5 (Subprioritas 0) agar aman memanggil FreeRTOS API (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`).

4. **Penambahan Fitur Uji Interaktif Konsol UART (`NonSecure/Core/Src/main.c`):**
   * Tombol `5` : Injeksi Hardware ADE9000 IRQ0 - Reverse Power Tamper (`ERR26`).
   * Tombol `6` : Injeksi Hardware ADE9000 IRQ1 - Missing Neutral Tamper (`ERR23`).
   * Tombol `7` : Injeksi Hardware ADE9000 IRQ1 - Hilang Tegangan Fase / Sag (`ERR24`).
   * Tombol `8` : Injeksi Hardware ADE9000 IRQ0 - Urutan Fase Terbalik (`ERR21`).
   * Pembaruan menu bantuan interaktif `h` / `?`.

5. **Pengujian Otomatis CTest (`firmware/tests/drivers/test_bsp_ade9000_irq.c`):**
   * Menguji inisialisasi awal & konfigurasi interrupt mask (`MASK0`, `MASK1`).
   * Menguji trigger IRQ0 (Reverse Power), verifikasi isi register, mekanisme Write-1-to-Clear, dan penerimaan queue di RTOS.
   * Menguji trigger IRQ0 (Wrong Phase Sequence).
   * Menguji trigger IRQ1 (Missing Neutral).
   * Menguji trigger IRQ1 (Voltage Sag / Phase Loss).

---

## 2. Tabel Modifikasi Berkas

| No | Berkas | Status | Rincian Perubahan |
|:---:|---|:---:|---|
| **1** | [`firmware/drivers/bsp/inc/bsp_ade9000_irq.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/inc/bsp_ade9000_irq.h) | **Baru** | Definisi bit register STATUS0/STATUS1 ADE9000, prototipe handler IRQ0 & IRQ1, status query, dan fungsi simulasi CTest. |
| **2** | [`firmware/drivers/bsp/src/bsp_ade9000_irq.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/src/bsp_ade9000_irq.c) | **Baru** | Implementasi GPIO PB0 & PB1 (Falling Edge, Pull-up, Priority 5), pembacaan SPI STATUS0/1, dispatch event ke RTOS queue, display refresh, dan Write-1-to-Clear. |
| **3** | [`firmware/tests/drivers/test_bsp_ade9000_irq.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/tests/drivers/test_bsp_ade9000_irq.c) | **Baru** | Unit test otomatis host CTest memvalidasi 4 vektor interupsi perangkat keras, status clearing, dan routing RTOS queue. |
| **4** | [`E1/Mock/Mock.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/E1/Mock/Mock.h) | **Diubah** | Menambahkan field `STATUS0`, `STATUS1`, `MASK0`, `MASK1` pada `ADE9000_Mock_t` dan prototipe fungsi setter simulasi. |
| **5** | [`E1/Mock/Mock.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/E1/Mock/Mock.c) | **Diubah** | Implementasi pembacaan/penulisan register status & mask dengan semantik perangkat keras *Write-1-to-Clear*. |
| **6** | [`firmware/CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/CMakeLists.txt) | **Diubah** | Menambahkan `bsp_ade9000_irq.c`, dependensi modul E1 ke `dlms_core`, dan mendaftarkan test suite `test_bsp_ade9000_irq`. |
| **7** | [`NonSecure/CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/CMakeLists.txt) | **Diubah** | Menambahkan `bsp_ade9000_irq.c` ke daftar source file build cross-target ARM. |
| **8** | [`Secure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure/Core/Src/main.c) | **Diubah** | Menambahkan `NVIC_SetTargetState(EXTI0_IRQn)` dan `NVIC_SetTargetState(EXTI1_IRQn)` mengarahkan vektor ke Non-Secure World. |
| **9** | [`NonSecure/Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/stm32u5xx_it.c) | **Diubah** | Menambahkan implementasi ISR `EXTI0_IRQHandler()` dan `EXTI1_IRQHandler()`. |
| **10** | [`NonSecure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/main.c) | **Diubah** | Memanggil `bsp_ade9000_irq_init()`, menambahkan tombol uji UART `'5'`..`'8'`, dan memperbarui menu bantuan `'h'`. |

---

## 3. Hasil Pengujian & Metrik Verifikasi

### 3.1. Pengujian Unit Host PC (MinGW GCC - CTest)
```
Test project G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/build
      Start  1: test_dlms_association ............   Passed    0.02 sec
      Start  2: test_dlms_apdu ..................   Passed    0.03 sec
      Start  3: test_dlms_integration ............   Passed    0.03 sec
      Start  4: test_dlms_tamper_log .............   Passed    0.05 sec
      Start  5: test_dlms_hdlc ...................   Passed    0.03 sec
      Start  6: test_dlms_server .................   Passed    0.03 sec
      Start  7: test_dlms_task ...................   Passed    0.03 sec
      Start  8: test_dlms_nvm ....................   Passed    0.03 sec
      Start  9: test_bsp_lpuart ..................   Passed    0.03 sec
      Start 10: test_bsp_pvd .....................   Passed    0.02 sec
      Start 11: test_bsp_ade9000_irq .............   Passed    0.02 sec
      Start 12: test_e3_app ......................   Passed    0.02 sec
      Start 13: test_fsm_statechart ..............   Passed    0.03 sec
      Start 14: test_load_profile ................   Passed    0.02 sec
      Start 15: test_load_profile_dlms_adapter ...   Passed    0.03 sec
      Start 16: test_nvram_storage ...............   Passed    0.03 sec
      Start 17: test_rtos_tasks ..................   Passed    0.02 sec
      Start 18: test_power_relay .................   Passed    0.02 sec
      Start 19: test_spln_display_tamper .........   Passed    0.02 sec
      Start 20: test_e1_to_e2_integration ........   Passed    0.03 sec

100% tests passed, 0 tests failed out of 20
Total Test time (real) = 0.60 sec
```

### 3.2. Kompilasi Target Fisik ARM Cortex-M33 (TrustZone)
* **Secure World:**
  - Status: **0 Error, 0 Warning**
  - RAM: 1,608 B / 256 KB (0.61%)
  - Flash ROM: 19,024 B / 504 KB (3.69%)
  - NSC Table: 64 B / 8 KB (0.78%)
  - Berkas: `Uji_Coba_Integrasi_TZ_S.elf`, `Uji_Coba_Integrasi_TZ_S.hex`
* **Non-Secure World:**
  - Status: **0 Error, 0 Warning**
  - RAM: 81,208 B / 512 KB (15.49%)
  - Flash ROM: 94,320 B / 1 MB (9.00%)
  - Berkas: `Uji_Coba_Integrasi_TZ_NS.elf`, `Uji_Coba_Integrasi_TZ_NS.hex`

