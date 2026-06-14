import time

PCF8563_I2C_ADDR = 0x51
REG_CTRL1 = 0x00
REG_CTRL2 = 0x01
REG_SECONDS = 0x02
REG_MINUTES = 0x03
REG_HOURS = 0x04
REG_DAY = 0x05
REG_WEEKDAY = 0x06
REG_MONTH = 0x07
REG_YEAR = 0x08
REG_ALRM_MIN = 0x09
REG_ALRM_HOUR = 0x0A
REG_ALRM_DAY = 0x0B
REG_ALRM_WDAY = 0x0C
REG_CLKOUT = 0x0D


class RTC_Clock:
    def __init__(self, i2c):
        if i2c is None:
            raise ValueError("No I2C argument!")
        self.__i2c = i2c
        try:
            self.__i2c.writeto_mem(PCF8563_I2C_ADDR, REG_CTRL1, bytes([0x00]))
            self.__i2c.writeto_mem(PCF8563_I2C_ADDR, REG_CTRL2, bytes([0x00]))
            self.set_clockout_1hz(True)
        except Exception as e:
            print(f"RTC initialization error: {e}")
            raise

    def __bcd2dec(self, val):
        return ((val >> 4) * 10) + (val & 0x0F)

    def __dec2bcd(self, val):
        return ((val // 10) << 4) | (val % 10)

    def set_clockout_1hz(self, state):
        val = (0x80 | 0x03) if state else 0x00
        try:
            self.__i2c.writeto_mem(PCF8563_I2C_ADDR, REG_CLKOUT, bytes([val]))
        except Exception as e:
            print("Clock output setting error:", e)

    def set_time(self, datetime=None):
        try:
            if datetime is None:
                raise ValueError("No datetime argument!")
            if len(datetime) < 7:
                raise ValueError(
                    "Invalid datetime argument! Expected (year, month, day, weekday, hour, minute, second)"
                )

            year, month, day, weekday, hour, minute, second = datetime

            if year < 2000 or year > 2099:
                raise ValueError("Year out of range (2000-2099)")
            if month < 1 or month > 12:
                raise ValueError("Month out of range (1-12)")
            if day < 1 or day > 31:
                raise ValueError("Day out of range (1-31)")
            if weekday < 0 or weekday > 6:
                raise ValueError("Weekday out of range (0-6)")
            if hour < 0 or hour > 23:
                raise ValueError("Hour out of range (0-23)")
            if minute < 0 or minute > 59:
                raise ValueError("Minute out of range (0-59)")
            if second < 0 or second > 59:
                raise ValueError("Second out of range (0-59)")
            year_bcd = year - 2000
            data = bytearray(7)
            data[0] = self.__dec2bcd(second) & 0x7F
            data[1] = self.__dec2bcd(minute) & 0x7F
            data[2] = self.__dec2bcd(hour) & 0x3F
            data[3] = self.__dec2bcd(day) & 0x3F
            data[4] = weekday & 0x07
            data[5] = self.__dec2bcd(month) & 0x1F

            year2 = year
            if year2 >= 2000:
                year2 -= 2000
            else:
                data[5] |= 0x80
                year2 = (year2 >= 1900) if year2 - 1900 else 0
            data[6] = self.__dec2bcd(year2)
            self.__i2c.writeto_mem(PCF8563_I2C_ADDR, REG_SECONDS, data)
        except Exception as e:
            raise

    def read_time(self):
        try:
            raw = self.__i2c.readfrom_mem(PCF8563_I2C_ADDR, REG_SECONDS, 7)
            second = self.__bcd2dec(raw[0] & 0x7F)
            minute = self.__bcd2dec(raw[1] & 0x7F)
            hour = self.__bcd2dec(raw[2] & 0x3F)
            day = self.__bcd2dec(raw[3] & 0x3F)
            weekday = self.__bcd2dec(raw[4] & 0x07)
            month = self.__bcd2dec(raw[5] & 0x1F)
            if raw[5] & 0x80:
                year = self.__bcd2dec(raw[6]) + 1900
            else:
                year = self.__bcd2dec(raw[6]) + 2000
            return (year, month, day, hour, minute, second)
        except Exception as e:
            print(f"Time reading error: {e}")
            return (2000, 1, 1, 0, 0, 0, 0)

    def is_clock_valid(self):
        try:
            raw = self.__i2c.readfrom_mem(PCF8563_I2C_ADDR, REG_SECONDS, 1)
            return not bool(raw[0] & 0x80)
        except Exception as e:
            print(f"Clock validity check error: {e}")
            return False

    def _get_last_sunday_date(self, year, month):
        days_in_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

        if year % 4 == 0 and (year % 100 != 0 or year % 400 == 0):
            days_in_month[1] = 29
        day = days_in_month[month - 1]
        while day >= 1:
            m = month if month > 2 else month + 12
            y = year if month > 2 else year - 1
            q = day
            K = y % 100
            J = y // 100
            h = (q + ((13 * (m + 1)) // 5) + K + (K // 4) + (J // 4) - 2 * J) % 7
            if h == 1:
                return day
            day -= 1

        return 1

    def is_dst_active(self):
        try:
            year, month, day, hour, minute, second = self.read_time()

            if month < 3 or month > 10:
                return False
            if 3 < month < 10:
                return True
            if month == 3:
                last_sunday = self._get_last_sunday_date(year, 3)
                if day > last_sunday:
                    return True
                elif day == last_sunday and hour >= 2:
                    return True
                return False
            if month == 10:
                last_sunday = self._get_last_sunday_date(year, 10)
                if day < last_sunday:
                    return True
                elif day == last_sunday and hour < 3:
                    return True
                return False

            return False
        except Exception as e:
            print(f"DST check error: {e}")
            return False

    def get_timezone_offset(self):
        if self.is_dst_active():
            return 2
        else:
            return 1

    def get_formatted_time(self):
        try:
            year, month, day, hour, minute, second = self.read_time()
            return (
                f"{year:04d}-{month:02d}-{day:02d} {hour:02d}:{minute:02d}:{second:02d}"
            )
        except Exception as e:
            print(f"Time formatting error: {e}")
            return "Invalid time"

    def set_time_system(self):
        try:
            aT = time.localtime(time.time())
            year = aT[0]
            month = aT[1]
            day = aT[2]
            hour = aT[3]
            minute = aT[4]
            second = aT[5]
            self.set_time((year, month, day, 0, hour, minute, second))
        except Exception as e:
            print("Time setting error:", e)


def apply_timezone_offset(utc_time, offset_hours):
    year, month, day, hour, minute, second = utc_time

    total_hours = hour + offset_hours

    if total_hours >= 24:
        hour = total_hours - 24
        day += 1
        days_in_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
        if year % 4 == 0 and (year % 100 != 0 or year % 400 == 0):
            days_in_month[1] = 29
        if day > days_in_month[month - 1]:
            day = 1
            month += 1
            if month > 12:
                month = 1
                year += 1
    elif total_hours < 0:
        hour = total_hours + 24
        day -= 1
        if day < 1:
            month -= 1
            if month < 1:
                month = 12
                year -= 1
            days_in_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
            if year % 4 == 0 and (year % 100 != 0 or year % 400 == 0):
                days_in_month[1] = 29
            day = days_in_month[month - 1]
    else:
        hour = total_hours

    return (year, month, day, hour, minute, second)
