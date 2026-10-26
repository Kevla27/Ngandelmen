import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.section import WD_ORIENTATION
from docx.oxml import parse_xml
from docx.oxml.ns import nsdecls

def populate_regisf_doc():
    doc = Document()

    # Landscape orientasi agar kolom Pin | Register Terkait | Alamat | Bit sangat lega
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
    title_run = title_p.add_run('TABEL REGISTER HARDWARE & ARSITEKTUR TRUSTZONE (REGISF)')
    title_run.font.name = 'Arial'
    title_run.font.size = Pt(13.5)
    title_run.font.bold = True
    title_run.font.color.rgb = RGBColor(0x00, 0x33, 0x66)
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    sub_p = doc.add_paragraph()
    sub_run = sub_p.add_run(
        'Smart Meter Listrik 3-Fasa Pascabayar (SPLN D3.006:2021 & B200) - STM32U585xx Arm Cortex-M33\n'
        'Pemisahan Domain: SECURE WORLD vs NON-SECURE WORLD vs SISTEM & REGISTER INTERNAL\n'
        'Format Kolom Resmi: Pin / Modul Fungsional | Register Terkait (Offset RM0456) | Alamat Fisik (Hex) | Bit / Field Konfigurasi'
    )
    sub_run.font.name = 'Arial'
    sub_run.font.size = Pt(8.5)
    sub_run.font.italic = True
    sub_run.font.color.rgb = RGBColor(0x55, 0x55, 0x55)
    sub_p.alignment = WD_ALIGN_PARAGRAPH.CENTER

    doc.add_paragraph()

    # Fungsi generic render tabel 4 kolom
    def render_four_col_table(title, data_rows, header_bg='003366'):
        add_heading_styled(title, 1)

        headers = ['Pin / Modul Fungsional', 'Register Terkait (Offset RM0456)', 'Alamat Fisik (Hex)', 'Bit / Field Konfigurasi & Nilai Target']
        col_widths = [1.8, 2.2, 1.8, 4.4]  # Total = 10.2 inci

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
                run.font.size = Pt(8)
                run.font.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

        # Rows
        for row_idx, rdata in enumerate(data_rows, start=1):
            row_cells = table.rows[row_idx].cells
            bg_color = 'FFF4EB' if header_bg == '990000' else ('F2F7FC' if row_idx % 2 == 1 else 'FFFFFF')
            if header_bg in ['555555', '333366']:
                bg_color = 'F4F6F6' if row_idx % 2 == 1 else 'FFFFFF'

            for col_idx, text in enumerate(rdata):
                row_cells[col_idx].text = text
                set_cell_background(row_cells[col_idx], bg_color)
                set_cell_margins(row_cells[col_idx], 35, 35, 30, 30)
                p = row_cells[col_idx].paragraphs[0]
                if col_idx in [0]:
                    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
                for run in p.runs:
                    run.font.name = 'Arial'
                    run.font.size = Pt(7)
                    if col_idx == 0:
                        run.font.bold = True
                        if header_bg == '990000':
                            run.font.color.rgb = RGBColor(0x99, 0x00, 0x00)
                        elif header_bg == '005599':
                            run.font.color.rgb = RGBColor(0x00, 0x44, 0x88)
        doc.add_paragraph()

    # ========================== 1. SECURE WORLD (19 PIN) ==========================
    secure_data = [
        ('PC14\n(TCXO_32K_IN)', 
         '1. RCC_BDCR (+0xF0)\n2. GPIOC_SECCFGR (+0x30)\n3. GPIOC_MODER (+0x00)', 
         '1. 0x56020CF0 (S)\n2. 0x52020830 (S)\n3. 0x52020800 (S)', 
         '1. Bit 2 LSEBYP=1 (Bypass Osc), Bit 0 LSEON=1, Bit 5 LSECSSON=1 (Monitor TCXO)\n2. Bit 14 SEC14 = 1 (Secure World Pad)\n3. Bits [29:28] MODER14 = 11b (Analog Mode saat LSE Bypass)'),

        ('PC15\n(UNUSED_PC15)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_SECCFGR (+0x30)', 
         '1. 0x52020800 (S)\n2. 0x52020830 (S)', 
         '1. Bits [31:30] MODER15 = 11b (Analog Mode / Zero Leakage)\n2. Bit 15 SEC15 = 1 (Secure World Pad)'),

        ('PB2\n(RTC_CAL_OUT)', 
         '1. RTC_CR (+0x18)\n2. GPIOB_AFRL (+0x20)\n3. GPIOB_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007818 (S)\n2. 0x52020420 (S)\n3. 0x52020430 (S)\n4. 0x50032500 (S)', 
         '1. Bit 29 COE=1 (Cal Output Enable), Bit 30 COSEL=0 (1 Hz) / 1 (512 Hz)\n2. Bits [11:8] AFSEL2 = 0000b (AF0 RTC_OUT2)\n3. Bit 2 SEC2 = 1 (Secure Pad)\n4. Bit 18 RTCSEC = 1 (Secure Peripheral)'),

        ('PC13\n(TAMP_CASE_OPEN)', 
         '1. TAMP_CR1 (+0x00)\n2. TAMP_SECCFGR (+0x20)\n3. GPIOC_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007C00 (S)\n2. 0x56007C20 (S)\n3. 0x52020830 (S)\n4. 0x50032500 (S)', 
         '1. Bit 0 TAMP1E = 1 (Enable Tamper 1 Active-Low / Tombol B1)\n2. Bit 0 TAMP1SEC = 1 (Secure Tamper Resource)\n3. Bit 13 SEC13 = 1 (Secure Pad)\n4. Bit 19 TAMPSEC = 1 (Secure Peripheral)'),

        ('PE6\n(TAMP_TERM_COVER)', 
         '1. TAMP_CR1 (+0x00)\n2. TAMP_SECCFGR (+0x20)\n3. GPIOE_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007C00 (S)\n2. 0x56007C20 (S)\n3. 0x52021030 (S)\n4. 0x50032500 (S)', 
         '1. Bit 2 TAMP3E = 1 (Enable Tamper 3 Active-Low)\n2. Bit 2 TAMP3SEC = 1 (Secure Tamper Resource)\n3. Bit 6 SEC6 = 1 (Secure Pad)\n4. Bit 19 TAMPSEC = 1 (Secure Peripheral)'),

        ('PE4\n(SENSOR_MAGNET)', 
         '1. TAMP_CR2 (+0x04)\n2. TAMP_SECCFGR (+0x20)\n3. GPIOE_SECCFGR (+0x30)\n4. GTZC1_TZSC (+0x100)', 
         '1. 0x56007C04 (S)\n2. 0x56007C20 (S)\n3. 0x52021030 (S)\n4. 0x50032500 (S)', 
         '1. Bit 6 TAMP7E = 1 (Enable Tamper 7 External Hall Sensor)\n2. Bit 6 TAMP7SEC = 1 (Secure Tamper Resource)\n3. Bit 4 SEC4 = 1 (Secure Pad)\n4. Bit 19 TAMPSEC = 1 (Secure Peripheral)'),

        ('PC3\n(AC_FAIL_DET)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_SECCFGR (+0x30)\n3. EXTI_EXTICR1 (+0x60)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52020800 (S)\n2. 0x52020830 (S)\n3. 0x56022060 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [7:6] MODER3 = 00b (Input Mode)\n2. Bit 3 SEC3 = 1 (Secure Pad)\n3. Bits [27:24] EXTI3 = 0x02 (Port C Routing)\n4. Bit 3 SEC3 = 1 (Secure Interrupt Vector)\n5. Bit 3 IM3 = 1 (Interrupt Unmasked)\n6. Bit 3 FT3 = 1 (Falling Edge Trigger)'),

        ('PD11\n(VBAT_SENSE)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_SECCFGR (+0x30)\n3. ADC4_CHSELR (+0x28)\n4. ADC4_CR (+0x08)\n5. GTZC2_TZSC (+0x100)', 
         '1. 0x52020C00 (S)\n2. 0x52020C30 (S)\n3. 0x56021028 (S)\n4. 0x56021008 (S)\n5. 0x56033100 (S)', 
         '1. Bits [23:22] MODER11 = 11b (Analog Mode)\n2. Bit 11 SEC11 = 1 (Secure Pad)\n3. Bit 15 CHSEL15 = 1 (Select Channel 15)\n4. Bit 0 ADEN = 1, Bit 2 ADSTART = 1\n5. Bit 0 ADC4SEC = 1 (Secure Peripheral)'),

        ('PC2\n(VCAP_SENSE)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_SECCFGR (+0x30)\n3. ADC4_CHSELR (+0x28)\n4. GTZC2_TZSC (+0x100)', 
         '1. 0x52020800 (S)\n2. 0x52020830 (S)\n3. 0x56021028 (S)\n4. 0x56033100 (S)', 
         '1. Bits [5:4] MODER2 = 11b (Analog Mode)\n2. Bit 2 SEC2 = 1 (Secure Pad)\n3. Bit 3 CHSEL3 = 1 (Select Channel 3)\n4. Bit 0 ADC4SEC = 1 (Secure Peripheral)'),

        ('PD12\n(EXT_WDI_PULSE)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_BSRR (+0x18)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x52020C00 (S)\n2. 0x52020C18 (S)\n3. 0x52020C30 (S)', 
         '1. Bits [25:24] MODER12 = 01b (Output Push-Pull)\n2. Bit 12 BS12 = 1 (Set Pulse), Bit 28 BR12 = 1 (Reset Pulse)\n3. Bit 12 SEC12 = 1 (Secure Pad)'),

        ('PA4\n(ADE_SS)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_OSPEEDR (+0x08)\n3. GPIOA_BSRR (+0x18)\n4. GPIOA_SECCFGR (+0x30)\n5. GTZC1_TZSC (+0x100)', 
         '1. 0x52020000 (S)\n2. 0x52020008 (S)\n3. 0x52020018 (S)\n4. 0x52020030 (S)\n5. 0x50032500 (S)', 
         '1. Bits [9:8] MODER4 = 01b (Output Push-Pull)\n2. Bits [9:8] OSPEED4 = 11b (Very High Speed)\n3. Bit 4 BS4 = 1 (High/CS Non-Active), Bit 20 BR4 = 1 (Low/CS Active)\n4. Bit 4 SEC4 = 1 (Secure Pad)\n5. Bit 0 SPI1SEC = 1 (Secure Peripheral)'),

        ('PA5\n(ADE_SCK)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. SPI1_CR1 (+0x00)\n5. SPI1_CFG2 (+0x0C)', 
         '1. 0x52020000 (S)\n2. 0x52020020 (S)\n3. 0x52020030 (S)\n4. 0x50013000 (S)\n5. 0x5001300C (S)', 
         '1. Bits [11:10] MODER5 = 10b (Alternate Function)\n2. Bits [23:20] AFSEL5 = 0101b (AF5 SPI1_SCK)\n3. Bit 5 SEC5 = 1 (Secure Pad)\n4. Bit 0 SPE = 1 (SPI Enable)\n5. Bit 22 MASTER = 1 (Master Mode)'),

        ('PA6\n(ADE_MISO)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. SPI1_RXDR (+0x30)', 
         '1. 0x52020000 (S)\n2. 0x52020020 (S)\n3. 0x52020030 (S)\n4. 0x50013030 (S)', 
         '1. Bits [13:12] MODER6 = 10b (Alternate Function)\n2. Bits [27:24] AFSEL6 = 0101b (AF5 SPI1_MISO)\n3. Bit 6 SEC6 = 1 (Secure Pad)\n4. Bits [31:0] RXDATA (Read 32-bit register)'),

        ('PA7\n(ADE_MOSI)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. SPI1_TXDR (+0x20)', 
         '1. 0x52020000 (S)\n2. 0x52020020 (S)\n3. 0x52020030 (S)\n4. 0x50013020 (S)', 
         '1. Bits [15:14] MODER7 = 10b (Alternate Function)\n2. Bits [31:28] AFSEL7 = 0101b (AF5 SPI1_MOSI)\n3. Bit 7 SEC7 = 1 (Secure Pad)\n4. Bits [31:0] TXDATA (Write 32-bit register)'),

        ('PB5\n(ADE_IRQ0)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_SECCFGR (+0x30)\n3. EXTI_EXTICR2 (+0x64)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52020400 (S)\n2. 0x52020430 (S)\n3. 0x56022064 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [11:10] MODER5 = 00b (Input Mode)\n2. Bit 5 SEC5 = 1 (Secure Pad)\n3. Bits [11:8] EXTI5 = 0x01 (Port B Routing)\n4. Bit 5 SEC5 = 1 (Secure Interrupt Vector)\n5. Bit 5 IM5 = 1 (Interrupt Unmasked)\n6. Bit 5 FT5 = 1 (Falling Edge Trigger)'),

        ('PB1\n(ADE_IRQ1)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_SECCFGR (+0x30)\n3. EXTI_EXTICR1 (+0x60)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52020400 (S)\n2. 0x52020430 (S)\n3. 0x56022060 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [3:2] MODER1 = 00b (Input Mode)\n2. Bit 1 SEC1 = 1 (Secure Pad)\n3. Bits [11:8] EXTI1 = 0x01 (Port B Routing)\n4. Bit 1 SEC1 = 1 (Secure Interrupt Vector)\n5. Bit 1 IM1 = 1 (Interrupt Unmasked)\n6. Bit 1 FT1 = 1 (Falling Edge Trigger)'),

        ('PE11\n(ADE_EVENT)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_SECCFGR (+0x30)\n3. EXTI_EXTICR3 (+0x68)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x52021030 (S)\n3. 0x56022068 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022004 (S)', 
         '1. Bits [23:22] MODER11 = 00b (Input Mode)\n2. Bit 11 SEC11 = 1 (Secure Pad)\n3. Bits [27:24] EXTI11 = 0x04 (Port E Routing)\n4. Bit 11 SEC11 = 1 (Secure Interrupt Vector)\n5. Bit 11 IM11 = 1 (Interrupt Unmasked)\n6. Bit 11 FT11 = 1 (Falling Edge Trigger)\n7. ADE9000 CFMODE[13:12] = 10b (Mode EVENT Sag/Swell/OI, Pengganti DREADY)'),

        ('PE7\n(ADE_RESET)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_ODR (+0x14)\n3. GPIOE_SECCFGR (+0x30)', 
         '1. 0x52021000 (S)\n2. 0x52021014 (S)\n3. 0x52021030 (S)', 
         '1. Bits [15:14] MODER7 = 01b (Output Push-Pull)\n2. Bit 7 ODR7 = 1 (Run Mode) / 0 (Hardware Reset)\n3. Bit 7 SEC7 = 1 (Secure Pad)'),

        ('PE8\n(ADE_CF3_ZX)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_SECCFGR (+0x30)\n3. EXTI_EXTICR3 (+0x68)\n4. EXTI_SECCFGR1 (+0x14)\n5. EXTI_IMR1 (+0x80)\n6. EXTI_RTSR1 (+0x00) & FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x52021030 (S)\n3. 0x56022068 (S)\n4. 0x56022014 (S)\n5. 0x56022080 (S)\n6. 0x56022000 (S) & 0x56022004 (S)', 
         '1. Bits [17:16] MODER8 = 00b (Input Digital)\n2. Bit 8 SEC8 = 1 (Secure Pad)\n3. Bits [3:0] EXTI8 = 0x04 (Port E Routing)\n4. Bit 8 SEC8 = 1 (Secure Interrupt Vector)\n5. Bit 8 IM8 = 1 (Interrupt Unmasked)\n6. Bit 8 RT8 = 1 / FT8 = 1 (Zero-Crossing Dual-Edge Trigger)\n7. Fungsi Ganda: Pulsa Energi Semu kVAh (CF3) / AC Zero-Crossing ZCS Shunt Trip PC8'),

        ('PE9\n(ADE_PM0)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_ODR (+0x14)\n3. GPIOE_SECCFGR (+0x30)', 
         '1. 0x52021000 (S)\n2. 0x52021014 (S)\n3. 0x52021030 (S)', 
         '1. Bits [19:18] MODER9 = 01b (Output Push-Pull)\n2. Bit 9 ODR9 = 0 (Run PSM0) / 1 (Sleep PSM3)\n3. Bit 9 SEC9 = 1 (Secure Pad)'),

        ('PE10\n(ADE_PM1)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_ODR (+0x14)\n3. GPIOE_SECCFGR (+0x30)', 
         '1. 0x52021000 (S)\n2. 0x52021014 (S)\n3. 0x52021030 (S)', 
         '1. Bits [21:20] MODER10 = 01b (Output Push-Pull)\n2. Bit 10 ODR10 (Kendali mode daya ADE saat padam)\n3. Bit 10 SEC10 = 1 (Secure Pad)'),

        ('PE12\n(ADE_CF1)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_PUPDR (+0x0C)\n3. GPIOE_SECCFGR (+0x30)\n4. EXTI_EXTICR4 (+0x6C)\n5. EXTI_SECCFGR1 (+0x14)\n6. EXTI_IMR1 (+0x80)\n7. EXTI_FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x5202100C (S)\n3. 0x52021030 (S)\n4. 0x5602206C (S)\n5. 0x56022014 (S)\n6. 0x56022080 (S)\n7. 0x56022004 (S)', 
         '1. Bits [25:24] MODER12 = 00b (Input Digital)\n2. Bits [25:24] PUPDR12 = 01b (Pull-Up)\n3. Bit 12 SEC12 = 1 (Secure Pad)\n4. Bits [3:0] EXTI12 = 0x04 (Port E Routing)\n5. Bit 12 SEC12 = 1 (Secure Interrupt Vector)\n6. Bit 12 IM12 = 1 (Interrupt Unmasked)\n7. Bit 12 FT12 = 1 (Falling Edge Pulse Trigger)\n8. CF1 Output: 1.000 imp/kWh (80 ms) paralel ke LED PB4 & Register Wh'),

        ('PE13\n(ADE_CF2)', 
         '1. GPIOE_MODER (+0x00)\n2. GPIOE_PUPDR (+0x0C)\n3. GPIOE_SECCFGR (+0x30)\n4. EXTI_EXTICR4 (+0x6C)\n5. EXTI_SECCFGR1 (+0x14)\n6. EXTI_IMR1 (+0x80)\n7. EXTI_FTSR1 (+0x04)', 
         '1. 0x52021000 (S)\n2. 0x5202100C (S)\n3. 0x52021030 (S)\n4. 0x5602206C (S)\n5. 0x56022014 (S)\n6. 0x56022080 (S)\n7. 0x56022004 (S)', 
         '1. Bits [27:26] MODER13 = 00b (Input Digital)\n2. Bits [27:26] PUPDR13 = 01b (Pull-Up)\n3. Bit 13 SEC13 = 1 (Secure Pad)\n4. Bits [7:4] EXTI13 = 0x04 (Port E Routing)\n5. Bit 13 SEC13 = 1 (Secure Interrupt Vector)\n6. Bit 13 IM13 = 1 (Interrupt Unmasked)\n7. Bit 13 FT13 = 1 (Falling Edge Pulse Trigger)\n8. CF2 Output: 1.000 imp/kvarh (80 ms) paralel ke LED PB5 & Register varh')
    ]
    render_four_col_table('1. Tabel Register Hardware: SECURE WORLD (23 Pin)', secure_data, '990000')

    # ========================== 2. NON-SECURE WORLD (25 PIN) ==========================
    nonsecure_data = [
        ('PH0\n(OSC_IN)', 
         '1. RCC_CR (+0x00)\n2. GPIOH_MODER (+0x00)\n3. GPIOH_SECCFGR (+0x30)', 
         '1. 0x46020C00 (NS)\n2. 0x42021C00 (NS)\n3. 0x42021C30 (NS)', 
         '1. Bit 16 HSEON = 1, Bit 17 HSERDY = 1 (HSE Active 16 MHz)\n2. Bits [1:0] MODER0 = 11b (Analog Mode)\n3. Bit 0 SEC0 = 0 (Non-Secure Pad)'),

        ('PH1\n(OSC_OUT)', 
         '1. GPIOH_MODER (+0x00)\n2. GPIOH_SECCFGR (+0x30)', 
         '1. 0x42021C00 (NS)\n2. 0x42021C30 (NS)', 
         '1. Bits [3:2] MODER1 = 11b (Analog Mode)\n2. Bit 1 SEC1 = 0 (Non-Secure Pad)'),

        ('PC0\n(LCD_I2C3_SCL)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_OTYPER (+0x04)\n3. GPIOC_AFRL (+0x20)\n4. GPIOC_SECCFGR (+0x30)\n5. I2C3_TIMINGR (+0x10)', 
         '1. 0x42020800 (NS)\n2. 0x42020804 (NS)\n3. 0x42020820 (NS)\n4. 0x42020830 (NS)\n5. 0x46002810 (NS)', 
         '1. Bits [1:0] MODER0 = 10b (Alternate Function)\n2. Bit 0 OT0 = 1 (Open-Drain)\n3. Bits [3:0] AFSEL0 = 0100b (AF4 I2C3_SCL)\n4. Bit 0 SEC0 = 0 (Non-Secure Pad)\n5. 0x3042C7C7 (Standard Mode 100 kHz)'),

        ('PC1\n(LCD_I2C3_SDA)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_OTYPER (+0x04)\n3. GPIOC_AFRL (+0x20)\n4. GPIOC_SECCFGR (+0x30)\n5. I2C3_CR1 (+0x00)', 
         '1. 0x42020800 (NS)\n2. 0x42020804 (NS)\n3. 0x42020820 (NS)\n4. 0x42020830 (NS)\n5. 0x46002800 (NS)', 
         '1. Bits [3:2] MODER1 = 10b (Alternate Function)\n2. Bit 1 OT1 = 1 (Open-Drain)\n3. Bits [7:4] AFSEL1 = 0100b (AF4 I2C3_SDA)\n4. Bit 1 SEC1 = 0 (Non-Secure Pad)\n5. Bit 0 PE = 1 (Peripheral Enable)'),

        ('PD2\n(LCD_PWR_EN)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [5:4] MODER2 = 01b (Output Push-Pull)\n2. Bit 2 ODR2 = 1 (Power LCD ON), ODR2 = 0 (OFF)\n3. Bit 2 SEC2 = 0 (Non-Secure Pad)'),

        ('PD3\n(LCD_BL_CTRL)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [7:6] MODER3 = 01b (Output Push-Pull)\n2. Bit 3 ODR3 = 1 (Backlight ON), ODR3 = 0 (OFF)\n3. Bit 3 SEC3 = 0 (Non-Secure Pad)'),

        ('PA0\n(BTN_SCROLL_UP)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_PUPDR (+0x0C)\n3. GPIOA_SECCFGR (+0x30)\n4. EXTI_EXTICR1 (+0x60)\n5. EXTI_IMR1 (+0x80)\n6. PWR_WUCR1 (+0x14)\n7. PWR_WUCR2 (+0x18)\n8. PWR_WUCR3 (+0x1C)', 
         '1. 0x42020000 (NS)\n2. 0x4202000C (NS)\n3. 0x42020030 (NS)\n4. 0x46022060 (NS)\n5. 0x46022080 (NS)\n6. 0x46020814 (NS)\n7. 0x46020818 (NS)\n8. 0x4602081C (NS)', 
         '1. Bits [1:0] MODER0 = 00b (Input Mode)\n2. Bits [1:0] PUPDR0 = 01b (Pull-Up)\n3. Bit 0 SEC0 = 0 (Non-Secure Pad)\n4. Bits [3:0] EXTI0 = 0x00 (Port A Line 0), IMR1[0]=1\n5. Bit 0 WUPEN1 = 1 (WKUP1 Enable)\n6. Bit 0 WUPP1 = 1 (Falling Edge Low Trigger)\n7. Bits [1:0] WUSEL1 = 00b (Source 0: PA0)'),

        ('PB6\n(BTN_SCROLL_DN)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_PUPDR (+0x0C)\n3. GPIOB_SECCFGR (+0x30)\n4. EXTI_EXTICR2 (+0x64)\n5. EXTI_IMR1 (+0x80)\n6. PWR_WUCR1 (+0x14)\n7. PWR_WUCR2 (+0x18)\n8. PWR_WUCR3 (+0x1C)', 
         '1. 0x42020400 (NS)\n2. 0x4202040C (NS)\n3. 0x42020430 (NS)\n4. 0x46022064 (NS)\n5. 0x46022080 (NS)\n6. 0x46020814 (NS)\n7. 0x46020818 (NS)\n8. 0x4602081C (NS)', 
         '1. Bits [13:12] MODER6 = 00b (Input Mode)\n2. Bits [13:12] PUPDR6 = 01b (Pull-Up)\n3. Bit 6 SEC6 = 0 (Non-Secure Pad)\n4. Bits [19:16] EXTI6 = 0x01 (Port B Line 6), IMR1[6]=1\n5. Bit 2 WUPEN3 = 1 (WKUP3 Enable, DS13086 Hal 129)\n6. Bit 2 WUPP3 = 1 (Falling Edge Low Trigger)\n7. Bits [5:4] WUSEL3 = 10b (Source 2: PB6)'),

        ('PB7\n(LED_ALM)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_BSRR (+0x18)\n3. GPIOB_SECCFGR (+0x30)', 
         '1. 0x42020400 (NS)\n2. 0x42020418 (NS)\n3. 0x42020430 (NS)', 
         '1. Bits [15:14] MODER7 = 01b (Output Push-Pull)\n2. Bit 7 BS7 = 1 (Set LED ON), Bit 23 BR7 = 1 (Reset LED OFF)\n3. Bit 7 SEC7 = 0 (Non-Secure Pad)'),

        ('PA2\n(MODEM_TX)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRL (+0x20)\n3. GPIOA_SECCFGR (+0x30)\n4. RCC_CCIPR1 (+0xE0)\n5. LPUART1_PRESC (+0x1C)\n6. LPUART1_BRR (+0x0C)\n7. LPUART1_CR1 (+0x00)\n8. LPUART1_TDR (+0x28)', 
         '1. 0x42020000 (NS)\n2. 0x42020020 (NS)\n3. 0x42020030 (NS)\n4. 0x46020CE0 (NS)\n5. 0x4600241C (NS)\n6. 0x4600240C (NS)\n7. 0x46002400 (NS)\n8. 0x46002428 (NS)', 
         '1. Bits [5:4] MODER2 = 10b (AF Mode)\n2. Bits [11:8] AFSEL2 = 1000b (AF8 LPUART1)\n3. Bit 2 SEC2 = 0 (Non-Secure Pad)\n4. Bits [11:10] LPUART1SEL = 10b (Pilih HSI16 16 MHz)\n5. Bits [3:0] PRESCALER = 0000b (/1 divider)\n6. BRR = (256×16MHz)/19200 = 0x0003_4155\n7. Bit 0 UE=1, Bit 3 TE=1, Bit 29 FIFOEN=1\n8. Bits [8:0] TDR (Transmit Data Register)'),

        ('PA3\n(MODEM_RX)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_PUPDR (+0x0C)\n3. GPIOA_AFRL (+0x20)\n4. GPIOA_SECCFGR (+0x30)\n5. LPUART1_CR1 (+0x00)\n6. LPUART1_CR3 (+0x08)\n7. LPUART1_RDR (+0x24)', 
         '1. 0x42020000 (NS)\n2. 0x4202000C (NS)\n3. 0x42020020 (NS)\n4. 0x42020030 (NS)\n5. 0x46002400 (NS)\n6. 0x46002408 (NS)\n7. 0x46002424 (NS)', 
         '1. Bits [7:6] MODER3 = 10b (AF Mode)\n2. Bits [7:6] PUPDR3 = 01b (Pull-Up)\n3. Bits [15:12] AFSEL3 = 1000b (AF8 LPUART1)\n4. Bit 3 SEC3 = 0 (Non-Secure Pad)\n5. Bit 2 RE = 1 (Receiver Enable)\n6. Bit 0 DMAR = 1 (DMA Rx Ring Buffer)\n7. Bits [8:0] RDR (Receive Data Register)'),

        ('PA9\n(CONSOLE_TX_VCP)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_AFRH (+0x24)\n3. GPIOA_SECCFGR (+0x30)\n4. USART1_CR1 (+0x00)\n5. USART1_BRR (+0x0C)\n6. USART1_TDR (+0x28)', 
         '1. 0x42020000 (NS)\n2. 0x42020024 (NS)\n3. 0x42020030 (NS)\n4. 0x40013800 (NS)\n5. 0x4001380C (NS)\n6. 0x40013828 (NS)', 
         '1. Bits [19:18] MODER9 = 10b (AF Mode)\n2. Bits [7:4] AFSEL9 = 0111b (AF7 USART1_TX)\n3. Bit 9 SEC9 = 0 (Non-Secure Pad)\n4. Bit 0 UE = 1, Bit 3 TE = 1\n5. BRR = 160MHz/115200 = 1389 = 0x056D (ST-LINK VCP)\n6. Bits [8:0] TDR (Transmit Konsol printf)'),

        ('PA10\n(CONSOLE_RX_VCP)', 
         '1. GPIOA_MODER (+0x00)\n2. GPIOA_PUPDR (+0x0C)\n3. GPIOA_AFRH (+0x24)\n4. GPIOA_SECCFGR (+0x30)\n5. USART1_CR1 (+0x00)\n6. USART1_RDR (+0x24)', 
         '1. 0x42020000 (NS)\n2. 0x4202000C (NS)\n3. 0x42020024 (NS)\n4. 0x42020030 (NS)\n5. 0x40013800 (NS)\n6. 0x40013824 (NS)', 
         '1. Bits [21:20] MODER10 = 10b (AF Mode)\n2. Bits [21:20] PUPDR10 = 01b (Pull-Up)\n3. Bits [11:8] AFSEL10 = 0111b (AF7 USART1_RX)\n4. Bit 10 SEC10 = 0 (Non-Secure Pad)\n5. Bit 2 RE = 1, Bit 5 RXNEIE = 1\n6. Bits [8:0] RDR (Receive Konsol Karakter Interaktif)'),

        ('PD4\n(RS232_PWR_EN)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [9:8] MODER4 = 01b (Output Push-Pull)\n2. Bit 4 ODR4 = 1 (Power ON) / 0 (FORCEOFF MAX3232)\n3. Bit 4 SEC4 = 0 (Non-Secure Pad)'),

        ('PD5\n(MODEM_12V_EN)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [11:10] MODER5 = 01b (Output Push-Pull)\n2. Bit 5 ODR5 = 1 (12V ON) / 0 (Putus Catu 12V)\n3. Bit 5 SEC5 = 0 (Non-Secure Pad)'),

        ('PC4\n(OPTICAL_TX)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_AFRL (+0x20)\n3. GPIOC_SECCFGR (+0x30)\n4. USART3_CR1 (+0x00)\n5. USART3_BRR (+0x0C)\n6. USART3_TDR (+0x28)', 
         '1. 0x42020800 (NS)\n2. 0x42020820 (NS)\n3. 0x42020830 (NS)\n4. 0x40004800 (NS)\n5. 0x4000480C (NS)\n6. 0x40004828 (NS)', 
         '1. Bits [9:8] MODER4 = 10b (AF Mode)\n2. Bits [19:16] AFSEL4 = 0111b (AF7 USART3_TX)\n3. Bit 4 SEC4 = 0 (Non-Secure Pad)\n4. Bit 0 UE = 1, Bit 3 TE = 1\n5. BRR = 160MHz/9600 = 16.667 = 0x411B\n6. Bits [8:0] TDR (Transmit Data Register)'),

        ('PC5\n(OPTICAL_RX)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_AFRL (+0x20)\n4. GPIOC_SECCFGR (+0x30)\n5. USART3_CR1 (+0x00)\n6. USART3_RDR (+0x24)\n7. PWR_WUCR1 (+0x14)\n8. PWR_WUCR2 (+0x18)\n9. PWR_WUCR3 (+0x1C)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020820 (NS)\n4. 0x42020830 (NS)\n5. 0x40004800 (NS)\n6. 0x40004824 (NS)\n7. 0x46020814 (NS)\n8. 0x46020818 (NS)\n9. 0x4602081C (NS)', 
         '1. Bits [11:10] MODER5 = 10b (AF Mode)\n2. Bits [11:10] PUPDR5 = 01b (Pull-Up)\n3. Bits [23:20] AFSEL5 = 0111b (AF7 USART3_RX)\n4. Bit 5 SEC5 = 0 (Non-Secure Pad)\n5. Bit 2 RE = 1\n6. Bits [8:0] RDR (Receive Data Register)\n7. Bit 4 WUPEN5 = 1 (WKUP5 Enable, DS13086 Hal 114)\n8. Bit 4 WUPP5 = 1 (Falling Edge Trigger)\n9. Bits [9:8] WUSEL5 = 00b (Source PC5)'),

        ('PC8\n(CTRL_RELAY_TRIP)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_BSRR (+0x18)\n4. GPIOC_SECCFGR (+0x30)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020818 (NS)\n4. 0x42020830 (NS)', 
         '1. Bits [17:16] MODER8 = 01b (Output Mode)\n2. Bits [17:16] PUPDR8 = 10b (Pull-Down)\n3. Bit 8 BS8 = 1 (Koil Shunt Trip Aktif: Buka MCCB)\n4. Bit 8 SEC8 = 0 (Non-Secure Pad)'),

        ('PC9\n(SPARE_GPIO)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_BSRR (+0x18)\n4. GPIOC_SECCFGR (+0x30)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020818 (NS)\n4. 0x42020830 (NS)', 
         '1. Bits [19:18] MODER9 = 01b (Output Mode)\n2. Bits [19:18] PUPDR9 = 10b (Pull-Down)\n3. Bit 9 BR9 = 1 (Tarik Low / Idle Safe State)\n4. Bit 9 SEC9 = 0 (Non-Secure Pad)'),

        ('PC7\n(RELAY_STATUS)', 
         '1. GPIOC_MODER (+0x00)\n2. GPIOC_PUPDR (+0x0C)\n3. GPIOC_IDR (+0x10)\n4. GPIOC_SECCFGR (+0x30)\n5. EXTI_EXTICR2 (+0x64)', 
         '1. 0x42020800 (NS)\n2. 0x4202080C (NS)\n3. 0x42020810 (NS)\n4. 0x42020830 (NS)\n5. 0x46022064 (NS)', 
         '1. Bits [15:14] MODER7 = 00b (Input Mode)\n2. Bits [15:14] PUPDR7 = 10b (Pull-Down)\n3. Bit 7 IDR7 (Auxiliary Feedback Status: 1=Closed, 0=Open)\n4. Bit 7 SEC7 = 0 (Non-Secure Pad)\n5. Bits [27:24] EXTI7 = 0x02 (Port C Line 7)'),

        ('PD8\n(AUX_CTRL_OUT1)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_ODR (+0x14)\n3. GPIOD_SECCFGR (+0x30)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C14 (NS)\n3. 0x42020C30 (NS)', 
         '1. Bits [17:16] MODER8 = 01b (Output Push-Pull)\n2. Bit 8 ODR8 (Driver Relai Bantu 1 Opto-SSR)\n3. Bit 8 SEC8 = 0 (Non-Secure Pad)'),

        ('PD10\n(AUX_CTRL_IN1)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_PUPDR (+0x0C)\n3. GPIOD_IDR (+0x10)\n4. GPIOD_SECCFGR (+0x30)\n5. EXTI_EXTICR3 (+0x68)\n6. EXTI_IMR1 (+0x80)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C0C (NS)\n3. 0x42020C10 (NS)\n4. 0x42020C30 (NS)\n5. 0x46022068 (NS)\n6. 0x46022080 (NS)', 
         '1. Bits [21:20] MODER10 = 00b (Input Mode)\n2. Bits [21:20] PUPDR10 = 01b (Pull-Up)\n3. Bit 10 IDR10 (Status Input Digital 1)\n4. Bit 10 SEC10 = 0 (Non-Secure Pad)\n5. Bits [19:16] EXTI10 = 0x03 (Port D Line 10)\n6. Bit 10 IM10 = 1 (Interrupt Unmasked)'),

        ('PD14\n(AUX_CTRL_IN2)', 
         '1. GPIOD_MODER (+0x00)\n2. GPIOD_PUPDR (+0x0C)\n3. GPIOD_IDR (+0x10)\n4. GPIOD_SECCFGR (+0x30)\n5. EXTI_EXTICR4 (+0x6C)\n6. EXTI_IMR1 (+0x80)', 
         '1. 0x42020C00 (NS)\n2. 0x42020C0C (NS)\n3. 0x42020C10 (NS)\n4. 0x42020C30 (NS)\n5. 0x4602206C (NS)\n6. 0x46022080 (NS)', 
         '1. Bits [29:28] MODER14 = 00b (Input Mode)\n2. Bits [29:28] PUPDR14 = 01b (Pull-Up)\n3. Bit 14 IDR14 (Status Input Digital 2)\n4. Bit 14 SEC14 = 0 (Non-Secure Pad)\n5. Bits [19:16] EXTI14 = 0x03 (Port D Line 14)\n6. Bit 14 IM14 = 1 (Interrupt Unmasked)'),

        ('PB12\n(FLASH_SPI2_CS)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_OSPEEDR (+0x08)\n3. GPIOB_BSRR (+0x18)\n4. GPIOB_SECCFGR (+0x30)\n5. GTZC1_TZSC (+0x100)', 
         '1. 0x42020400 (NS)\n2. 0x42020408 (NS)\n3. 0x42020418 (NS)\n4. 0x42020430 (NS)\n5. 0x40032500 (NS)', 
         '1. Bits [25:24] MODER12 = 01b (Output Push-Pull)\n2. Bits [25:24] OSPEED12 = 11b (Very High Speed)\n3. Bit 12 BS12 = 1 (CS High), Bit 28 BR12 = 1 (CS Low)\n4. Bit 12 SEC12 = 0 (Non-Secure Pad)\n5. Bit 1 SPI2SEC = 0 (Non-Secure Peripheral)'),

        ('PB13\n(FLASH_SPI2_SCK)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_AFRH (+0x24)\n3. GPIOB_SECCFGR (+0x30)\n4. SPI2_CR1 (+0x00)\n5. SPI2_CFG2 (+0x0C)', 
         '1. 0x42020400 (NS)\n2. 0x42020424 (NS)\n3. 0x42020430 (NS)\n4. 0x40003800 (NS)\n5. 0x4000380C (NS)', 
         '1. Bits [27:26] MODER13 = 10b (Alternate Function)\n2. Bits [23:20] AFSEL13 = 0101b (AF5 SPI2_SCK)\n3. Bit 13 SEC13 = 0 (Non-Secure Pad)\n4. Bit 0 SPE = 1 (SPI Enable)\n5. Bit 22 MASTER = 1 (Master Mode)'),

        ('PB14\n(FLASH_SPI2_MISO)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_AFRH (+0x24)\n3. GPIOB_SECCFGR (+0x30)\n4. SPI2_RXDR (+0x30)', 
         '1. 0x42020400 (NS)\n2. 0x42020424 (NS)\n3. 0x42020430 (NS)\n4. 0x40003830 (NS)', 
         '1. Bits [29:28] MODER14 = 10b (Alternate Function)\n2. Bits [27:24] AFSEL14 = 0101b (AF5 SPI2_MISO)\n3. Bit 14 SEC14 = 0 (Non-Secure Pad)\n4. Bits [31:0] RXDATA (Read Flash Data)'),

        ('PB15\n(FLASH_SPI2_MOSI)', 
         '1. GPIOB_MODER (+0x00)\n2. GPIOB_AFRH (+0x24)\n3. GPIOB_SECCFGR (+0x30)\n4. SPI2_TXDR (+0x20)', 
         '1. 0x42020400 (NS)\n2. 0x42020424 (NS)\n3. 0x42020430 (NS)\n4. 0x40003820 (NS)', 
         '1. Bits [31:30] MODER15 = 10b (Alternate Function)\n2. Bits [31:28] AFSEL15 = 0101b (AF5 SPI2_MOSI)\n3. Bit 15 SEC15 = 0 (Non-Secure Pad)\n4. Bits [31:0] TXDATA (Write Flash Data)')
    ]
    render_four_col_table('2. Tabel Register Hardware: NON-SECURE WORLD (27 Pin)', nonsecure_data, '005599')

    # ========================== 3. SYSTEM & SHARED PINS (5 PIN) ==========================
    system_data = [
        ('NRST\n(RESET_SYSTEM)', 
         '1. RCC_CSR (+0xF4)', 
         '1. 0x56020CF4 (S)\n   0x46020CF4 (NS)', 
         '1. Bit 26 PINRSTF = 1 (NRST external pin reset flag)\n   Bit 23 RMVF = 1 (Remove reset flags / clear status)'),

        ('VBAT\n(VBAT_IN)', 
         '1. PWR_BDCR1 (+0x20)\n2. RCC_BDCR (+0xF0)', 
         '1. 0x56020820 (S)\n2. 0x56020CF0 (S)', 
         '1. Bit 0 MONEN = 1 (Backup domain voltage monitor enable), Bit 1 VBATF\n2. Bit 15 RTCEN = 1, Bit 0 LSEON = 1'),

        ('PA13\n(SWDIO)', 
         '1. GPIOA_AFRH (+0x24)', 
         '1. 0x52020024 (S)', 
         '1. Bits [23:20] AFSEL13 = 0000b (AF0 SWDIO Debug Interface)'),

        ('PA14\n(SWCLK)', 
         '1. GPIOA_AFRH (+0x24)', 
         '1. 0x52020024 (S)', 
         '1. Bits [27:24] AFSEL14 = 0000b (AF0 SWCLK Debug Interface)'),

        ('Rail DC\n(LED_POWER_ON)', 
         '1. Hardwired Analog HW', 
         '1. N/A (Analog Power Hardware)', 
         '1. Jalur catu fasa hardware non-MCU (Disuplai langsung dari regulator power supply board)')
    ]
    render_four_col_table('3. Tabel Register Hardware: SISTEM & SINYAL BERSAMA (5 Pin)', system_data, '555555')

    # ========================== 4. INTERNAL SYSTEM & FIREWALL (NON-PIN) ==========================
    internal_data = [
        ('FLASH_OPTR\n(Option Register)', 
         '1. FLASH_OPTR (+0x40)', 
         '1. 0x40022040 (NS)\n   0x50022040 (S)', 
         '1. Bit 31 TZEN = 1 (Enable TrustZone Perangkat Keras)\n   Bits [7:0] RDP = 0xCC (Readout Protection Level 2 / JTAG Permanently Locked)\n   Bits [10:9] BOR_LEV = 00b (Brownout Reset Level 0: 1,71 V Threshold)'),

        ('FLASH_SECBOOTADD0R\n(Boot Address)', 
         '1. FLASH_SECBOOTADD0R (+0x90)', 
         '1. 0x50022090 (S)', 
         '1. Bits [31:7] SECBOOTADD0 = 0x0C000000 >> 7 (Vektor Bootloader Secure Flash Bank 1)'),

        ('FLASH_SECWM1R1..R2\n(Flash Bank 1 Watermark)', 
         '1. FLASH_SECWM1R1 (+0x80)\n2. FLASH_SECWM1R2 (+0x84)', 
         '1. 0x50022080 (S)\n2. 0x50022084 (S)', 
         '1. Bits [6:0] SECWM1_PSTRT = 0 (Page Mulai: 0)\n2. Bits [22:16] SECWM1_PEND = 63 (Page Akhir: 63, Total 128 KB Flash Secure)'),

        ('GTZC1_MPCBB1\n(Firewall SRAM1 192KB)', 
         '1. GTZC1_MPCBB1_CR (+0x00)\n2. GTZC1_MPCBB1_SECCFGR0..31 (+0x100)', 
         '1. 0x50032800 (S)\n2. 0x50032900 s.d.\n   0x5003297C (S)', 
         '1. Bit 0 SRWILADIS = 1 (Memicu Security Fault saat akses ilegal)\n2. Blok 0..31 = 1 (Secure: Buffer ADE9000 & Kunci AES)\n   Blok 32..383 = 0 (Non-Secure: Heap FreeRTOS & Stack DLMS)'),

        ('GTZC1_MPCBB3\n(Firewall SRAM3 512KB)', 
         '1. GTZC1_MPCBB3_CR (+0x00)\n2. GTZC1_MPCBB3_SECCFGR0..31 (+0x300)', 
         '1. 0x50032800 (S)\n2. 0x50032B00 s.d.\n   0x50032B7C (S)', 
         '1. SRWILADIS = 1, INVSECSTATE = 0\n2. Seluruh array SECCFGR = 0x00000000 (Seluruh 512 KB dibuka Non-Secure untuk Heap FreeRTOS & Load Profile)'),

        ('NVIC->ITNS\n(Interrupt Targeting)', 
         '1. NVIC->ITNS[0..15]', 
         '1. 0xE000E380 s.d.\n   0xE000E3BC (S)', 
         '1. ITNS bit = 1 untuk 11 Vektor Non-Secure: LPUART1, USART1, USART3, I2C1, I2C3, GPDMA1_Ch0, PVD, EXTI0, EXTI6, RTC, TAMP\n   ITNS bit = 0 untuk Vektor Secure: EXTI3 (ADE IRQ), EXTI1, EXTI5, EXTI11, TAMP_SEC'),

        ('SCB_NS->VTOR\n(Vektor Non-Secure)', 
         '1. SCB_NS->VTOR', 
         '1. 0xE000ED08 (NS)', 
         '1. VTOR = 0x08100000 (Alamat Vektor Tabel Non-Secure Flash Bank 2 sebelum Secure World melompat via funcptr_NS NonSecure_ResetHandler)'),

        ('SAU Controller\n(NSC Veneer Gateway)', 
         '1. SAU->CTRL\n2. SAU->RBAR / RLAR', 
         '1. 0xE000EDD0 (S)\n2. 0xE000EDD8 s.d.\n   0xE000EDDC (S)', 
         '1. Bit 0 ENABLE = 1, Bit 1 ALLNS = 0\n2. Region 0: Start = 0x0C0FE000, Limit = 0x0C0FFFFF (Area NSC 32 KB untuk instruksi Secure Gateway SG)'),

        ('GPDMA1 Controller\n(DMA Security Channels)', 
         '1. GPDMA1_SECCFGR (+0x00)\n2. GPDMA1_PRIVCFGR (+0x04)', 
         '1. 0x50020000 (S)\n2. 0x50020004 (S)', 
         '1. SEC0 = 0 (Channel 0 Non-Secure: RX Modem Buffer)\n   SEC1 = 1 (Channel 1 Secure: RX SPI1 ADE9000 Metering)\n2. PRIV0 = 0 (Unprivileged task access)'),

        ('TAMP Backup & Auto-Erase\n(Brankas Sabotase Fisik)', 
         '1. TAMP_BKP0R..BKP31R (+0x30)\n2. TAMP_ERCFGR (+0x24)', 
         '1. 0x56007C30 s.d.\n   0x56007CAC (S)\n2. 0x56007C24 (S)', 
         '1. 32 Register 32-bit (128 Byte): Kunci Master DLMS GUEK & Akumulasi kWh Terakhir di domain baterai VSW\n2. Bit 0 ERCFG0 = 1 (Auto-Erase seluruh kunci & BKPxR dalam 1 siklus clock saat PC13 case open terpicu)'),

        ('SAES Hardware Engine\n(DLMS/COSEM Accelerator)', 
         '1. SAES_CR (+0x00)\n2. SAES_KEYR0..3 (+0x04)', 
         '1. 0x50060400 (S)\n2. 0x50060404 s.d.\n   0x50060410 (S)', 
         '1. Bit 0 EN = 1, MODE: 00b (Enkripsi) / 01b (Dekripsi), CHMOD = 011b (Mode GCM Galois/Counter)\n2. 128-bit Key Register dimuat langsung dari hardware secret key'),

        ('RNG Hardware Engine\n(True Random Generator)', 
         '1. RNG_CR (+0x00)\n2. RNG_DR (+0x08)', 
         '1. 0x50060800 (S)\n2. 0x50060808 (S)', 
         '1. Bit 2 RNGEN = 1 (TRNG Enable)\n2. Bits [31:0] RNDATA (Nilai acak entropi fisik termal silikon untuk Challenge DLMS Authentication)'),

        ('PWR Low-Power & PVD\n(Manajemen Daya Pemadaman)', 
         '1. PWR_CR1 (+0x00)\n2. PWR_SVMCR (+0x10)\n3. PWR_BDCR2 (+0x24)', 
         '1. 0x46020800 (NS)\n2. 0x46020810 (NS)\n3. 0x56020824 (S)', 
         '1. Bits [2:0] LPMS = 010b (Stop 2 Mode ~2,5 uA), Bit 8 DBP = 1 (Buka akses tulis domain RTC)\n2. Bit 24 PVDE = 1, Bits [23:21] PLS = 011b (PVD Level 3: 2,5 V Last-Gasp Trigger)\n3. Bit 0 VBE = 1 (Charger Superkapasitor / Baterai Cadangan ON)'),

        ('Dual-Watchdog Supervisor\n(IWDG & WWDG Anti-Hang)', 
         '1. IWDG_KR (+0x00)\n2. IWDG_PR (+0x04)\n3. IWDG_RLR (+0x08)\n4. WWDG_CR (+0x00)', 
         '1. 0x40003000 (NS)\n2. 0x40003004 (NS)\n3. 0x40003008 (NS)\n4. 0x40002C00 (NS)', 
         '1. KR = 0x5555 (Unlock), 0xCCCC (Start), 0xAAAA (Refresh)\n2. PR = 100b (/64), RLR = 1000 (Timeout ~2.000 ms pada LSI 32 kHz independen)\n4. Bit 7 WDGA = 1 (Window Watchdog untuk timing pengawasan task metrologi)'),

        ('ICACHE Control\n(Instruction Cache 160MHz)', 
         '1. ICACHE_CR (+0x00)', 
         '1. 0x50030400 (S)', 
         '1. Bit 0 EN = 1 (ICACHE Aktif penuh untuk eksekusi kode dari Flash pada 160 MHz tanpa wait-state penalty)')
    ]
    render_four_col_table('4. Tabel Register Internal Sistem: FIREWALL, BOOT, NVIC, DMA, KRIPTO, & WATCHDOG', internal_data, '333366')

    # Simpan berkas langsung menimpa REGISF.docx (atau backup jika file sedang dibuka oleh pengguna)
    target_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/REGISF.docx'
    alt_target_path = 'G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/REGISF_Updated.docx'
    try:
        doc.save(target_path)
        print(f'SUCCESS: Complete registers populated into {target_path}')
    except PermissionError:
        doc.save(alt_target_path)
        print(f'WARNING: {target_path} sedang dibuka oleh aplikasi lain (Word). Disimpan ke {alt_target_path}')

if __name__ == '__main__':
    populate_regisf_doc()

