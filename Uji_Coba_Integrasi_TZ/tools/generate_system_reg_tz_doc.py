import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.section import WD_ORIENTATION
from docx.oxml import parse_xml
from docx.oxml.ns import nsdecls

def create_system_reg_tz_doc():
    doc = Document()

    # Landscape orientasi agar lebar tabel 10.2 inci muat dengan sangat rapi
    for section in doc.sections:
        section.orientation = WD_ORIENTATION.LANDSCAPE
        section.page_width = Inches(11.0)
        section.page_height = Inches(8.5)
        section.top_margin = Inches(0.4)
        section.bottom_margin = Inches(0.4)
        section.left_margin = Inches(0.4)
        section.right_margin = Inches(0.4)

    def set_cell_background(cell, fill_hex):
        tcPr = cell._tc.get_or_add_tcPr()
        shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{fill_hex}"/>')
        tcPr.append(shd)

    def set_cell_margins(cell, top=45, bottom=45, left=50, right=50):
        tcPr = cell._tc.get_or_add_tcPr()
        tcMar = parse_xml(f'<w:tcMar {nsdecls("w")}><w:top w:w="{top}" w:type="dxa"/><w:bottom w:w="{bottom}" w:type="dxa"/><w:left w:w="{left}" w:type="dxa"/><w:right w:w="{right}" w:type="dxa"/></w:tcMar>')
        tcPr.append(tcMar)

    def add_heading_styled(text, level=1):
        h = doc.add_paragraph()
        r = h.add_run(text)
        r.font.name = 'Arial'
        r.font.bold = True
        if level == 1:
            r.font.size = Pt(11)
            r.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
            h.paragraph_format.space_before = Pt(8)
            h.paragraph_format.space_after = Pt(3)
        elif level == 2:
            r.font.size = Pt(9.5)
            r.font.color.rgb = RGBColor(0x00, 0x55, 0x99)
            h.paragraph_format.space_before = Pt(6)
            h.paragraph_format.space_after = Pt(2)
        return h

    # ========================== HEADER DOKUMEN ==========================
    title_p = doc.add_paragraph()
    title_run = title_p.add_run('SPESIFIKASI REGISTER SISTEM, MEMORI FIREWALL, & KEAMANAN TRUSTZONE (NON-PIN)')
    title_run.font.name = 'Arial'
    title_run.font.size = Pt(13)
    title_run.font.bold = True
    title_run.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    sub_p = doc.add_paragraph()
    sub_run = sub_p.add_run(
        'Smart Meter Listrik 3-Fasa Pascabayar (SPLN D3.006:2021 & B200) - STM32U585xx Arm Cortex-M33\n'
        'Dokumen Pelengkap Arsitektur Internal: Boot Address, Partisi Flash/RAM, Target Interupsi NVIC, DMA Security, & Auto-Erase Kunci\n'
        'Rujukan Resmi: STMicroelectronics RM0456 Rev. 5 & Dokumen Spesifikasi Produk B200 PT Len Industri'
    )
    sub_run.font.name = 'Arial'
    sub_run.font.size = Pt(8.5)
    sub_run.font.italic = True
    sub_run.font.color.rgb = RGBColor(0x55, 0x55, 0x55)
    sub_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    doc.add_paragraph()

    # Intro singkat
    p_intro = doc.add_paragraph()
    p_intro.paragraph_format.line_spacing = 1.15
    r_intro = p_intro.add_run(
        "Dokumen ini memuat register-register pengendali internal silikon mikrokontroler STM32U585xx yang TIDAK TERHUBUNG LANGSUNG "
        "ke kaki pin fisik luar (Non-Pin System Registers), namun MUTLAK DIPERLUKAN untuk mengaktifkan partisi keamanan TrustZone, "
        "mengisolasi memori SRAM/Flash, memetakan vektor interupsi RTOS, melakukan enkripsi perangkat keras DLMS, dan melindungi meter dari sabotase fisik. "
        "Register-register ini bekerja pada Lapisan Fondasi Arsitektur (Level 0 s.d. Level 4) mendampingi Tabel Pemetaan 49 Kaki Pin I/O (Level 5)."
    )
    r_intro.font.name = 'Arial'
    r_intro.font.size = Pt(8.5)
    r_intro.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    # Fungsi generic render tabel register sistem
    def render_system_table(section_title, data_rows, header_bg='003366'):
        add_heading_styled(section_title, 1)

        headers = ['No', 'Nama Register (RM0456)', 'Offset', 'Alamat Fisik (Hex)', 'Domain TZ', 'Bit / Field Konfigurasi Kunci', 'Fungsi & Peran Strategis dalam Smart Meter']
        col_widths = [0.35, 1.8, 0.7, 1.35, 1.0, 3.2, 1.8]  # Total = 10.2 inci

        table = doc.add_table(rows=len(data_rows)+1, cols=len(headers))
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        table.autofit = False

        # Header row
        hdr_cells = table.rows[0].cells
        for i, name in enumerate(headers):
            hdr_cells[i].text = name
            set_cell_background(hdr_cells[i], header_bg)
            set_cell_margins(hdr_cells[i], 50, 50, 30, 30)
            p = hdr_cells[i].paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(7.5)
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

        # Data rows
        for row_idx, rdata in enumerate(data_rows, start=1):
            row_cells = table.rows[row_idx].cells
            tz_domain = rdata[4]
            bg_color = 'FFF4EB' if 'Secure' in tz_domain and 'Non' not in tz_domain else ('F2F7FC' if row_idx % 2 == 1 else 'FFFFFF')
            if 'Core' in tz_domain or 'System' in tz_domain:
                bg_color = 'F4F6F6' if row_idx % 2 == 1 else 'FFFFFF'

            for col_idx, text in enumerate(rdata):
                row_cells[col_idx].text = text
                set_cell_background(row_cells[col_idx], bg_color)
                set_cell_margins(row_cells[col_idx], 35, 35, 30, 30)
                p = row_cells[col_idx].paragraphs[0]
                if col_idx in [0, 2, 4]:
                    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                for run in p.runs:
                    run.font.name = 'Arial'
                    run.font.size = Pt(6.5)
                    if col_idx in [1, 4]:
                        run.font.bold = True
                        if 'Secure' in tz_domain and 'Non' not in tz_domain:
                            run.font.color.rgb = RGBColor(0x99, 0x00, 0x00)
                        elif 'Non-Secure' in tz_domain:
                            run.font.color.rgb = RGBColor(0x00, 0x44, 0x88)
        doc.add_paragraph()

    # ========================== 1. BOOT & OPTION BYTES ==========================
    boot_data = [
        ('1', 'FLASH_OPTR', '+0x40', '0x40022040 (NS)\n0x50022040 (S)', 'System Security', 
         'Bit 31 TZEN = 1 (Enable TrustZone Perangkat Keras)\nBits [7:0] RDP = 0xCC (Readout Protection Level 2 / JTAG Permanently Locked)\nBits [10:9] BOR_LEV = 00b (Brownout Reset Level 0: 1,71 V Threshold)', 
         'Kunci Hardware TrustZone & Proteksi Firmware Anti-Tamper: Mencegah ekstraksi firmware melalui port SWD/JTAG pada meter terpasang (Mandat B200 Hal. 51).'),

        ('2', 'FLASH_SECBOOTADD0R', '+0x90', '0x50022090 (S)', 'Secure World', 
         'Bits [31:7] SECBOOTADD0 = 0x0C000000 >> 7 (Alamat Boot Vektor Secure)', 
         'Alamat Vektor Boot Pertama Reset: CPU Cortex-M33 wajib mulai mengeksekusi kode dari Secure Flash Base (0x0C000000) sebelum melompat ke Non-Secure World.'),

        ('3', 'FLASH_SECWM1R1\nFLASH_SECWM1R2', '+0x80\n+0x84', '0x50022080 (S)\n0x50022084 (S)', 'Secure World', 
         'Bits [6:0] SECWM1_PSTRT = 0 (Page Mulai: 0)\nBits [22:16] SECWM1_PEND = 63 (Page Akhir: 63, Total 128 KB Flash Secure)', 
         'Secure Watermark Flash Bank 1: Mengunci sektor 0 s.d. 63 (128 KB) untuk Secure Bootloader, Driver ADE9000, & Kripto DLMS; sisanya untuk Non-Secure.'),

        ('4', 'FLASH_ACR', '+0x00', '0x40022000 (NS)\n0x50022000 (S)', 'System Core', 
         'Bits [3:0] LATENCY = 4WS (4 Wait States pada frekuensi VOS1 160 MHz)\nBit 8 PRFTEN = 1 (Prefetch Buffer Enable)', 
         'Access Control Register Flash: Mengatur siklus tunda baca memori Flash internal agar pembacaan program tidak corrupt saat MCU berjalan di kecepatan 160 MHz.')
    ]
    render_system_table('1. Boot Sistem, Proteksi Pembacaan Firmware, & Option Bytes (FLASH & OPTION)', boot_data, '003366')

    # ========================== 2. FIREWALL MEMORI & PERIFERAL ==========================
    firewall_data = [
        ('1', 'GTZC1_MPCBB1_CR', '+0x00', '0x50032800 (S)', 'Secure World', 
         'Bit 0 SRWILADIS = 1 (Secure Read/Write Illegal Access Interrupt Enabled)\nBit 31 INVSECSTATE = 0 (Normal polarity: 1=Secure, 0=Non-Secure)', 
         'Kontroler Firewall RAM (MPCBB1 - SRAM1 192 KB): Mematikan akses ilegal ke RAM Secure dan memicu Security Fault jika terjadi pelanggaran.'),

        ('2', 'GTZC1_MPCBB1_SECCFGR0..31', '+0x100 s.d.\n+0x17C', '0x50032900 s.d.\n0x5003297C (S)', 'Secure World', 
         '32 Register konfigurasi (1 bit per blok 512 Byte):\nBlok 0..31 = 1 (Secure: 16 KB untuk buffer ADE9000 & Kunci AES)\nBlok 32..383 = 0 (Non-Secure: Buffer FreeRTOS, Stack DLMS, & Display)', 
         'Partisi Granular Blok SRAM1: Memisahkan memori kerja data metering & kripto agar aplikasi Non-Secure tidak dapat membaca variabel metrologi di RAM.'),

        ('3', 'GTZC1_MPCBB3_SECCFGR0..31', '+0x300 s.d.\n+0x37C', '0x50032B00 s.d.\n0x50032B7C (S)', 'Secure World', 
         'Konfigurasi Blok SRAM3 (512 KB): Diatur Non-Secure penuh untuk Heap FreeRTOS dan buffer data Load Profile 3-bulan (diinisialisasi pada main.c baris 205-275).', 
         'Partisi Memori Utama Non-Secure (SRAM3): Tempat kerja aplikasi metering DLMS/COSEM, RTOS task stack, dan buffer besar antrean komunikasi.'),

        ('4', 'GTZC1_TZSC_SECCFGR1', '+0x100', '0x50032500 (S)', 'Secure World', 
         'Bit 0 SPI1SEC = 1 (SPI1 ADE9000 Secure)\nBit 1 SPI2SEC = 0 (SPI2 Flash Eksternal Non-Secure)\nBit 18 RTCSEC = 1 (RTC Secure)\nBit 19 TAMPSEC = 1 (Anti-Tamper Secure)', 
         'Atribusi Keamanan Bus Periferal 1: Mengunci periferal metrologi dan anti-tamper agar hanya bisa diakses oleh instruksi Secure CPU.'),

        ('5', 'GTZC1_TZSC_SECCFGR2', '+0x104', '0x50032504 (S)', 'Secure World', 
         'Bit 6 LPUART1SEC = 0 (Modem 4G Non-Secure)\nBit 2 USART3SEC = 0 (Port Optik Non-Secure)\nBit 9 I2C3SEC = 0 (Driver Display LCD Non-Secure)', 
         'Atribusi Keamanan Bus Periferal 2: Menyerahkan kendali periferal komunikasi dan display kepada task FreeRTOS Non-Secure.'),

        ('6', 'GTZC1_TZSC_SECCFGR3', '+0x108', '0x50032508 (S)', 'Secure World', 
         'Bit 0 ADC4SEC = 1 (ADC4 Pemantau Baterai & Superkapasitor Secure)', 
         'Atribusi Keamanan Bus Periferal 3: Menjamin status kesehatan baterai dan superkapasitor dimonitor secara aman di Secure World.'),

        ('7', 'GTZC1_TZICR\nGTZC1_TZISR', '+0x14\n+0x10', '0x50032414 (S)\n0x50032410 (S)', 'Secure World', 
         'Bit 0 ILLEGF (Illegal Access Flag): 1 = Terjadi upaya pembacaan/penulisan ilegal oleh Non-Secure ke register Secure; tulis 1 ke TZICR untuk clear flag.', 
         'Detektor Pelanggaran Akses Firewall: Menghasilkan interupsi darurat Security Fault saat ada bug atau upaya penetrasi pada kode Non-Secure.')
    ]
    render_system_table('2. Partisi Memori RAM & Peripheral Firewall (GTZC MPCBB & TZSC)', firewall_data, '990000')

    # ========================== 3. CORE & INTERRUPT ROUTING ==========================
    core_data = [
        ('1', 'SCB->VTOR (Secure)', 'Base SCS', '0xE002ED08 (S)', 'Core Secure', 
         'Bits [31:7] TBLOFF = 0x0C000000 >> 7', 
         'Vector Table Offset Register Secure: Menunjuk ke tabel vektor interupsi Secure (Reset, NMI, HardFault, Interupsi ADE9000, Tamper ISR).'),

        ('2', 'SCB_NS->VTOR (Non-Secure)', 'Base SCS', '0xE000ED08 (NS)', 'Core Non-Secure', 
         'Bits [31:7] TBLOFF = 0x08100000 >> 7 (Flash Bank 2 Base)', 
         'Vector Table Offset Register Non-Secure: Menunjuk ke tabel vektor interupsi Non-Secure (FreeRTOS SysTick, PendSV, UART Modem, Display LCD).'),

        ('3', 'SAU->CTRL', 'Base SAU', '0xE000EDD0 (S)', 'Core Security', 
         'Bit 0 ENABLE = 1 (Enable SAU Controller)\nBit 1 ALLNS = 0 (Memory not defined in SAU is Secure)', 
         'Security Attribution Unit (SAU) Control: Mengaktifkan pemetaan gerbang Non-Secure Callable (NSC) veneer internal prosesor Cortex-M33.'),

        ('4', 'SAU->RBAR\nSAU->RLAR', 'Base SAU', '0xE000EDD8 (S)\n0xE000EDDC (S)', 'Core Security', 
         'Region 0: Start = 0x0C0FE000, Limit = 0x0C0FFFFF (32 KB NSC Area)\nBit 0 ENABLE = 1, Bit 1 NSC = 1 (Non-Secure Callable)', 
         'Definisi Wilayah Gerbang NSC Veneer: Tempat instruksi Secure Gateway (SG) berada, memungkinkan Non-Secure memanggil API Secure secara terkontrol (partition_stm32u575xx.h).'),

        ('5', 'NVIC->ITNS[0..15]', 'Base NVIC', '0xE000E380 s.d.\n0xE000E3BC (S)', 'Core Security', 
         'Interrupt Target Non-Secure Register (1 bit per interrupt vector):\n• ITNS for LPUART1, USART3, GPDMA1_Ch0, PVD = 1 (Arahkan ke Non-Secure NVIC)\n• ITNS for EXTI3 (ADE IRQ), TAMP, RTC = 0 (Arahkan ke Secure NVIC)', 
         'Pengarah Vektor Interupsi Hardware TrustZone: Register vital yang membagi apakah sebuah interupsi ditangani oleh Core Secure atau Core Non-Secure RTOS (main.c baris 356-367).'),

        ('6', 'SysTick->CTRL\nSysTick->LOAD\nSysTick->VAL', 'Base SysTick', '0xE000E010 s.d.\n0xE000E018 (NS)', 'Non-Secure Core', 
         'CTRL: Bit 0 ENABLE=1, Bit 1 TICKINT=1, Bit 2 CLKSOURCE=1\nLOAD: Reload value untuk interval periodik 1,0 ms (Frekuensi HCLK / 1000)', 
         'Detak Sistem FreeRTOS Scheduler (1 ms Heartbeat): Menyuplai interval waktu bagi context switching task metering, display, dan stack DLMS.'),

        ('7', 'SCB->ICSR', 'Base SCB', '0xE000ED04 (NS)', 'Non-Secure Core', 
         'Bit 28 PENDSVSET = 1 (Memicu Interupsi PendSV)', 
         'Pemicu Context Switching RTOS: Digunakan oleh kernel FreeRTOS untuk melakukan pergantian task eksekusi secara kooperatif / preemptive.')
    ]
    render_system_table('3. Core Cortex-M33, Vector Table, & Routing Interupsi (SCB, SAU, NVIC ITNS)', core_data, '005599')

    # ========================== 4. DMA ENGINE ==========================
    dma_data = [
        ('1', 'GPDMA1_SECCFGR', '+0x00', '0x50020000 (S)', 'Secure World', 
         'Bits [15:0] SECx:\n• SEC0 = 0 (Channel 0 Non-Secure untuk USART2/LPUART1 Modem RX Buffer)\n• SEC1 = 1 (Channel 1 Secure untuk SPI1 ADE9000 Metering RX)', 
         'Alokasi Keamanan Saluran DMA: Menentukan apakah sebuah saluran transfer memori dikontrol oleh Secure World atau Non-Secure World.'),

        ('2', 'GPDMA1_PRIVCFGR', '+0x04', '0x50020004 (S)', 'Secure World', 
         'Bits [15:0] PRIVx: PRIV0 = 0 (Channel 0 dapat diakses Unprivileged task)', 
         'Hak Akses Privilese Saluran DMA: Memungkinkan driver UART FreeRTOS mengakses DMA tanpa memerlukan mode kernel handler.'),

        ('3', 'GPDMA1_Channel0->CCR', '+0x50', '0x40020050 (NS)', 'Non-Secure World', 
         'Bit 0 EN = 1 (Channel Enable)\nBit 8 HTIE = 1 (Half Transfer Interrupt), Bit 9 TCIE = 1 (Transfer Complete Interrupt)', 
         'Kontrol Channel 0 GPDMA: Mengaktifkan transfer sirkular RX serial dari modem tanpa menyela CPU Cortex-M33.'),

        ('4', 'GPDMA1_Channel0->CSAR\nGPDMA1_Channel0->CDAR', '+0x88\n+0x8C', '0x40020088 (NS)\n0x4002008C (NS)', 'Non-Secure World', 
         'CSAR = Alamat LPUART1_RDR (0x46002424) / USART2_RDR\nCDAR = Alamat Buffer RAM g_uart_rx_dma_buf di SRAM3', 
         'Source & Destination Address Register DMA: Mengalirkan data byte yang diterima dari port serial langsung ke circular array RAM.')
    ]
    render_system_table('4. Mesin DMA & Keamanan Saluran Komunikasi (GPDMA1 TrustZone)', dma_data, '333399')

    # ========================== 5. KRIPTOGRAFI & AUTO-ERASE ==========================
    crypto_data = [
        ('1', 'TAMP_BKP0R s.d.\nTAMP_BKP31R', '+0x30 s.d.\n+0xAC', '0x56007C30 s.d.\n0x56007CAC (S)', 'Secure (VSW Domain)', 
         '32 Register 32-bit (Total 128 Byte):\n• BKP0..BKP3: Kunci Enkripsi Master DLMS / GUEK (Global Unicast Encryption Key)\n• BKP4..BKP7: Kunci Otentikasi Penagihan\n• BKP8..BKP15: Nilai Akumulasi kWh Terakhir saat Padam & Timestamp Tamper', 
         'Brankas Data Penagihan & Kunci Rahasia Baterai (VBAT): Data tetap aman dan terjaga selama pemadaman listrik hingga > 10 tahun karena disuplai domain VSW.'),

        ('2', 'TAMP_ERCFGR', '+0x24', '0x56007C24 (S)', 'Secure World', 
         'Bit 0 ERCFG0 = 1 (Enable Hardware Auto-Erase on Tamper Event 1 / Case Open)', 
         'Penghancur Kunci Otomatis (Anti-Sabotase Fisik): Jika sakelar tutup meter (PC13) terpicu, hardware TAMP secara instan menghapus seluruh isi TAMP_BKPxR dan kunci SAES dalam 1 SIKLUS CLOCK! Kunci tidak dapat diekstrak oleh penyerang fisik di laboratorium.'),

        ('3', 'SAES_CR', '+0x00', '0x50060400 (S)', 'Secure World', 
         'Bit 0 EN = 1 (Akselerator AES Aktif)\nBits [4:3] MODE: 00b = Enkripsi, 01b = Dekripsi\nBits [6:5] CHMOD: 011b = Mode GCM (Galois/Counter Mode sesuai mandat DLMS Green Book & SPLN D5.002)', 
         'Akselerator Kriptografi Perangkat Keras Secure AES-GCM 128/256-bit: Menjalankan enkripsi frame DLMS/COSEM langsung di Secure World hardware tanpa mengekspos kunci ke RAM Non-Secure.'),

        ('4', 'RNG_CR\nRNG_DR', '+0x00\n+0x08', '0x50060800 (S)\n0x50060808 (S)', 'Secure World', 
         'RNG_CR: Bit 2 RNGEN = 1 (True Random Number Generator Enable)\nRNG_DR: Bits [31:0] RNDATA (Nilai Acak Entropi Fisik Termal Silikon)', 
         'Penghasil Bilangan Acak Benar (TRNG): Menghasilkan nilai Challenge / Nonce untuk mekanisme otentikasi HES-ke-Meter guna mencegah serangan pemutaran ulang (Replay Attack).')
    ]
    render_system_table('5. Brankas Kriptografi & Auto-Erase Sabotase Fisik (TAMP & SAES)', crypto_data, '660066')

    # ========================== 6. KONTROL DAYA & PVD ==========================
    power_data = [
        ('1', 'PWR_CR1', '+0x00', '0x46020800 (NS)\n0x56020800 (S)', 'System Power', 
         'Bits [2:0] LPMS[2:0] = 010b (Stop 2 Mode: Konsumsi Arus ~2,5 uA)\nBit 8 DBP = 1 (Buka proteksi penulisan domain Backup RTC & TAMP)', 
         'Pemilih Mode Daya Rendah Padam Listrik: Mengatur level tidur CPU ke Stop 2 dengan retensi penuh memori RAM saat pemadaman listrik jala-jala.'),

        ('2', 'PWR_SVMCR', '+0x10', '0x46020810 (NS)\n0x56020810 (S)', 'System Power', 
         'Bit 24 PVDE = 1 (Enable Programmable Voltage Detector)\nBits [23:21] PLS[2:0] = 011b (Threshold Level 3 = 2,50 Volt)\nBit 25 AVD_ON = 1', 
         'Pengawas Tegangan Suplai Dini (PVD): Memicu interupsi darurat PVD_PVM_IRQn saat tegangan VDD anjlok ke 2,5 V, memberi waktu MCU melakukan Last-Gasp flash save.'),

        ('3', 'PWR_BDCR2', '+0x24', '0x56020824 (S)', 'Secure World', 
         'Bit 0 VBE = 1 (Enable VBAT Battery Charging)\nBit 1 VBRS = 0 (Resistor 5 kOhm) / 1 (Resistor 1,5 kOhm)', 
         'Charger Baterai Cadangan / Superkapasitor: Mengisi ulang superkapasitor backup RTC saat listrik PLN kembali menyala secara otomatis.'),

        ('4', 'PWR_VOSR', '+0x0C', '0x4602080C (NS)\n0x5602080C (S)', 'System Power', 
         'Bits [17:16] VOS = 01b (Voltage Scaling Range 1: Operasi Penuh 160 MHz)', 
         'Regulator Tegangan Inti CPU (Core Voltage): Menyuplai daya internal VCore untuk kestabilan kalkulasi FFT/Harmonisa ADE9000 pada kecepatan 160 MHz.')
    ]
    render_system_table('6. Kontrol Daya Rendah, Deteksi Padam PVD, & Pengisian Baterai (PWR)', power_data, '006666')

    # ========================== 7. DUAL-WATCHDOG ==========================
    watchdog_data = [
        ('1', 'IWDG_KR\nIWDG_PR\nIWDG_RLR', 'Base IWDG', '0x40003000 (NS)\n0x50003000 (S)', 'System Reliability', 
         'KR = 0x5555 (Unlock), KR = 0xCCCC (Start), KR = 0xAAAA (Refresh)\nPR = Bits [2:0] = 100b (Divider /64)\nRLR = Bits [11:0] = 1000 (Timeout ~2.000 ms pada LSI 32 kHz)', 
         'Independent Watchdog (IWDG): Pengawas independen berbasis osilator LSI internal. Jika osilator utama macet atau kode RTOS mengalami deadlock, IWDG mereset paksa MCU dalam 2 detik.'),

        ('2', 'WWDG_CR\nWWDG_CFR', 'Base WWDG', '0x40002C00 (NS)\n0x50002C00 (S)', 'System Reliability', 
         'CR: Bit 7 WDGA = 1 (Aktif), Bits [6:0] T[6:0] (Downcounter)\nCFR: Bits [6:0] W[6:0] (Window Threshold: Refresh hanya boleh dilakukan saat T < W)', 
         'Window Watchdog (WWDG): Menjamin task eksekusi metrologi berjalan tepat waktu. Jika task refresh dieksekusi terlalu cepat (anomali) atau terlambat, WWDG memicu reset sistem.')
    ]
    render_system_table('7. Pengawas Keandalan Operasi Sistem (Dual-Watchdog IWDG & WWDG)', watchdog_data, '663300')

    # Simpan berkas
    target_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Register_Sistem_Internal_NonPin_STM32U585.docx'
    doc.save(target_path)
    print(f'SUCCESS: Document saved to {target_path}')

if __name__ == '__main__':
    create_system_reg_tz_doc()

