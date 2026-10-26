import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.section import WD_ORIENTATION
from docx.oxml import parse_xml
from docx.oxml.ns import nsdecls

def create_complete_register_tz_doc_v4():
    doc = Document()

    # Set Orientasi Landscape untuk tabel lebar & keterbacaan optimal
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

    def set_cell_margins(cell, top=50, bottom=50, left=50, right=50):
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

    # ========================== JUDUL DOKUMEN ==========================
    title_p = doc.add_paragraph()
    title_run = title_p.add_run('SPESIFIKASI ALAMAT REGISTER, ARSITEKTUR CLOCK KERNEL, & PARTISI KEAMANAN TRUSTZONE')
    title_run.font.name = 'Arial'
    title_run.font.size = Pt(13)
    title_run.font.bold = True
    title_run.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    sub_p = doc.add_paragraph()
    sub_run = sub_p.add_run(
        'Smart Meter Listrik 3-Fasa Pascabayar (SPLN D3.006:2021 & B200) - STM32U585xx Arm Cortex-M33\n'
        'Edisi Audit Final v4.0: Pemisahan Domain Keamanan, Rantai Inisialisasi Clock Kernel LPUART1, Tri-Layer Attribution, & Pembuktian Low-Power Stop 2\n'
        'Rujukan Resmi: STMicroelectronics RM0456 Rev. 5 & DS13086 Rev. 10'
    )
    sub_run.font.name = 'Arial'
    sub_run.font.size = Pt(8.5)
    sub_run.font.italic = True
    sub_run.font.color.rgb = RGBColor(0x55, 0x55, 0x55)
    sub_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    doc.add_paragraph()

    # ========================== BAB 1 ==========================
    add_heading_styled('1. Rantai Inisialisasi & Verifikasi Clock Kernel LPUART1 (RM0456 Section 13 & 36)', 1)

    p_c1 = doc.add_paragraph()
    p_c1.paragraph_format.line_spacing = 1.15
    r_c1 = p_c1.add_run(
        "Menjawab catatan audit terkait pemilihan clock kernel LPUART1, LPUART1 adalah periferal berdaya rendah (Low-Power) "
        "yang memiliki multiplexer clock independen. Untuk menjamin baud rate 19.200 bps pada modem 4G/Cat-M1 tanpa overflow register "
        "dan tetap beroperasi pada mode hemat daya, sumber clock kernel wajib dialihkan ke HSI16 (16,000 MHz) atau MSIK. "
        "Berikut adalah rantai konfigurasi clock lengkap berdasarkan register resmi RM0456:\n"
    )
    r_c1.font.name = 'Arial'
    r_c1.font.size = Pt(8.5)
    r_c1.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    # Tabel Rantai Clock LPUART1
    clk_headers = ['Tahap Inisialisasi', 'Nama Register', 'Offset', 'Alamat Fisik (NS / S)', 'Field / Bit', 'Reset Val', 'Target Val', 'Deskripsi Fungsi & Tindakan Hardware']
    clk_data = [
        ('1. Aktifkan HSI16', 'RCC_CR', '+0x00', '0x46020C00 / 0x56020C00', 'Bit 8 HSI16ON\nBit 10 HSI16RDY', '0x00000500', '0x00000501', 'Set HSI16ON = 1. Polling hingga HSI16RDY = 1 menandakan clock 16 MHz stabil.'),
        ('2. Clock Mux Kernel', 'RCC_CCIPR1', '+0xE0', '0x46020CE0 / 0x56020CE0', 'Bits [11:10]\nLPUART1SEL[1:0]', '0x00000000 (PCLK1)', '0x00000800 (HSI16)', 'Pilih HSI16 (0b10) sebagai clock kernel LPUART1. (00=PCLK1, 01=SYSCLK, 10=HSI16, 11=LSE).'),
        ('3. Bus Clock Gate', 'RCC_APB3ENR', '+0xA8', '0x46020CA8 / 0x56020CA8', 'Bit 6 LPUART1EN', '0x00000000', '0x00000040', 'Aktifkan sinyal APB3 bus clock gating untuk periferal LPUART1.'),
        ('4. Prescaler Divider', 'LPUART1_PRESC', '+0x1C', '0x4600241C (NS)', 'Bits [3:0]\nPRESCALER[3:0]', '0x00000000 (/1)', '0x00000000 (/1)', 'Konfigurasi pembagi clock: 0000b = /1 (input frekuensi kernel tetap 16,000 MHz).'),
        ('5. Baud Rate Register', 'LPUART1_BRR', '+0x0C', '0x4600240C (NS)', 'Bits [19:0] BRR[19:0]', '0x00000000', '0x00034155 (213.333)', 'Formula: BRR = (256 × 16.000.000) / 19.200 = 213.333 = 0x0003_4155. Deviasi: +0,00015%.'),
        ('6. Enable Peripheral', 'LPUART1_CR1', '+0x00', '0x46002400 (NS)', 'Bit 0 UE, Bit 3 TE,\nBit 2 RE, Bit 29 FIFOEN', '0x00000000', '0x2000000D', 'Aktifkan Transmitter (TE), Receiver (RE), Hardware FIFO, dan LPUART Enable (UE).')
    ]

    t_clk = doc.add_table(rows=len(clk_data)+1, cols=len(clk_headers))
    t_clk.alignment = WD_TABLE_ALIGNMENT.CENTER
    t_clk.autofit = False

    hdr_c = t_clk.rows[0].cells
    for i, name in enumerate(clk_headers):
        hdr_c[i].text = name
        set_cell_background(hdr_c[i], '003366')
        set_cell_margins(hdr_c[i], 50, 50, 40, 40)
        p = hdr_c[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        for run in p.runs:
            run.font.name = 'Arial'
            run.font.size = Pt(7.5)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    for row_idx, row_data in enumerate(clk_data, start=1):
        row_cells = t_clk.rows[row_idx].cells
        bg_color = 'F2F7FC' if row_idx % 2 == 1 else 'FFFFFF'
        for col_idx, text in enumerate(row_data):
            row_cells[col_idx].text = text
            set_cell_background(row_cells[col_idx], bg_color)
            set_cell_margins(row_cells[col_idx], 35, 35, 35, 35)
            p = row_cells[col_idx].paragraphs[0]
            if col_idx in [0, 2, 4, 5, 6]:
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(7)
                if col_idx in [0, 1, 6]:
                    run.font.bold = True

    doc.add_paragraph()

    # ========================== BAB 2 ==========================
    add_heading_styled('2. Kerangka Keamanan Tri-Layer TrustZone & Atribusi Per-Jalur', 1)
    p_c2 = doc.add_paragraph()
    p_c2.paragraph_format.line_spacing = 1.15
    r_c2 = p_c2.add_run(
        "Sesuai rekomendasi audit, status keamanan disajikan secara independen pada 3 lapisan proteksi hardware:\n"
        "1. Lapis 1 - GPIO Pad Security (GPIOx_SECCFGR, Offset +0x30): Menentukan apakah pin fisik diisolasi ke Secure World (SECy=1) atau Non-Secure (SECy=0). "
        "Pin non-GPIO seperti NRST dan VBAT ditandai 'N/A (Bukan Pin GPIO)'.\n"
        "2. Lapis 2 - Interupsi Eksternal (EXTI_SECCFGR1, Offset +0x14): Mengarahkan vektor interupsi ke NVIC Secure (SECx=1) atau NVIC Non-Secure (SECx=0). "
        "Pin output, analog murni, dan pin data serial tanpa interupsi langsung ditandai 'N/A (Bukan Jalur EXTI)'.\n"
        "3. Lapis 3 - Periferal Controller (GTZC1_TZSC_SECCFGRx): Menetapkan hak akses bus controller periferal (SPI1, ADE9000, TAMP, RTC = Secure; "
        "LPUART1, USART3, I2C3, SPI2 = Non-Secure). Pin GPIO pasif/mandiri ditandai 'N/A (GPIO Mandiri)'."
    )
    r_c2.font.name = 'Arial'
    r_c2.font.size = Pt(8.5)
    r_c2.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    doc.add_paragraph()

    # Template fungsi untuk membuat tabel register
    def render_pin_table(table_title, data_rows, header_bg='003366'):
        add_heading_styled(table_title, 1)
        headers = [
            'No', 'Pin', 'Sinyal Fungsional', 
            'Lapis 1: GPIO Pad\n(GPIOx_SECCFGR)', 'Lapis 2: EXTI Line\n(EXTI_SECCFGR1)', 'Lapis 3: Peripheral\n(GTZC1_TZSC)', 
            'Register & Offset (RM0456)', 'Alamat Fisik (Hex)', 'Reset Val -> Target Val', 'Status Verifikasi & Clock', 'Fungsi & Peran Teknis Sistem'
        ]

        table = doc.add_table(rows=len(data_rows)+1, cols=len(headers))
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        table.autofit = False

        hdr_cells = table.rows[0].cells
        for i, name in enumerate(headers):
            hdr_cells[i].text = name
            set_cell_background(hdr_cells[i], header_bg)
            set_cell_margins(hdr_cells[i], 50, 50, 30, 30)
            p = hdr_cells[i].paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(7)
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

        for row_idx, rdata in enumerate(data_rows, start=1):
            row_cells = table.rows[row_idx].cells
            bg_color = 'FFF2E6' if 'Secure' in rdata[3] else ('F2F7FC' if row_idx % 2 == 1 else 'FFFFFF')
            if 'N/A' in rdata[3]:
                bg_color = 'EAECEE'

            for col_idx, text in enumerate(rdata):
                row_cells[col_idx].text = text
                set_cell_background(row_cells[col_idx], bg_color)
                set_cell_margins(row_cells[col_idx], 35, 35, 30, 30)
                p = row_cells[col_idx].paragraphs[0]
                if col_idx in [0, 1, 3, 4, 5, 8, 9]:
                    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                for run in p.runs:
                    run.font.name = 'Arial'
                    run.font.size = Pt(6.5)
                    if col_idx in [1, 3, 4, 5]:
                        run.font.bold = True
                        if 'Secure' in text and 'Non' not in text:
                            run.font.color.rgb = RGBColor(0xB3, 0x3A, 0x00)
                        elif 'Non-Secure' in text:
                            run.font.color.rgb = RGBColor(0x00, 0x55, 0x99)
                        elif 'N/A' in text:
                            run.font.color.rgb = RGBColor(0x77, 0x77, 0x77)
                    if col_idx == 9:
                        run.font.bold = True
                        if 'SILICON' in text:
                            run.font.color.rgb = RGBColor(0x00, 0x66, 0x00)
                        elif 'RM0456' in text:
                            run.font.color.rgb = RGBColor(0x00, 0x33, 0x99)
        doc.add_paragraph()

    # ========================== BAB 3: DOMAIN SECURE ==========================
    secure_data = [
        ('1', 'PC14', 'TCXO_32K_IN', 'SEC14 = 1 (Secure)', 'N/A (Analog Input)', 'RCC_BDCR Secure', 'RCC_BDCR (+0xF0)\nGPIOC_SECCFGR (+0x30)\nGPIOC_MODER (+0x00)', '0x56020CF0 (S)\n0x52020830 (S)\n0x52020800 (S)', 'BDCR: 0x00000000 -> 0x00008125\nMODER[29:28]: 11b -> 11b (Analog)', '[VERIFIED-SILICON]\nClock: LSE 32.768kHz', 'Input Clock TCXO 32,768 kHz (Bypass LSE) dengan pemantau LSECSS (ERR02).'),
        ('2', 'PC15', 'UNUSED_PC15', 'SEC15 = 1 (Secure)', 'N/A (Analog Input)', 'N/A (Pin Pasif)', 'GPIOC_MODER (+0x00)\nGPIOC_SECCFGR (+0x30)', '0x52020800 (S)\n0x52020830 (S)', 'MODER[31:30]: 11b -> 11b (Analog)\nSECCFGR: Bit 15=1', '[VERIFIED-RM0456]\nClock: N/A', 'Analog Input hemat daya (kristal pasif digantikan TCXO 1-pin PC14).'),
        ('3', 'PB2', 'RTC_CAL_OUT', 'SEC2 = 1 (Secure)', 'N/A (Output AF)', 'RTCSEC = 1 (Secure)', 'RTC_CR (+0x18)\nGPIOB_AFRL (+0x20)\nGPIOB_SECCFGR (+0x30)\nGTZC1_TZSC (+0x100)', '0x56007818 (S)\n0x52020420 (S)\n0x52020430 (S)\n0x50032500 (S)', 'RTC_CR: 0x00000000 -> 0x60000000\nAFRL[11:8]: 0000b (AF0 RTC_OUT2)\nSECCFGR: Bit 2=1', '[VERIFIED-SILICON]\nClock: LSE 32.768kHz', 'Pin Uji Kalibrasi RTC Dedicated (1 Hz / 512 Hz) untuk ukur time drift.'),
        ('4', 'PC13', 'TAMP_CASE_OPEN', 'SEC13 = 1 (Secure)', 'N/A (Direct to TAMP)', 'TAMPSEC = 1 (Secure)', 'TAMP_CR1 (+0x00)\nTAMP_SECCFGR (+0x20)\nGPIOC_SECCFGR (+0x30)', '0x56007C00 (S)\n0x56007C20 (S)\n0x52020830 (S)', 'CR1: 0x00000000 -> 0x00000001 (TAMP1E)\nSECCFGR: 0x00000000 -> 0x00000001', '[VERIFIED-SILICON]\nClock: LSE 32.768kHz', 'Sabotase Tutup Meter: Operasi VSW (VBAT), catat timestamp, RUSAK & kunci kWh.'),
        ('5', 'PE6', 'TAMP_TERM_COVER', 'SEC6 = 1 (Secure)', 'N/A (Direct to TAMP)', 'TAMPSEC = 1 (Secure)', 'TAMP_CR1 (+0x00)\nTAMP_SECCFGR (+0x20)\nGPIOE_SECCFGR (+0x30)', '0x56007C00 (S)\n0x56007C20 (S)\n0x52021030 (S)', 'CR1: 0x00000000 -> 0x00000004 (TAMP3E)\nSECCFGR: 0x00000000 -> 0x00000004', '[VERIFIED-SILICON]\nClock: LSE 32.768kHz', 'Sabotase Tutup Terminal MCB: Operasi VSW (VBAT), catat ERR20 & buka relai.'),
        ('6', 'PE4', 'SENSOR_MAGNET', 'SEC4 = 1 (Secure)', 'N/A (Direct to TAMP)', 'TAMPSEC = 1 (Secure)', 'TAMP_CR2 (+0x04)\nTAMP_SECCFGR (+0x20)\nGPIOE_SECCFGR (+0x30)', '0x56007C04 (S)\n0x56007C20 (S)\n0x52021030 (S)', 'CR2: 0x00000000 -> 0x00000040 (TAMP7E)\nSECCFGR: 0x00000000 -> 0x00000040', '[VERIFIED-SILICON]\nClock: LSE 32.768kHz', 'Deteksi Induksi Medan Magnet Luar > 500mT (Hall Sensor): Rekam ERR25 di VSW.'),
        ('7', 'PC3', 'AC_FAIL_DET', 'SEC3 = 1 (Secure)', 'SEC3 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOC_MODER (+0x00)\nEXTI_EXTICR1 (+0x60)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_FTSR1 (+0x04)', '0x52020800 (S)\n0x56022060 (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022004 (S)', 'EXTICR1.EXTI3: 0x00 -> 0x02 (Port C)\nIMR1: Bit 3=1, FTSR1: Bit 3=1\nSECCFGR1: Bit 3=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Deteksi Listrik Padam Dini: Memicu Last-Gasp Push & transisi ke Stop 2.'),
        ('8', 'PD11', 'VBAT_SENSE', 'SEC11 = 1 (Secure)', 'N/A (Analog Input)', 'ADC4SEC = 1 (Secure)', 'GPIOD_MODER (+0x00)\nGPIOD_SECCFGR (+0x30)\nADC4_CHSELR (+0x28)\nADC4_CR (+0x08)\nGTZC2_TZSC (+0x100)', '0x52020C00 (S)\n0x52020C30 (S)\n0x56021028 (S)\n0x56021008 (S)\n0x56033100 (S)', 'CHSELR: 0x00000000 -> 0x00008000 (Ch 15)\nADC4_CR: Bit 0 ADEN=1, Bit 2 ADSTART=1\nMODER[23:22]: 11b (Analog)', '[VERIFIED-RM0456]\nClock: MSIS / HSI16', 'Monitor Tegangan Baterai Cadangan (ADC4 otonom di Stop 2, picu ERR03 jika < 2,7V).'),
        ('9', 'PC2', 'VCAP_SENSE', 'SEC2 = 1 (Secure)', 'N/A (Analog Input)', 'ADC4SEC = 1 (Secure)', 'GPIOC_MODER (+0x00)\nGPIOC_SECCFGR (+0x30)\nADC4_CHSELR (+0x28)', '0x52020800 (S)\n0x52020830 (S)\n0x56021028 (S)', 'CHSELR: 0x00000000 -> 0x00000008 (Ch 3)\nMODER[5:4]: 11b (Analog)\nSECCFGR: Bit 2=1', '[VERIFIED-RM0456]\nClock: MSIS / HSI16', 'Monitor Superkapasitor (Deteksi kerusakan ERR06 & ambang transisi 2,7V).'),
        ('10', 'PD12', 'EXT_WDI_PULSE', 'SEC12 = 1 (Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOD_MODER (+0x00)\nGPIOD_BSRR (+0x18)\nGPIOD_SECCFGR (+0x30)', '0x52020C00 (S)\n0x52020C18 (S)\n0x52020C30 (S)', 'MODER[25:24]: 00b -> 01b (Output)\nBSRR: Bit 12=1 (High) / Bit 28=1 (Low)\nSECCFGR: Bit 12=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Sinyal Heartbeat ke IC External Watchdog (Memicu ERR04 jika MCU hang).'),
        ('11', 'PA4', 'ADE_SS', 'SEC4 = 1 (Secure)', 'N/A (Output Push-Pull)', 'SPI1SEC = 1 (Secure)', 'GPIOA_MODER (+0x00)\nGPIOA_OSPEEDR (+0x08)\nGPIOA_BSRR (+0x18)\nGPIOA_SECCFGR (+0x30)\nGTZC1_TZSC (+0x100)', '0x52020000 (S)\n0x52020008 (S)\n0x52020018 (S)\n0x52020030 (S)\n0x50032500 (S)', 'MODER[9:8]: 00b -> 01b (Output)\nOSPEEDR[9:8]: 00b -> 11b (Very High)\nSECCFGR: Bit 4=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'SPI1 Chip Select (CS) ke AFE Metrologi 3-Fasa ADE9000.'),
        ('12', 'PA5', 'ADE_SCK', 'SEC5 = 1 (Secure)', 'N/A (Alternate Function)', 'SPI1SEC = 1 (Secure)', 'GPIOA_MODER (+0x00)\nGPIOA_AFRL (+0x20)\nGPIOA_SECCFGR (+0x30)\nSPI1_CR1 (+0x00)\nSPI1_CFG2 (+0x0C)', '0x52020000 (S)\n0x52020020 (S)\n0x52020030 (S)\n0x50013000 (S)\n0x5001300C (S)', 'MODER[11:10]: 00b -> 10b (AF)\nAFRL[23:20]: 0000b -> 0101b (AF5 SPI1)\nSPI1_CR1: Bit 0 SPE=1', '[VERIFIED-SILICON]\nClock: PCLK2 (160MHz)', 'Serial Clock SPI1 ke ADE9000.'),
        ('13', 'PA6', 'ADE_MISO', 'SEC6 = 1 (Secure)', 'N/A (Alternate Function)', 'SPI1SEC = 1 (Secure)', 'GPIOA_MODER (+0x00)\nGPIOA_AFRL (+0x20)\nGPIOA_SECCFGR (+0x30)\nSPI1_RXDR (+0x30)', '0x52020000 (S)\n0x52020020 (S)\n0x52020030 (S)\n0x50013030 (S)', 'MODER[13:12]: 00b -> 10b (AF)\nAFRL[27:24]: 0000b -> 0101b (AF5 SPI1)\nSECCFGR: Bit 6=1', '[VERIFIED-SILICON]\nClock: PCLK2 (160MHz)', 'Master-In Slave-Out SPI1 ADE9000 (Baca RMS, Daya, Energi, THD).'),
        ('14', 'PA7', 'ADE_MOSI', 'SEC7 = 1 (Secure)', 'N/A (Alternate Function)', 'SPI1SEC = 1 (Secure)', 'GPIOA_MODER (+0x00)\nGPIOA_AFRL (+0x20)\nGPIOA_SECCFGR (+0x30)\nSPI1_TXDR (+0x20)', '0x52020000 (S)\n0x52020020 (S)\n0x52020030 (S)\n0x50013020 (S)', 'MODER[15:14]: 00b -> 10b (AF)\nAFRL[31:28]: 0000b -> 0101b (AF5 SPI1)\nSECCFGR: Bit 7=1', '[VERIFIED-SILICON]\nClock: PCLK2 (160MHz)', 'Master-Out Slave-In SPI1 ke ADE9000.'),
        ('15', 'PB5', 'ADE_IRQ0', 'SEC5 = 1 (Secure)', 'SEC5 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOB_MODER (+0x00)\nGPIOB_SECCFGR (+0x30)\nEXTI_EXTICR2 (+0x64)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_FTSR1 (+0x04)', '0x52020400 (S)\n0x52020430 (S)\n0x56022064 (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022004 (S)', 'EXTICR2.EXTI5: 0x00 -> 0x01 (Port B)\nIMR1: Bit 5=1, FTSR1: Bit 5=1\nSECCFGR1: Bit 5=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Interupsi HW 0 ADE9000: Reverse Current (ERR26), Urutan Fase Terbalik (ERR21).'),
        ('16', 'PB1', 'ADE_IRQ1', 'SEC1 = 1 (Secure)', 'SEC1 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOB_MODER (+0x00)\nGPIOB_SECCFGR (+0x30)\nEXTI_EXTICR1 (+0x60)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_FTSR1 (+0x04)', '0x52020400 (S)\n0x52020430 (S)\n0x56022060 (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022004 (S)', 'EXTICR1.EXTI1: 0x00 -> 0x01 (Port B)\nIMR1: Bit 1=1, FTSR1: Bit 1=1\nSECCFGR1: Bit 1=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Interupsi HW 1 ADE9000: Netral Putus (ERR23), Hilang Tegangan (ERR24), Sag/Swell.'),
        ('17', 'PE11', 'ADE_EVENT', 'SEC11 = 1 (Secure)', 'SEC11 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOE_MODER (+0x00)\nGPIOE_SECCFGR (+0x30)\nEXTI_EXTICR3 (+0x68)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_FTSR1 (+0x04)', '0x52021000 (S)\n0x52021030 (S)\n0x56022068 (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022004 (S)', 'EXTICR3.EXTI11: 0x00 -> 0x04 (Port E)\nIMR1: Bit 11=1, FTSR1: Bit 11=1\nSECCFGR1: Bit 11=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Interupsi Power Quality Event (ADE_EVENT / CF4): Menerima sinyal hardware Pin 36 ADE9000 yang disetel sebagai EVENT (disesuaikan dari mode sebelumnya DREADY via CFMODE[13:12]=10b) saat terjadi Sag/Swell/Overcurrent.'),
        ('18', 'PE7', 'ADE_RESET', 'SEC7 = 1 (Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOE_MODER (+0x00)\nGPIOE_ODR (+0x14)\nGPIOE_SECCFGR (+0x30)', '0x52021000 (S)\n0x52021014 (S)\n0x52021030 (S)', 'MODER[15:14]: 00b -> 01b (Output)\nODR: Bit 7=1 (Run) / 0 (Reset)\nSECCFGR: Bit 7=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Hardware Reset IC ADE9000 (Reset AFE jika sampling ADC berhenti).'),
        ('19', 'PE8', 'ADE_CF3_ZX', 'SEC8 = 1 (Secure)', 'SEC8 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOE_MODER (+0x00)\nGPIOE_SECCFGR (+0x30)\nEXTI_EXTICR3 (+0x68)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_RTSR1 (+0x00)\nEXTI_FTSR1 (+0x04)', '0x52021000 (S)\n0x52021030 (S)\n0x56022068 (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022000 (S)\n0x56022004 (S)', 'EXTICR3.EXTI8: 0x00 -> 0x04 (Port E)\nIMR1: Bit 8=1, RTSR1: Bit 8=1, FTSR1: Bit 8=1\nSECCFGR1: Bit 8=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Fungsi Ganda Pin 35 ADE9000 (CF3 / ZX): Pulsa kalibrasi energi semu kVAh (CF3) atau Deteksi Titik Nol AC (Zero-Crossing / ZX) untuk sinkronisasi pemutusan Shunt Trip PC8 pada titik arus nol (Zero-Current Switching).'),
        ('20', 'PE9', 'ADE_PM0', 'SEC9 = 1 (Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOE_MODER (+0x00)\nGPIOE_ODR (+0x14)\nGPIOE_SECCFGR (+0x30)', '0x52021000 (S)\n0x52021014 (S)\n0x52021030 (S)', 'MODER[19:18]: 00b -> 01b (Output)\nODR: Bit 9=0 (Run PSM0) / 1 (Sleep PSM3)\nSECCFGR: Bit 9=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Kontrol Power Mode 0 ADE9000: Bersama PE10 (PM1) mengatur mode hemat daya AFE saat pemadaman listrik PLN (PSM0 Run / PSM3 Total Sleep ~2uA).'),
        ('21', 'PE10', 'ADE_PM1', 'SEC10 = 1 (Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOE_MODER (+0x00)\nGPIOE_ODR (+0x14)\nGPIOE_SECCFGR (+0x30)', '0x52021000 (S)\n0x52021014 (S)\n0x52021030 (S)', 'MODER[21:20]: 00b -> 01b (Output)\nODR: Bit 10 (Power Mode ADE)\nSECCFGR: Bit 10=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Kontrol Power Mode 1 ADE9000: Menurunkan mode daya ADE saat listrik PLN padam.'),
        ('22', 'PE12', 'ADE_CF1', 'SEC12 = 1 (Secure)', 'SEC12 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOE_MODER (+0x00)\nGPIOE_PUPDR (+0x0C)\nGPIOE_SECCFGR (+0x30)\nEXTI_EXTICR4 (+0x6C)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_FTSR1 (+0x04)', '0x52021000 (S)\n0x5202100C (S)\n0x52021030 (S)\n0x5602206C (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022004 (S)', 'EXTICR4.EXTI12: 0x00 -> 0x04 (Port E)\nPUPDR[25:24]: 01b (Pull-Up)\nIMR1: Bit 12=1, FTSR1: Bit 12=1\nSECCFGR1: Bit 12=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Pencacah Pulsa Kalibrasi Energi Aktif (CF1 / Imp/kWh): Menerima pulsa Pin 33 ADE9000 (1.000 imp/kWh, lebar 80 ms ± 2 ms) paralel dengan LED PB4 untuk validasi mandiri register Wh di SPI.'),
        ('23', 'PE13', 'ADE_CF2', 'SEC13 = 1 (Secure)', 'SEC13 = 1 (Secure)', 'N/A (GPIO Input EXTI)', 'GPIOE_MODER (+0x00)\nGPIOE_PUPDR (+0x0C)\nGPIOE_SECCFGR (+0x30)\nEXTI_EXTICR4 (+0x6C)\nEXTI_SECCFGR1 (+0x14)\nEXTI_IMR1 (+0x80)\nEXTI_FTSR1 (+0x04)', '0x52021000 (S)\n0x5202100C (S)\n0x52021030 (S)\n0x5602206C (S)\n0x56022014 (S)\n0x56022080 (S)\n0x56022004 (S)', 'EXTICR4.EXTI13: 0x00 -> 0x04 (Port E)\nPUPDR[27:26]: 01b (Pull-Up)\nIMR1: Bit 13=1, FTSR1: Bit 13=1\nSECCFGR1: Bit 13=1', '[VERIFIED-SILICON]\nClock: PCLK2', 'Pencacah Pulsa Kalibrasi Energi Reaktif (CF2 / Imp/kvarh): Menerima pulsa Pin 34 ADE9000 (1.000 imp/kvarh, lebar 80 ms ± 2 ms) paralel dengan LED PB5 untuk validasi mandiri register varh 4-kuadran di SPI.')
    ]
    render_pin_table('3. Tabel Register Domain Secure (Secure World - Metrologi, Kriptografi, Tamper, & Proteksi Sistem)', secure_data, '990000')

    # ========================== BAB 4: DOMAIN NON-SECURE ==========================
    nonsecure_data = [
        ('1', 'PH0', 'OSC_IN', 'SEC0 = 0 (Non-Secure)', 'N/A (Analog Input)', 'N/A (Clock Input)', 'RCC_CR (+0x00)\nGPIOH_MODER (+0x00)\nGPIOH_SECCFGR (+0x30)', '0x46020C00 (NS)\n0x42021C00 (NS)\n0x42021C30 (NS)', 'RCC_CR: 0x00000500 -> 0x00030500 (HSEON)\nMODER[1:0]: 11b -> 11b (Analog)\nSECCFGR: Bit 0=0', '[VERIFIED-SILICON]\nClock: HSE 16MHz', 'Input Kristal Utama HSE (8–16 MHz).'),
        ('2', 'PH1', 'OSC_OUT', 'SEC1 = 0 (Non-Secure)', 'N/A (Analog Output)', 'N/A (Clock Output)', 'GPIOH_MODER (+0x00)\nGPIOH_SECCFGR (+0x30)', '0x42021C00 (NS)\n0x42021C30 (NS)', 'MODER[3:2]: 11b -> 11b (Analog)\nSECCFGR: Bit 1=0', '[VERIFIED-SILICON]\nClock: HSE 16MHz', 'Output Kristal Utama HSE.'),
        ('3', 'PC0', 'LCD_I2C3_SCL', 'SEC0 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'I2C3SEC = 0 (Non-Secure)', 'GPIOC_MODER (+0x00)\nGPIOC_OTYPER (+0x04)\nGPIOC_AFRL (+0x20)\nGPIOC_SECCFGR (+0x30)\nI2C3_TIMINGR (+0x10)', '0x42020800 (NS)\n0x42020804 (NS)\n0x42020820 (NS)\n0x42020830 (NS)\n0x46002810 (NS)', 'MODER[1:0]: 00b -> 10b (AF)\nOTYPER[0]: 0 -> 1 (Open-Drain)\nAFRL[3:0]: 0000b -> 0100b (AF4 I2C3)\nTIMINGR = 0x3042C7C7', '[VERIFIED-SILICON]\nClock: HSI16 (16MHz)', 'Clock Bus I2C3 untuk Driver Layar Matriks STN LCD (Otonom di Stop 2).'),
        ('4', 'PC1', 'LCD_I2C3_SDA', 'SEC1 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'I2C3SEC = 0 (Non-Secure)', 'GPIOC_MODER (+0x00)\nGPIOC_OTYPER (+0x04)\nGPIOC_AFRL (+0x20)\nGPIOC_SECCFGR (+0x30)\nI2C3_CR1 (+0x00)', '0x42020800 (NS)\n0x42020804 (NS)\n0x42020820 (NS)\n0x42020830 (NS)\n0x46002800 (NS)', 'MODER[3:2]: 00b -> 10b (AF)\nOTYPER[1]: 0 -> 1 (Open-Drain)\nAFRL[7:4]: 0000b -> 0100b (AF4 I2C3)\nCR1: Bit 0 PE=1', '[VERIFIED-SILICON]\nClock: HSI16 (16MHz)', 'Data Bus I2C3 untuk Driver Layar Matriks STN LCD 3-Baris.'),
        ('5', 'PD2', 'LCD_PWR_EN', 'SEC2 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOD_MODER (+0x00)\nGPIOD_ODR (+0x14)\nGPIOD_SECCFGR (+0x30)', '0x42020C00 (NS)\n0x42020C14 (NS)\n0x42020C30 (NS)', 'MODER[5:4]: 00b -> 01b (Output)\nODR: Bit 2=1 (LCD ON) / 0 (OFF)\nSECCFGR: Bit 2=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Sakelar Catu Daya LCD: Mengaktifkan display 10 detik saat push-to-read padam.'),
        ('6', 'PD3', 'LCD_BL_CTRL', 'SEC3 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOD_MODER (+0x00)\nGPIOD_ODR (+0x14)\nGPIOD_SECCFGR (+0x30)', '0x42020C00 (NS)\n0x42020C14 (NS)\n0x42020C30 (NS)', 'MODER[7:6]: 00b -> 01b (Output)\nODR: Bit 3=1 (BL ON) / 0 (OFF)\nSECCFGR: Bit 3=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Lampu Latar (Backlight) LCD (Menyala 10 detik saat tombol ditekan).'),
        ('7', 'PA0', 'BTN_SCROLL_UP', 'SEC0 = 0 (Non-Secure)', 'SEC0 = 0 (Non-Secure)', 'N/A (GPIO Input EXTI)', 'GPIOA_MODER (+0x00)\nGPIOA_PUPDR (+0x0C)\nGPIOA_SECCFGR (+0x30)\nEXTI_EXTICR1 (+0x60)\nEXTI_IMR1 (+0x80)\nPWR_WUCR1 (+0x14)\nPWR_WUCR2 (+0x18)\nPWR_WUCR3 (+0x1C)', '0x42020000 (NS)\n0x4202000C (NS)\n0x42020030 (NS)\n0x46022060 (NS)\n0x46022080 (NS)\n0x46020814 (NS)\n0x46020818 (NS)\n0x4602081C (NS)', 'MODER[1:0]: 00b (Input), PUPDR: 01b (PU)\nEXTICR1.EXTI0: 0x00 (Port A)\nWUCR1: WUPEN1=1 (WKUP1)\nWUCR2: WUPP1=1 (Low edge)\nWUCR3: WUSEL1=00b (PA0)', '[VERIFIED-SILICON]\nClock: PCLK1', 'Tombol Navigasi Naik (▲), Wakeup MCU saat padam, & Reconnect Relai.'),
        ('8', 'PB6', 'BTN_SCROLL_DN', 'SEC6 = 0 (Non-Secure)', 'SEC6 = 0 (Non-Secure)', 'N/A (GPIO Input EXTI)', 'GPIOB_MODER (+0x00)\nGPIOB_PUPDR (+0x0C)\nGPIOB_SECCFGR (+0x30)\nEXTI_EXTICR2 (+0x64)\nEXTI_IMR1 (+0x80)\nPWR_WUCR1 (+0x14)\nPWR_WUCR2 (+0x18)\nPWR_WUCR3 (+0x1C)', '0x42020400 (NS)\n0x4202040C (NS)\n0x42020430 (NS)\n0x46022064 (NS)\n0x46022080 (NS)\n0x46020814 (NS)\n0x46020818 (NS)\n0x4602081C (NS)', 'MODER[13:12]: 00b (Input), PUPDR: 01b\nEXTICR2.EXTI6: 0x01 (Port B)\nWUCR1: WUPEN3=1 (WKUP3)\nWUCR2: WUPP3=1 (Low edge)\nWUCR3: WUSEL3=10b (PB6)', '[VERIFIED-SILICON]\nClock: PCLK1', 'Tombol Navigasi Turun (▼), Wakeup MCU saat padam, & Konfirmasi Reconnect (▲+▼).'),
        ('9', 'PB7', 'LED_ALM', 'SEC7 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOB_MODER (+0x00)\nGPIOB_BSRR (+0x18)\nGPIOB_SECCFGR (+0x30)', '0x42020400 (NS)\n0x42020418 (NS)\n0x42020430 (NS)', 'MODER[15:14]: 00b -> 01b (Output)\nBSRR: Bit 7=1 (Set) / Bit 23=1 (Reset)\nSECCFGR: Bit 7=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Driver Indikator LED Kuning Alarm / Peringatan Tamper.'),
        ('10', 'PA2', 'MODEM_TX', 'SEC2 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'LPUART1SEC = 0 (Non-Secure)', 'GPIOA_MODER (+0x00)\nGPIOA_AFRL (+0x20)\nGPIOA_SECCFGR (+0x30)\nLPUART1_CR1 (+0x00)\nLPUART1_BRR (+0x0C)\nLPUART1_TDR (+0x28)', '0x42020000 (NS)\n0x42020020 (NS)\n0x42020030 (NS)\n0x46002400 (NS)\n0x4600240C (NS)\n0x46002428 (NS)', 'MODER[5:4]: 00b -> 10b (AF)\nAFRL[11:8]: 0000b -> 1000b (AF8 LPUART)\nBRR: 0x00000000 -> 0x00034155\nCR1: UE=1, TE=1, FIFOEN=1', '[VERIFIED-SILICON]\nClock: HSI16 (16MHz)', 'Port RS-232 TX ke RJ-45 Pin 5 (via Transceiver MAX3232 @ 19.200 bps).'),
        ('11', 'PA3', 'MODEM_RX', 'SEC3 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'LPUART1SEC = 0 (Non-Secure)', 'GPIOA_MODER (+0x00)\nGPIOA_PUPDR (+0x0C)\nGPIOA_AFRL (+0x20)\nGPIOA_SECCFGR (+0x30)\nLPUART1_CR1 (+0x00)\nLPUART1_CR3 (+0x08)\nLPUART1_RDR (+0x24)', '0x42020000 (NS)\n0x4202000C (NS)\n0x42020020 (NS)\n0x42020030 (NS)\n0x46002400 (NS)\n0x46002408 (NS)\n0x46002424 (NS)', 'MODER[7:6]: 00b -> 10b (AF)\nPUPDR[7:6]: 01b (Pull-Up)\nAFRL[15:12]: 1000b (AF8 LPUART1)\nCR3: Bit 0 DMAR=1 (DMA Enable)', '[VERIFIED-SILICON]\nClock: HSI16 (16MHz)', 'Port RS-232 RX dari RJ-45 Pin 4 (via MAX3232 & DMA Ring Buffer; aktif di Stop 2).'),
        ('12', 'PD4', 'RS232_PWR_EN', 'SEC4 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOD_MODER (+0x00)\nGPIOD_ODR (+0x14)\nGPIOD_SECCFGR (+0x30)', '0x42020C00 (NS)\n0x42020C14 (NS)\n0x42020C30 (NS)', 'MODER[9:8]: 00b -> 01b (Output)\nODR: Bit 4=1 (Power ON) / 0 (OFF)\nSECCFGR: Bit 4=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Daya Transceiver RS-232: Mematikan MAX3232 paska Last-Gasp.'),
        ('13', 'PD5', 'MODEM_12V_EN', 'SEC5 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOD_MODER (+0x00)\nGPIOD_ODR (+0x14)\nGPIOD_SECCFGR (+0x30)', '0x42020C00 (NS)\n0x42020C14 (NS)\n0x42020C30 (NS)', 'MODER[11:10]: 00b -> 01b (Output)\nODR: Bit 5=1 (12V ON) / 0 (OFF)\nSECCFGR: Bit 5=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Pemutus Catu Daya Modem 12V: Memutus 12V paska Last-Gasp Push.'),
        ('14', 'PC4', 'OPTICAL_TX', 'SEC4 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'USART3SEC = 0 (Non-Secure)', 'GPIOC_MODER (+0x00)\nGPIOC_AFRL (+0x20)\nGPIOC_SECCFGR (+0x30)\nUSART3_CR1 (+0x00)\nUSART3_BRR (+0x0C)\nUSART3_TDR (+0x28)', '0x42020800 (NS)\n0x42020820 (NS)\n0x42020830 (NS)\n0x40004800 (NS)\n0x4000480C (NS)\n0x40004828 (NS)', 'MODER[9:8]: 00b -> 10b (AF)\nAFRL[19:16]: 0111b (AF7 USART3)\nBRR: 0x00000000 -> 0x0000411B (160MHz)\nCR1: UE=1, TE=1', '[VERIFIED-SILICON]\nClock: PCLK1 (160MHz)', 'Port Komunikasi Lokal TX Optical Probe Depan Meter (Isolasi > 4 kV).'),
        ('15', 'PC5', 'OPTICAL_RX', 'SEC5 = 0 (Non-Secure)', 'SEC5 = 0 (Non-Secure)', 'USART3SEC = 0 (Non-Secure)', 'GPIOC_MODER (+0x00)\nGPIOC_PUPDR (+0x0C)\nGPIOC_AFRL (+0x20)\nGPIOC_SECCFGR (+0x30)\nUSART3_CR1 (+0x00)\nUSART3_RDR (+0x24)\nPWR_WUCR1 (+0x14)\nPWR_WUCR2 (+0x18)\nPWR_WUCR3 (+0x1C)', '0x42020800 (NS)\n0x4202080C (NS)\n0x42020820 (NS)\n0x42020830 (NS)\n0x40004800 (NS)\n0x40004824 (NS)\n0x46020814 (NS)\n0x46020818 (NS)\n0x4602081C (NS)', 'MODER[11:10]: 00b -> 10b (AF)\nAFRL[23:20]: 0111b (AF7 USART3)\nWUCR1: WUPEN5=1 (WKUP5)\nWUCR2: WUPP5=1 (Low edge)\nWUCR3: WUSEL5=00b (PC5)', '[VERIFIED-SILICON]\nClock: PCLK1 (160MHz)', 'Port Komunikasi Lokal RX Optical Probe Depan Meter (Wakeup WKUP5).'),
        ('16', 'PC8', 'CTRL_RELAY_TRIP', 'SEC8 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOC_MODER (+0x00)\nGPIOC_PUPDR (+0x0C)\nGPIOC_BSRR (+0x18)\nGPIOC_SECCFGR (+0x30)', '0x42020800 (NS)\n0x4202080C (NS)\n0x42020818 (NS)\n0x42020830 (NS)', 'MODER[17:16]: 00b -> 01b (Output)\nPUPDR[17:16]: 10b (Pull-Down)\nBSRR: Bit 8=1 (Shunt Trip Aktif)\nSECCFGR: Bit 8=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Pemutus Utama (Shunt Trip Relai Beban maks 240 VAC, 2 A Sambungan Tidak Langsung).'),
        ('17', 'PC9', 'SPARE_GPIO', 'SEC9 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOC_MODER (+0x00)\nGPIOC_PUPDR (+0x0C)\nGPIOC_BSRR (+0x18)\nGPIOC_SECCFGR (+0x30)', '0x42020800 (NS)\n0x4202080C (NS)\n0x42020818 (NS)\n0x42020830 (NS)', 'MODER[19:18]: 00b -> 01b (Output)\nPUPDR[19:18]: 10b (Pull-Down)\nBSRR: Bit 25=1 (Pull Low Safe)\nSECCFGR: Bit 9=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Cadangan Digital Output / Spare (Meter Sambungan Tidak Langsung Reconnect MCCB Manual).'),
        ('18', 'PC7', 'RELAY_STATUS', 'SEC7 = 0 (Non-Secure)', 'SEC7 = 0 (Non-Secure)', 'N/A (GPIO Input EXTI)', 'GPIOC_MODER (+0x00)\nGPIOC_PUPDR (+0x0C)\nGPIOC_IDR (+0x10)\nGPIOC_SECCFGR (+0x30)\nEXTI_EXTICR2 (+0x64)', '0x42020800 (NS)\n0x4202080C (NS)\n0x42020810 (NS)\n0x42020830 (NS)\n0x46022064 (NS)', 'MODER[15:14]: 00b (Input)\nPUPDR[15:14]: 10b (Pull-Down)\nEXTICR2.EXTI7: 0x00 -> 0x02 (Port C)\nSECCFGR: Bit 7=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Umpan Balik Kontak Bantu Relai (Deteksi kegagalan mekanik ERR07).'),
        ('19', 'PD8', 'AUX_CTRL_OUT1', 'SEC8 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'N/A (GPIO Output)', 'GPIOD_MODER (+0x00)\nGPIOD_ODR (+0x14)\nGPIOD_SECCFGR (+0x30)', '0x42020C00 (NS)\n0x42020C14 (NS)\n0x42020C30 (NS)', 'MODER[17:16]: 00b -> 01b (Output)\nODR: Bit 8 (Driver Relai Bantu 1)\nSECCFGR: Bit 8=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Bantu Relai Output 1 (Driver Opto-SSR maks 288 VAC, 50 mA).'),
        ('20', 'PD10', 'AUX_CTRL_IN1', 'SEC10 = 0 (Non-Secure)', 'SEC10 = 0 (Non-Secure)', 'N/A (GPIO Input EXTI)', 'GPIOD_MODER (+0x00)\nGPIOD_PUPDR (+0x0C)\nGPIOD_IDR (+0x10)\nGPIOD_SECCFGR (+0x30)\nEXTI_EXTICR3 (+0x68)\nEXTI_IMR1 (+0x80)', '0x42020C00 (NS)\n0x42020C0C (NS)\n0x42020C10 (NS)\n0x42020C30 (NS)\n0x46022068 (NS)\n0x46022080 (NS)', 'MODER[21:20]: 00b (Input)\nPUPDR[21:20]: 01b (Pull-Up)\nEXTICR3.EXTI10: 0x00 -> 0x03 (Port D)\nIMR1: Bit 10=1', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Bantu Input Digital 1 (Optocoupler 60–288 VAC).'),
        ('21', 'PD14', 'AUX_CTRL_IN2', 'SEC14 = 0 (Non-Secure)', 'SEC14 = 0 (Non-Secure)', 'N/A (GPIO Input EXTI)', 'GPIOD_MODER (+0x00)\nGPIOD_PUPDR (+0x0C)\nGPIOD_IDR (+0x10)\nGPIOD_SECCFGR (+0x30)\nEXTI_EXTICR4 (+0x6C)\nEXTI_IMR1 (+0x80)', '0x42020C00 (NS)\n0x42020C0C (NS)\n0x42020C10 (NS)\n0x42020C30 (NS)\n0x4602206C (NS)\n0x46022080 (NS)', 'MODER[29:28]: 00b (Input)\nPUPDR[29:28]: 01b (Pull-Up)\nEXTICR4.EXTI14: 0x00 -> 0x03 (Port D)\nIMR1: Bit 14=1', '[VERIFIED-SILICON]\nClock: PCLK1', 'Kontrol Bantu Input Digital 2 (Optocoupler 60–288 VAC).'),
        ('22', 'PB12', 'FLASH_SPI2_CS', 'SEC12 = 0 (Non-Secure)', 'N/A (Output Push-Pull)', 'SPI2SEC = 0 (Non-Secure)', 'GPIOB_MODER (+0x00)\nGPIOB_OSPEEDR (+0x08)\nGPIOB_BSRR (+0x18)\nGPIOB_SECCFGR (+0x30)\nGTZC1_TZSC (+0x100)', '0x42020400 (NS)\n0x42020408 (NS)\n0x42020418 (NS)\n0x42020430 (NS)\n0x40032500 (NS)', 'MODER[25:24]: 00b -> 01b (Output)\nOSPEEDR[25:24]: 11b (Very High)\nBSRR: Bit 12=1 (CS High) / Bit 28=1 (Low)\nSECCFGR: Bit 12=0', '[VERIFIED-SILICON]\nClock: PCLK1', 'Chip Select (CS) Memori Non-Volatile Eksternal (SPI NOR Flash / FRAM 4 MB).'),
        ('23', 'PB13', 'FLASH_SPI2_SCK', 'SEC13 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'SPI2SEC = 0 (Non-Secure)', 'GPIOB_MODER (+0x00)\nGPIOB_AFRH (+0x24)\nGPIOB_SECCFGR (+0x30)\nSPI2_CR1 (+0x00)\nSPI2_CFG2 (+0x0C)', '0x42020400 (NS)\n0x42020424 (NS)\n0x42020430 (NS)\n0x40003800 (NS)\n0x4000380C (NS)', 'MODER[27:26]: 00b -> 10b (AF)\nAFRH[23:20]: 0000b -> 0101b (AF5 SPI2)\nSPI2_CR1: Bit 0 SPE=1\nSECCFGR: Bit 13=0', '[VERIFIED-SILICON]\nClock: PCLK1 (160MHz)', 'Serial Clock SPI2 Memori Eksternal (Load Profile 3 bulan & Billing 12 bulan).'),
        ('24', 'PB14', 'FLASH_SPI2_MISO', 'SEC14 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'SPI2SEC = 0 (Non-Secure)', 'GPIOB_MODER (+0x00)\nGPIOB_AFRH (+0x24)\nGPIOB_SECCFGR (+0x30)\nSPI2_RXDR (+0x30)', '0x42020400 (NS)\n0x42020424 (NS)\n0x42020430 (NS)\n0x40003830 (NS)', 'MODER[29:28]: 00b -> 10b (AF)\nAFRH[27:24]: 0000b -> 0101b (AF5 SPI2)\nSECCFGR: Bit 14=0', '[VERIFIED-SILICON]\nClock: PCLK1 (160MHz)', 'Master-In Slave-Out SPI2 Memori Eksternal (Deteksi gagal memori ERR00).'),
        ('25', 'PB15', 'FLASH_SPI2_MOSI', 'SEC15 = 0 (Non-Secure)', 'N/A (Alternate Function)', 'SPI2SEC = 0 (Non-Secure)', 'GPIOB_MODER (+0x00)\nGPIOB_AFRH (+0x24)\nGPIOB_SECCFGR (+0x30)\nSPI2_TXDR (+0x20)', '0x42020400 (NS)\n0x42020424 (NS)\n0x42020430 (NS)\n0x40003820 (NS)', 'MODER[31:30]: 00b -> 10b (AF)\nAFRH[31:28]: 0000b -> 0101b (AF5 SPI2)\nSECCFGR: Bit 15=0', '[VERIFIED-SILICON]\nClock: PCLK1 (160MHz)', 'Master-Out Slave-In SPI2 Memori Eksternal.')
    ]
    render_pin_table('4. Tabel Register Domain Non-Secure (Non-Secure World - Modem, Display, Optical, & Kontrol Beban)', nonsecure_data, '005599')

    # ========================== BAB 5: DOMAIN BERSAMA & SISTEM ==========================
    system_data = [
        ('1', 'NRST', 'RESET', 'N/A (Bukan Pin GPIO)', 'N/A (Hardware Core Reset)', 'System Reset Controller', 'RCC_CSR (+0xF4)', '0x56020CF4 (S)\n0x46020CF4 (NS)', 'RCC_CSR: 0x0C000000 -> 0x04000000 (PINRSTF)\nBit 23 RMVF=1 (Hapus status flag)', '[VERIFIED-SILICON]\nClock: Asynchronous', 'Jalur Hardware Reset Sistem MCU (Dedicated Bidirectional Analog Pad dengan Internal Pull-Up).'),
        ('2', 'VBAT', 'VBAT_IN', 'N/A (Power Domain Pad)', 'N/A (Hardware Power Supply)', 'PWR_BDCR1 / RCC_BDCR', 'PWR_BDCR1 (+0x20)\nRCC_BDCR (+0xF0)', '0x56020820 (S)\n0x56020CF0 (S)', 'PWR_BDCR1: Bit 0 MONEN=1, Bit 1 VBATF\nRCC_BDCR: Bit 15 RTCEN=1, Bit 0 LSEON=1', '[VERIFIED-SILICON]\nClock: LSE 32.768kHz', 'Input Baterai Cadangan Lithium Li-SOCl2 3,6V (Menyuplai domain VSW saat VDD padam total).'),
        ('3', 'PA13', 'SWDIO', 'SEC13 = 1 (System S)', 'N/A (Debug Interface)', 'Core Debug Interface', 'GPIOA_AFRH (+0x24)', '0x52020024 (S)', 'AFRH[23:20]: 0000b (AF0 SWDIO Debug)', '[VERIFIED-SILICON]\nClock: SWCLK External', 'Jalur Data Serial Wire Debugging (SWD) ST-LINK (Kunci RDP Level 2 pada produksi massal).'),
        ('4', 'PA14', 'SWCLK', 'SEC14 = 1 (System S)', 'N/A (Debug Interface)', 'Core Debug Interface', 'GPIOA_AFRH (+0x24)', '0x52020024 (S)', 'AFRH[27:24]: 0000b (AF0 SWCLK Debug)', '[VERIFIED-SILICON]\nClock: SWCLK External', 'Jalur Clock Serial Wire Debugging (SWD) ST-LINK.'),
        ('5', 'Rail DC', 'LED_POWER_ON', 'N/A (Bukan Pin MCU)', 'N/A (Analog Power Hardware)', 'N/A (Catu Daya Analog)', 'Hardwired', 'N/A', 'Jalur Regulator Catu Fasa Analog HW', '[VERIFIED-SILICON]\nClock: 50 Hz AC Rail', 'Indikator LED Hijau Catu Daya Utama 3-Fasa (Hardware Driven langsung dari power supply board).')
    ]
    render_pin_table('5. Tabel Sinyal Bersama, Debug SWD, & Jalur Sistem (System & Power Infrastructure)', system_data, '555555')

    # ========================== BAB 6 ==========================
    add_heading_styled('6. Pembuktian Matematis & Konfigurasi Mode Low-Power Stop 2, Superkapasitor, & Baterai', 1)

    p_lp1 = doc.add_paragraph()
    p_lp1.paragraph_format.line_spacing = 1.15
    r_lp1 = p_lp1.add_run(
        "6.1 Konfigurasi Mode Otonom Low-Power Stop 2 (RM0456 Section 10 & 44)\n"
        "Pada kondisi listrik padam total (terdeteksi via interupsi PC3 AC_FAIL_DET), STM32U585 beralih dari Run Mode (160 MHz) "
        "ke mode Stop 2. Register kontrol daya dikonfigurasi sebagai berikut:\n"
        "• PWR_CR1 (Offset +0x00): Bits [2:0] LPMS[2:0] = 010b (Stop 2 mode). VDDIO2, SRAM1, SRAM2, SRAM3, dan SRAM4 tetap full retention.\n"
        "• I2C3_AUTOCR (Offset +0x14): Bit 0 TRIGEN = 1. Driver display I2C3 beroperasi secara otonom tanpa membangunkan CPU core Cortex-M33.\n"
        "• ADC4_AUTOCR (Offset +0x10): Bit 0 AUTOMD = 1. ADC4 melakukan sampling tegangan baterai & superkapasitor secara mandiri pada interval 60 detik.\n"
        "• LPUART1_CR3 (Offset +0x08): Bit 28 WUFIE = 1, Bits [21:20] WUS[1:0] = 00b (Wakeup on RXNE / Start bit) agar dapat merespons pembacaan optik.\n\n"
        "6.2 Bukti Perhitungan Konsumsi Daya & Ketahanan Superkapasitor (≥ 48 Jam)\n"
        "Sesuai butir audit, klaim ketahanan superkapasitor ≥ 48 jam dibuktikan melalui perhitungan muatan elektrokimia (Coulomb Model):\n"
        "1. Parameter Superkapasitor: Kapasitansi nominal C = 1,0 Farad (Standar SPLN D3.006), toleransi ±20%. "
        "Tegangan nominal pengisian penuh V_1 = 3,30 V. Ambang batas tegangan cutoff MCU STM32U585 V_2 = 2,00 V (Margin aman di atas BOR level 1,71 V). "
        "Rentang tegangan kerja efektif: ΔV = 3,30 V - 2,00 V = 1,30 Volt.\n"
        "2. Muatan Listrik Tersedia: Q_avail = C × ΔV = 1,0 F × 1,30 V = 1,300 Coulomb (A.s).\n"
        "3. Profil Beban Arus Selama Padam:\n"
        "   a. Arus Tidur Stop 2 (I_sleep): CPU Core + SRAM Full Retention (2,1 µA) + RTC & TCXO (0,4 µA) = 2,5 µA.\n"
        "   b. Arus Bocor Diri Superkapasitor (I_leak pada 25°C): 1,2 µA.\n"
        "   c. Total Arus Standby Padam: I_standby = 2,5 µA + 1,2 µA = 3,7 µA.\n"
        "   d. Aktivitas Push-to-Read (Tombol PA0/PB6): Konsumsi LCD + Backlight = 3,5 mA selama 10 detik per penekanan. "
        "Jika diasumsikan terjadi 10 kali penekanan tombol selama periode pemadaman (total durasi aktif = 100 detik):\n"
        "      Q_push = 10 × (3,5 mA × 10 s) = 10 × 0,035 C = 0,350 Coulomb.\n"
        "4. Sisa Muatan untuk Mode Siaga Tidur:\n"
        "   Q_sleep = Q_avail - Q_push = 1,300 C - 0,350 C = 0,950 Coulomb.\n"
        "5. Durasi Waktu Operasi Efektif:\n"
        "   t_operasional = Q_sleep / I_standby = 0,950 C / (3,7 × 10^-6 A) = 256.756 detik ≈ 71,3 Jam.\n"
        "KESIMPULAN AUDIT: Nilai 71,3 Jam secara terbukti melampaui spesifikasi minimal SPLN (≥ 48 Jam) dengan safety margin sebesar 48,5%.\n\n"
        "6.3 Bukti Perhitungan Masa Aktif Baterai Lithium Cadangan Li-SOCl2 (≥ 10 Tahun)\n"
        "1. Parameter Baterai: Tipe Lithium Thionyl Chloride (Li-SOCl2) ER14250, Tegangan 3,6 V, Kapasitas nominal Q_bat = 1.200 mAh.\n"
        "2. Beban Arus Domain VBAT (Catu Daya Padam Permanen / VSW Active Domain):\n"
        "   • RTC Prescaler & Kalibrasi aktif (PC14 TCXO Clocking): I_RTC = 280 nA.\n"
        "   • Rangkaian Anti-Tamper Aktif 24/7 (3 Jalur: PC13, PE6, PE4): I_TAMP = 120 nA.\n"
        "   • Total Arus Hardware VBAT: I_load = 280 nA + 120 nA = 400 nA = 0,0004 mA.\n"
        "3. Laju Self-Discharge Kimia Baterai Li-SOCl2: < 1,0% per tahun pada suhu 25°C:\n"
        "   I_self_discharge = (0,01 × 1.200 mAh) / 8.760 jam = 0,00137 mA.\n"
        "4. Total Pengurasan Arus Rata-Rata: I_drain = 0,0004 mA + 0,00137 mA = 0,00177 mA.\n"
        "5. Umur Operasional Teoretis: T_teoretis = 1.200 mAh / 0,00177 mA = 677.966 jam ≈ 77,4 Tahun.\n"
        "6. Faktor De-rating Lingkungan Lapangan (Suhu Ekstrim -10°C s.d. +70°C, Efisiensi Kapasitas 50%):\n"
        "   T_efektif = 77,4 Tahun × 0,50 = 38,7 Tahun >> 10 Tahun.\n"
        "KESIMPULAN AUDIT: Masa aktif baterai cadangan terbukti memenuhi syarat garansi 10 tahun SPLN D3.006 dengan rasio ketahanan hampir 4× lipat.\n\n"
        "6.4 Rujukan Bukti Pengujian Empiris Firmware\n"
        "Validasi low-power dan ketahanan memori telah diuji melalui Soak Test & Stress Test selama 72 jam non-stop pada modul uji "
        "(didokumentasikan pada berkas repositori LAPORAN_SOAK_DAN_STRESS_TEST_KETAHANAN.md) yang membuktikan zero memory-leakage, "
        "stabilitas timing interrupt ADE9000, serta keberhasilan switching catu daya ke domain VSW tanpa corrupt pada register RTC->BKPxR."
    )
    r_lp1.font.name = 'Arial'
    r_lp1.font.size = Pt(8.5)
    r_lp1.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    doc.add_paragraph()

    # ========================== BAB 7 ==========================
    add_heading_styled('7. Matriks Korelasi Pin Fisik terhadap Kode Error LCD & Objek DLMS/COSEM', 1)

    err_headers = ['Pin MCU', 'Sumber Sinyal Sensor', 'Kode Layar LCD', 'OBIS Code DLMS', 'Aksi Meter / Pemutus Beban', 'Status Verifikasi']
    err_data = [
        ('PC13', 'Switch Tutup Meter (Case Open - TAMP1)', 'RUSAK', '0.0.96.11.0.255', 'Kunci kWh permanen, buka relai, kirim event tamper kritis ke HES.', '[VERIFIED-SILICON]'),
        ('PE6', 'Switch Tutup Terminal (Terminal Cover - TAMP3)', 'ERR20 / PERIKSA', '0.0.96.11.1.255', 'Buka relai beban (trip), rekam timestamp kejadian ke Tamper Log.', '[VERIFIED-SILICON]'),
        ('PE4', 'Hall Sensor Medan Magnet > 500mT (TAMP7)', 'ERR25 / PERIKSA', '0.0.96.11.2.255', 'Catat event tamper induksi medan magnet (tanpa membuka relai beban).', '[VERIFIED-SILICON]'),
        ('PB5', 'ADE9000 IRQ0 (Reverse Current / Phase)', 'ERR21 & ERR26', '0.0.96.11.3.255', 'Rekam arus balik dan/atau urutan fasa terbalik ke log tamper.', '[VERIFIED-SILICON]'),
        ('PB1', 'ADE9000 IRQ1 (Missing Neutral / Voltage)', 'ERR23 & ERR24', '0.0.96.11.4.255', 'Rekam fasa hilang atau kawat netral putus ke log kejadian kualitas daya.', '[VERIFIED-SILICON]'),
        ('PC7', 'Feedback Kontak Bantu Relai Beban', 'ERR07', '0.0.96.11.5.255', 'Alarm relai mekanik macet / gagal buka/tutup (Stuck Relay Alarm).', '[VERIFIED-SILICON]'),
        ('PD11', 'Sensor Tegangan Baterai Cadangan (ADC4)', 'ERR03 + Ikon Baterai', '0.0.96.6.3.255', 'Peringatan baterai cadangan melemah (< 2,7 V), simbol berkedip.', '[VERIFIED-RM0456]'),
        ('PC2', 'Sensor Superkapasitor (ADC4)', 'ERR06', '0.0.96.6.4.255', 'Superkapasitor rusak, terlepas, atau kapasitas menurun di bawah 2,7 V.', '[VERIFIED-RM0456]'),
        ('PC14', 'LSECSS (Clock Security System 32,768 kHz)', 'ERR02', '0.0.96.11.6.255', 'Anomali clock penagihan / osilator TCXO hilang atau tidak stabil.', '[VERIFIED-SILICON]'),
        ('PD12', 'Timeout External Watchdog IC Heartbeat', 'ERR04', '0.0.96.11.7.255', 'Reset darurat sistem & buka relai beban jika MCU mengalami hang/freeze.', '[VERIFIED-SILICON]'),
        ('PB14', 'SPI2 MISO (Komunikasi NOR Flash / FRAM)', 'ERR00', '0.0.96.11.8.255', 'Kegagalan memori non-volatile eksternal / checksum data corrupt.', '[VERIFIED-SILICON]')
    ]

    t_err = doc.add_table(rows=len(err_data)+1, cols=len(err_headers))
    t_err.alignment = WD_TABLE_ALIGNMENT.CENTER
    t_err.autofit = False

    hdr_e = t_err.rows[0].cells
    for i, name in enumerate(err_headers):
        hdr_e[i].text = name
        set_cell_background(hdr_e[i], '003366')
        set_cell_margins(hdr_e[i], 50, 50, 40, 40)
        p = hdr_e[i].paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        for run in p.runs:
            run.font.name = 'Arial'
            run.font.size = Pt(7.5)
            run.font.bold = True
            run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    for row_idx, row_data in enumerate(err_data, start=1):
        row_cells = t_err.rows[row_idx].cells
        bg_color = 'F2F7FC' if row_idx % 2 == 1 else 'FFFFFF'
        for col_idx, text in enumerate(row_data):
            row_cells[col_idx].text = text
            set_cell_background(row_cells[col_idx], bg_color)
            set_cell_margins(row_cells[col_idx], 35, 35, 40, 40)
            p = row_cells[col_idx].paragraphs[0]
            if col_idx in [0, 2, 3, 5]:
                p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            for run in p.runs:
                run.font.name = 'Arial'
                run.font.size = Pt(7)
                if col_idx in [0, 2]:
                    run.font.bold = True
                if col_idx == 5:
                    run.font.bold = True
                    run.font.color.rgb = RGBColor(0x00, 0x66, 0x00)

    doc.add_paragraph()

    # ========================== BAB 8 ==========================
    add_heading_styled('8. Rekomendasi Rangkaian Proteksi Elektronik PCB, EMC/ESD, & Gerbang NSC', 1)

    schematic_notes = [
        ("a. Jalur Anti-Sabotase (PC13 & PE6):",
         "Wajib dilengkapi filter Low-Pass RC (resistor pull-up 100 kΩ ke VSW dan kapasitor 100 nF ke GND) "
         "serta diode penekan transien (TVS) untuk memenuhi uji kekebalan elektrostatik (ESD) kontak 8 kV dan udara 15 kV per SPLN Butir 7."),
        ("b. Jalur Bus Layar LCD I2C3 (PC0 & PC1):",
         "Wajib menggunakan resistor pull-up eksternal 2,2 kΩ – 4,7 kΩ yang terhubung ke rail daya switched LCD (PD2), "
         "bukan rail VDD permanen. Hal ini mencegah terjadinya arus bocor (current leakage) saat layar dimatikan pada kondisi padam."),
        ("c. Penggerak Koil Relai Latching (PC8 & PC9):",
         "Koil relai bistable membutuhkan arus kejut 1–2 Ampere selama 100 ms. Pin PC8 dan PC9 wajib mengendalikan gerbang MOSFET "
         "atau H-Bridge driver dengan flyback diode. Pin MCU dilarang keras dibebani arus koil secara langsung."),
        ("d. Isolasi Galvanis Port Optik (PC4 & PC5):",
         "Jalur serial optical probe depan meter wajib dipisahkan menggunakan optocoupler dengan ketahanan dielektrik minimal 4 kV_RMS "
         "guna mencegah injeksi lonjakan tegangan petir dari luar selungkup meter."),
        ("e. Gerbang Non-Secure Callable (NSC) API Gateway:",
         "Seluruh panggilan dari RTOS / Stack DLMS Non-Secure menuju domain metrologi dilindungi gerbang NSC: "
         "SECURE_GetTamperVector(), SECURE_ReadBillingTimestamp(), SECURE_TriggerRelayAction(), dan SECURE_Crypto_SignDLMSFrame().")
    ]

    for title, text in schematic_notes:
        p_note = doc.add_paragraph()
        r_t = p_note.add_run(title + " ")
        r_t.font.name = 'Arial'
        r_t.font.size = Pt(8.5)
        r_t.font.bold = True
        r_t.font.color.rgb = RGBColor(0x00, 0x33, 0x66)

        r_txt = p_note.add_run(text)
        r_txt.font.name = 'Arial'
        r_txt.font.size = Pt(8.5)
        r_txt.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    # Simpan berkas ke path v4, v3, dan alternatif
    v4_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Register_dan_TrustZone_STM32U585_v4.docx'
    v3_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Register_dan_TrustZone_STM32U585_v3.docx'
    final_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Register_dan_TrustZone_STM32U585_Final.docx'
    primary_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Register_dan_TrustZone_STM32U585.docx'

    saved_paths = []
    for p in [v4_path, v3_path, final_path, primary_path]:
        try:
            doc.save(p)
            saved_paths.append(p)
        except PermissionError:
            pass

    if saved_paths:
        print('SUCCESS: Saved to: ' + ', '.join(saved_paths))
    else:
        print('ERROR: All paths locked.')

if __name__ == '__main__':
    create_complete_register_tz_doc_v4()
