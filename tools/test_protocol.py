#!/usr/bin/env python3
"""
Pico通信テストスクリプト

使用方法:
    python tools/test_protocol.py /dev/cu.usbmodem*

依存ライブラリ:
    pip install pyserial cobs
"""

import sys
import struct
import time
import serial
from cobs import cobs


class PicoProtocol:
    # リクエストタイプ
    REQUEST_MOTOR_COMMAND = 0x00
    REQUEST_GET_VERSION = 0x01
    REQUEST_GET_STATUS = 0x02
    REQUEST_GET_CONFIG = 0x03
    REQUEST_SET_CONFIG = 0x04
    REQUEST_GET_DEBUG_OUTPUT = 0x05

    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=1.0)
        time.sleep(0.1)  # 接続待ち
        self.ser.reset_input_buffer()

    def close(self):
        self.ser.close()

    def _calculate_checksum(self, data):
        return sum(data) & 0xFFFF

    def _send_request(self, request_type, payload=b''):
        checksum = self._calculate_checksum(payload)
        header = struct.pack('<BBH', request_type, len(payload), checksum)
        packet = header + payload
        encoded = cobs.encode(packet) + b'\x00'
        self.ser.write(encoded)
        self.ser.flush()

    def _receive_response(self, timeout=1.0):
        self.ser.timeout = timeout
        data = b''
        start_time = time.time()
        while time.time() - start_time < timeout:
            byte = self.ser.read(1)
            if byte == b'\x00':
                break
            if byte:
                data += byte
        if not data:
            return None
        try:
            return cobs.decode(data)
        except Exception as e:
            print(f"COBS decode error: {e}")
            return None

    def get_version(self):
        """GET_VERSION: ファームウェアバージョン取得"""
        self._send_request(self.REQUEST_GET_VERSION)
        response = self._receive_response()
        if response and len(response) >= 8:
            resp_type, payload_len, checksum, major, minor, patch, _ = struct.unpack(
                '<BBHBBBB', response[:8])
            return {
                'response_type': resp_type,
                'major': major,
                'minor': minor,
                'patch': patch,
                'version': f"v{major}.{minor}.{patch}"
            }
        return None

    def get_status(self):
        """GET_STATUS: ステータス取得"""
        self._send_request(self.REQUEST_GET_STATUS)
        response = self._receive_response()
        if response and len(response) >= 16:
            resp_type, payload_len, checksum = struct.unpack('<BBH', response[:4])
            status, error_code, _, comm_err, _, _, uptime = struct.unpack(
                '<HBBHHHI', response[4:16])
            return {
                'response_type': resp_type,
                'status': status,
                'error_code': error_code,
                'comm_error_count': comm_err,
                'uptime_ms': uptime
            }
        return None

    def get_config(self):
        """GET_CONFIG: 設定取得"""
        self._send_request(self.REQUEST_GET_CONFIG)
        response = self._receive_response()
        if response and len(response) >= 34:
            resp_type, payload_len, checksum = struct.unpack('<BBH', response[:4])
            kp, ki, kd, max_rpm = struct.unpack('<ffff', response[4:20])
            ppr, = struct.unpack('<H', response[20:22])
            gear, wheel_d, track_w = struct.unpack('<fff', response[22:34])
            return {
                'response_type': resp_type,
                'pid_kp': kp,
                'pid_ki': ki,
                'pid_kd': kd,
                'max_rpm': max_rpm,
                'encoder_ppr': ppr,
                'gear_ratio': gear,
                'wheel_diameter': wheel_d,
                'track_width': track_w
            }
        return None

    def motor_command(self, linear_x, angular_z):
        """MOTOR_COMMAND: 速度指令送信"""
        payload = struct.pack('<ff', linear_x, angular_z)
        self._send_request(self.REQUEST_MOTOR_COMMAND, payload)
        response = self._receive_response()
        if response and len(response) >= 14:
            resp_type, payload_len, checksum = struct.unpack('<BBH', response[:4])
            enc_l, enc_r, status = struct.unpack('<iiH', response[4:14])
            return {
                'response_type': resp_type,
                'encoder_l': enc_l,
                'encoder_r': enc_r,
                'status': status
            }
        return None

    def get_debug_output(self):
        """GET_DEBUG_OUTPUT: デバッグ出力取得"""
        self._send_request(self.REQUEST_GET_DEBUG_OUTPUT)
        response = self._receive_response()
        if response and len(response) >= 36:
            resp_type, payload_len, checksum = struct.unpack('<BBH', response[:4])
            enc_l, enc_r = struct.unpack('<ii', response[4:12])
            target_l, target_r, current_l, current_r = struct.unpack('<ffff', response[12:28])
            pwm_l, pwm_r = struct.unpack('<ff', response[28:36])
            return {
                'response_type': resp_type,
                'encoder_l': enc_l,
                'encoder_r': enc_r,
                'target_rpm_l': target_l,
                'target_rpm_r': target_r,
                'current_rpm_l': current_l,
                'current_rpm_r': current_r,
                'pwm_l': pwm_l,
                'pwm_r': pwm_r
            }
        return None


def test_version(pico):
    """Step 1: GET_VERSIONテスト"""
    print("\n=== Step 1: GET_VERSION ===")
    result = pico.get_version()
    if result:
        print(f"  Version: {result['version']}")
        print("  [OK] Core0通信成功")
        return True
    else:
        print("  [NG] 応答なし")
        return False


def test_status(pico):
    """Step 2: GET_STATUSテスト"""
    print("\n=== Step 2: GET_STATUS ===")
    result = pico.get_status()
    if result:
        print(f"  Status: 0x{result['status']:04X}")
        print(f"  Error Code: 0x{result['error_code']:02X}")
        print(f"  Comm Errors: {result['comm_error_count']}")
        print(f"  Uptime: {result['uptime_ms']} ms")
        print("  [OK] ステータス取得成功")
        return True
    else:
        print("  [NG] 応答なし")
        return False


def test_config(pico):
    """Step 3: GET_CONFIGテスト"""
    print("\n=== Step 3: GET_CONFIG ===")
    result = pico.get_config()
    if result:
        print(f"  PID: Kp={result['pid_kp']}, Ki={result['pid_ki']}, Kd={result['pid_kd']}")
        print(f"  Max RPM: {result['max_rpm']}")
        print(f"  Encoder PPR: {result['encoder_ppr']}")
        print(f"  Gear Ratio: {result['gear_ratio']}")
        print(f"  Wheel Diameter: {result['wheel_diameter']} m")
        print(f"  Track Width: {result['track_width']} m")
        print("  [OK] 設定取得成功")
        return True
    else:
        print("  [NG] 応答なし")
        return False


def test_motor_command_zero(pico):
    """Step 4: MOTOR_COMMAND（停止）テスト"""
    print("\n=== Step 4: MOTOR_COMMAND (停止) ===")
    result = pico.motor_command(0.0, 0.0)
    if result:
        print(f"  Encoder L: {result['encoder_l']}")
        print(f"  Encoder R: {result['encoder_r']}")
        print(f"  Status: 0x{result['status']:04X}")
        print("  [OK] MOTOR_COMMAND成功")
        return True
    else:
        print("  [NG] 応答なし")
        return False


def test_debug_output(pico):
    """Step 5: GET_DEBUG_OUTPUTテスト"""
    print("\n=== Step 5: GET_DEBUG_OUTPUT ===")
    result = pico.get_debug_output()
    if result:
        print(f"  Encoder: L={result['encoder_l']}, R={result['encoder_r']}")
        print(f"  Target RPM: L={result['target_rpm_l']:.1f}, R={result['target_rpm_r']:.1f}")
        print(f"  Current RPM: L={result['current_rpm_l']:.1f}, R={result['current_rpm_r']:.1f}")
        print(f"  PWM: L={result['pwm_l']:.2f}, R={result['pwm_r']:.2f}")
        print("  [OK] デバッグ出力取得成功")
        return True
    else:
        print("  [NG] 応答なし")
        return False


def test_encoder_change(pico):
    """Step 6: エンコーダ変化テスト（手動）"""
    print("\n=== Step 6: エンコーダ変化テスト ===")
    print("  エンコーダを手で回してください...")
    print("  5秒間カウントを監視します")

    prev_l, prev_r = 0, 0
    for i in range(10):
        result = pico.motor_command(0.0, 0.0)
        if result:
            enc_l, enc_r = result['encoder_l'], result['encoder_r']
            if i == 0:
                prev_l, prev_r = enc_l, enc_r
            diff_l = enc_l - prev_l
            diff_r = enc_r - prev_r
            print(f"  [{i+1}/10] L={enc_l} (diff={diff_l:+d}), R={enc_r} (diff={diff_r:+d})")
            prev_l, prev_r = enc_l, enc_r
        time.sleep(0.5)

    print("  エンコーダが変化していれば成功")


def main():
    if len(sys.argv) < 2:
        print("Usage: python test_protocol.py <serial_port>")
        print("Example: python test_protocol.py /dev/cu.usbmodem1101")
        sys.exit(1)

    port = sys.argv[1]
    print(f"Connecting to {port}...")

    try:
        pico = PicoProtocol(port)
    except Exception as e:
        print(f"Connection failed: {e}")
        sys.exit(1)

    print("Connected!")

    try:
        # 基本テスト
        if not test_version(pico):
            print("\n通信に失敗しました。接続を確認してください。")
            return

        test_status(pico)
        test_config(pico)
        test_motor_command_zero(pico)
        test_debug_output(pico)

        # インタラクティブテスト
        input("\nEnterを押すとエンコーダテストを開始します...")
        test_encoder_change(pico)

        print("\n=== テスト完了 ===")

    finally:
        pico.close()


if __name__ == '__main__':
    main()
