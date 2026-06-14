import time
from machine import UART
from config import get_config

cfg = get_config()


class STM32UART:
    def __init__(self, uart_device: UART):
        self.uart = uart_device

    def crc8_atm(self, data: bytes) -> int:
        crc = 0x00
        for b in data:
            crc ^= b
            for _ in range(8):
                if crc & 0x80:
                    crc = ((crc << 1) ^ 0x07) & 0xFF
                else:
                    crc = (crc << 1) & 0xFF
        return crc

    def uart_message_application(
        self, cmd: int, param_addr: int = 0, payload: bytes = b""
    ) -> bytes:
        frame = bytearray(cfg.get_frame_len_app())
        frame[0] = cfg.get_dev_addr()
        frame[1] = 0x00
        frame[2] = cmd & 0xFF
        frame[3] = param_addr & 0xFF

        for i, b in enumerate(payload):
            if 4 + i >= cfg.get_frame_len_app() - 1:
                break
            frame[4 + i] = b

        frame[cfg.get_frame_len_app() - 1] = self.crc8_atm(
            frame[: cfg.get_frame_len_app() - 1]
        )
        return bytes(frame)

    def read_exact(self, n: int, timeout_ms: int = 500):
        buf = bytearray()
        t0 = time.ticks_ms()
        while len(buf) < n and time.ticks_diff(time.ticks_ms(), t0) < timeout_ms:
            chunk = self.uart.read(n - len(buf))
            if chunk:
                buf.extend(chunk)
            else:
                time.sleep_ms(2)
        if len(buf) == n:
            return bytes(buf)
        return None

    def _flush_rx(self):
        while True:
            d = self.uart.read()
            if not d:
                break
            time.sleep_ms(1)

    def uart_loop_application(self, frame: bytes):
        self._flush_rx()
        self.uart.write(frame)
        t0 = time.ticks_ms()
        while time.ticks_diff(time.ticks_ms(), t0) < 1500:
            b = self.uart.read(1)
            if not b:
                time.sleep_ms(2)
                continue
            if b[0] != cfg.get_dev_addr():
                continue

            rest = self.read_exact(cfg.get_frame_len_app() - 1, 1200)
            if rest is None:
                return None

            data = bytes([cfg.get_dev_addr()]) + rest
            if (
                self.crc8_atm(data[: cfg.get_frame_len_app() - 1])
                != data[cfg.get_frame_len_app() - 1]
            ):
                continue
            return data
        return None

    def parse_resp(self, resp: bytes):
        if not resp or len(resp) != cfg.get_frame_len_app():
            return None
        addr = resp[0]
        status = resp[1]
        cmd = resp[2]
        param = resp[3]
        payload = resp[4 : cfg.get_frame_len_app() - 1]
        crc = resp[cfg.get_frame_len_app() - 1]
        return addr, status, cmd, param, payload, crc

    def i16_from_be(self, payload: bytes, offset=0) -> int:
        v = (payload[offset] << 8) | payload[offset + 1]
        if v & 0x8000:
            v -= 0x10000
        return v

    def u16_from_be(self, payload, offset=0) -> int:
        return (payload[offset] << 8) | payload[offset + 1]

    def i32_from_be(self, payload, offset=0) -> int:
        v = (
            (payload[offset] << 24)
            | (payload[offset + 1] << 16)
            | (payload[offset + 2] << 8)
            | payload[offset + 3]
        )
        if v & 0x80000000:
            v -= 0x100000000
        return v

    def u32_from_be(self, payload, offset=0) -> int:
        return (
            (payload[offset] << 24)
            | (payload[offset + 1] << 16)
            | (payload[offset + 2] << 8)
            | payload[offset + 3]
        )

    def adc_to_voltage(self, adc_value, vref=3.3):
        return (adc_value / 4095.0) * vref

    def vadc_to_vin(self, vadc, r_top=200_000.0, r_bottom=68_000.0):
        return vadc * (r_top + r_bottom) / r_bottom

    def unpack_log_timestamp(self, ts: int):
        year = (ts >> 26) & 0x3F
        month = (ts >> 22) & 0x0F
        day = (ts >> 17) & 0x1F
        hour = (ts >> 12) & 0x1F
        minute = (ts >> 6) & 0x3F
        second = ts & 0x3F

        return {
            "year": 2000 + year,
            "month": month,
            "day": day,
            "hour": hour,
            "minute": minute,
            "second": second,
            "iso": "20{:02d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}".format(
                year, month, day, hour, minute, second
            ),
        }

    def _send_cmd(self, cmd, param=0, payload=b"", check_status=True):
        resp = self.uart_loop_application(
            self.uart_message_application(cmd, param, payload)
        )
        parsed = self.parse_resp(resp)
        if not parsed:
            return None
        _, status, _, _, data, _ = parsed
        if check_status and status != cfg.get_status_ok():
            return None
        return data

    def req_ping(self):
        return self._send_cmd(0x00, 0x00, check_status=False) is not None

    def req_serial(self):
        p = self._send_cmd(0x01, 0x00, check_status=False)
        return self.u32_from_be(p, 0) if p else 0

    def req_fw_hw_version(self):
        p = self._send_cmd(0x01, 0x01, check_status=False)
        if not p:
            return ["", ""]
        return ["{}.{}.{}".format(p[0], p[1], p[2]), "{}.{}".format(p[3], p[4])]

    def req_build_date(self):
        p = self._send_cmd(0x01, 0x02, check_status=False)
        if not p:
            return ""
        return bytes(p[:10]).split(b"\x00")[0].decode("ascii")

    def req_prod_date(self):
        p = self._send_cmd(0x01, 0x03, check_status=False)
        if not p:
            return ""
        return bytes(p[:10]).split(b"\x00")[0].decode("ascii")

    def req_adc(self):
        p = self._send_cmd(0x02, 0x00, check_status=False)
        if not p:
            return [0, 0]
        return [self.u16_from_be(p, 0), self.u16_from_be(p, 2)]

    def req_sht40(self):
        p = self._send_cmd(0x03, 0x00)
        if not p:
            return [0.0, 0.0]
        return [self.i16_from_be(p, 0) / 100.0, self.u16_from_be(p, 2) / 100.0]

    def req_bme280(self):
        p = self._send_cmd(0x03, 0x01)
        if not p:
            return [0.0, 0.0, 0.0]
        return [
            self.i32_from_be(p, 0) / 100.0,
            self.u32_from_be(p, 4) / 100.0,
            self.u32_from_be(p, 8) / 100.0,
        ]

    def req_lcd_clear(self):
        p = self._send_cmd(0x03, 0x02)
        return 1 if p is not None else 0

    def req_lcd_set_backlight(self, on: int):
        p = self._send_cmd(0x03, 0x03, bytes([on & 0xFF]))
        return 1 if p is not None else 0

    def req_radar(self):
        p = self._send_cmd(0x03, 0x04)
        if not p:
            return [0, 0, 0, 0]
        return [
            self.u16_from_be(p, 0),
            self.u16_from_be(p, 2),
            self.u16_from_be(p, 4),
            self.u16_from_be(p, 6),
        ]

    def req_get_input_states(self, channel: int):
        p = self._send_cmd(0x02, channel)
        return p[0] if p else 0

    def req_output_states(self):
        p = self._send_cmd(0x04, 0x00)
        if not p:
            return [0, 0, 0, 0, 0, 0]
        return [p[0], p[1], p[2], p[3], p[4], p[5]]

    def req_set_output(self, channel: int, on: int):
        p = self._send_cmd(0x04, channel, bytes([on & 0xFF]))
        return 1 if p is not None else 0

    def req_set_pwm(self, channel: int, duty: int):
        p = self._send_cmd(0x05, channel, bytes([duty & 0xFF]))
        return 1 if p is not None else 0

    def req_set_rgb(self, r: int, g: int, b: int, brightness: int):
        p = self._send_cmd(
            0x05, 0x05, bytes([r & 0xFF, g & 0xFF, b & 0xFF, brightness & 0xFF])
        )
        if not p:
            return [0, 0, 0, 0]
        return [p[0], p[1], p[2], p[3]]

    def req_set_buzzer(self, freq: int, volume: int):
        pl = bytes([(freq >> 8) & 0xFF, freq & 0xFF, volume & 0xFF])
        p = self._send_cmd(0x05, 0x06, pl)
        if not p:
            return None
        return {
            "freq": self.u16_from_be(p, 0),
            "volume": p[2],
        }

    def req_rtc_read(self):
        p = self._send_cmd(0x06, 0x00)
        if not p:
            return ""
        return "20{:02d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
            p[0], p[1], p[2], p[4], p[5], p[6]
        )

    def req_ina_read(self):
        p = self._send_cmd(0x07, 0x00)
        if not p:
            return [0.0, 0.0, 0.0, 0.0, 0.0]
        bus_voltage = self.u32_from_be(p, 0) / 1_000_000.0
        shunt_voltage = self.i32_from_be(p, 4) / 1_000_000.0
        current = self.i32_from_be(p, 8) / 1_000_000.0
        power = self.u32_from_be(p, 12) / 1_000_000.0
        id = self.u16_from_be(p, 16)
        return [bus_voltage, shunt_voltage, current, power, id]

    def req_rtc_write(
        self, yy: int, mo: int, dd: int, wd: int, hh: int, mi: int, ss: int
    ):
        pl = bytes(
            [
                yy & 0xFF,
                mo & 0xFF,
                dd & 0xFF,
                wd & 0xFF,
                hh & 0xFF,
                mi & 0xFF,
                ss & 0xFF,
            ]
        )
        p = self._send_cmd(0x06, 0x01, pl)
        if not p:
            return ""
        return "20{:02d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}".format(
            p[0], p[1], p[2], p[4], p[5], p[6]
        )

    def req_rtc_wakeup(self, seconds: int):
        pl = bytes([(seconds >> 8) & 0xFF, seconds & 0xFF])
        p = self._send_cmd(0x06, 0x02, pl)
        return 1 if p is not None else 0

    def req_alarm_set(self, hh: int, mi: int, ss: int, daily: int = 1):
        pl = bytes([hh & 0xFF, mi & 0xFF, ss & 0xFF, daily & 0xFF])
        p = self._send_cmd(0x06, 0x03, pl)
        return 1 if p is not None else 0

    def req_alarm_off(self):
        p = self._send_cmd(0x06, 0x04)
        return 1 if p is not None else 0

    def req_timestamp(self):
        p = self._send_cmd(0x06, 0x05)
        if not p:
            return ""
        return "20{:02d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}".format(
            p[0], p[1], p[2], p[4], p[5], p[6]
        )

    def req_fram_read(self, addr: int, data_len: int):
        if addr < 0 or addr > 0x07FF:
            return None
        if data_len <= 0 or data_len > 16:
            return None

        pl = bytes([(addr >> 8) & 0xFF, addr & 0xFF, data_len & 0xFF])
        p = self._send_cmd(0x08, 0x01, pl)
        return p[:data_len] if p else None

    def req_fram_write(self, addr: int, data: bytes):
        if addr < 0 or addr > 0x07FF:
            return 0
        if not data or len(data) > 16:
            return 0

        pl = bytes([(addr >> 8) & 0xFF, addr & 0xFF, len(data) & 0xFF]) + data
        p = self._send_cmd(0x08, 0x00, pl)
        return 1 if p is not None else 0

    def req_fram_read_flags(self):
        p = self._send_cmd(0x08, 0x02)
        flags = {
            "FRAM_FLAG_EXT_RTC_PRESENT": 1 if p and (p[0] & (1 << 0)) else 0,
            "FRAM_FLAG_FLASH_PRESENT": 1 if p and (p[0] & (1 << 1)) else 0,
            "FRAM_FLAG_DISPLAY_PRESENT": 1 if p and (p[0] & (1 << 2)) else 0,
            "FRAM_FLAG_SHT40_PRESENT": 1 if p and (p[0] & (1 << 3)) else 0,
            "FRAM_FLAG_BME280_PRESENT": 1 if p and (p[0] & (1 << 4)) else 0,
            "FRAM_FLAG_INA226_PRESENT": 1 if p and (p[0] & (1 << 5)) else 0,
            "FRAM_FLAG_ADC_PRESENT": 1 if p and (p[0] & (1 << 6)) else 0,
            "FRAM_FLAG_CAN_PRESENT": 1 if p and (p[0] & (1 << 7)) else 0,
        }
        return flags

    def req_fram_write_flags(self, flag_byte: int):
        pl = bytes([flag_byte & 0xFF])
        p = self._send_cmd(0x08, 0x03, pl)
        return 1 if p is not None else 0

    def fram_read_string(self, addr: int, max_len: int):
        result = bytearray()
        offset = 0

        while offset < max_len:
            chunk_len = min(16, max_len - offset)
            chunk = self.req_fram_read(addr + offset, chunk_len)
            if chunk is None:
                raise Exception(
                    "FRAM read failed at addr={} len={}".format(
                        addr + offset, chunk_len
                    )
                )

            for b in chunk:
                if b == 0:
                    return result.decode("utf-8", "ignore")
                result.append(b)

            offset += chunk_len

        return result.decode("utf-8", "ignore")

    def fram_write_string(self, addr: int, value: str, max_len: int):
        raw = value.encode("utf-8")
        if len(raw) >= max_len:
            raw = raw[: max_len - 1]

        padded = raw + b"\x00" + bytes(max_len - len(raw) - 1)

        offset = 0
        while offset < max_len:
            chunk = padded[offset : offset + 16]
            ok = self.req_fram_write(addr + offset, chunk)
            if not ok:
                return 0
            offset += len(chunk)

        return 1

    def req_flash_read(self, addr: int, data_len: int):
        if addr < 0 or addr > 0x01FFFFFF:
            return None
        if data_len <= 0 or data_len > 16:
            return None

        pl = bytes(
            [
                (addr >> 24) & 0xFF,
                (addr >> 16) & 0xFF,
                (addr >> 8) & 0xFF,
                addr & 0xFF,
                data_len & 0xFF,
            ]
        )
        p = self._send_cmd(0x08, 0x11, pl)
        return p[:data_len] if p else None

    def req_flash_write(self, addr: int, data: bytes):
        if addr < 0 or addr > 0x01FFFFFF:
            return 0
        if not data or len(data) > 15:
            return 0

        pl = (
            bytes(
                [
                    (addr >> 24) & 0xFF,
                    (addr >> 16) & 0xFF,
                    (addr >> 8) & 0xFF,
                    addr & 0xFF,
                    len(data) & 0xFF,
                ]
            )
            + data
        )
        p = self._send_cmd(0x08, 0x10, pl)
        return 1 if p is not None else 0

    def write_fram_config_field(self, field_name, value):
        if field_name == "device_id":
            return self.fram_write_string(cfg.get_fram_addr_device_id(), str(value), 32)
        return 0

    def read_fram_config(self):
        data = {}
        data["device_id"] = (
            self.fram_read_string(cfg.get_fram_addr_device_id(), 32) or ""
        )
        return data

    def req_reset(self):
        p = self._send_cmd(0x99, 0x99)
        return 1 if p is not None else 0

    def req_log_count(self):
        p = self._send_cmd(0x08, 0x14)
        if not p:
            return 0
        return self.u32_from_be(p, 0)

    def req_log_read(self, index: int, newest_first: int = 0):
        if index < 0:
            return None

        mode = 1 if newest_first else 0
        pl = bytes(
            [
                mode & 0xFF,
                (index >> 24) & 0xFF,
                (index >> 16) & 0xFF,
                (index >> 8) & 0xFF,
                index & 0xFF,
            ]
        )

        p = self._send_cmd(0x08, 0x13, pl)
        if not p:
            return None

        ts = self.u32_from_be(p, 4)
        pressure_pa = self.u32_from_be(p, 16)

        rec = {
            "sequence": self.u32_from_be(p, 0),
            "timestamp": ts,
            "datetime": self.unpack_log_timestamp(ts).get("iso", "").replace("T", " "),
            "temperature": self.i32_from_be(p, 8) / 100.0,
            "humidity": self.u32_from_be(p, 12) / 100.0,
            "pressure_pa": pressure_pa,
            "pressure_hpa": pressure_pa / 100.0,
            "crc32": self.u32_from_be(p, 20),
        }
        return rec

    def req_log_append(
        self,
        timestamp: int,
        temperature_x100: int,
        humidity_x100: int,
        pressure_pa: int,
    ):
        pl = bytes(
            [
                (timestamp >> 24) & 0xFF,
                (timestamp >> 16) & 0xFF,
                (timestamp >> 8) & 0xFF,
                timestamp & 0xFF,
                (temperature_x100 >> 24) & 0xFF,
                (temperature_x100 >> 16) & 0xFF,
                (temperature_x100 >> 8) & 0xFF,
                temperature_x100 & 0xFF,
                (humidity_x100 >> 24) & 0xFF,
                (humidity_x100 >> 16) & 0xFF,
                (humidity_x100 >> 8) & 0xFF,
                humidity_x100 & 0xFF,
                (pressure_pa >> 24) & 0xFF,
                (pressure_pa >> 16) & 0xFF,
                (pressure_pa >> 8) & 0xFF,
                pressure_pa & 0xFF,
            ]
        )

        p = self._send_cmd(0x08, 0x12, pl)
        return 1 if p is not None else 0

    def req_log_clear(self):
        p = self._send_cmd(0x08, 0x15)
        return 1 if p is not None else 0
