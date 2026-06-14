import time, ntptime, sys, uselect
from machine import UART
from wireless import WiFi
from stm32_uart import STM32UART
from program import Program
from config import get_config
import esp

esp.osdebug(None)

cfg = get_config()


def main():
    program = Program()

    uart_device = UART(1, baudrate=115200, tx=20, rx=21, timeout=50, timeout_char=10)
    stm32 = STM32UART(uart_device)
    program.stm32 = stm32

    poll = uselect.poll()
    poll.register(sys.stdin, uselect.POLLIN)

    while True:
        now = time.ticks_ms()

        data = uart_device.read(1)
        print("Read from UART: {}".format(data))
        events = poll.poll(0)
        if events:
            line = sys.stdin.readline()
            if line:
                line = line.strip()
                print(line)
                # program.read_usb(line)

        # fw, hw = stm32.req_fw_hw_version()
        # build_data = stm32.req_build_date()
        # prod_date = stm32.req_prod_date()
        # print(
        #     "FW: {}, HW: {}, Build: {}, Prod: {}".format(fw, hw, build_data, prod_date)
        # )

        # date = stm32.req_rtc_read()
        # print("RTC: {}".format(date))

        # t, h = stm32.req_sht40()
        # print("Temp: {}, Hum: {}".format(t, h))

        # tb, hb, pb = stm32.req_bme280()
        # print("Temp: {}, Hum: {}, Press: {}".format(tb, hb, pb))

        # v0, v1 = stm32.req_adc()
        # v0_voltage = stm32.adc_to_voltage(v0)
        # v1_voltage = stm32.adc_to_voltage(v1)
        # vadc0 = stm32.vadc_to_vin(v0_voltage)
        # vadc1 = stm32.vadc_to_vin(v1_voltage)
        # print(
        #     "ADC: {}, {} ({}V, {}V) -> Vin: {}V, {}V".format(
        #         v0, v1, v0_voltage, v1_voltage, vadc0, vadc1
        #     )
        # )
        # date = stm32.req_rtc_read()
        # print("RTC: {}".format(date))

        # stm32.req_set_rgb(255, 0, 0, 100)
        # time.sleep(0.5)
        # stm32.req_set_rgb(0, 255, 0, 80)
        # time.sleep(0.5)
        # stm32.req_set_rgb(0, 0, 255, 50)
        # time.sleep(0.5)
        # stm32.req_set_rgb(0, 0, 0, 0)

        # stm32.req_set_buzzer(1000, 100)
        # time.sleep(0.5)
        # stm32.req_set_buzzer(2000, 80)
        # time.sleep(0.5)
        # stm32.req_set_buzzer(3000, 50)
        # time.sleep(0.5)
        # stm32.req_set_buzzer(0, 0)
        # time.sleep(0.5)

        time.sleep(cfg.get_poll_interval_ms() / 1000.0)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("Fatal error: {}".format(e))
