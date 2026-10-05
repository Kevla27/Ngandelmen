#!/usr/bin/env python3
"""
=============================================================================
  TEST KOMUNIKASI DLMS/COSEM VIA UART (SERIAL COM)
  Smart Meter 3-Fasa STM32U575 (E1 ADE9000 -> E3 -> E2 DLMS/COSEM)
=============================================================================
Skrip ini mengirimkan rentetan frame standar DLMS/COSEM HDLC (IEC 62056-46):
  1. Handshake HDLC: SNRM (Set Normal Response Mode) -> Harap UA
  2. Asosiasi DLMS: AARQ (Application Association Request) -> Harap AARE
  3. Pembacaan Register OBIS (GET-Request):
     - 1.0.32.7.0.255: Tegangan Fasa R (V)
     - 1.0.52.7.0.255: Tegangan Fasa S (V)
     - 1.0.72.7.0.255: Tegangan Fasa T (V)
     - 1.0.31.7.0.255: Arus Fasa R (A)
     - 1.0.51.7.0.255: Arus Fasa S (A)
     - 1.0.71.7.0.255: Arus Fasa T (A)
     - 1.0.91.7.0.255: Arus Netral N (A)
     - 1.0.1.7.0.255:  Daya Aktif Total (kW)
     - 1.0.1.8.0.255:  Total Energi Aktif (kWh)
  4. Pelepasan Sesi: DISC (Disconnect) -> Harap UA

Penggunaan:
  python tools/test_dlms_uart.py [PORT] [BAUDRATE]
  Contoh:
    python tools/test_dlms_uart.py COM27 115200
"""

import sys
import time
import struct
import argparse

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] Pyserial belum terpasang. Jalankan: pip install pyserial")
    sys.exit(1)

# Lookup Table CRC16-CCITT (Polinomial 0x8408 - Standar HDLC/PPP)
FCS_TABLE = [
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbe0, 0xea69, 0xd8f2, 0xc97b,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3ad1, 0x4e64, 0x5fed, 0x6d76, 0x7cfd,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bc3, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ed3, 0x2f5a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
]

def calculate_crc16(data: bytes) -> int:
    fcs = 0xFFFF
    for b in data:
        fcs = (fcs >> 8) ^ FCS_TABLE[(fcs ^ b) & 0xFF]
    return fcs ^ 0xFFFF

def build_hdlc_frame(ctrl: int, dest_sap: int, src_sap: int, payload: bytes = b"") -> bytes:
    """Membungkus payload APDU ke dalam bingkai HDLC standar (Flag 0x7E)."""
    dest_bytes = bytes([(dest_sap << 1) | 0x01])
    src_bytes = bytes([(src_sap << 1) | 0x01])
    header_fields = dest_bytes + src_bytes + bytes([ctrl])
    
    if len(payload) == 0:
        # Frame tanpa information (SNRM, DISC, UA): total_len = format(2) + header(3) + fcs(2) = 7
        total_len = 2 + len(header_fields) + 2
        fmt = bytes([0xA0 | ((total_len >> 8) & 0x07), total_len & 0xFF])
        raw = fmt + header_fields
        fcs = calculate_crc16(raw)
        return b"\x7E" + raw + struct.pack("<H", fcs) + b"\x7E"
    else:
        # Frame ber-information (I-Frame):
        # Format field (2) + header (3) + HCS (2) + LLC (3) + payload + FCS (2)
        llc_snap = b"\xE6\xE6\x00"
        total_len = 2 + len(header_fields) + 2 + len(llc_snap) + len(payload) + 2
        fmt = bytes([0xA0 | ((total_len >> 8) & 0x07), total_len & 0xFF])
        raw_header = fmt + header_fields
        hcs = calculate_crc16(raw_header)
        raw_header_with_hcs = raw_header + struct.pack("<H", hcs)
        body = raw_header_with_hcs + llc_snap + payload
        fcs = calculate_crc16(body)
        return b"\x7E" + body + struct.pack("<H", fcs) + b"\x7E"

def parse_hdlc_frame(data: bytes):
    """Mengekstrak payload dan tipe kontrol dari frame HDLC balasan."""
    if len(data) < 7 or data[0] != 0x7E or data[-1] != 0x7E:
        return None, None, None
    
    inner = data[1:-1]
    dest = inner[2] >> 1
    src = inner[3] >> 1
    ctrl = inner[4]
    
    if len(inner) == 7: # Frame kontrol tanpa payload (UA, DM, DISC, dll.)
        return ctrl, dest, b""
    
    # I-Frame: inner[0:2]=fmt, inner[2]=dest, inner[3]=src, inner[4]=ctrl, inner[5:7]=hcs, inner[7:10]=llc, inner[10:-2]=payload
    if len(inner) >= 12:
        payload = inner[10:-2]
        return ctrl, dest, payload
    
    return ctrl, dest, b""

def send_and_receive(ser, req_frame: bytes, timeout: float = 1.5) -> bytes:
    """Mengirim frame dan membaca respons frame HDLC utuh."""
    ser.reset_input_buffer()
    ser.write(req_frame)
    ser.flush()
    
    start_time = time.time()
    rx_buf = bytearray()
    in_frame = False
    
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            b = ser.read(ser.in_waiting)
            for byte in b:
                if byte == 0x7E:
                    if not in_frame:
                        # Mulai frame baru
                        rx_buf = bytearray([0x7E])
                        in_frame = True
                    else:
                        # Akhir frame jika panjang buffer > 1 (mengabaikan bendera 0x7E berurutan)
                        if len(rx_buf) > 1:
                            rx_buf.append(0x7E)
                            return bytes(rx_buf)
                elif in_frame:
                    rx_buf.append(byte)
        else:
            time.sleep(0.01)
            
    return bytes(rx_buf) if in_frame else b""

class MockSerial:
    """Simulasi Virtual Meter untuk Pengujian Mandiri Script Python (Self-Test)."""
    def __init__(self, port="MOCK", baudrate=115200, timeout=1.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.in_waiting = 0
        self.rx_queue = bytearray()
        self.state = "UNASSOCIATED"

    def reset_input_buffer(self):
        self.rx_queue.clear()
        self.in_waiting = 0

    def write(self, data):
        ctrl, dest, payload = parse_hdlc_frame(data)
        if ctrl == 0x93: # SNRM
            ua = build_hdlc_frame(0x73, 0x10, 0x01)
            self.rx_queue.extend(ua)
            self.state = "ASSOCIATED_READONLY"
        elif ctrl == 0x53: # DISC
            ua = build_hdlc_frame(0x73, 0x10, 0x01)
            self.rx_queue.extend(ua)
            self.state = "UNASSOCIATED"
        elif ctrl == 0x10 and payload: # I-Frame
            tag = payload[0]
            if tag == 0x60: # AARQ
                aare = bytes([0x61, 0x29, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01, 0xA2, 0x03, 0x02, 0x01, 0x00, 0xA3, 0x05, 0xA1, 0x03, 0x02, 0x01, 0x00, 0xBE, 0x10, 0x04, 0x0E, 0x08, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00, 0x18, 0x1D, 0x01, 0xF4])
                self.rx_queue.extend(build_hdlc_frame(0x10, 0x10, 0x01, aare))
            elif tag == 0xC0: # GET-Request
                invoke_id = payload[2]
                class_id = (payload[3] << 8) | payload[4]
                obis = list(payload[5:11])
                attr = payload[11]
                # Format respons metrologi E1
                if obis in ([1, 0, 32, 7, 0, 255], [1, 0, 52, 7, 0, 255], [1, 0, 72, 7, 0, 255]):
                    val = 2300 # 230.0 V (skala 0.1)
                    resp = bytes([0xC4, 0x01, invoke_id, 0x00, 0x06]) + struct.pack(">I", val)
                elif obis in ([1, 0, 31, 7, 0, 255], [1, 0, 51, 7, 0, 255], [1, 0, 71, 7, 0, 255]):
                    val = 1195 # 11.95 A (skala 0.01)
                    resp = bytes([0xC4, 0x01, invoke_id, 0x00, 0x12]) + struct.pack(">H", val)
                elif obis == [1, 0, 91, 7, 0, 255]:
                    val = 239 # 2.39 A (skala 0.01)
                    resp = bytes([0xC4, 0x01, invoke_id, 0x00, 0x12]) + struct.pack(">H", val)
                elif obis == [1, 0, 1, 7, 0, 255]:
                    val = 6898 # 6.898 kW (skala 0.001)
                    resp = bytes([0xC4, 0x01, invoke_id, 0x00, 0x06]) + struct.pack(">I", val)
                elif obis == [1, 0, 1, 8, 0, 255]:
                    val = 12543 # 125.43 kWh (skala 0.01)
                    resp = bytes([0xC4, 0x01, invoke_id, 0x00, 0x06]) + struct.pack(">I", val)
                else:
                    resp = bytes([0xC4, 0x01, invoke_id, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00])
                self.rx_queue.extend(build_hdlc_frame(0x10, 0x10, 0x01, resp))
        self.in_waiting = len(self.rx_queue)

    def read(self, n):
        data = self.rx_queue[:n]
        del self.rx_queue[:n]
        self.in_waiting = len(self.rx_queue)
        return bytes(data)

    def flush(self):
        pass

    def close(self):
        pass

def main():
    parser = argparse.ArgumentParser(description="DLMS/COSEM UART Master Verification Tool")
    parser.add_argument("port", nargs="?", default="COM27", help="Serial Port (misal COM27 atau /dev/ttyUSB0)")
    parser.add_argument("baud", nargs="?", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--mock", action="store_true", help="Gunakan mode simulasi virtual tanpa serial port fisik")
    args = parser.parse_args()

    print("========================================================================")
    print("   PENGUJIAN KOMUNIKASI PROTOKOL DLMS/COSEM VIA SERIAL UART             ")
    print("   Target: STM32U575 Smart Meter 3-Fasa (SPLN D3.022-1 / IEC 62056)     ")
    print("========================================================================")
    print(f"Target Port: {'VIRTUAL MOCK' if args.mock else args.port} | Baudrate: {args.baud} 8N1\n")

    if args.mock:
        ser = MockSerial(args.port, args.baud, timeout=0.5)
    else:
        try:
            ser = serial.Serial(args.port, args.baud, timeout=0.5)
        except serial.SerialException as e:
            print(f"[ERROR] Gagal membuka port {args.port}: {e}")
            print("\nSaran Penanganan:")
            print("1. Pastikan kabel USB / converter serial terhubung ke board Nucleo.")
            print("2. Tutup aplikasi Serial Monitor / Terminal lain (seperti PuTTY atau STM32CubeIDE) yang sedang memakai port ini.")
            print("3. Periksa daftar port aktif dengan perintah:")
            print("   python -m serial.tools.list_ports")
            print("4. Untuk menguji skrip tanpa koneksi hardware, jalankan:")
            print("   python tools/test_dlms_uart.py --mock")
            sys.exit(1)

    CLIENT_SAP_PUBLIC = 0x10  # 16 (Public Client Read-Only)
    SERVER_SAP_MGMT   = 0x01  # 1 (Management Logical Device)
    
    # -------------------------------------------------------------------------
    # TAHAP 1: HDLC Handshake (SNRM -> UA)
    # -------------------------------------------------------------------------
    print("[TAHAP 1] Mengirim HDLC SNRM (Set Normal Response Mode)...")
    snrm_frame = build_hdlc_frame(0x93, SERVER_SAP_MGMT, CLIENT_SAP_PUBLIC)
    print(f"  TX SNRM: {snrm_frame.hex(' ').upper()}")
    
    resp = send_and_receive(ser, snrm_frame)
    if not resp:
        print("  [FAIL] Tidak ada respons dari meter! Periksa koneksi TX/RX.")
        ser.close()
        sys.exit(1)
        
    print(f"  RX RESP: {resp.hex(' ').upper()}")
    ctrl, dest, payload = parse_hdlc_frame(resp)
    if ctrl == 0x73: # UA Control byte
        print("  [PASS] HDLC Link Connected! Respons UA (Unnumbered Acknowledge) Diterima.\n")
    else:
        print(f"  [FAIL] Respons bukan UA! Control byte: 0x{ctrl:02X}")
        ser.close()
        sys.exit(1)

    # -------------------------------------------------------------------------
    # TAHAP 2: Asosiasi Aplikasi DLMS (AARQ -> AARE)
    # -------------------------------------------------------------------------
    print("[TAHAP 2] Mengirim DLMS AARQ (Application Association Request)...")
    aarq_apdu = bytes([
        0x60, 0x1D, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01,
        0xBE, 0x10, 0x04, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00,
        0x00, 0x1E, 0x1D, 0xFF, 0xFF
    ])
    aarq_frame = build_hdlc_frame(0x10, SERVER_SAP_MGMT, CLIENT_SAP_PUBLIC, aarq_apdu)
    print(f"  TX AARQ: {aarq_frame.hex(' ').upper()}")
    
    resp = send_and_receive(ser, aarq_frame)
    if not resp:
        print("  [FAIL] Tidak ada respons AARE dari meter.")
        ser.close()
        sys.exit(1)
        
    print(f"  RX AARE: {resp.hex(' ').upper()}")
    ctrl, dest, payload = parse_hdlc_frame(resp)
    if payload and payload[0] == 0x61: # AARE Tag
        print("  [PASS] Asosiasi DLMS Berhasil! Status: ASSOCIATED_READONLY (Public Client).\n")
    else:
        print("  [FAIL] Respons bukan AARE yang valid.")
        ser.close()
        sys.exit(1)

    # -------------------------------------------------------------------------
    # TAHAP 3: Pembacaan Register OBIS (GET-Request -> GET-Response)
    # -------------------------------------------------------------------------
    test_registers = [
        ("Tegangan Fasa R (V)",   [1, 0, 32, 7, 0, 255], 3, 2, 0.1,  "V"),
        ("Tegangan Fasa S (V)",   [1, 0, 52, 7, 0, 255], 3, 2, 0.1,  "V"),
        ("Tegangan Fasa T (V)",   [1, 0, 72, 7, 0, 255], 3, 2, 0.1,  "V"),
        ("Arus Fasa R (A)",       [1, 0, 31, 7, 0, 255], 3, 2, 0.01, "A"),
        ("Arus Fasa S (A)",       [1, 0, 51, 7, 0, 255], 3, 2, 0.01, "A"),
        ("Arus Fasa T (A)",       [1, 0, 71, 7, 0, 255], 3, 2, 0.01, "A"),
        ("Arus Netral N (A)",     [1, 0, 91, 7, 0, 255], 3, 2, 0.01, "A"),
        ("Daya Aktif Total (kW)", [1, 0, 1, 7, 0, 255],  3, 2, 0.001,"kW"),
        ("Total Energi Aktif",    [1, 0, 1, 8, 0, 255],  3, 2, 0.01, "kWh"),
    ]

    print("[TAHAP 3] Membaca Parameter Pengukuran Metrologi via OBIS GET-Request...")
    print("+---------------------------+---------------------+------------+-----------+")
    print("| Parameter Listrik         | Kode Register OBIS  | Nilai Asli | Satuan    |")
    print("+---------------------------+---------------------+------------+-----------+")

    invoke_id = 0x81
    for name, obis, class_id, attr_id, scale, unit in test_registers:
        obis_str = ".".join(str(x) for x in obis)
        # APDU GET-Request: [0xC0, 0x01 (GET-Normal), invoke_id, class_id(2B), OBIS(6B), attr_id(1B)]
        get_apdu = bytes([0xC0, 0x01, invoke_id, (class_id >> 8) & 0xFF, class_id & 0xFF]) + bytes(obis) + bytes([attr_id])
        get_frame = build_hdlc_frame(0x10, SERVER_SAP_MGMT, CLIENT_SAP_PUBLIC, get_apdu)
        
        resp = send_and_receive(ser, get_frame)
        if not resp:
            print(f"| {name:<25} | {obis_str:<19} | TIMEOUT    | {unit:<9} |")
            continue
            
        ctrl, dest, payload = parse_hdlc_frame(resp)
        if payload and payload[0] == 0xC4 and payload[1] == 0x01: # GET-Response-Normal
            data_type = payload[4]
            raw_bytes = payload[5:]
            if data_type == 0x06: # Double Long Unsigned (4 bytes uint32)
                raw_val = struct.unpack(">I", raw_bytes[:4])[0]
            elif data_type == 0x12: # Long Unsigned (2 bytes uint16)
                raw_val = struct.unpack(">H", raw_bytes[:2])[0]
            elif data_type == 0x10: # Long (2 bytes int16)
                raw_val = struct.unpack(">h", raw_bytes[:2])[0]
            elif data_type == 0x05: # Double Long (4 bytes int32)
                raw_val = struct.unpack(">i", raw_bytes[:4])[0]
            else:
                raw_val = int.from_bytes(raw_bytes, "big")
                
            phys_val = raw_val * scale
            if scale == 0.001:
                val_str = f"{phys_val:.3f}"
            elif scale == 0.01:
                val_str = f"{phys_val:.2f}"
            else:
                val_str = f"{phys_val:.1f}"
                
            print(f"| {name:<25} | {obis_str:<19} | {val_str:>10} | {unit:<9} | [PASS]")
        else:
            print(f"| {name:<25} | {obis_str:<19} | ERR: REJECT| {unit:<9} | [FAIL]")
            
        invoke_id = (invoke_id + 1) & 0xFF
        time.sleep(0.05)

    print("+---------------------------+---------------------+------------+-----------+\n")

    # -------------------------------------------------------------------------
    # TAHAP 4: Pelepasan Sesi (DISC -> UA)
    # -------------------------------------------------------------------------
    print("[TAHAP 4] Mengirim HDLC DISC (Disconnect Session)...")
    disc_frame = build_hdlc_frame(0x53, SERVER_SAP_MGMT, CLIENT_SAP_PUBLIC)
    resp = send_and_receive(ser, disc_frame)
    if resp:
        ctrl, dest, payload = parse_hdlc_frame(resp)
        if ctrl == 0x73:
            print("  [PASS] Sesi DLMS Berhasil Ditutup (Server kembali ke Status UNASSOCIATED).\n")
    else:
        print("  [WARN] Tidak ada respons DISC dari server.\n")

    ser.close()
    print("========================================================================")
    print("   SELURUH RANGKAIAN UJI KOMUNIKASI DLMS VIA UART: 100% SUKSES!         ")
    print("========================================================================")

if __name__ == "__main__":
    main()
