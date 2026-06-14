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

        self.SERVER_IP = ""
        self.SERVER_PORT = 8080

        self.POST_TIME = 10

        self.HW_VERSION = 0
        self.SW_VERSION = 0

        self.TEMPERATURE = 1
        self.HUMIDITY = 1
        self.PRESSURE = 1
        self.SHT = 0
        self.CLOCK = 1
        self.LOGGING_ENABLED = 1
        self.SET_TIME = 1

        self.RELAY_VERSION = False
        self.RELAY1_PIN = 12
        self.RELAY2_PIN = 13
        self.RELAY3_PIN = 14
        self.RELAY4_PIN = 15

        self.FRAME_HEADER = b"\xaa\xff\x03\x00"
        self.FRAME_END = b"\x55\xcc"
        self.FRAME_LEN = 30

        self.LED_PINS = [19, 20, 18]
        if self.RELAY_VERSION:
            self.LED_PINS = [7, 8, 6]

        self.BUZZER_PIN = 11
        if self.RELAY_VERSION:
            self.BUZZER_PIN = 13

        self.SWITCH1_PIN = 17
        self.SWITCH2_PIN = 16
        if self.RELAY_VERSION:
            self.SWITCH1_PIN = 21
            self.SWITCH2_PIN = 20

        self.WIFI_RECONNECT_MS = 5000
        self.MQTT_RECONNECT_MS = 5000
        self.WIFI_CONNECT_TIMEOUT = 5
        self.DEBOUNCE_IRQ_MS = 50
        self.POLL_INTERVAL_MS = 10
        self.NO_WIFI_SLEEP_MS = 200
        self.RTC_SYNC_MAX_RETRIES = 10

        self.I2C_ID = 0
        self.I2C_SDA = 0
        self.I2C_SCL = 1
        self.I2C_FREQ = 100000
        self.LCD_ADDR = 0x27
        self.LCD_ROWS = 2
        self.LCD_COLS = 16

    def get_frame_header(self):
        return self.FRAME_HEADER

    def get_frame_end(self):
        return self.FRAME_END

    def get_frame_len(self):
        return self.FRAME_LEN

    def set_api_key(self, api_key):
        self.API_KEY = api_key

    def set_device_id(self, device_id):
        self.DEVICE_ID = device_id

    def set_serial_number(self, serial_number):
        self.SERIAL_NUMBER = serial_number

    def set_wifi_enabled(self, enabled):
        self.WIFI_ENABLED = enabled

    def set_wifi_config(self, ssid, password):
        self.WIFI_SSID = ssid
        self.WIFI_PASSWORD = password

    def set_ntp_server_ip(self, ip):
        self.NTP_SERVER_IP = ip

    def set_mqtt_config(self, server, port, user, password, keepalive):
        self.MQTT_SERVER = server
        self.MQTT_PORT = port
        self.MQTT_USER = user
        self.MQTT_PASSWORD = password
        self.MQTT_KEEPALIVE = keepalive

    def set_server_config(self, ip, port):
        self.SERVER_IP = ip
        self.SERVER_PORT = port

    def set_post_time(self, post_time):
        self.POST_TIME = post_time

    def set_hw_version(self, hw_version):
        self.HW_VERSION = hw_version

    def set_sw_version(self, sw_version):
        self.SW_VERSION = sw_version

    def set_temperature_enabled(self, enabled):
        self.TEMPERATURE = enabled

    def set_humidity_enabled(self, enabled):
        self.HUMIDITY = enabled

    def set_pressure_enabled(self, enabled):
        self.PRESSURE = enabled

    def set_sht_type(self, sht_type):
        self.SHT = sht_type

    def set_clock_enabled(self, enabled):
        self.CLOCK = enabled

    def set_logging_enabled(self, enabled):
        self.LOGGING_ENABLED = enabled

    def set_set_time_enabled(self, enabled):
        self.SET_TIME = enabled

    def get_api_key(self):
        return self.API_KEY

    def get_device_id(self):
        return self.DEVICE_ID

    def get_serial_number(self):
        return self.SERIAL_NUMBER

    def is_wifi_enabled(self):
        return self.WIFI_ENABLED

    def get_ntp_server_ip(self):
        return self.NTP_SERVER_IP

    def get_ssid(self):
        return self.WIFI_SSID

    def get_password(self):
        return self.WIFI_PASSWORD

    def get_server_config(self):
        return self.SERVER_IP, self.SERVER_PORT

    def get_post_time(self):
        return self.POST_TIME

    def get_hw_version(self):
        return self.HW_VERSION

    def get_sw_version(self):
        return self.SW_VERSION

    def is_temperature_enabled(self):
        return self.TEMPERATURE

    def is_humidity_enabled(self):
        return self.HUMIDITY

    def is_pressure_enabled(self):
        return self.PRESSURE

    def get_sht_type(self):
        return self.SHT

    def is_clock_enabled(self):
        return self.CLOCK

    def is_logging_enabled(self):
        return self.LOGGING_ENABLED

    def is_set_time_enabled(self):
        return self.SET_TIME

    def is_relay_version(self):
        return self.RELAY_VERSION

    def get_relay_pins(self):
        return self.RELAY1_PIN, self.RELAY2_PIN, self.RELAY3_PIN, self.RELAY4_PIN

    def get_led_pins(self):
        return self.LED_PINS

    def get_buzzer_pin(self):
        return self.BUZZER_PIN

    def get_switch_pins(self):
        return self.SWITCH1_PIN, self.SWITCH2_PIN

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

    def get_mqtt_server(self):
        return self.MQTT_SERVER

    def get_mqtt_port(self):
        return self.MQTT_PORT

    def get_mqtt_user(self):
        return self.MQTT_USER

    def get_mqtt_password(self):
        return self.MQTT_PASSWORD

    def get_mqtt_keepalive(self):
        return self.MQTT_KEEPALIVE

    def get_i2c_config(self):
        return self.I2C_ID, self.I2C_SDA, self.I2C_SCL, self.I2C_FREQ

    def get_lcd_config(self):
        return self.LCD_ADDR, self.LCD_ROWS, self.LCD_COLS


_cfg_singleton = None


def get_config():
    global _cfg_singleton
    if _cfg_singleton is None:
        _cfg_singleton = Config()
    return _cfg_singleton
