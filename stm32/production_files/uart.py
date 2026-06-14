import serial
import time
import datetime


def crc8(data: bytes) -> int:
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x07
            else:
                crc <<= 1
            crc &= 0xFF
    return crc


ser = serial.Serial("COM10", 115200, timeout=1)
time.sleep(0.1)

# _______________________________________________________________________________
# BOOTLOADER PING test
# message = bytes([0xB2] + [0x00] + [0x00] * 61)
# ping = message + bytes([crc8(message)])

# ser.write(ping)
# response = ser.read(64)
# print("TX:", ping.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# _______________________________________________________________________________
# APPLICATION PING test
# message = bytes([0xB2] + [0x00] + [0x00] * 13)
# ping = message + bytes([crc8(message)])

# ser.write(ping)
# response = ser.read(16)
# print("TX:", ping.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

message = bytes([0xB2] + [0x00] + [0x01] + [0x00] + [0x00] * 11)
ping = message + bytes([crc8(message)])

ser.write(ping)
response = ser.read(16)
print("TX:", ping.hex(" "))
print("RX:", response.hex(" ") if response else "brak odpowiedzi")

message = bytes([0xB2] + [0x00] + [0x01] + [0x01] + [0x00] * 11)
ping = message + bytes([crc8(message)])

ser.write(ping)
response = ser.read(16)
print("TX:", ping.hex(" "))
print("RX:", response.hex(" ") if response else "brak odpowiedzi")


message = bytes([0xB2] + [0x00] + [0x01] + [0x02] + [0x00] * 11)
ping = message + bytes([crc8(message)])

ser.write(ping)
response = ser.read(16)
print("TX:", ping.hex(" "))
print("RX:", response.hex(" ") if response else "brak odpowiedzi")

message = bytes([0xB2] + [0x00] + [0x01] + [0x03] + [0x00] * 11)
ping = message + bytes([crc8(message)])

ser.write(ping)
response = ser.read(16)
print("TX:", ping.hex(" "))
print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# _______________________________________________________________________________
# INA226 test
# read_ina226 = bytes([0xB2, 0x00, 0x11, 0x00] + [0x00] * 12)
# led_pwm0 = bytes([0xB2, 0x00, 0x04, 0x02, 0xFF] + [0x00] * 11)
# led_pwm1 = bytes([0xB2, 0x00, 0x04, 0x02, 0x80] + [0x00] * 11)
# led_pwm2 = bytes([0xB2, 0x00, 0x04, 0x02, 0x40] + [0x00] * 11)
# led_pwm3 = bytes([0xB2, 0x00, 0x04, 0x02, 0x20] + [0x00] * 11)
# led_pwm4 = bytes([0xB2, 0x00, 0x04, 0x02, 0x00] + [0x00] * 11)


# def s16(x):
#     return x - 65536 if x & 0x8000 else x


# ser.write(read_ina226)
# response = ser.read(16)
# print("TX:", read_ina226.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# bus = (response[4] << 8) | response[5]
# cur = s16((response[6] << 8) | response[7])
# pwr = (response[8] << 8) | response[9]
# print(bus, "mV", cur, "mA", pwr, "mW")

# ser.write(led_pwm0)
# response = ser.read(16)
# print("TX:", led_pwm0.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# ser.write(read_ina226)
# response = ser.read(16)
# print("TX:", read_ina226.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# bus = (response[4] << 8) | response[5]
# cur = s16((response[6] << 8) | response[7])
# pwr = (response[8] << 8) | response[9]
# print(bus, "mV", cur, "mA", pwr, "mW")
# time.sleep(1)

# ser.write(led_pwm1)
# response = ser.read(16)
# print("TX:", led_pwm1.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# ser.write(read_ina226)
# response = ser.read(16)
# print("TX:", read_ina226.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# bus = (response[4] << 8) | response[5]
# cur = s16((response[6] << 8) | response[7])
# pwr = (response[8] << 8) | response[9]
# print(bus, "mV", cur, "mA", pwr, "mW")
# time.sleep(1)

# ser.write(led_pwm2)
# response = ser.read(16)
# print("TX:", led_pwm2.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# ser.write(read_ina226)
# response = ser.read(16)
# print("TX:", read_ina226.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# bus = (response[4] << 8) | response[5]
# cur = s16((response[6] << 8) | response[7])
# pwr = (response[8] << 8) | response[9]
# print(bus, "mV", cur, "mA", pwr, "mW")
# time.sleep(1)

# ser.write(led_pwm3)
# response = ser.read(16)
# print("TX:", led_pwm3.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# ser.write(read_ina226)
# response = ser.read(16)
# print("TX:", read_ina226.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# bus = (response[4] << 8) | response[5]
# cur = s16((response[6] << 8) | response[7])
# pwr = (response[8] << 8) | response[9]
# print(bus, "mV", cur, "mA", pwr, "mW")
# time.sleep(1)

# ser.write(led_pwm4)
# response = ser.read(16)
# print("TX:", led_pwm4.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# ser.write(read_ina226)
# response = ser.read(16)
# print("TX:", read_ina226.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# bus = (response[4] << 8) | response[5]
# cur = s16((response[6] << 8) | response[7])
# pwr = (response[8] << 8) | response[9]
# print(bus, "mV", cur, "mA", pwr, "mW")

# _______________________________________________________________________________
# RELAY test
# relay_on = bytes([0xB2, 0x00, 0x10, 0x01, 0x01] + [0x00] * 11)
# relay_off = bytes([0xB2, 0x00, 0x10, 0x01, 0x00] + [0x00] * 11)

# ser.write(relay_on)
# response = ser.read(16)
# print("TX:", relay_on.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(2)

# ser.write(relay_off)
# response = ser.read(16)
# print("TX:", relay_off.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# _______________________________________________________________________________
# PCF8563T RTC test
# rtc_set_time = bytes([0xB2, 0x00, 0x09, 0x00, 33, 9, 21, 3, 1, 10, 26] + [0x00] * 5)

message = bytes([0xB2] + [0x00] + [0x09] + [0x01] + [0x00] * 11)
rtc_get_time = message + bytes([crc8(message)])
# rtc_set_1hz_clock = bytes([0xB2, 0x00, 0x09, 0x02] + [0x00] * 12)
# rtc_disable_1hz_clock = bytes([0xB2, 0x00, 0x09, 0x03] + [0x00] * 12)

# ser.write(rtc_set_time)
# response = ser.read(16)
# print("TX:", rtc_set_time.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

ser.write(rtc_get_time)
response = ser.read(16)
print("TX:", rtc_get_time.hex(" "))
print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# uint8_t data[8];

# int pcf8563t_get_datetime(uint8_t *seconds, uint8_t *minutes, uint8_t *hours,
#                           uint8_t *day, uint8_t *weekday,
#                           uint8_t *month, uint8_t *year)
# pcf8563t_get_datetime(&data[0], &data[1], &data[2], &data[3],
#                     &data[4], &data[5], &data[6]);

# data[7] = pcf8563t_get_vl_flag();

year = response[10] + 2000
month = response[9]
weekday = response[8]
day = response[7]
hour = response[6]
minute = response[5]
second = response[4]
print(
    f"RTC: {year:04}-{month:02}-{day:02} (WD:{weekday}) {hour:02}:{minute:02}:{second:02}"
)

# ser.write(rtc_set_1hz_clock)
# response = ser.read(16)
# print("TX:", rtc_set_1hz_clock.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(5)

# ser.write(rtc_disable_1hz_clock)
# response = ser.read(16)
# print("TX:", rtc_disable_1hz_clock.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# _______________________________________________________________________________
# MCP23017 GPIO test

# write_GPIOB0_low_mcp23017 = bytes([0xB2, 0x00, 0x08, 0x00] + [0x00] * 12)
# write_GPIOB0_high_mcp23017 = bytes([0xB2, 0x00, 0x08, 0x01] + [0x00] * 12)

# ser.write(write_GPIOB0_high_mcp23017)
# response = ser.read(16)
# print("TX:", write_GPIOB0_high_mcp23017.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ser.write(write_GPIOB0_low_mcp23017)
# response = ser.read(16)
# print("TX:", write_GPIOB0_low_mcp23017.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# set_GPIOB1_mcp23017_input = bytes([0xB2, 0x00, 0x08, 0x02] + [0x00] * 12)
# read_GPIOB1_mcp23017_input = bytes([0xB2, 0x00, 0x08, 0x03] + [0x00] * 12)
# ser.write(set_GPIOB1_mcp23017_input)
# response = ser.read(16)
# print("TX:", set_GPIOB1_mcp23017_input.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(2)

# ser.write(read_GPIOB1_mcp23017_input)
# response = ser.read(16)
# print("TX:", read_GPIOB1_mcp23017_input.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# _______________________________________________________________________________
# EEPROM read/write test
# Adresy EEPROM: 0x0000 – 0x0FFF

# write_eeprom = bytes([0xB2, 0x00, 0x07, 0x00, 0x00, 0xAD, 0xBE, 0xEF] + [0x00] * 8)
# read_eeprom = bytes([0xB2, 0x00, 0x07, 0x01, 0x00, 0xAD] + [0x00] * 10)

# ser.write(write_eeprom)
# response = ser.read(16)
# print("TX:", write_eeprom.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ser.write(read_eeprom)
# response = ser.read(16)
# print("TX:", read_eeprom.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# _______________________________________________________________________________

# message = bytes([0xB2] + [0x00] + [0x01] + [0x00] * 12)
# serial_number = message + bytes([crc8(message)])
# adc_measurement = bytes([0xB2, 0x00, 0x02, 0x00] + [0x00] * 12)
# temp_measurement = bytes([0xB2, 0x00, 0x03, 0x00] + [0x00] * 12)
# led_on = bytes([0xB2, 0x00, 0x04, 0x01, 0x01] + [0x00] * 11)
# led_off = bytes([0xB2, 0x00, 0x04, 0x01, 0x00] + [0x00] * 11)
# led_pwm0 = bytes([0xB2, 0x00, 0x04, 0x02, 0xFF] + [0x00] * 11)
# led_pwm1 = bytes([0xB2, 0x00, 0x04, 0x02, 0x80] + [0x00] * 11)
# led_pwm2 = bytes([0xB2, 0x00, 0x04, 0x02, 0x40] + [0x00] * 11)
# led_pwm3 = bytes([0xB2, 0x00, 0x04, 0x02, 0x20] + [0x00] * 11)
# led_pwm4 = bytes([0xB2, 0x00, 0x04, 0x02, 0x00] + [0x00] * 11)
message = bytes([0xB2] + [0x00] + [0x05] + [0x00] * 12)
temp_measurement2 = message + bytes([crc8(message)])
# # where: YY - year, MM - month, DD - day, WD - weekday (1-7), hh - hour, mm - minute, ss - second
# # B2 00 06 01 YY MM DD WD hh mm ss 00 00 00 00 00
# rtc_get = bytes([0xB2, 0x00, 0x06, 0x00] + [0x00] * 12)

# year_bytes = (datetime.datetime.now().year - 2000).to_bytes(1, "big")
# month_bytes = (datetime.datetime.now().month).to_bytes(1, "big")
# day_bytes = (datetime.datetime.now().day).to_bytes(1, "big")
# weekday_bytes = (datetime.datetime.now().isoweekday()).to_bytes(1, "big")
# hour_bytes = (datetime.datetime.now().hour).to_bytes(1, "big")
# minute_bytes = (datetime.datetime.now().minute).to_bytes(1, "big")
# second_bytes = (datetime.datetime.now().second).to_bytes(1, "big")

# rtc_set = bytes(
#     [0xB2, 0x00, 0x06, 0x01, year_bytes[0], month_bytes[0], day_bytes[0], weekday_bytes[0], hour_bytes[0], minute_bytes[0], second_bytes[0]] + [0x00] * 5
# )

# ser.write(rtc_set)
# response = ser.read(16)
# print("TX:", rtc_set.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ser.write(led_on)
# response = ser.read(16)
# print("TX:", led_on.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(1)
# ser.write(led_off)
# response = ser.read(16)
# print("TX:", led_off.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ser.write(led_pwm0)
# response = ser.read(16)
# print("TX:", led_pwm0.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(1)
# ser.write(led_pwm1)
# response = ser.read(16)
# print("TX:", led_pwm1.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(1)
# ser.write(led_pwm2)
# response = ser.read(16)
# print("TX:", led_pwm2.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(1)
# ser.write(led_pwm3)
# response = ser.read(16)
# print("TX:", led_pwm3.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# time.sleep(1)
# ser.write(led_pwm4)
# response = ser.read(16)
# print("TX:", led_pwm4.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ser.write(serial_number)
# response = ser.read(16)
# print("TX:", serial_number.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ser.write(adc_measurement)
# response = ser.read(16)
# print("TX:", adc_measurement.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# ch_0 = int.from_bytes(response[4:6], "big", signed=False)
# ch_1 = int.from_bytes(response[6:8], "big", signed=False)
# v_ch0 = ch_0 * 3.3 / 4095.0
# v_ch1 = ch_1 * 3.3 / 4095.0
# print(f"ADC CH0: {ch_0} / {v_ch0:.3f}V")
# print(f"ADC CH1: {ch_1} / {v_ch1:.3f}V")


# ser.write(temp_measurement)
# response = ser.read(16)
# print("TX:", temp_measurement.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")

# temp_raw = (response[4] << 8) | response[5]
# rh_raw = (response[6] << 8) | response[7]
# temp_c = temp_raw / 100.0
# rh = rh_raw / 100.0

# print(f"Temperatura: {temp_c:.2f}°C")
# print(f"Wilgotność:  {rh:.2f}%rH")

ser.write(temp_measurement2)
response = ser.read(16)
print("TX:", temp_measurement2.hex(" "))
print("RX:", response.hex(" ") if response else "brak odpowiedzi")

temp_c = int.from_bytes(response[4:8], "big", signed=True) / 100.0
hum = int.from_bytes(response[8:12], "big") / 1024.0
press = int.from_bytes(response[12:16], "big") / 256.0 / 100.0

print(f"Temperatura: {temp_c:.2f}°C")
print(f"Wilgotność:  {hum:.2f}%rH")
print(f"Ciśnienie:   {press:.2f}hPa")

# ser.write(rtc_get)
# response = ser.read(16)
# print("TX:", rtc_get.hex(" "))
# print("RX:", response.hex(" ") if response else "brak odpowiedzi")
# year = response[4] + 2000
# month = response[5]
# day = response[6]
# weekday = response[7]
# hour = response[8]
# minute = response[9]
# second = response[10]
# print(
#     f"RTC: {year:04}-{month:02}-{day:02} (WD:{weekday}) {hour:02}:{minute:02}:{second:02}"
# )


ser.close()


# RESET CAUSE b2 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00
# CLEAR RESET FLAG b2 00 07 01 00 00 00 00 00 00 00 00 00 00 00 00
# IWDG START (presc=4, reaload=0x0FFF) b2 00 07 10 04 0f ff 00 00 00 00 00 00 00 00 00
# IWDG KICK b2 00 07 11 00 00 00 00 00 00 00 00 00 00 00 00
# WWDG START (presc=3, window=0x60, counter=0x7F) b2 00 07 20 03 60 7f 00 00 00 00 00 00 00 00 00
# WWDG KICK (counter=0x7F) b2 00 07 21 7f 00 00 00 00 00 00 00 00 00 00 00
