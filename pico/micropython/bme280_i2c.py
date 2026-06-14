import time
from ustruct import unpack, unpack_from
from array import array

BME280_I2CADDR = 0x76

BME280_OSAMPLE_1 = 1
BME280_OSAMPLE_2 = 2
BME280_OSAMPLE_4 = 3
BME280_OSAMPLE_8 = 4
BME280_OSAMPLE_16 = 5

BME280_REGISTER_CONTROL_HUM = 0xF2
BME280_REGISTER_STATUS = 0xF3
BME280_REGISTER_CONTROL = 0xF4

MODE_SLEEP = const(0)
MODE_FORCED = const(1)
MODE_NORMAL = const(3)

BME280_TIMEOUT = const(100)


class BME280:
    def __init__(
        self, i2c=None, mode=BME280_OSAMPLE_8, address=BME280_I2CADDR, **kwargs
    ):
        if type(mode) is tuple and len(mode) == 3:
            self._mode_hum, self._mode_temp, self._mode_press = mode
        elif type(mode) == int:
            self._mode_hum, self._mode_temp, self._mode_press = mode, mode, mode
        else:
            raise ValueError(
                "Wrong type for the mode parameter, must be int or a 3 element tuple"
            )

        for mode in (self._mode_hum, self._mode_temp, self._mode_press):
            if mode not in [
                BME280_OSAMPLE_1,
                BME280_OSAMPLE_2,
                BME280_OSAMPLE_4,
                BME280_OSAMPLE_8,
                BME280_OSAMPLE_16,
            ]:
                raise ValueError(
                    "Unexpected mode value {0}. Set mode to one of "
                    "BME280_OSAMPLE_1, BME280_OSAMPLE_2, BME280_OSAMPLE_4, "
                    "BME280_OSAMPLE_8 or BME280_OSAMPLE_16".format(mode)
                )

        self.address = address
        if i2c is None:
            raise ValueError("An I2C object is required.")
        self.i2c = i2c
        self.__sealevel = 101325
        dig_88_a1 = self.i2c.readfrom_mem(self.address, 0x88, 26)
        dig_e1_e7 = self.i2c.readfrom_mem(self.address, 0xE1, 7)

        (
            self.dig_T1,
            self.dig_T2,
            self.dig_T3,
            self.dig_P1,
            self.dig_P2,
            self.dig_P3,
            self.dig_P4,
            self.dig_P5,
            self.dig_P6,
            self.dig_P7,
            self.dig_P8,
            self.dig_P9,
            _,
            self.dig_H1,
        ) = unpack("<HhhHhhhhhhhhBB", dig_88_a1)

        self.dig_H2, self.dig_H3, self.dig_H4, self.dig_H5, self.dig_H6 = unpack(
            "<hBbhb", dig_e1_e7
        )
        self.dig_H4 = (self.dig_H4 * 16) + (self.dig_H5 & 0xF)
        self.dig_H5 //= 16
        self._l1_barray = bytearray(1)
        self._l8_barray = bytearray(8)
        self._l3_resultarray = array("i", [0, 0, 0])

        self._l1_barray[0] = self._mode_temp << 5 | self._mode_press << 2 | MODE_SLEEP
        self.i2c.writeto_mem(self.address, BME280_REGISTER_CONTROL, self._l1_barray)
        self.t_fine = 0

    def read_raw_data(self, result):
        self._l1_barray[0] = self._mode_hum
        self.i2c.writeto_mem(self.address, BME280_REGISTER_CONTROL_HUM, self._l1_barray)
        self._l1_barray[0] = self._mode_temp << 5 | self._mode_press << 2 | MODE_FORCED
        self.i2c.writeto_mem(self.address, BME280_REGISTER_CONTROL, self._l1_barray)
        for _ in range(5):
            if self.i2c.readfrom_mem(self.address, BME280_REGISTER_STATUS, 1)[0] & 0x08:
                break
            time.sleep_ms(1)
        for _ in range(BME280_TIMEOUT):
            if self.i2c.readfrom_mem(self.address, BME280_REGISTER_STATUS, 1)[0] & 0x08:
                time.sleep_ms(10)
            else:
                break
        else:
            raise RuntimeError("Sensor BME280 not ready")
        self.i2c.readfrom_mem_into(self.address, 0xF7, self._l8_barray)
        readout = self._l8_barray
        raw_press = ((readout[0] << 16) | (readout[1] << 8) | readout[2]) >> 4
        raw_temp = ((readout[3] << 16) | (readout[4] << 8) | readout[5]) >> 4
        raw_hum = (readout[6] << 8) | readout[7]

        result[0] = raw_temp
        result[1] = raw_press
        result[2] = raw_hum

    def read_compensated_data(self, result=None):
        self.read_raw_data(self._l3_resultarray)
        raw_temp, raw_press, raw_hum = self._l3_resultarray
        var1 = (raw_temp / 16384.0 - self.dig_T1 / 1024.0) * self.dig_T2
        var2 = raw_temp / 131072.0 - self.dig_T1 / 8192.0
        var2 = var2 * var2 * self.dig_T3
        self.t_fine = int(var1 + var2)
        temp = (var1 + var2) / 5120.0
        temp = max(-40, min(85, temp))
        var1 = (self.t_fine / 2.0) - 64000.0
        var2 = var1 * var1 * self.dig_P6 / 32768.0 + var1 * self.dig_P5 * 2.0
        var2 = (var2 / 4.0) + (self.dig_P4 * 65536.0)
        var1 = (self.dig_P3 * var1 * var1 / 524288.0 + self.dig_P2 * var1) / 524288.0
        var1 = (1.0 + var1 / 32768.0) * self.dig_P1
        if var1 == 0.0:
            pressure = 30000
        else:
            p = ((1048576.0 - raw_press) - (var2 / 4096.0)) * 6250.0 / var1
            var1 = self.dig_P9 * p * p / 2147483648.0
            var2 = p * self.dig_P8 / 32768.0
            pressure = p + (var1 + var2 + self.dig_P7) / 16.0
            pressure = max(30000, min(110000, pressure))
        h = self.t_fine - 76800.0
        h = (raw_hum - (self.dig_H4 * 64.0 + self.dig_H5 / 16384.0 * h)) * (
            self.dig_H2
            / 65536.0
            * (
                1.0
                + self.dig_H6 / 67108864.0 * h * (1.0 + self.dig_H3 / 67108864.0 * h)
            )
        )
        humidity = h * (1.0 - self.dig_H1 * h / 524288.0)
        if humidity < 0:
            humidity = 0
        if humidity > 100:
            humidity = 100.0

        if result:
            result[0] = temp
            result[1] = pressure
            result[2] = humidity
            return result

        return array("f", (temp, pressure, humidity))

    def temperature(self):
        try:
            t, p, h = self.read_compensated_data()
            return t
        except Exception as e:
            print("Temperature measurement error:", e)
            return 0.0

    def relative_humidity(self):
        try:
            t, p, h = self.read_compensated_data()
            return h
        except Exception as e:
            print("Humidity measurement error:", e)
            return 0.0

    def atm_pressure(self):
        try:
            t, p, h = self.read_compensated_data()
            p_calc = p / 100
            return p_calc
        except Exception as e:
            print("Pressure measurement error:", e)
            return 0.0

    @property
    def sealevel(self):
        return self.__sealevel

    @sealevel.setter
    def sealevel(self, value):
        if 30000 < value < 120000:
            self.__sealevel = value

    @property
    def altitude(self):
        from math import pow

        try:
            p = 44330 * (
                1.0 - pow(self.read_compensated_data()[1] / self.__sealevel, 0.1903)
            )
        except:
            p = 0.0
        return p

    @property
    def dew_point(self):
        from math import log

        t, p, h = self.read_compensated_data()
        h = (log(h, 10) - 2) / 0.4343 + (17.62 * t) / (243.12 + t)
        return 243.12 * h / (17.62 - h)

    @property
    def values(self):
        t, p, h = self.read_compensated_data()

        return ("{:.2f}C".format(t), "{:.2f}hPa".format(p / 100), "{:.2f}%".format(h))
