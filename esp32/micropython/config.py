class Config:
    def __init__(self):
        self.API_KEY = ""
        self.DEVICE_ID = -1
        self.SERIAL_NUMBER = ""

        self.WIFI_ENABLED = True
        self.WIFI_SSID = ""
        self.WIFI_PASSWORD = ""

        self.NTP_SERVER_IP = ""

        self.MQTT_SERVER = ""
        self.MQTT_PORT = 1883
        self.MQTT_USER = ""
        self.MQTT_PASSWORD = ""
        self.MQTT_KEEPALIVE = 60

        self.HW_VERSION = 0
        self.SW_VERSION = 0

        self.STATUS_GPIO = 18
        self.STM32_GPIO = 19
        self.LED_GPIO = 22

        self.DEV_ADDR = 0xB2
        self.FRAME_LEN_APP = 32
        self.STATUS_OK = 0x40
        self.STATUS_ERR = 0x7F

        self.FRAM_ADDR_DEVICE_ID = 0x008
        self.FRAM_ADDR_MEASURE_INTERVAL = 0x128

        self.WIFI_RECONNECT_MS = 5000
        self.MQTT_RECONNECT_MS = 5000
        self.WIFI_CONNECT_TIMEOUT = 5
        self.DEBOUNCE_IRQ_MS = 50
        self.POLL_INTERVAL_MS = 10
        self.NO_WIFI_SLEEP_MS = 200
        self.RTC_SYNC_MAX_RETRIES = 10

        self.FRAM_FLAG_EXT_RTC_PRESENT = 0
        self.FRAM_FLAG_FLASH_PRESENT = 0
        self.FRAM_FLAG_DISPLAY_PRESENT = 0
        self.FRAM_FLAG_SHT40_PRESENT = 0
        self.FRAM_FLAG_BME280_PRESENT = 0
        self.FRAM_FLAG_INA226_PRESENT = 0
        self.FRAM_FLAG_ADC_PRESENT = 0
        self.FRAM_FLAG_CAN_PRESENT = 0

    def get_ext_rtc_present(self):
        return self.FRAM_FLAG_EXT_RTC_PRESENT

    def get_flash_present(self):
        return self.FRAM_FLAG_FLASH_PRESENT

    def get_display_present(self):
        return self.FRAM_FLAG_DISPLAY_PRESENT

    def get_sht40_present(self):
        return self.FRAM_FLAG_SHT40_PRESENT

    def get_bme280_present(self):
        return self.FRAM_FLAG_BME280_PRESENT

    def get_ina226_present(self):
        return self.FRAM_FLAG_INA226_PRESENT

    def get_adc_present(self):
        return self.FRAM_FLAG_ADC_PRESENT

    def get_can_present(self):
        return self.FRAM_FLAG_CAN_PRESENT

    def set_ext_rtc_present(self, present):
        self.FRAM_FLAG_EXT_RTC_PRESENT = present

    def set_flash_present(self, present):
        self.FRAM_FLAG_FLASH_PRESENT = present

    def set_display_present(self, present):
        self.FRAM_FLAG_DISPLAY_PRESENT = present

    def set_sht40_present(self, present):
        self.FRAM_FLAG_SHT40_PRESENT = present

    def set_bme280_present(self, present):
        self.FRAM_FLAG_BME280_PRESENT = present

    def set_ina226_present(self, present):
        self.FRAM_FLAG_INA226_PRESENT = present

    def set_adc_present(self, present):
        self.FRAM_FLAG_ADC_PRESENT = present

    def set_can_present(self, present):
        self.FRAM_FLAG_CAN_PRESENT = present

    def set_hw_version(self, hw_version):
        self.HW_VERSION = hw_version

    def set_sw_version(self, sw_version):
        self.SW_VERSION = sw_version

    def get_hw_version(self):
        return self.HW_VERSION

    def get_sw_version(self):
        return self.SW_VERSION

    def is_wifi_enabled(self):
        return self.WIFI_ENABLED

    def set_wifi_enabled(self, enabled):
        self.WIFI_ENABLED = enabled

    def get_serial_number(self):
        return self.SERIAL_NUMBER

    def set_serial_number(self, serial_number):
        self.SERIAL_NUMBER = serial_number

    def get_api_key(self):
        return self.API_KEY

    def set_api_key(self, api_key):
        self.API_KEY = api_key

    def get_device_id(self):
        return self.DEVICE_ID

    def set_device_id(self, device_id):
        self.DEVICE_ID = device_id

    def get_wifi_ssid(self):
        return self.WIFI_SSID

    def set_wifi_ssid(self, ssid):
        self.WIFI_SSID = ssid

    def get_wifi_password(self):
        return self.WIFI_PASSWORD

    def set_wifi_password(self, password):
        self.WIFI_PASSWORD = password

    def get_ntp_server_ip(self):
        return self.NTP_SERVER_IP

    def set_ntp_server_ip(self, ntp_server_ip):
        self.NTP_SERVER_IP = ntp_server_ip

    def get_mqtt_server(self):
        return self.MQTT_SERVER

    def set_mqtt_server(self, mqtt_server):
        self.MQTT_SERVER = mqtt_server

    def get_mqtt_port(self):
        return self.MQTT_PORT

    def set_mqtt_port(self, mqtt_port):
        self.MQTT_PORT = mqtt_port

    def get_mqtt_user(self):
        return self.MQTT_USER

    def set_mqtt_user(self, mqtt_user):
        self.MQTT_USER = mqtt_user

    def get_mqtt_password(self):
        return self.MQTT_PASSWORD

    def set_mqtt_password(self, mqtt_password):
        self.MQTT_PASSWORD = mqtt_password

    def get_mqtt_keepalive(self):
        return self.MQTT_KEEPALIVE

    def get_status_gpio(self):
        return self.STATUS_GPIO

    def get_stm32_gpio(self):
        return self.STM32_GPIO

    def get_led_gpio(self):
        return self.LED_GPIO

    def get_dev_addr(self):
        return self.DEV_ADDR

    def get_frame_len_app(self):
        return self.FRAME_LEN_APP

    def get_status_ok(self):
        return self.STATUS_OK

    def get_status_err(self):
        return self.STATUS_ERR

    def get_fram_addr_device_id(self):
        return self.FRAM_ADDR_DEVICE_ID

    def get_fram_addr_measure_interval(self):
        return self.FRAM_ADDR_MEASURE_INTERVAL

    def get_wifi_reconnect_ms(self):
        return self.WIFI_RECONNECT_MS

    def get_mqtt_reconnect_ms(self):
        return self.MQTT_RECONNECT_MS

    def get_wifi_connect_timeout(self):
        return self.WIFI_CONNECT_TIMEOUT

    def get_debounce_irq_ms(self):
        return self.DEBOUNCE_IRQ_MS

    def get_poll_interval_ms(self):
        return self.POLL_INTERVAL_MS

    def get_no_wifi_sleep_ms(self):
        return self.NO_WIFI_SLEEP_MS

    def get_rtc_sync_max_retries(self):
        return self.RTC_SYNC_MAX_RETRIES


_cfg_singleton = None


def get_config():
    global _cfg_singleton
    if _cfg_singleton is None:
        _cfg_singleton = Config()
    return _cfg_singleton
