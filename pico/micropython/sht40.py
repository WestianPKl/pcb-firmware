import time


class SHT40:
    def __init__(self, i2c, addr=0x44):
        if i2c == None:
            raise ValueError("No I2C argument!")
        self.__i2c = i2c
        self.__addr = addr

    def __check_crc(self, data):
        crc = 0xFF
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 0x80:
                    crc = (crc << 1) ^ 0x31
                else:
                    crc <<= 1
                crc &= 0xFF
        return crc

    def _raw_data(self):
        try:
            self.__i2c.writeto(self.__addr, b"\xfd")
            time.sleep_ms(100)
            raw = self.__i2c.readfrom(self.__addr, 6)
            return (raw[0] << 8) + raw[1], (raw[3] << 8) + raw[4]
        except Exception as e:
            print("Reading error:", e)
            return 0, 0

    def measurement(self):
        try:
            t, h = self._raw_data()
            temp = -45 + (175 * (t / 65535))
            hum = 100 * (h / 65535)
            meas = (temp, hum)
            return meas
        except Exception as e:
            print("Measurement error:", e)
            return 0.0, 0.0

    def temperature(self):
        try:
            temp, _ = self.measurement()
            return temp
        except Exception as e:
            print("Temperature measurement error:", e)
            return 0.0

    def relative_humidity(self):
        try:
            _, hum = self.measurement()
            return hum
        except Exception as e:
            print("Humidity measurement error:", e)
            return 0.0

    def heater(self, heater_status: bool):
        try:
            if heater_status:
                self.__i2c.writeto(self.__addr, b"\x39")
                time.sleep_ms(100)
            else:
                self.__i2c.writeto(self.__addr, b"\x30\x66")
                time.sleep_ms(100)
        except Exception as e:
            print("Heater setting error:", e)

    def register_status(self):
        message = []
        try:
            self.__i2c.writeto(self.__addr, b"\xf3\x2d")
            time.sleep_ms(100)
            raw = self.__i2c.readfrom(self.__addr, 3)
            status_bytes = raw[0:2]
            crc = raw[2]
            if self.__check_crc(status_bytes) != crc:
                message.append("CRC error!")
            else:
                status = (status_bytes[0] << 8) | status_bytes[1]
                message.append({"Register status (HEX):": hex(status)})

                if status & (1 << 0):
                    message.append("→ Write data checksum status.")
                if status & (1 << 1):
                    message.append("→ Command status.")
                if status & (1 << 4):
                    message.append("→ Soft reset.")
                if status & (1 << 10):
                    message.append("→ RH tracking alert.")
                if status & (1 << 11):
                    message.append("→ Temperature tracking alert.")
                if status & (1 << 13):
                    message.append("→ Heater ON.")
                if status & (1 << 15):
                    message.append("→ Alert pending.")
        except Exception as e:
            message.append(f"Register error: {e}")
        return message

    def soft_reset(self):
        try:
            self.__i2c.writeto(self.__addr, b"\x94")
            time.sleep_ms(100)
        except Exception as e:
            print("Soft reset error:", e)

    def read_serial_number(self):
        self.__i2c.writeto(self.__addr, b"\x89")
        time.sleep_ms(2)
        data = self.__i2c.readfrom(self.__addr, 6)
        if len(data) != 6:
            raise RuntimeError("Incomplete I2C response")
        if (
            self.__check_crc(data[0:2]) != data[2]
            or self.__check_crc(data[3:5]) != data[5]
        ):
            raise RuntimeError("CRC error in SHT3x response")
        serial = (data[0] << 24) | (data[1] << 16) | (data[3] << 8) | data[4]
        return serial
