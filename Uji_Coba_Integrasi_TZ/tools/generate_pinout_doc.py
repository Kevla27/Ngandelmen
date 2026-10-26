import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml import parse_xml
from docx.oxml.ns import nsdecls

def create_meter_pinout_doc():
    doc = Document()

    # Set standard margins (0.5 inch for wide tables)
    for section in doc.sections:
        section.top_margin = Inches(0.5)
        section.bottom_margin = Inches(0.5)
        section.left_margin = Inches(0.5)
        section.right_margin = Inches(0.5)
        section.page_width = Inches(8.5)
        section.page_height = Inches(11.0)

    def set_cell_background(cell, fill_hex):
        tcPr = cell._tc.get_or_add_tcPr()
        shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{fill_hex}"/>')
        tcPr.append(shd)

    def set_cell_margins(cell, top=80, bottom=80, left=100, right=100):
        tcPr = cell._tc.get_or_add_tcPr()
        tcMar = parse_xml(f'<w:tcMar {nsdecls("w")}><w:top w:w="{top}" w:type="dxa"/><w:bottom w:w="{bottom}" w:type="dxa"/><w:left w:w="{left}" w:type="dxa"/><w:right w:w="{right}" w:type="dxa"/></w:tcMar>')
        tcPr.append(tcMar)

    # Title
    title_p = doc.add_paragraph()
    title_run = title_p.add_run('SPESIFIKASI KONFIGURASI PIN HARDWARE MIKROKONTROLER')
    title_run.font.name = 'Arial'
    title_run.font.size = Pt(15)
    title_run.font.bold = True
    title_run.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    subtitle_p = doc.add_paragraph()
    sub_run = subtitle_p.add_run('Smart Meter Listrik 3-Fasa Pascabayar (SPLN D3.006:2021) - STM32U585xx TrustZone & RS-232 Port')
    sub_run.font.name = 'Arial'
    sub_run.font.size = Pt(10)
    sub_run.font.italic = True
    sub_run.font.color.rgb = RGBColor(0x55, 0x55, 0x55)
    subtitle_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    # Section 1
    h1 = doc.add_paragraph()
    r1 = h1.add_run('1. Antarmuka Port Komunikasi Dua Arah RJ-45 (RS-232 Sesuai Tabel 3 SPLN D3.006:2021)')
    r1.font.name = 'Arial'
    r1.font.size = Pt(11)
    r1.font.bold = True
    r1.font.color.rgb = RGBColor(0x00, 0x33, 0x66)

    table_rj = doc.add_table(rows=9, cols=5)
    table_rj.alignment = WD_TABLE_ALIGNMENT.CENTER
    table_rj.autofit = False

    headers_rj = ['Pin RJ-45', 'Fungsi RS-232 (SPLN)', 'Status Pin', 'Sinyal ke MCU STM32U585', 'Keterangan Fungsional']

    hdr_cells = table_rj.rows[0].cells
    for i, name in enumerate(headers_rj):
        hdr_cells[i].text = name
        set_cell_background(hdr_cells[i], '003366')
        set_cell_margins(hdr_cells[i])
        p = hdr_cells[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        for run in p.runs:
            run.font.name = 'Arial'
            run.font.size = Pt(8.5)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    data_rj = [
        ('Pin 1', 'NC', 'Tidak Terhubung', '-', 'Not Connected (Mengambang)'),
        ('Pin 2', 'NC', 'Tidak Terhubung', '-', 'Not Connected (Mengambang)'),
        ('Pin 3', 'NC', 'Tidak Terhubung', '-', 'Not Connected (Mengambang)'),
        ('Pin 4', 'RX (in)', 'Data Masuk', 'Transceiver MAX3232 -> PA3 (LPUART1_RX)', 'Jalur Terima Data dari Modem ke Meter'),
        ('Pin 5', 'TX (out)', 'Data Keluar', 'PA2 (LPUART1_TX) -> Transceiver MAX3232', 'Jalur Kirim Data dari Meter ke Modem'),
        ('Pin 6', 'GROUND', 'Ground Referensi', 'GND Sistem Meter (0V)', 'Ground Bersama (Common Signal Ground)'),
        ('Pin 7', 'NC', 'Tidak Terhubung', '-', 'Not Connected (Mengambang)'),
        ('Pin 8', 'NC', 'Tidak Terhubung', '-', 'Not Connected (Mengambang)')
    ]

    for row_idx, row_data in enumerate(data_rj, start=1):
        row_cells = table_rj.rows[row_idx].cells
        bg_color = 'F2F5F8' if row_idx % 2 == 1 else 'FFFFFF'
        for col_idx, text in enumerate(row_data):
            row_cells[col_idx].text = text
            set_cell_background(row_cells[col_idx], bg_color)
            set_cell_margins(row_cells[col_idx], 60, 60, 80, 80)
            p = row_cells[col_idx].paragraphs[0]
            if col_idx in [0, 1, 2]:
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(8)

    doc.add_paragraph()

    # Section 2
    h2 = doc.add_paragraph()
    r2 = h2.add_run('2. Tabel Master Konfigurasi Pin MCU STM32U585xx Lengkap (RS-232 Dedicated)')
    r2.font.name = 'Arial'
    r2.font.size = Pt(11)
    r2.font.bold = True
    r2.font.color.rgb = RGBColor(0x00, 0x33, 0x66)

    master_headers = ['No', 'Pin MCU', 'Sinyal Fungsional', 'Domain Daya', 'Domain Keamanan', 'Mode I/O & Speed', 'Rujukan SPLN', 'Fungsi & Peran Teknis Sistem']
    master_data = [
        ('1', 'PA2', 'MODEM_TX', 'VDD', 'Non-Secure', 'AF8 (Push-Pull, High)', 'Tabel 3 & 6.12.1', 'Jalur TX ke RJ-45 Pin 5: RS-232 TX (out) @ 19.200 bps via MAX3232'),
        ('2', 'PA3', 'MODEM_RX', 'VDD', 'Non-Secure', 'AF8 (Pull-Up, High)', 'Tabel 3 & 6.12.1', 'Jalur RX dari RJ-45 Pin 4: RS-232 RX (in) via DMA Ring-Buffer HDLC'),
        ('3', 'PB10', 'MODEM_PWR_EN', 'VDD', 'Non-Secure', 'Output (Pull-Down, Low)', 'Butir 6.13', 'Saklar Daya DC 12V 5W & Hard Reset Modem via Power Cycle (Pemutus MOSFET 12V)'),
        ('4', 'PC4', 'OPTICAL_TX', 'VDD', 'Non-Secure', 'AF7 (Push-Pull, High)', 'Butir 6.12.2', 'Port Komunikasi Lokal Optical Probe Depan Meter (USART3 TX @ 9.600 bps)'),
        ('5', 'PC5', 'OPTICAL_RX', 'VDD', 'Non-Secure', 'AF7 (Pull-Up, High)', 'Butir 6.12.2', 'Jalur Terima Data Optical Probe Depan Meter (USART3 RX @ 9.600 bps)'),
        ('6', 'VBAT', 'VBAT_IN', 'VBAT Supply', 'Supply Pin', 'Power Pin', 'Butir 6.4', 'Input Baterai Cadangan Lithium Li-SOCl2 3.6V (Suplai Backup Domain & VSW)'),
        ('7', 'PC13', 'TAMP_CASE_OPEN', 'VSW (VBAT)', 'Secure World', 'Input EXTI / TAMP_IN1', 'Tabel 6 No 1', 'Sabotase Tutup Meter: Beroperasi di Baterai VBAT, Rekam RUSAK & Kunci KWh'),
        ('8', 'PE6', 'TAMP_TERM_COVER', 'VSW (VBAT)', 'Secure World', 'Input EXTI / TAMP_IN3', 'Tabel 6 No 2', 'Sabotase Tutup Terminal MCB: Beroperasi di VBAT, Rekam ERR20 & Trip Relai'),
        ('9', 'PC0', 'SENSOR_MAGNET', 'VDD', 'Non-Secure', 'Input EXTI0 Falling', 'Tabel 6 No 7', 'Deteksi Induksi Medan Magnet Luar > 500 mT dari Sensor Hall (ERR25)'),
        ('10', 'PC15', 'LCD_PWR_GATE', 'VSW (VBAT)', 'Non-Secure', 'Output (VSW Low Speed)', 'Butir 6.11', 'Saklar Daya Display Darurat: Nyalakan Daya LCD 10 Detik Saat Listrik Padam'),
        ('11', 'PE4', 'BTN_WAKE_LCD', 'VSW (VBAT)', 'Non-Secure', 'Input / WKUP1', 'Butir 6.10 & 6.11', 'Tombol Bangunkan LCD Darurat: Push-to-Read Saat Pemadaman Listrik'),
        ('12', 'PB8', 'I2C1_SCL', 'VDD', 'Non-Secure', 'AF4 (Open-Drain, High)', 'Butir 6.11', 'Clock Bus I2C1 Layar OLED/LCD SSD1306 (Carousel Parameter SPLN Gbr 4)'),
        ('13', 'PB9', 'I2C1_SDA', 'VDD', 'Non-Secure', 'AF4 (Open-Drain, High)', 'Butir 6.11', 'Data Bus I2C1 Layar OLED/LCD SSD1306'),
        ('14', 'PB2', 'BTN_SCROLL_UP', 'VDD', 'Non-Secure', 'Input (Pull-Up Internal)', 'Butir 6.10', 'Tombol Navigasi Manual Layar Scrolling Naik (▲)'),
        ('15', 'PB6', 'BTN_SCROLL_DN', 'VDD', 'Non-Secure', 'Input (Pull-Up Internal)', 'Butir 6.10 & Tab 5', 'Tombol Navigasi Layar Scrolling Turun (▼) & Konfirmasi Reconnect Relai'),
        ('16', 'PB7', 'BUZZER_ALM', 'VDD', 'Non-Secure', 'Output (Push-Pull, Low)', 'Butir 6.9 c', 'Driver Transistor Buzzer Audio & Lampu Indikator LED Kuning Alarm/Tamper'),
        ('17', 'PB4', 'LED_PULSE_KWH', 'VDD', 'Non-Secure', 'Output (Push-Pull, High)', 'Butir 6.9 a', 'Indikator LED Merah 1: Keluaran Pulsa Kalibrasi Energi Aktif (imp/kWh)'),
        ('18', 'PB5', 'LED_PULSE_KVAR', 'VDD', 'Non-Secure', 'Output (Push-Pull, High)', 'Butir 6.9 b', 'Indikator LED Merah 2: Keluaran Pulsa Kalibrasi Energi Reaktif (imp/kVARh)'),
        ('19', 'PC8', 'CTRL_RELAY_TRIP', 'VDD', 'Non-Secure', 'Output (Pull-Down, Low)', 'Tabel 2 & Tab 5', 'Kontrol Shunt Trip MCCB Eksternal (Pemutus Beban Sambungan Tidak Langsung)'),
        ('20', 'PC9', 'SPARE / CTRL_OUT', 'VDD', 'Non-Secure', 'Output (Pull-Down, Low)', 'Tabel 2 (Cadangan)', 'Cadangan Kontrol / Spare Digital Output (Bukan Relai Close karena MCCB Manual)'),
        ('21', 'PC7', 'RELAY_STATUS', 'VDD', 'Non-Secure', 'Input (Pull-Down)', 'Tabel 4 No 7', 'Membaca Kontak Bantu Relai / Deteksi Relai Macet/Gagal Operasi (ERR07)'),
        ('22', 'PA1', 'VBAT_SENSE', 'VDD', 'Non-Secure', 'Analog (ADC1_IN6)', 'Butir 6.11 & Tab 4', 'Monitor Tegangan Baterai Cadangan (Memicu ERR03 Bila < 2.7 V)'),
        ('23', 'PA4', 'ADE_NSS', 'VDD', 'Non-Secure', 'Output (Push-Pull, VHigh)', 'Metrologi ADE9000', 'SPI Chip Select (CS) AFE Metrologi 3-Fasa ADE9000'),
        ('24', 'PA5', 'ADE_SCK', 'VDD', 'Non-Secure', 'AF5 (SPI1_SCK, VHigh)', 'Metrologi ADE9000', 'Serial Clock SPI1 ADE9000'),
        ('25', 'PA6', 'ADE_MISO', 'VDD', 'Non-Secure', 'AF5 (SPI1_MISO, VHigh)', 'Metrologi ADE9000', 'Master-In Slave-Out SPI1 ADE9000'),
        ('26', 'PA7', 'ADE_MOSI', 'VDD', 'Non-Secure', 'AF5 (SPI1_MOSI, VHigh)', 'Metrologi ADE9000', 'Master-Out Slave-In SPI1 ADE9000'),
        ('27', 'PB0', 'ADE_IRQ0', 'VDD', 'Non-Secure', 'Input EXTI0 Falling', 'Tabel 6 No 3 & 8', 'Interupsi HW ADE9000: Arus Balik (ERR26) & Urutan Fase Terbalik (ERR21)'),
        ('28', 'PB1', 'ADE_IRQ1', 'VDD', 'Non-Secure', 'Input EXTI1 Falling', 'Tabel 6 No 5 & 6', 'Interupsi HW ADE9000: Netral Putus (ERR23) & Hilang Tegangan (ERR24)'),
        ('29', 'PA9', 'VCP_TX', 'VDD', 'Non-Secure', 'AF7 (USART1_TX, High)', 'Console IO', 'Transmit Serial Debugging ST-LINK VCP @ 115.200 bps'),
        ('30', 'PA10', 'VCP_RX', 'VDD', 'Non-Secure', 'AF7 (USART1_RX, High)', 'Console IO', 'Receive Serial Debugging ST-LINK VCP (Konsol BIST Shell Interaktif)'),
        ('31', 'PA13', 'SWDIO', 'VDD', 'Non-Secure', 'AF0 (JTMS-SWDIO)', 'ST-LINK Debug', 'Jalur Data Serial Wire Debugging (SWD) ST-LINK'),
        ('32', 'PA14', 'SWCLK', 'VDD', 'Non-Secure', 'AF0 (JTCK-SWCLK)', 'ST-LINK Debug', 'Jalur Clock Serial Wire Debugging (SWD) ST-LINK'),
        ('33', 'PE12', 'ADE_CF1', 'VDD', 'Secure World', 'Input (Pull-Up, EXTI12)', 'Butir 6.10 & 6.9 a', 'Pulsa Kalibrasi Energi Aktif Total (CF1 Pin 33 ADE9000, 1000 imp/kWh, 80 ms) paralel ke LED Merah 1 PB4 untuk validasi mandiri register Wh'),
        ('34', 'PE13', 'ADE_CF2', 'VDD', 'Secure World', 'Input (Pull-Up, EXTI13)', 'Butir 6.10 & 6.9 b', 'Pulsa Kalibrasi Energi Reaktif Total (CF2 Pin 34 ADE9000, 1000 imp/kvarh, 80 ms) paralel ke LED Merah 2 PB5 untuk validasi mandiri register varh 4-kuadran'),
        ('35', 'PE8', 'ADE_CF3_ZX', 'VDD', 'Secure World', 'Input EXTI8 Dual-Edge', 'Butir 5.2 & Tabel 2', 'Fungsi Ganda Pin 35 ADE9000: Pulsa Energi Semu (CF3 / imp/kVAh) atau Zero-Crossing (ZX) untuk Zero-Current Switching Shunt Trip'),
        ('36', 'PE11', 'ADE_EVENT', 'VDD', 'Secure World', 'Input EXTI11 Falling', 'Butir 10.4 & Tabel 10-11', 'Power Quality Hardware Event (CF4 ADE9000 disetel sebagai EVENT, sebelumnya DREADY): Deteksi Cepat Sag/Dip, Swell, & Overcurrent'),
        ('37', 'PE7', 'ADE_RESET', 'VDD', 'Secure World', 'Output Push-Pull, High', 'Keandalan AFE', 'Hardware Reset Pin AFE ADE9000 (Pin 30 ADE9000) untuk pemulihan otonom jika sampling ADC terhenti'),
        ('38', 'PE9', 'ADE_PM0', 'VDD', 'Secure World', 'Output Push-Pull, Low', 'Butir 6.4 (Hemat Daya)', 'Kontrol Power Mode 0 ADE9000: Bersama PM1 mengatur PSM0 Run / PSM3 Total Sleep ~2uA saat listrik padam'),
        ('39', 'PE10', 'ADE_PM1', 'VDD', 'Secure World', 'Output Push-Pull, Low', 'Butir 6.4 (Hemat Daya)', 'Kontrol Power Mode 1 ADE9000: Menurunkan mode daya ADE saat listrik PLN padam')
    ]

    table_master = doc.add_table(rows=len(master_data)+1, cols=len(master_headers))
    table_master.alignment = WD_TABLE_ALIGNMENT.CENTER
    table_master.autofit = False

    hdr_m = table_master.rows[0].cells
    for i, name in enumerate(master_headers):
        hdr_m[i].text = name
        set_cell_background(hdr_m[i], '003366')
        set_cell_margins(hdr_m[i], 80, 80, 60, 60)
        p = hdr_m[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        for run in p.runs:
            run.font.name = 'Arial'
            run.font.size = Pt(8)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    for row_idx, row_data in enumerate(master_data, start=1):
        row_cells = table_master.rows[row_idx].cells
        bg_color = 'F2F5F8' if row_idx % 2 == 1 else 'FFFFFF'
        if 'Secure World' in row_data[4]:
            bg_color = 'FFF2E6' # Highlight Secure World
        elif 'VSW (VBAT)' in row_data[3]:
            bg_color = 'EBF5FB' # Highlight VBAT Domain
        for col_idx, text in enumerate(row_data):
            row_cells[col_idx].text = text
            set_cell_background(row_cells[col_idx], bg_color)
            set_cell_margins(row_cells[col_idx], 50, 50, 50, 50)
            p = row_cells[col_idx].paragraphs[0]
            if col_idx in [0, 1, 3, 4]:
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(7.5)
                if col_idx == 1:
                    run.font.bold = True

    doc.add_paragraph()

    # Section 3
    h3 = doc.add_paragraph()
    r3 = h3.add_run('3. Rincian Konfigurasi Register & Bit-Bit Terkait (STM32U585)')
    r3.font.name = 'Arial'
    r3.font.size = Pt(11)
    r3.font.bold = True
    r3.font.color.rgb = RGBColor(0x00, 0x33, 0x66)

    reg_headers = ['Blok Periferal', 'Register Terkait', 'Bit-Bit Penting & Nilai', 'Deskripsi Konfigurasi Teknis']
    reg_data = [
        ('LPUART1 (Modem RS-232)', 'LPUART1_BRR\nLPUART1_CR1\nLPUART1_CR3', 'BRR = 0x00208D55 (19.200 bps)\nCR1: UE=1, TE=1, RE=1, FIFOEN=1\nCR3: DMAR=1 (DMA Rx Circular)', 'Konfigurasi serial asynchronous 8N1 @ 19.200 bps khusus RS-232 3-Wire (TX, RX, GND) via MAX3232 ke RJ-45 Pin 4 & Pin 5.'),
        ('TAMP (VBAT Domain)', 'PWR_BDCR1\nTAMP_CR1\nTAMP_SECCFGR', 'BDCR1: LSEON=1, RTCEN=1\nCR1: TAMP1E=1 (PC13), TAMP3E=1 (PE6)\nSECCFGR: TAMP1SEC=1, TAMP3SEC=1', 'Mengaktifkan sensor anti-sabotase tutup meter (PC13) dan tutup terminal (PE6) di bawah daya baterai VBAT dengan proteksi isolasi Secure World.'),
        ('Display Darurat (PC15 & PE4)', 'PWR_WUSR\nGPIOC_MODER', 'PWR_WUSR: WKUP1 diaktifkan (PE4)\nGPIOC_MODER[31:30] = 01b (PC15 Output)', 'Tombol PE4 membangunkan MCU dari mode Shutdown saat pemadaman, dan PC15 menyalakan saklar MOSFET daya display selama jendela 10 detik.'),
        ('I2C1 Display OLED/LCD', 'GPIOB_MODER\nGPIOB_OTYPER\nI2C1_TIMINGR', 'MODER[17:16]=10b, MODER[19:18]=10b\nOTYPER Bit 8=1, Bit 9=1 (Open-Drain)\nTIMINGR = 0x3042C7C7 (100 kHz)', 'Antarmuka I2C1 standar kecepatan 100 kHz pada bus PB8/PB9 untuk menampilkan carousel informasi pelanggan sesuai standar SPLN Gambar 4.'),
        ('AFE Metrologi ADE9000', 'GPIOA_AFRL\nEXTI_FTSR1\nEXTI_IMR1', 'AFRL Bits [31:16] = AF5 (SPI1)\nFTSR1 Bit 0=1 (PB0), Bit 1=1 (PB1)\nIMR1 Bit 0=1, Bit 1=1 (Unmask IRQ)', 'Komunikasi SPI1 full duplex kecepatan tinggi untuk pembacaan register energi dan penanganan interupsi hardware EXTI Falling Edge untuk tamper arus.'),
        ('Relai Beban Latching', 'GPIOC_MODER\nGPIOC_PUPDR', 'MODER[17:16]=01b, MODER[19:18]=01b\nPUPDR Bits = 10b (Pull-Down)', 'Keluaran pulsa 100 ms untuk menggerakkan koil pembuka (PC8 Trip) dan koil penutup (PC9 Close) relai pemutus beban.')
    ]

    table_reg = doc.add_table(rows=len(reg_data)+1, cols=len(reg_headers))
    table_reg.alignment = WD_TABLE_ALIGNMENT.CENTER
    table_reg.autofit = False

    hdr_r = table_reg.rows[0].cells
    for i, name in enumerate(reg_headers):
        hdr_r[i].text = name
        set_cell_background(hdr_r[i], '003366')
        set_cell_margins(hdr_r[i], 80, 80, 60, 60)
        p = hdr_r[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        for run in p.runs:
            run.font.name = 'Arial'
            run.font.size = Pt(8)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    for row_idx, row_data in enumerate(reg_data, start=1):
        row_cells = table_reg.rows[row_idx].cells
        bg_color = 'F2F5F8' if row_idx % 2 == 1 else 'FFFFFF'
        for col_idx, text in enumerate(row_data):
            row_cells[col_idx].text = text
            set_cell_background(row_cells[col_idx], bg_color)
            set_cell_margins(row_cells[col_idx], 50, 50, 50, 50)
            p = row_cells[col_idx].paragraphs[0]
            if col_idx in [0, 1]:
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(7.5)
                if col_idx in [0, 1]:
                    run.font.bold = True

    doc.add_paragraph()

    # Section 4
    h4 = doc.add_paragraph()
    r4 = h4.add_run('4. Catatan Hasil Audit Hardware & Keselarasan SPLN D3.006:2021')
    r4.font.name = 'Arial'
    r4.font.size = Pt(11)
    r4.font.bold = True
    r4.font.color.rgb = RGBColor(0x00, 0x33, 0x66)

    notes = [
        ("a. Antarmuka Fisik Kompartemen Modem (Butir 6.5, 6.12.1, 6.13 & Tabel 3):", 
         "Sesuai SPLN, modem adalah modul lepas-pasang (removable) di kompartemen terpisah dengan 2 konektor: RJ-45 (Pin 4: RX, Pin 5: TX, Pin 6: GND) dan DC Power Jack 12V. "
         "Pin MODEM_RESET dan MODEM_STATUS ditiadakan dari konektor fisik karena tidak memiliki jalur pin fisik pada RJ-45 (Pin 1, 2, 3, 7, 8 adalah NC). "
         "Fungsi reset modem dilakukan secara andal melalui Hard Power Cycle 12V menggunakan PB10 (MODEM_PWR_EN memutus gate MOSFET 12V selama 5 detik). "
         "Status modem dideteksi secara logis via polling frame DLMS/COSEM pada LPUART1 (PA2/PA3)."),
        ("b. Domain Daya Cadangan VBAT / VSW (Datasheet STM32U585 DS13086):",
         "Pin PC13 (TAMP_IN1) dan PE6 (TAMP_IN3) disuplai oleh domain VSW (disuplai VBAT) dengan proteksi isolasi TrustZone Secure World. "
         "Hal ini menjamin pencatatan tamper tutup meter (RUSAK) dan tutup terminal (ERR20) tetap aktif saat listrik padam total. "
         "Pin PE4 (WKUP1) dan PC15 (VSW output gate) melayani pembacaan display darurat saat pemadaman (Push-to-Read 10 detik)."),
        ("c. Komunikasi Serial RS-232 Dedicated:",
         "Sesuai pilihan konfigurasi RS-232 dedicated, pin multiplexer/selektor RS-485 (PB13) ditiadakan. "
         "Hardware transceiver menggunakan IC MAX3232 / ST3232 standar 3.3V to RS-232 levels, menyederhanakan routing PCB dan membebaskan I/O mikrokontroler.")
    ]

    for title, text in notes:
        p_note = doc.add_paragraph()
        r_title = p_note.add_run(title + " ")
        r_title.font.name = 'Arial'
        r_title.font.size = Pt(8.5)
        r_title.font.bold = True
        r_title.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
        
        r_text = p_note.add_run(text)
        r_text.font.name = 'Arial'
        r_text.font.size = Pt(8.5)
        r_text.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    primary_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Konfigurasi_Pin_STM32U585_SPLN_D3.006.docx'
    final_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Konfigurasi_Pin_STM32U585_SPLN_D3.006_Final.docx'
    try:
        doc.save(primary_path)
        print('SUCCESS: File saved at ' + primary_path)
    except PermissionError:
        print('WARNING: Primary file locked in Word: ' + primary_path)
    try:
        doc.save(final_path)
        print('SUCCESS: Backup copy saved at ' + final_path)
    except PermissionError:
        print('WARNING: Final file locked in Word: ' + final_path)

if __name__ == '__main__':
    create_meter_pinout_doc()

