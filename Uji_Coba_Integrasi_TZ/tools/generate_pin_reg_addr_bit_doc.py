import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.section import WD_ORIENTATION
from docx.oxml import parse_xml
from docx.oxml.ns import nsdecls

def create_pin_reg_addr_bit_doc():
    doc = Document()

    # Landscape orientasi agar kolom Register, Alamat, dan Bit sangat lega
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
            r.font.size = Pt(11.5)
            r.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
            h.paragraph_format.space_before = Pt(8)
            h.paragraph_format.space_after = Pt(3)
        elif level == 2:
            r.font.size = Pt(10)
            r.font.color.rgb = RGBColor(0x00, 0x55, 0x99)
            h.paragraph_format.space_before = Pt(6)
            h.paragraph_format.space_after = Pt(2)
        return h

    # ========================== HEADER DOKUMEN ==========================
    title_p = doc.add_paragraph()
    title_run = title_p.add_run('TABEL ALAMAT REGISTER & KONFIGURASI BIT HARDWARE PIN MCU')
    title_run.font.name = 'Arial'
    title_run.font.size = Pt(13.5)
    title_run.font.bold = True
    title_run.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    sub_p = doc.add_paragraph()
    sub_run = sub_p.add_run(
        'Smart Meter Listrik 3-Fasa Pascabayar (SPLN D3.006:2021 & B200) - STM32U585xx Cortex-M33\n'
        'Pemisahan Domain: TABEL SECURE WORLD vs TABEL NON-SECURE WORLD vs TABEL SISTEM & BERSAMA\n'
        'Format Kolom: Pin (MCU & Sinyal) | Register (Offset) | Alamat Fisik (Hex) | Bit / Field Konfigurasi (RM0456)'
    )
    sub_run.font.name = 'Arial'
    sub_run.font.size = Pt(8.5)
    sub_run.font.italic = True
    sub_run.font.color.rgb = RGBColor(0x55, 0x55, 0x55)
    sub_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    doc.add_paragraph()

    # Fungsi generic render tabel pin|register|alamat|bit
    def render_four_col_table(title, data_rows, header_bg='003366'):
        add_heading_styled(title, 1)

        headers = ['No', 'Pin (MCU & Sinyal)', 'Register Terkait (Offset RM0456)', 'Alamat Fisik (Hex)', 'Bit / Field Konfigurasi & Nilai Target', 'Fungsi Sinyal & Peran Teknis']
        col_widths = [0.35, 1.4, 1.9, 1.35, 3.8, 1.4]  # Total = 10.2 inches (Lega di Landscape)

        table = doc.add_table(rows=len(data_rows)+1, cols=len(headers))
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        table.autofit = False

        # Header
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

        # Rows
        for row_idx, rdata in enumerate(data_rows, start=1):
            row_cells = table.rows[row_idx].cells
            # Zebra coloring
            bg_color = 'FFF4EB' if header_bg == '990000' else ('F2F7FC' if row_idx % 2 == 1 else 'FFFFFF')
            if header_bg == '555555':
                bg_color = 'F4F6F6' if row_idx % 2 == 1 else 'FFFFFF'

            for col_idx, text in enumerate(rdata):
                row_cells[col_idx].text = text
                set_cell_background(row_cells[col_idx], bg_color)
                set_cell_margins(row_cells[col_idx], 35, 35, 30, 30)
                p = row_cells[col_idx].paragraphs[0]
                if col_idx in [0, 1]:
                    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                for run in p.runs:
                    run.font.name = 'Arial'
                    run.font.size = Pt(6.5)
                    if col_idx == 1:
                        run.font.bold = True
                        if header_bg == '990000':
                            run.font.color.rgb = RGBColor(0x99, 0x00, 0x00)
                        elif header_bg == '005599':
                            run.font.color.rgb = RGBColor(0x00, 0x44, 0x88)
        doc.add_paragraph()

    # ========================== 1. SECURE WORLD (19 PIN) ==========================
    secure_data = [
        ('1', 'PC14\n(TCXO_32K_IN)', 
         '1. RCC_BDCR (+0xF0)\n2. GPIOC_SECCFGR (+0x30)\n3. GPIOC_MODER (+0x00)', 
         '1. 0x56020CF0 (S)\n2. 0x52020830 (S)\n3. 0x52020800 (S)', 
         '1. Bit 2 LSEBYP=1 (Bypass Osc), Bit 0 LSEON=1, Bit 5 LSECSSON=1 (Monitor)\n2. Bit 14 SEC14 = 1 (Secure World Pad)\n3. Bits [29:28] MODER14 = 11b (Analog Mode saat LSE Bypass)', 
         'Input Clock TCXO 32,768 kHz (Bypass LSE) dengan pemantau LSECSS (ERR02).'),

        ('2', 'PC15\n(UNUSED_PC15)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_SECCFGR (+0x30)', 
         '1. 0x52020800 (S)\n2. 0x52020830 (S)', 
         '1. Bits [31:30] MODER15 = 11b (Analog Mode / Low Leakage)\n2. Bit 15 SEC15 = 1 (Secure World Pad)', 
         'Pin Analog hemat daya (kristal pasif digantikan TCXO 1-pin PC14).'),

        ('3', 'PB2\n(RTC_CAL_OUT)', 
         '1. RTC_CR (+0x18)\n2. GPIOB_AFRL (+0x20)\n3. GPIOB_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007818 (S)\n2. 0x52020420 (S)\n3. 0x52020430 (S)\n4. 0x50032500 (S)', 
         '1. Bit 29 COE=1 (Cal Output Enable), Bit 30 COSEL=0 (1 Hz) / 1 (512 Hz)\n2. Bits [11:8] AFSEL2 = 0000b (AF0 RTC_OUT2)\n3. Bit 2 SEC2 = 1 (Secure Pad)\n4. Bit 18 RTCSEC = 1 (Secure Peripheral)', 
         'Pin Uji Kalibrasi RTC Dedicated (1 Hz / 512 Hz) untuk ukur time drift.'),

        ('4', 'PC13\n(TAMP_CASE_OPEN)', 
         '1. TAMP_CR1 (+0x00)\n2. TAMP_SECCFGR (+0x20)\n3. GPIOC_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007C00 (S)\n2. 0x56007C20 (S)\n3. 0x52020830 (S)\n4. 0x50032500 (S)', 
         '1. Bit 0 TAMP1E = 1 (Enable Tamper 1 Active-Low)\n2. Bit 0 TAMP1SEC = 1 (Secure Tamper Resource)\n3. Bit 13 SEC13 = 1 (Secure Pad)\n4. Bit 19 TAMPSEC = 1 (Secure Peripheral)', 
         'Sabotase Tutup Meter: Operasi VSW (VBAT), catat timestamp, RUSAK & kunci kWh.'),

        ('5', 'PE6\n(TAMP_TERM_COVER)', 
         '1. TAMP_CR1 (+0x00)\n2. TAMP_SECCFGR (+0x20)\n3. GPIOE_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007C00 (S)\n2. 0x56007C20 (S)\n3. 0x52021030 (S)\n4. 0x50032500 (S)', 
         '1. Bit 2 TAMP3E = 1 (Enable Tamper 3 Active-Low)\n2. Bit 2 TAMP3SEC = 1 (Secure Tamper Resource)\n3. Bit 6 SEC6 = 1 (Secure Pad)\n4. Bit 19 TAMPSEC = 1 (Secure Peripheral)', 
         'Sabotase Tutup Terminal MCB: Operasi VSW (VBAT), catat ERR20 & buka relai.'),

        ('6', 'PE4\n(SENSOR_MAGNET)', 
         '1. TAMP_CR2 (+0x04)\n2. TAMP_SECCFGR (+0x20)\n3. GPIOE_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007C04 (S)\n2. 0x56007C20 (S)\n3. 0x52021030 (S)\n4. 0x50032500 (S)', 
         '1. Bit 6 TAMP7E = 1 (Enable Tamper 7 External Trigger)\n2. Bit 6 TAMP7SEC = 1 (Secure Tamper Resource)\n3. Bit 4 SEC4 = 1 (Secure Pad)\n4. Bit 19 TAMPSEC = 1 (Secure Peripheral)', 
         'Deteksi Induksi Medan Magnet Luar > 500mT (Hall Sensor): Rekam ERR25 di VSW.'),

        ('7', 'PC3\n(AC_FAIL_DET)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_SECCFGR (+0x30)\n3. EXTI_EXTICR1 (+0x60)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52020800 (S)\n2. 0x52020830 (S)\n3. 0x56022060 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [7:6] MODER3 = 00b (Input Mode)\n2. Bit 3 SEC3 = 1 (Secure Pad)\n3. Bits [27:24] EXTI3 = 0x02 (Port C Routing)\n4. Bit 3 SEC3 = 1 (Secure Interrupt Vector)\n5. Bit 3 IM3 = 1 (Interrupt Unmasked)\n6. Bit 3 FT3 = 1 (Falling Edge Trigger)', 
         'Deteksi Listrik Padam Dini: Memicu Last-Gasp Push & transisi ke Stop 2.'),

        ('8', 'PD11\n(VBAT_SENSE)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_SECCFGR (+0x30)\n3. ADC4_CHSELR (+0x28)\n4. ADC4_CR (+0x08)\n5. GTZC2_TZSC (+0x100)', 
         '1. 0x52020C00 (S)\n2. 0x52020C30 (S)\n3. 0x56021028 (S)\n4. 0x56021008 (S)\n5. 0x56033100 (S)', 
         '1. Bits [23:22] MODER11 = 11b (Analog Mode)\n2. Bit 11 SEC11 = 1 (Secure Pad)\n3. Bit 15 CHSEL15 = 1 (Select Channel 15)\n4. Bit 0 ADEN = 1, Bit 2 ADSTART = 1\n5. Bit 0 ADC4SEC = 1 (Secure Peripheral)', 
         'Monitor Tegangan Baterai Cadangan (ADC4 otonom di Stop 2, picu ERR03 jika < 2,7V).'),

        ('9', 'PC2\n(VCAP_SENSE)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_SECCFGR (+0x30)\n3. ADC4_CHSELR (+0x28)\n4. GTZC2_TZSC (+0x100)', 
         '1. 0x52020800 (S)\n2. 0x52020830 (S)\n3. 0x56021028 (S)\n4. 0x56033100 (S)', 
         '1. Bits [5:4] MODER2 = 11b (Analog Mode)\n2. Bit 2 SEC2 = 1 (Secure Pad)\n3. Bit 3 CHSEL3 = 1 (Select Channel 3)\n4. Bit 0 ADC4SEC = 1 (Secure Peripheral)', 
         'Monitor Superkapasitor (Deteksi kerusakan ERR06 & ambang transisi 2,7V).'),

        ('10', 'PD12\n(EXT_WDI_PULSE)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_BSRR (+0x18)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x52020C00 (S)\n2. 0x52020C18 (S)\n3. 0x52020C30 (S)', 
         '1. Bits [25:24] MODER12 = 01b (Output Push-Pull)\n2. Bit 12 BS12 = 1 (Set Pulse), Bit 28 BR12 = 1 (Reset Pulse)\n3. Bit 12 SEC12 = 1 (Secure Pad)', 
         'Sinyal Heartbeat ke IC External Watchdog (Memicu ERR04 jika MCU hang).'),

        ('11', 'PA4\n(ADE_SS)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_OSPEEDR (+0x08)\n3. GPIOA_BSRR (+0x18)\n4. GPIOA_SECCFGR (+0x30)\n5. GTZC1_TZSC (+0x100)', 
         '1. 0x52020000 (S)\n2. 0x52020008 (S)\n3. 0x52020018 (S)\n4. 0x52020030 (S)\n5. 0x50032500 (S)', 
         '1. Bits [9:8] MODER4 = 01b (Output Push-Pull)\n2. Bits [9:8] OSPEED4 = 11b (Very High Speed)\n3. Bit 4 BS4 = 1 (High/CS Non-Active), Bit 20 BR4 = 1 (Low/CS Active)\n4. Bit 4 SEC4 = 1 (Secure Pad)\n5. Bit 0 SPI1SEC = 1 (Secure Peripheral)', 
         'SPI1 Chip Select (CS) ke AFE Metrologi 3-Fasa ADE9000.'),

        ('12', 'PA5\n(ADE_SCK)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. SPI1_CR1 (+0x00)\n5. SPI1_CFG2 (+0x0C)', 
         '1. 0x52020000 (S)\n2. 0x52020020 (S)\n3. 0x52020030 (S)\n4. 0x50013000 (S)\n5. 0x5001300C (S)', 
         '1. Bits [11:10] MODER5 = 10b (Alternate Function)\n2. Bits [23:20] AFSEL5 = 0101b (AF5 SPI1_SCK)\n3. Bit 5 SEC5 = 1 (Secure Pad)\n4. Bit 0 SPE = 1 (SPI Enable)\n5. Bit 22 MASTER = 1 (Master Mode)', 
         'Serial Clock SPI1 ke ADE9000.'),

        ('13', 'PA6\n(ADE_MISO)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. SPI1_RXDR (+0x30)', 
         '1. 0x52020000 (S)\n2. 0x52020020 (S)\n3. 0x52020030 (S)\n4. 0x50013030 (S)', 
         '1. Bits [13:12] MODER6 = 10b (Alternate Function)\n2. Bits [27:24] AFSEL6 = 0101b (AF5 SPI1_MISO)\n3. Bit 6 SEC6 = 1 (Secure Pad)\n4. Bits [31:0] RXDATA (Read 32-bit register)', 
         'Master-In Slave-Out SPI1 ADE9000 (Baca RMS, Daya, Energi, THD).'),

        ('14', 'PA7\n(ADE_MOSI)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. SPI1_TXDR (+0x20)', 
         '1. 0x52020000 (S)\n2. 0x52020020 (S)\n3. 0x52020030 (S)\n4. 0x50013020 (S)', 
         '1. Bits [15:14] MODER7 = 10b (Alternate Function)\n2. Bits [31:28] AFSEL7 = 0101b (AF5 SPI1_MOSI)\n3. Bit 7 SEC7 = 1 (Secure Pad)\n4. Bits [31:0] TXDATA (Write 32-bit register)', 
         'Master-Out Slave-In SPI1 ke ADE9000.'),

        ('15', 'PB5\n(ADE_IRQ0)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_SECCFGR (+0x30)\n3. EXTI_EXTICR2 (+0x64)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52020400 (S)\n2. 0x52020430 (S)\n3. 0x56022064 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [11:10] MODER5 = 00b (Input Mode)\n2. Bit 5 SEC5 = 1 (Secure Pad)\n3. Bits [11:8] EXTI5 = 0x01 (Port B Routing)\n4. Bit 5 SEC5 = 1 (Secure Interrupt Vector)\n5. Bit 5 IM5 = 1 (Interrupt Unmasked)\n6. Bit 5 FT5 = 1 (Falling Edge Trigger)', 
         'Interupsi HW 0 ADE9000: Reverse Current (ERR26), Urutan Fase Terbalik (ERR21).'),

        ('16', 'PB1\n(ADE_IRQ1)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_SECCFGR (+0x30)\n3. EXTI_EXTICR1 (+0x60)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52020400 (S)\n2. 0x52020430 (S)\n3. 0x56022060 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [3:2] MODER1 = 00b (Input Mode)\n2. Bit 1 SEC1 = 1 (Secure Pad)\n3. Bits [11:8] EXTI1 = 0x01 (Port B Routing)\n4. Bit 1 SEC1 = 1 (Secure Interrupt Vector)\n5. Bit 1 IM1 = 1 (Interrupt Unmasked)\n6. Bit 1 FT1 = 1 (Falling Edge Trigger)', 
         'Interupsi HW 1 ADE9000: Netral Putus (ERR23), Hilang Tegangan (ERR24), Sag/Swell.'),

        ('17', 'PE11\n(ADE_EVENT)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_SECCFGR (+0x30)\n3. EXTI_EXTICR3 (+0x68)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x52021030 (S)\n3. 0x56022068 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [23:22] MODER11 = 00b (Input Mode)\n2. Bit 11 SEC11 = 1 (Secure Pad)\n3. Bits [27:24] EXTI11 = 0x04 (Port E Routing)\n4. Bit 11 SEC11 = 1 (Secure Interrupt Vector)\n5. Bit 11 IM11 = 1 (Interrupt Unmasked)\n6. Bit 11 FT11 = 1 (Falling Edge Trigger)', 
         'Interupsi Power Quality Event (ADE_EVENT / CF4): Menerima sinyal Pin 36 ADE9000 yang dikonfigurasi sebagai EVENT (disesuaikan dari fungsi sebelumnya DREADY via CFMODE[13:12]=10b) saat terjadi tegangan kedip (Dip/Sag per Tabel 10), lonjakan (Swell per Tabel 11), atau arus lebih (Overcurrent per Butir 10.4) untuk memicu pembekuan rekaman gangguan dan log Power Quality.'),

        ('18', 'PE7\n(ADE_RESET)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_ODR (+0x14)\n3. GPIOE_SECCFGR (+0x30)', 
         '1. 0x52021000 (S)\n2. 0x52021014 (S)\n3. 0x52021030 (S)', 
         '1. Bits [15:14] MODER7 = 01b (Output Push-Pull)\n2. Bit 7 ODR7 = 1 (Run Mode) / 0 (Hardware Reset)\n3. Bit 7 SEC7 = 1 (Secure Pad)', 
         'Hardware Reset IC ADE9000 (Reset AFE jika sampling ADC berhenti).'),

        ('19', 'PE8\n(ADE_CF3_ZX)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_SECCFGR (+0x30)\n3. EXTI_EXTICR3 (+0x68)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_RTSR1 (+0x00) & FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x52021030 (S)\n3. 0x56022068 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022000 (S) & 0x56022004 (S)', 
         '1. Bits [17:16] MODER8 = 00b (Input Digital)\n2. Bit 8 SEC8 = 1 (Secure Pad)\n3. Bits [3:0] EXTI8 = 0x04 (Port E Routing)\n4. Bit 8 SEC8 = 1 (Secure Interrupt Vector)\n5. Bit 8 IM8 = 1 (Interrupt Unmasked)\n6. Bit 8 RT8 = 1 / FT8 = 1 (Zero-Crossing Rising & Falling Trigger)', 
         'Fungsi Ganda Pin 35 ADE9000 (CF3 / ZX): Dapat difungsikan sebagai pulsa kalibrasi energi semu kVAh (CF3) atau Deteksi Perlintasan Nol AC (Zero-Crossing / ZX) untuk sinkronisasi pemutusan Shunt Trip PC8 di titik arus nol (Zero-Current Switching) guna meminimalkan lonjakan tegangan dan arcing kontak.'),

        ('20', 'PE9\n(ADE_PM0)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_ODR (+0x14)\n3. GPIOE_SECCFGR (+0x30)', 
         '1. 0x52021000 (S)\n2. 0x52021014 (S)\n3. 0x52021030 (S)', 
         '1. Bits [19:18] MODER9 = 01b (Output Push-Pull)\n2. Bit 9 ODR9 = 0 (Run PSM0) / 1 (Sleep PSM3)\n3. Bit 9 SEC9 = 1 (Secure Pad)', 
         'Kontrol Power Mode 0 ADE9000: Bersama PE10 (PM1) mengatur mode hemat daya AFE saat pemadaman listrik PLN (PSM0 Run / PSM3 Total Sleep ~2uA).'),

        ('21', 'PE10\n(ADE_PM1)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_ODR (+0x14)\n3. GPIOE_SECCFGR (+0x30)', 
         '1. 0x52021000 (S)\n2. 0x52021014 (S)\n3. 0x52021030 (S)', 
         '1. Bits [21:20] MODER10 = 01b (Output Push-Pull)\n2. Bit 10 ODR10 (Kendali mode daya ADE9000 saat padam)\n3. Bit 10 SEC10 = 1 (Secure Pad)', 
         'Kontrol Power Mode 1 ADE9000: Menurunkan mode daya ADE saat listrik PLN padam.'),

        ('22', 'PE12\n(ADE_CF1)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_PUPDR (+0x0C)\n3. GPIOE_SECCFGR (+0x30)\n4. EXTI_EXTICR4 (+0x6C)\n5. EXTI_SECCFGR1 (+0x14)\n6. EXTI_IMR1 (+0x80)\n7. EXTI_FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x5202100C (S)\n3. 0x52021030 (S)\n4. 0x5602206C (S)\n5. 0x56022014 (S)\n6. 0x56022080 (S)\n7. 0x56022004 (S)', 
         '1. Bits [25:24] MODER12 = 00b (Input Digital)\n2. Bits [25:24] PUPDR12 = 01b (Pull-Up)\n3. Bit 12 SEC12 = 1 (Secure Pad)\n4. Bits [3:0] EXTI12 = 0x04 (Port E Routing)\n5. Bit 12 SEC12 = 1 (Secure Interrupt Vector)\n6. Bit 12 IM12 = 1 (Interrupt Unmasked)\n7. Bit 12 FT12 = 1 (Falling Edge Pulse Trigger)', 
         'Pencacah Pulsa Kalibrasi Energi Aktif (CF1 / Imp/kWh): Menerima pulsa Pin 33 ADE9000 (1.000 imp/kWh, lebar 80 ms ± 2 ms per SPLN D3.006 Butir 6.10 & 6.9 a) paralel dengan LED PB4 untuk validasi mandiri register Wh SPI (AWATTHR, BWATTHR, CWATTHR).'),

        ('23', 'PE13\n(ADE_CF2)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_PUPDR (+0x0C)\n3. GPIOE_SECCFGR (+0x30)\n4. EXTI_EXTICR4 (+0x6C)\n5. EXTI_SECCFGR1 (+0x14)\n6. EXTI_IMR1 (+0x80)\n7. EXTI_FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x5202100C (S)\n3. 0x52021030 (S)\n4. 0x5602206C (S)\n5. 0x56022014 (S)\n6. 0x56022080 (S)\n7. 0x56022004 (S)', 
         '1. Bits [27:26] MODER13 = 00b (Input Digital)\n2. Bits [27:26] PUPDR13 = 01b (Pull-Up)\n3. Bit 13 SEC13 = 1 (Secure Pad)\n4. Bits [7:4] EXTI13 = 0x04 (Port E Routing)\n5. Bit 13 SEC13 = 1 (Secure Interrupt Vector)\n6. Bit 13 IM13 = 1 (Interrupt Unmasked)\n7. Bit 13 FT13 = 1 (Falling Edge Pulse Trigger)', 
         'Pencacah Pulsa Kalibrasi Energi Reaktif (CF2 / Imp/kvarh): Menerima pulsa Pin 34 ADE9000 (1.000 imp/kvarh, lebar 80 ms ± 2 ms per SPLN D3.006 Butir 6.10, 6.9 b & Butir 7.1) paralel dengan LED PB5 untuk validasi mandiri register varh 4-kuadran di SPI.')
    ]
    render_four_col_table('1. Tabel Register & Alamat Hardware: SECURE WORLD (23 Pin)', secure_data, '990000')

    # ========================== 2. NON-SECURE WORLD (25 PIN) ==========================
    nonsecure_data = [
        ('1', 'PH0\n(OSC_IN)', 
         '1. RCC_CR (+0x00)\n2. GPIOH_MODER (+0x00)\n3. GPIOH_SECCFGR (+0x30)', 
         '1. 0x46020C00 (NS)\n2. 0x42021C00 (NS)\n3. 0x42021C30 (NS)', 
         '1. Bit 16 HSEON = 1, Bit 17 HSERDY = 1 (HSE Active)\n2. Bits [1:0] MODER0 = 11b (Analog Mode)\n3. Bit 0 SEC0 = 0 (Non-Secure Pad)', 
         'Input Kristal Utama HSE (8–16 MHz).'),

        ('2', 'PH1\n(OSC_OUT)', 
         '1. GPIOH_MODER (+0x00)\n2. GPIOH_SECCFGR (+0x30)', 
         '1. 0x42021C00 (NS)\n2. 0x42021C30 (NS)', 
         '1. Bits [3:2] MODER1 = 11b (Analog Mode)\n2. Bit 1 SEC1 = 0 (Non-Secure Pad)', 
         'Output Kristal Utama HSE.'),

        ('3', 'PC0\n(LCD_I2C3_SCL)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_OTYPER (+0x04)\n3. GPIOC_AFRL (+0x20)\n4. GPIOC_SECCFGR (+0x30)\n5. I2C3_TIMINGR (+0x10)', 
         '1. 0x42020800 (NS)\n2. 0x42020804 (NS)\n3. 0x42020820 (NS)\n4. 0x42020830 (NS)\n5. 0x46002810 (NS)', 
         '1. Bits [1:0] MODER0 = 10b (Alternate Function)\n2. Bit 0 OT0 = 1 (Open-Drain)\n3. Bits [3:0] AFSEL0 = 0100b (AF4 I2C3_SCL)\n4. Bit 0 SEC0 = 0 (Non-Secure Pad)\n5. 0x3042C7C7 (Standard Mode 100 kHz)', 
         'Clock Bus I2C3 untuk Driver Layar Matriks STN LCD (Otonom di Stop 2).'),

        ('4', 'PC1\n(LCD_I2C3_SDA)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_OTYPER (+0x04)\n3. GPIOC_AFRL (+0x20)\n4. GPIOC_SECCFGR (+0x30)\n5. I2C3_CR1 (+0x00)', 
         '1. 0x42020800 (NS)\n2. 0x42020804 (NS)\n3. 0x42020820 (NS)\n4. 0x42020830 (NS)\n5. 0x46002800 (NS)', 
         '1. Bits [3:2] MODER1 = 10b (Alternate Function)\n2. Bit 1 OT1 = 1 (Open-Drain)\n3. Bits [7:4] AFSEL1 = 0100b (AF4 I2C3_SDA)\n4. Bit 1 SEC1 = 0 (Non-Secure Pad)\n5. Bit 0 PE = 1 (Peripheral Enable)', 
         'Data Bus I2C3 untuk Driver Layar Matriks STN LCD 3-Baris.'),

        ('5', 'PD2\n(LCD_PWR_EN)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [5:4] MODER2 = 01b (Output Push-Pull)\n2. Bit 2 ODR2 = 1 (Power LCD ON), ODR2 = 0 (OFF)\n3. Bit 2 SEC2 = 0 (Non-Secure Pad)', 
         'Sakelar Catu Daya LCD: Mengaktifkan display 10 detik saat push-to-read padam.'),

        ('6', 'PD3\n(LCD_BL_CTRL)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [7:6] MODER3 = 01b (Output Push-Pull)\n2. Bit 3 ODR3 = 1 (Backlight ON), ODR3 = 0 (OFF)\n3. Bit 3 SEC3 = 0 (Non-Secure Pad)', 
         'Kontrol Lampu Latar (Backlight) LCD (Menyala 10 detik saat tombol ditekan).'),

        ('7', 'PA0\n(BTN_SCROLL_UP)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_PUPDR (+0x0C)\n3. GPIOA_SECCFGR (+0x30)\n4. EXTI_EXTICR1 (+0x60)\n5. EXTI_IMR1 (+0x80)\n6. PWR_WUCR1 (+0x14)\n7. PWR_WUCR2 (+0x18)\n8. PWR_WUCR3 (+0x1C)', 
         '1. 0x42020000 (NS)\n2. 0x4202000C (NS)\n3. 0x42020030 (NS)\n4. 0x46022060 (NS)\n5. 0x46022080 (NS)\n6. 0x46020814 (NS)\n7. 0x46020818 (NS)\n8. 0x4602081C (NS)', 
         '1. Bits [1:0] MODER0 = 00b (Input Mode)\n2. Bits [1:0] PUPDR0 = 01b (Pull-Up)\n3. Bit 0 SEC0 = 0 (Non-Secure Pad)\n4. Bits [3:0] EXTI0 = 0x00 (Port A Line 0), IMR1[0]=1\n5. Bit 0 WUPEN1 = 1 (WKUP1 Enable)\n6. Bit 0 WUPP1 = 1 (Falling Edge Low Trigger)\n7. Bits [1:0] WUSEL1 = 00b (Source 0: PA0)', 
         'Tombol Navigasi Naik (▲), Wakeup MCU saat padam, & Reconnect Relai.'),

        ('8', 'PB6\n(BTN_SCROLL_DN)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_PUPDR (+0x0C)\n3. GPIOB_SECCFGR (+0x30)\n4. EXTI_EXTICR2 (+0x64)\n5. EXTI_IMR1 (+0x80)\n6. PWR_WUCR1 (+0x14)\n7. PWR_WUCR2 (+0x18)\n8. PWR_WUCR3 (+0x1C)', 
         '1. 0x42020400 (NS)\n2. 0x4202040C (NS)\n3. 0x42020430 (NS)\n4. 0x46022064 (NS)\n5. 0x46022080 (NS)\n6. 0x46020814 (NS)\n7. 0x46020818 (NS)\n8. 0x4602081C (NS)', 
         '1. Bits [13:12] MODER6 = 00b (Input Mode)\n2. Bits [13:12] PUPDR6 = 01b (Pull-Up)\n3. Bit 6 SEC6 = 0 (Non-Secure Pad)\n4. Bits [19:16] EXTI6 = 0x01 (Port B Line 6), IMR1[6]=1\n5. Bit 2 WUPEN3 = 1 (WKUP3 Enable, DS13086 Hal 129)\n6. Bit 2 WUPP3 = 1 (Falling Edge Low Trigger)\n7. Bits [5:4] WUSEL3 = 10b (Source 2: PB6)', 
         'Tombol Navigasi Turun (▼), Wakeup MCU saat padam, & Konfirmasi Reconnect (▲+▼).'),

        ('9', 'PB7\n(LED_ALM)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_BSRR (+0x18)\n3. GPIOB_SECCFGR (+0x30)', 
         '1. 0x42020400 (NS)\n2. 0x42020418 (NS)\n3. 0x42020430 (NS)', 
         '1. Bits [15:14] MODER7 = 01b (Output Push-Pull)\n2. Bit 7 BS7 = 1 (Set LED ON), Bit 23 BR7 = 1 (Reset LED OFF)\n3. Bit 7 SEC7 = 0 (Non-Secure Pad)', 
         'Driver Indikator LED Kuning Alarm / Peringatan Tamper.'),

        ('10', 'PA2\n(MODEM_TX)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. RCC_CCIPR1 (+0xE0)\n5. LPUART1_PRESC (+0x1C)\n6. LPUART1_BRR (+0x0C)\n7. LPUART1_CR1 (+0x00)\n8. LPUART1_TDR (+0x28)', 
         '1. 0x42020000 (NS)\n2. 0x42020020 (NS)\n3. 0x42020030 (NS)\n4. 0x46020CE0 (NS)\n5. 0x4600241C (NS)\n6. 0x4600240C (NS)\n7. 0x46002400 (NS)\n8. 0x46002428 (NS)', 
         '1. Bits [5:4] MODER2 = 10b (AF Mode)\n2. Bits [11:8] AFSEL2 = 1000b (AF8 LPUART1)\n3. Bit 2 SEC2 = 0 (Non-Secure Pad)\n4. Bits [11:10] LPUART1SEL = 10b (Pilih HSI16 16 MHz)\n5. Bits [3:0] PRESCALER = 0000b (/1 divider)\n6. BRR = (256×16MHz)/19200 = 0x0003_4155\n7. Bit 0 UE=1, Bit 3 TE=1, Bit 29 FIFOEN=1\n8. Bits [8:0] TDR (Transmit Data Register)', 
         'Port RS-232 TX ke RJ-45 Pin 5 (via Transceiver MAX3232 @ 19.200 bps).'),

        ('11', 'PA3\n(MODEM_RX)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_PUPDR (+0x0C)\n3. GPIOA_AFRL (+0x20)\n4. GPIOA_SECCFGR (+0x30)\n5. LPUART1_CR1 (+0x00)\n6. LPUART1_CR3 (+0x08)\n7. LPUART1_RDR (+0x24)', 
         '1. 0x42020000 (NS)\n2. 0x4202000C (NS)\n3. 0x42020020 (NS)\n4. 0x42020030 (NS)\n5. 0x46002400 (NS)\n6. 0x46002408 (NS)\n7. 0x46002424 (NS)', 
         '1. Bits [7:6] MODER3 = 10b (AF Mode)\n2. Bits [7:6] PUPDR3 = 01b (Pull-Up)\n3. Bits [15:12] AFSEL3 = 1000b (AF8 LPUART1)\n4. Bit 3 SEC3 = 0 (Non-Secure Pad)\n5. Bit 2 RE = 1 (Receiver Enable)\n6. Bit 0 DMAR = 1 (DMA Rx Ring Buffer)\n7. Bits [8:0] RDR (Receive Data Register)', 
         'Port RS-232 RX dari RJ-45 Pin 4 (via MAX3232 & DMA Ring Buffer; aktif di Stop 2).'),

        ('12', 'PD4\n(RS232_PWR_EN)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [9:8] MODER4 = 01b (Output Push-Pull)\n2. Bit 4 ODR4 = 1 (Power ON) / 0 (FORCEOFF MAX3232)\n3. Bit 4 SEC4 = 0 (Non-Secure Pad)', 
         'Kontrol Daya Transceiver RS-232: Mematikan MAX3232 paska Last-Gasp.'),

        ('13', 'PD5\n(MODEM_12V_EN)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [11:10] MODER5 = 01b (Output Push-Pull)\n2. Bit 5 ODR5 = 1 (12V ON) / 0 (Putus Catu 12V)\n3. Bit 5 SEC5 = 0 (Non-Secure Pad)', 
         'Kontrol Pemutus Catu Daya Modem 12V: Memutus 12V paska Last-Gasp Push.'),

        ('14', 'PC4\n(OPTICAL_TX)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_AFRL (+0x20)\n3. GPIOC_SECCFGR (+0x30)\n4. USART3_CR1 (+0x00)\n5. USART3_BRR (+0x0C)\n6. USART3_TDR (+0x28)', 
         '1. 0x42020800 (NS)\n2. 0x42020820 (NS)\n3. 0x42020830 (NS)\n4. 0x40004800 (NS)\n5. 0x4000480C (NS)\n6. 0x40004828 (NS)', 
         '1. Bits [9:8] MODER4 = 10b (AF Mode)\n2. Bits [19:16] AFSEL4 = 0111b (AF7 USART3_TX)\n3. Bit 4 SEC4 = 0 (Non-Secure Pad)\n4. Bit 0 UE = 1, Bit 3 TE = 1\n5. BRR = 160MHz/9600 = 16.667 = 0x411B\n6. Bits [8:0] TDR (Transmit Data Register)', 
         'Port Komunikasi Lokal TX Optical Probe Depan Meter (Isolasi > 4 kV).'),

        ('15', 'PC5\n(OPTICAL_RX)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_AFRL (+0x20)\n4. GPIOC_SECCFGR (+0x30)\n5. USART3_CR1 (+0x00)\n6. USART3_RDR (+0x24)\n7. PWR_WUCR1 (+0x14)\n8. PWR_WUCR2 (+0x18)\n9. PWR_WUCR3 (+0x1C)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020820 (NS)\n4. 0x42020830 (NS)\n5. 0x40004800 (NS)\n6. 0x40004824 (NS)\n7. 0x46020814 (NS)\n8. 0x46020818 (NS)\n9. 0x4602081C (NS)', 
         '1. Bits [11:10] MODER5 = 10b (AF Mode)\n2. Bits [11:10] PUPDR5 = 01b (Pull-Up)\n3. Bits [23:20] AFSEL5 = 0111b (AF7 USART3_RX)\n4. Bit 5 SEC5 = 0 (Non-Secure Pad)\n5. Bit 2 RE = 1\n6. Bits [8:0] RDR (Receive Data Register)\n7. Bit 4 WUPEN5 = 1 (WKUP5 Enable, DS13086 Hal 114)\n8. Bit 4 WUPP5 = 1 (Falling Edge Trigger)\n9. Bits [9:8] WUSEL5 = 00b (Source PC5)', 
         'Port Komunikasi Lokal RX Optical Probe Depan Meter (Wakeup WKUP5).'),

        ('16', 'PC8\n(CTRL_RELAY_TRIP)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_BSRR (+0x18)\n4. GPIOC_SECCFGR (+0x30)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020818 (NS)\n4. 0x42020830 (NS)', 
         '1. Bits [17:16] MODER8 = 01b (Output Mode)\n2. Bits [17:16] PUPDR8 = 10b (Pull-Down)\n3. Bit 8 BS8 = 1 (Koil Shunt Trip Aktif: Buka MCCB)\n4. Bit 8 SEC8 = 0 (Non-Secure Pad)', 
         'Kontrol Pemutus Utama (Shunt Trip Relai Beban maks 240 VAC, 2 A Sambungan Tidak Langsung).'),

        ('17', 'PC9\n(SPARE_GPIO)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_BSRR (+0x18)\n4. GPIOC_SECCFGR (+0x30)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020818 (NS)\n4. 0x42020830 (NS)', 
         '1. Bits [19:18] MODER9 = 01b (Output Mode)\n2. Bits [19:18] PUPDR9 = 10b (Pull-Down)\n3. Bit 9 BR9 = 1 (Tarik Low / Idle Safe State)\n4. Bit 9 SEC9 = 0 (Non-Secure Pad)', 
         'Cadangan Digital Output / Spare (Meter Sambungan Tidak Langsung Reconnect MCCB Manual).'),

        ('18', 'PC7\n(RELAY_STATUS)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_IDR (+0x10)\n4. GPIOC_SECCFGR (+0x30)\n5. EXTI_EXTICR2 (+0x64)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020810 (NS)\n4. 0x42020830 (NS)\n5. 0x46022064 (NS)', 
         '1. Bits [15:14] MODER7 = 00b (Input Mode)\n2. Bits [15:14] PUPDR7 = 10b (Pull-Down)\n3. Bit 7 IDR7 (Auxiliary Feedback Status: 1=Closed, 0=Open)\n4. Bit 7 SEC7 = 0 (Non-Secure Pad)\n5. Bits [27:24] EXTI7 = 0x02 (Port C Line 7)', 
         'Umpan Balik Kontak Bantu Relai (Deteksi kegagalan mekanik ERR07).'),

        ('19', 'PD8\n(AUX_CTRL_OUT1)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [17:16] MODER8 = 01b (Output Push-Pull)\n2. Bit 8 ODR8 (Driver Relai Bantu 1 Opto-SSR)\n3. Bit 8 SEC8 = 0 (Non-Secure Pad)', 
         'Kontrol Bantu Relai Output 1 (Driver Opto-SSR maks 288 VAC, 50 mA).'),

        ('20', 'PD10\n(AUX_CTRL_IN1)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_PUPDR (+0x0C)\n3. GPIOD_IDR (+0x10)\n4. GPIOD_SECCFGR (+0x30)\n5. EXTI_EXTICR3 (+0x68)\n6. EXTI_IMR1 (+0x80)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C0C (NS)\n3. 0x42020C10 (NS)\n4. 0x42020C30 (NS)\n5. 0x46022068 (NS)\n6. 0x46022080 (NS)', 
         '1. Bits [21:20] MODER10 = 00b (Input Mode)\n2. Bits [21:20] PUPDR10 = 01b (Pull-Up)\n3. Bit 10 IDR10 (Status Input Digital 1)\n4. Bit 10 SEC10 = 0 (Non-Secure Pad)\n5. Bits [19:16] EXTI10 = 0x03 (Port D Line 10)\n6. Bit 10 IM10 = 1 (Interrupt Unmasked)', 
         'Kontrol Bantu Input Digital 1 (Optocoupler 60–288 VAC).'),

        ('21', 'PD14\n(AUX_CTRL_IN2)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_PUPDR (+0x0C)\n3. GPIOD_IDR (+0x10)\n4. GPIOD_SECCFGR (+0x30)\n5. EXTI_EXTICR4 (+0x6C)\n6. EXTI_IMR1 (+0x80)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C0C (NS)\n3. 0x42020C10 (NS)\n4. 0x42020C30 (NS)\n5. 0x4602206C (NS)\n6. 0x46022080 (NS)', 
         '1. Bits [29:28] MODER14 = 00b (Input Mode)\n2. Bits [29:28] PUPDR14 = 01b (Pull-Up)\n3. Bit 14 IDR14 (Status Input Digital 2)\n4. Bit 14 SEC14 = 0 (Non-Secure Pad)\n5. Bits [19:16] EXTI14 = 0x03 (Port D Line 14)\n6. Bit 14 IM14 = 1 (Interrupt Unmasked)', 
         'Kontrol Bantu Input Digital 2 (Optocoupler 60–288 VAC).'),

        ('22', 'PB12\n(FLASH_SPI2_CS)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_OSPEEDR (+0x08)\n3. GPIOB_BSRR (+0x18)\n4. GPIOB_SECCFGR (+0x30)\n5. GTZC1_TZSC (+0x100)', 
         '1. 0x42020400 (NS)\n2. 0x42020408 (NS)\n3. 0x42020418 (NS)\n4. 0x42020430 (NS)\n5. 0x40032500 (NS)', 
         '1. Bits [25:24] MODER12 = 01b (Output Push-Pull)\n2. Bits [25:24] OSPEED12 = 11b (Very High Speed)\n3. Bit 12 BS12 = 1 (CS High), Bit 28 BR12 = 1 (CS Low)\n4. Bit 12 SEC12 = 0 (Non-Secure Pad)\n5. Bit 1 SPI2SEC = 0 (Non-Secure Peripheral)', 
         'Chip Select (CS) Memori Non-Volatile Eksternal (SPI NOR Flash / FRAM 4 MB).'),

        ('23', 'PB13\n(FLASH_SPI2_SCK)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_AFRH (+0x24)\n3. GPIOB_SECCFGR (+0x30)\n4. SPI2_CR1 (+0x00)\n5. SPI2_CFG2 (+0x0C)', 
         '1. 0x42020400 (NS)\n2. 0x42020424 (NS)\n3. 0x42020430 (NS)\n4. 0x40003800 (NS)\n5. 0x4000380C (NS)', 
         '1. Bits [27:26] MODER13 = 10b (Alternate Function)\n2. Bits [23:20] AFSEL13 = 0101b (AF5 SPI2_SCK)\n3. Bit 13 SEC13 = 0 (Non-Secure Pad)\n4. Bit 0 SPE = 1 (SPI Enable)\n5. Bit 22 MASTER = 1 (Master Mode)', 
         'Serial Clock SPI2 Memori Eksternal (Load Profile 3 bulan & Billing 12 bulan).'),

        ('24', 'PB14\n(FLASH_SPI2_MISO)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_AFRH (+0x24)\n3. GPIOB_SECCFGR (+0x30)\n4. SPI2_RXDR (+0x30)', 
         '1. 0x42020400 (NS)\n2. 0x42020424 (NS)\n3. 0x42020430 (NS)\n4. 0x40003830 (NS)', 
         '1. Bits [29:28] MODER14 = 10b (Alternate Function)\n2. Bits [27:24] AFSEL14 = 0101b (AF5 SPI2_MISO)\n3. Bit 14 SEC14 = 0 (Non-Secure Pad)\n4. Bits [31:0] RXDATA (Read Flash Data)', 
         'Master-In Slave-Out SPI2 Memori Eksternal (Deteksi gagal memori ERR00).'),

        ('25', 'PB15\n(FLASH_SPI2_MOSI)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_AFRH (+0x24)\n3. GPIOB_SECCFGR (+0x30)\n4. SPI2_TXDR (+0x20)', 
         '1. 0x42020400 (NS)\n2. 0x42020424 (NS)\n3. 0x42020430 (NS)\n4. 0x40003820 (NS)', 
         '1. Bits [31:30] MODER15 = 10b (Alternate Function)\n2. Bits [31:28] AFSEL15 = 0101b (AF5 SPI2_MOSI)\n3. Bit 15 SEC15 = 0 (Non-Secure Pad)\n4. Bits [31:0] TXDATA (Write Flash Data)', 
         'Master-Out Slave-In SPI2 Memori Eksternal.')
    ]
    render_four_col_table('2. Tabel Register & Alamat Hardware: NON-SECURE WORLD (25 Pin)', nonsecure_data, '005599')

    # ========================== 3. SYSTEM & SHARED PINS (5 PIN) ==========================
    system_data = [
        ('1', 'NRST\n(RESET_SYSTEM)', 
         '1. RCC_CSR (+0xF4)', 
         '1. 0x56020CF4 (S)\n   0x46020CF4 (NS)', 
         '1. Bit 26 PINRSTF = 1 (NRST external pin reset flag)\n   Bit 23 RMVF = 1 (Remove reset flags / clear status)', 
         'Jalur Hardware Reset Sistem MCU (Dedicated Bidirectional Analog Pad dengan Internal Pull-Up, bukan GPIO).'),

        ('2', 'VBAT\n(VBAT_IN)', 
         '1. PWR_BDCR1 (+0x20)\n2. RCC_BDCR (+0xF0)', 
         '1. 0x56020820 (S)\n2. 0x56020CF0 (S)', 
         '1. Bit 0 MONEN = 1 (Backup domain voltage monitor enable), Bit 1 VBATF\n2. Bit 15 RTCEN = 1, Bit 0 LSEON = 1', 
         'Input Catu Baterai Cadangan Li-SOCl2 3,6V (Menyuplai domain VSW saat daya utama padam).'),

        ('3', 'PA13\n(SWDIO)', 
         '1. GPIOA_AFRH (+0x24)', 
         '1. 0x52020024 (S)', 
         '1. Bits [23:20] AFSEL13 = 0000b (AF0 SWDIO Debug Interface)', 
         'Jalur Data Serial Wire Debugging (SWD) ST-LINK (Dikunci via RDP Level 2 pada produksi massal).'),

        ('4', 'PA14\n(SWCLK)', 
         '1. GPIOA_AFRH (+0x24)', 
         '1. 0x52020024 (S)', 
         '1. Bits [27:24] AFSEL14 = 0000b (AF0 SWCLK Debug Interface)', 
         'Jalur Clock Serial Wire Debugging (SWD) ST-LINK.'),

        ('5', 'Rail DC\n(LED_POWER_ON)', 
         '1. Hardwired Analog HW', 
         '1. N/A (Analog Power Hardware)', 
         '1. Jalur catu fasa hardware non-MCU (Disuplai langsung dari regulator power supply board)', 
         'Indikator LED Hijau Catu Daya Utama 3-Fasa (Hardware Driven langsung dari power supply board).')
    ]
    render_four_col_table('3. Tabel Register & Alamat Hardware: SISTEM & SINYAL BERSAMA (5 Pin)', system_data, '555555')

    # Simpan berkas
    target_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/Tabel_Pin_Register_Alamat_Bit_STM32U585.docx'
    doc.save(target_path)
    print(f'SUCCESS: Document saved to {target_path}')

if __name__ == '__main__':
    create_pin_reg_addr_bit_doc()

