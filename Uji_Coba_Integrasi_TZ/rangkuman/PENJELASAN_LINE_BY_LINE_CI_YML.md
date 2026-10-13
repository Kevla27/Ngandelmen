# 📑 Penjelasan Mendalam Baris demi Baris File `.github/workflows/ci.yml`

Dokumen ini berisi bedah teknis secara **baris demi baris (*line-by-line*)** dari alur kerja otomatisasi **Continuous Integration (CI)** pada file [`.github/workflows/ci.yml`](file:///.github/workflows/ci.yml) untuk proyek *firmware* Smart Meter 3-Fasa STM32U575 (TrustZone).

---

## 📋 Daftar Isi
1. [Bagian 1: Header, Pemicu Event (Triggers), & Concurrency (Baris 1–13)](#1-bagian-1-header-pemicu-event-triggers--concurrency-baris-113)
2. [Bagian 2: Job 1 – Pengujian Host Unit & Integrasi (Baris 14–55)](#2-bagian-2-job-1--pengujian-host-unit--integrasi-baris-1455)
3. [Bagian 3: Job 2 – Kompilasi Firmware Target ARM STM32U575 (Baris 56–129)](#3-bagian-3-job-2--kompilasi-firmware-target-arm-stm32u575-baris-56129)

---

## 1. Bagian 1: Header, Pemicu Event (Triggers), & Concurrency (Baris 1–13)

```yaml
1: name: CI - Smart Meter Firmware & Automated Tests
2: 
3: on:
4:   push:
5:     branches: [ "main", "master", "develop" ]
6:   pull_request:
7:     branches: [ "main", "master" ]
8:   workflow_dispatch:
9: 
10: concurrency:
11:   group: ${{ github.workflow }}-${{ github.ref }}
12:   cancel-in-progress: true
13: 
```

### 🔍 Analisis Baris demi Baris:
* **Baris 1 (`name: CI - Smart Meter Firmware & Automated Tests`)**:
  Menentukan nama resmi alur kerja (*workflow name*). Nama ini akan muncul sebagai judul utama pada tab **Actions** di situs web repositori GitHub.
* **Baris 3 (`on:`)**:
  Kunci deklarasi pembuka yang mendefinisikan *event-event* apa saja yang dapat memicu eksekusi alur kerja ini.
* **Baris 4–5 (`push: branches: [ "main", "master", "develop" ]`)**:
  Memerintahkan GitHub Actions untuk otomatis memicu pengujian setiap kali ada *commit* baru yang dikirimkan (*push*) ke salah satu dari cabang utama: `main`, `master`, atau `develop`.
* **Baris 6–7 (`pull_request: branches: [ "main", "master" ]`)**:
  Menjalankan pengujian otomatis setiap kali ada pengajuan *Pull Request* baru atau pembaruan kode pada *Pull Request* yang menargetkan cabang `main` atau `master`. Ini menjamin kode baru tidak akan merusak sistem sebelum digabungkan (*merged*).
* **Baris 8 (`workflow_dispatch:`)**:
  Menambahkan tombol manual **"Run workflow"** pada antarmuka web GitHub Actions. Sangat berguna jika developer ingin menjalankan ulang tes kapan saja tanpa harus membuat commit kosong.
* **Baris 10 (`concurrency:`)**:
  Mengatur kebijakan penanganan eksekusi bersamaan (*concurrent runs*).
* **Baris 11 (`group: ${{ github.workflow }}-${{ github.ref }}`)**:
  Membuat penanda grup unik berdasarkan nama workflow dan nama cabang Git saat ini (misalnya `CI - Smart Meter Firmware & Automated Tests-refs/heads/main`).
* **Baris 12 (`cancel-in-progress: true`)**:
  **Optimalisasi Kuota CI:** Jika developer melakukan *push* commit baru saat proses kompilasi commit sebelumnya masih berjalan di cabang yang sama, alur kerja yang lama akan langsung dibatalkan dan digantikan dengan alur kerja commit terbaru. Ini menghemat kuota menit build GitHub Actions.

---

## 2. Bagian 2: Job 1 – Pengujian Host Unit & Integrasi (Baris 14–55)

```yaml
14: jobs:
15:   # =========================================================================
16:   # JOB 1: Host Automated Unit & Integration Tests (18 Test Suites)
17:   # =========================================================================
18:   host-automated-tests:
19:     name: 🧪 Run Host Unit & Integration Tests (18 Suites)
20:     runs-on: ubuntu-latest
21: 
22:     steps:
23:       - name: 📥 Checkout Repository
24:         uses: actions/checkout@v4
```

### 🔍 Analisis Baris demi Baris:
* **Baris 14 (`jobs:`)**:
  Blok utama yang berisi daftar seluruh tugas komputasi (*jobs*) yang akan dijalankan oleh GitHub Actions.
* **Baris 18 (`host-automated-tests:`)**:
  Pengidentifikasi (*ID*) unik untuk Job 1 di dalam file YAML.
* **Baris 19 (`name: 🧪 Run Host Unit & Integration Tests (18 Suites)`)**:
  Label nama deskriptif yang dilengkapi emoji agar mudah dipantau statusnya (apakah sedang berjalan, berhasil hijau, atau gagal merah).
* **Baris 20 (`runs-on: ubuntu-latest`)**:
  Menentukan spesifikasi mesin virtual cloud yang digunakan. Dalam hal ini, menggunakan sistem operasi **Linux Ubuntu LTS terbaru** yang disediakan gratis oleh GitHub.
* **Baris 22 (`steps:`)**:
  Awal urutan langkah-langkah kerja (*sequential steps*) yang akan dieksekusi di dalam mesin virtual Ubuntu tersebut.
* **Baris 23–24 (`uses: actions/checkout@v4`)**:
  Memanggil GitHub Action resmi untuk menyalin (*git clone*) repositori kode Anda ke dalam direktori kerja runner mesin virtual.

---

```yaml
26:       - name: 🛠️ Install Build Tools (CMake, Ninja, GCC)
27:         run: |
28:           sudo apt-get update
29:           sudo apt-get install -y cmake ninja-build build-essential
```

### 🔍 Analisis Baris demi Baris:
* **Baris 26 (`- name: ...`)**:
  Judul langkah pemasangan alat-alat kompilasi.
* **Baris 27 (`run: |`)**:
  Sintaks YAML untuk mengeksekusi skrip terminal (*bash*) multi-baris.
* **Baris 28 (`sudo apt-get update`)**:
  Memperbarui daftar paket repositori Ubuntu Linux agar siap memasang versi paket terbaru.
* **Baris 29 (`sudo apt-get install -y cmake ninja-build build-essential`)**:
  Memasang perangkat lunak yang dibutuhkan:
  * `cmake`: Alat otomatisasi konfigurasi build antar-platform.
  * `ninja-build`: Mesin kompilasi berbasis graf dependensi yang jauh lebih cepat dibanding Makefile tradisional.
  * `build-essential`: Paket kompilator GCC, G++, dan library standar C (`libc6-dev`).

---

```yaml
31:       - name: ⚙️ Configure CMake for Host Tests
32:         run: |
33:           if [ -d "Uji_Coba_Integrasi_TZ" ]; then
34:             PROJECT_DIR="Uji_Coba_Integrasi_TZ"
35:           else
36:             PROJECT_DIR="."
37:           fi
38:           echo "PROJECT_DIR=$PROJECT_DIR" >> $GITHUB_ENV
39:           rm -rf host_build "$PROJECT_DIR/firmware/build" "$PROJECT_DIR/NonSecure/firmware/build"
40:           cmake -B host_build -S "$PROJECT_DIR/firmware" -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```

### 🔍 Analisis Baris demi Baris:
* **Baris 33–37 (`if [ -d "Uji_Coba_Integrasi_TZ" ] ...`)**:
  **Deteksi Struktur Repositori Cerdas:** Mengecek apakah folder `Uji_Coba_Integrasi_TZ` ada (struktur mono-repo `Day_Sekian`). Jika ada, variabel `PROJECT_DIR` diatur ke folder tersebut; jika tidak ada (misalnya subfolder tersebut di kemudian hari dijadikan root repositori mandiri), maka menggunakan direktori saat ini (`.`).
* **Baris 38 (`echo "PROJECT_DIR=$PROJECT_DIR" >> $GITHUB_ENV`)**:
  Mengekspor variabel `PROJECT_DIR` ke *environment variables* global GitHub Actions agar dapat diakses oleh *step* berikutnya.
* **Baris 39 (`rm -rf host_build ...`)**:
  **Pembersihan Mutlak (*Clean Build*):** Menghapus folder build lama jika ada, mencegah sisa-sisa cache konfigurasi dari Windows lokal terbawa ke dalam runner Linux.
* **Baris 40 (`cmake -B host_build -S "$PROJECT_DIR/firmware" ...`)**:
  Menjalankan perintah konfigurasi CMake:
  * `-B host_build`: Menentukan folder output biner build bernama `host_build`.
  * `-S "$PROJECT_DIR/firmware"`: Menunjuk lokasi file target utama [`firmware/CMakeLists.txt`](file:///G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/CMakeLists.txt).
  * `-GNinja`: Menginstruksikan CMake untuk menghasilkan instruksi build berformat Ninja.
  * `-DCMAKE_BUILD_TYPE=Debug`: **Kritis.** Mengaktifkan mode Debug sehingga `-DNDEBUG` dinonaktifkan. Dengan demikian, seluruh 270+ makro `assert()` dalam kode tes unit aktif bekerja memvalidasi logika dan inisialisasi memori.
  * `-DBUILD_TESTS=ON`: Mengaktifkan opsi pembuatan 18 file *executable* pengujian host.

---

```yaml
42:       - name: 🔨 Compile Test Executables
43:         run: |
44:           cmake --build host_build --parallel
45: 
46:       - name: 🚀 Execute All Tests via CTest
47:         run: |
48:           ctest --test-dir host_build --output-on-failure --verbose
49: 
50:       - name: 📊 Summarize Test Results
51:         if: always()
52:         run: |
53:           echo "### 🧪 Ringkasan Pengujian Otomatis (CTest)" >> $GITHUB_STEP_SUMMARY
54:           echo "Seluruh 18 suite pengujian host (16 Unit Test DLMS/RTOS + 2 Integrasi E1/E2/E3 & SPLN Gambar 4) telah dieksekusi." >> $GITHUB_STEP_SUMMARY
```

### 🔍 Analisis Baris demi Baris:
* **Baris 42–44 (`cmake --build host_build --parallel`)**:
  Mengompilasi seluruh modul C `dlms_core` serta ke-18 berkas pengujian secara simultan dengan memanfaatkan seluruh core prosesor virtual runner runner GitHub (`--parallel`).
* **Baris 46–48 (`ctest --test-dir host_build --output-on-failure --verbose`)**:
  * `ctest`: Program orkestrator tes bawaan CMake.
  * `--test-dir host_build`: Masuk ke folder biner tes.
  * `--output-on-failure`: Menampilkan output log baris kode persis yang gagal jika terjadi kegagalan/assert fail.
  * `--verbose`: Menampilkan log eksekusi dari masing-masing tes nomor 1 sampai 18 di konsol.
* **Baris 51 (`if: always()`)**:
  Memastikan langkah rangkuman tetap dieksekusi meskipun ada tes sebelumnya yang berstatus gagal (*failed*).
* **Baris 52–54 (`$GITHUB_STEP_SUMMARY`)**:
  Menuliskan laporan hasil ringkas berformat Markdown ke panel ringkasan GitHub Actions (*Job Summary*).

---

## 3. Bagian 3: Job 2 – Kompilasi Firmware Target ARM STM32U575 (Baris 56–129)

```yaml
56:   # =========================================================================
57:   # JOB 2: STM32U575 ARM Cross-Compilation Build Verification
58:   # =========================================================================
59:   stm32-firmware-build:
60:     name: ⚡ STM32 ARM Cross-Compilation (Secure + Non-Secure)
61:     runs-on: ubuntu-latest
62:     needs: [host-automated-tests]
```

### 🔍 Analisis Baris demi Baris:
* **Baris 59 (`stm32-firmware-build:`)**:
  Pengidentifikasi (*ID*) unik untuk Job 2 (kompilasi firmware embedded asli).
* **Baris 60 (`name: ...`)**:
  Nama yang tampil di antarmuka GitHub Actions.
* **Baris 61 (`runs-on: ubuntu-latest`)**:
  Menggunakan lingkungan Linux Ubuntu LTS runner.
* **Baris 62 (`needs: [host-automated-tests]`)**:
  **Dependensi Kritis (Fail-Fast):** Menentukan bahwa Job 2 **HANYA AKAN BERJALAN** jika Job 1 (`host-automated-tests`) telah selesai dengan status sukses 100%. Jika ada 1 saja unit test logika yang gagal di Job 1, kompilasi target STM32 tidak akan dilanjutkan.

---

```yaml
64:     steps:
65:       - name: 📥 Checkout Repository
66:         uses: actions/checkout@v4
67: 
68:       - name: 🧰 Install ARM GNU Toolchain
69:         uses: carlosperate/arm-none-eabi-gcc-action@v1
70:         with:
71:           release: '13.3.Rel1'
72: 
73:       - name: 🛠️ Install Ninja & CMake
74:         run: |
75:           sudo apt-get update
76:           sudo apt-get install -y ninja-build cmake
77: 
78:       - name: 🔍 Verify ARM Toolchain Version
79:         run: |
80:           arm-none-eabi-gcc --version
```

### 🔍 Analisis Baris demi Baris:
* **Baris 65–66 (`actions/checkout@v4`)**: Mengunduh kode sumber proyek ke runner Job 2.
* **Baris 68–71 (`carlosperate/arm-none-eabi-gcc-action@v1`)**:
  Memasang compiler cross-compiler resmi **Arm GNU Toolchain** (`arm-none-eabi-gcc`) rilis `13.3.Rel1`. Toolchain ini memiliki dukungan penuh untuk arsitektur target mikrokontroler STM32U575: core **ARM Cortex-M33**, Floating Point Unit (FPU `fpv5-sp-d16`), dan ekstensi keamanan hardware ARMv8-M TrustZone (`-mcmse`).
* **Baris 73–76**: Memasang mesin build `ninja` dan `cmake` di lingkungan Ubuntu runner.
* **Baris 78–80 (`arm-none-eabi-gcc --version`)**:
  Mengeksekusi perintah cek versi compiler ARM untuk memastikan biner compiler berhasil terpasang di sistem `PATH`.

---

```yaml
82:       - name: ⚙️ Configure CMake for STM32 TrustZone
83:         run: |
84:           if [ -d "Uji_Coba_Integrasi_TZ" ]; then
85:             TARGET_DIR="Uji_Coba_Integrasi_TZ"
86:           else
87:             TARGET_DIR="."
88:           fi
89:           echo "TARGET_DIR=$TARGET_DIR" >> $GITHUB_ENV
90:           cd "$TARGET_DIR"
91:           rm -rf build Secure/build NonSecure/build
92:           cmake -B build/Debug -S . -GNinja \
93:             -DCMAKE_TOOLCHAIN_FILE="$(pwd)/gcc-arm-none-eabi.cmake" \
94:             -DCMAKE_BUILD_TYPE=Debug
```

### 🔍 Analisis Baris demi Baris:
* **Baris 84–90**: Menavigasi direktori terminal ke folder proyek target STM32.
* **Baris 91 (`rm -rf build Secure/build NonSecure/build`)**:
  **Solusi Error CMakeCache.txt:** Menghapus seluruh folder build Secure, Non-Secure, dan root target sebelum konfigurasi dimulai. Ini menjamin CMake tidak akan mengalami *conflict error* akibat path Windows (`g:/PPPP/...`) yang tersimpan di file cache lama.
* **Baris 92–94 (`cmake -B build/Debug ...`)**:
  Mengonfigurasi multi-konteks TrustZone:
  * `-B build/Debug`: Folder hasil kompilasi utama.
  * `-S .`: Menunjuk ke [`CMakeLists.txt`](file:///G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/CMakeLists.txt) akar proyek.
  * `-GNinja`: Menggunakan build system Ninja.
  * `-DCMAKE_TOOLCHAIN_FILE="$(pwd)/gcc-arm-none-eabi.cmake"`: Mengarahkan kompilasi menggunakan file toolchain ARM cross-compiler [`gcc-arm-none-eabi.cmake`](file:///G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/gcc-arm-none-eabi.cmake).
  * `-DCMAKE_BUILD_TYPE=Debug`: Menyertakan simbol debug untuk firmware.

---

```yaml
96:       - name: 🔨 Compile STM32 Firmware Targets (Secure & Non-Secure)
97:         run: |
98:           cd "$TARGET_DIR"
99:           ninja -C build/Debug
```

### 🔍 Analisis Baris demi Baris:
* **Baris 96–99 (`ninja -C build/Debug`)**:
  Mengeksekusi proses kompilasi kode biner mikrokontroler:
  1. Mengompilasi proyek **Secure World** (`Uji_Coba_Integrasi_TZ_S`), memproses library fungsi panggilan antar-domain aman (*Non-Secure Callable*), dan menghasilkan file antarmuka `secure_nsclib.o`.
  2. Mengompilasi proyek **Non-Secure World** (`Uji_Coba_Integrasi_TZ_NS`) yang berisi FreeRTOS, aplikasi metrologi, layar OLED, dan protokol DLMS/COSEM, lalu menautkannya dengan `secure_nsclib.o`.
  3. Menghasilkan file biner executable `.elf` dan file biner Intel Hex `.hex` siap *flash* untuk kedua dunia (Secure dan Non-Secure).

---

```yaml
101:       - name: 📏 Calculate Memory Footprint (Flash & RAM)
102:         run: |
103:           cd "$TARGET_DIR"
104:           echo "=== 🔒 SECURE WORLD MEMORY USAGE ==="
105:           arm-none-eabi-size Secure/build/Uji_Coba_Integrasi_TZ_S.elf
106:           echo ""
107:           echo "=== 🌐 NON-SECURE WORLD MEMORY USAGE ==="
108:           arm-none-eabi-size NonSecure/build/Uji_Coba_Integrasi_TZ_NS.elf
109: 
110:           echo "### ⚡ Hasil Kompilasi Target STM32U575" >> $GITHUB_STEP_SUMMARY
111:           echo "| Target | File | Status |" >> $GITHUB_STEP_SUMMARY
112:           echo "| :--- | :--- | :--- |" >> $GITHUB_STEP_SUMMARY
113:           echo "| Secure World | \`Uji_Coba_Integrasi_TZ_S.elf\` / \`.hex\` | ✅ Sukses (Clean Build) |" >> $GITHUB_STEP_SUMMARY
114:           echo "| Non-Secure World | \`Uji_Coba_Integrasi_TZ_NS.elf\` / \`.hex\` | ✅ Sukses (Clean Build) |" >> $GITHUB_STEP_SUMMARY
```

### 🔍 Analisis Baris demi Baris:
* **Baris 101–108 (`arm-none-eabi-size ...`)**:
  Memanggil utilitas analisis memori binary ARM untuk membaca besaran segmen:
  * `text`: Alokasi kode program di Flash Memory (ROM).
  * `data`: Variabel terinisialisasi yang memakan Flash dan RAM.
  * `bss`: Variabel tanpa inisialisasi yang dialokasikan di SRAM (RAM).
  * Memberikan visibilitas langsung apakah pembaruan kode membuat memori membengkak atau melebihi limit chip STM32U575 (ROM 1 MB / RAM 512 KB).
* **Baris 110–114**:
  Menyajikan tabel ringkasan status kompilasi firmware ke halaman ringkasan GitHub Actions.

---

```yaml
116:       - name: 📦 Upload Firmware Artifacts (HEX & ELF)
117:         uses: actions/upload-artifact@v4
118:         with:
119:           name: stm32-firmware-binaries
120:           path: |
121:             **/Secure/build/*.elf
122:             **/Secure/build/*.hex
123:             **/Secure/build/*.map
124:             **/NonSecure/build/*.elf
125:             **/NonSecure/build/*.hex
126:             **/NonSecure/build/*.map
127:           retention-days: 14
```

### 🔍 Analisis Baris demi Baris:
* **Baris 116–117 (`actions/upload-artifact@v4`)**:
  Aksi resmi GitHub untuk menyimpan file biner hasil kompilasi ke server penyimpanan GitHub agar dapat diunduh oleh pengguna.
* **Baris 118–119 (`name: stm32-firmware-binaries`)**:
  Nama file arsip `.zip` yang akan muncul di menu **Artifacts** pada halaman workflow run.
* **Baris 120–126 (`path: | ...`)**:
  Daftar pola file yang akan diikutsertakan ke dalam berkas unduhan:
  * `*.hex`: Berkas format Intel HEX yang siap ditransfer langsung ke mikrokontroler menggunakan alat seperti STM32CubeProgrammer atau ST-Link CLI.
  * `*.elf`: Berkas Executable and Linkable Format lengkap dengan tabel simbol untuk keperluan *hardware debugging* (JTAG/SWD).
  * `*.map`: Berkas teks pemetaan memori lengkap untuk keperluan audit ukuran setiap fungsi dan variabel.
* **Baris 127 (`retention-days: 14`)**:
  Menyimpan berkas unduhan tersebut di cloud GitHub selama 14 hari sebelum otomatis dihapus untuk efisiensi ruang penyimpanan.

