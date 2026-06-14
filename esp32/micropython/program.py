import machine, time, ujson, os
from machine import RTC, Pin
from mqtt_simple import MQTTClient
from config import get_config
from nvs_system import NVSSystem

cfg = get_config()
nvs_sys = NVSSystem()


class Program:
    def __init__(self):
        self.client = None
        self.status_data = {}
        self.load_data()
        self.device_id = ""
        self.topic_status = b""
        self.ip_address = None
        self.stm32_gpio = Pin(cfg.get_stm32_gpio(), Pin.OUT)
        self.led = Pin(cfg.get_led_gpio(), Pin.OUT)
        self.pending_read = False
        self.pending_restart = False
        self.irq_dropped = 0
        self.last_irq_ms = 0
        self.status = Pin(cfg.get_status_gpio(), Pin.IN, Pin.PULL_DOWN)
        self.status.irq(
            handler=self._irq_status,
            trigger=Pin.IRQ_RISING,
        )
        self.rtc = RTC()
        self.stm32 = None
        self.errors = {}
        self.wifi = None
        self.start_ticks_ms = time.ticks_ms()

        self.input_channels = [
            (0x01, "BTN1"),
            (0x02, "BTN2"),
            (0x03, "ESP32_STATUS"),
        ]

        self.output_channels = [
            (0x01, "LED1"),
            (0x02, "LED2"),
            (0x03, "PB12"),
            (0x04, "PC0"),
            (0x05, "PC1"),
            (0x06, "PC2"),
            (0x07, "PC3"),
            (0x08, "ESP32_STATUS"),
        ]

        self.pwm_channels = [
            (0x01, "TIM1_CH1"),
            (0x02, "TIM2_CH3"),
            (0x03, "TIM4_CH3"),
            (0x04, "TIM4_CH4"),
        ]

    def _irq_status(self, pin):
        now = time.ticks_ms()
        if self.pending_read:
            self.irq_dropped += 1
            return
        if time.ticks_diff(now, self.last_irq_ms) < cfg.get_debounce_irq_ms():
            return
        self.last_irq_ms = now
        self.pending_read = True

    def _get_service_ymd(self):
        try:
            rtc = self.stm32.req_rtc_read()
            if rtc and len(rtc) >= 10:
                return rtc[0:4] + rtc[5:7] + rtc[8:10]
        except Exception:
            pass

        dt = time.localtime()
        return "{:04d}{:02d}{:02d}".format(dt[0], dt[1], dt[2])

    def set_wifi(self, wifi):
        self.wifi = wifi

    def sync_ids_from_stm32_fram(self):
        if self.stm32 is None:
            return

        cfg_data = self.stm32.read_fram_config()
        if cfg_data.get("device_id"):
            self.status_data["device_id"] = cfg_data["device_id"]

        flags = self.stm32.req_fram_read_flags()

        print(flags)
        cfg.set_ext_rtc_present(flags.get("FRAM_FLAG_EXT_RTC_PRESENT", 0) == 1)
        cfg.set_flash_present(flags.get("FRAM_FLAG_FLASH_PRESENT", 0) == 1)
        cfg.set_display_present(flags.get("FRAM_FLAG_DISPLAY_PRESENT", 0) == 1)
        cfg.set_sht40_present(flags.get("FRAM_FLAG_SHT40_PRESENT", 0) == 1)
        cfg.set_bme280_present(flags.get("FRAM_FLAG_BME280_PRESENT", 0) == 1)
        cfg.set_ina226_present(flags.get("FRAM_FLAG_INA226_PRESENT", 0) == 1)
        cfg.set_adc_present(flags.get("FRAM_FLAG_ADC_PRESENT", 0) == 1)
        cfg.set_can_present(flags.get("FRAM_FLAG_CAN_PRESENT", 0) == 1)

        try:
            serial = self.stm32.req_serial()
            cfg.set_serial_number(serial)
            self.status_data["serial_number"] = serial
        except Exception as e:
            self.errors["SERIAL_READ"] = str(e)

        self.save_data()
        self.apply_config()

    def apply_config(self):
        device_id = self.status_data.get("device_id", "")
        wifi_ssid = self.status_data.get("wifi_ssid", "")
        mqtt_server = self.status_data.get("mqtt_server", "")
        ntp_server = self.status_data.get("ntp_server", "")
        wifi_enabled = self.status_data.get("wifi_enabled", 0)
        hw_version = self.status_data.get("hw_version", "")
        sw_version = self.status_data.get("sw_version", "")
        serial_number = self.status_data.get("serial_number", "")

        wifi_password = nvs_sys.get_secret("wifi_password", "")
        api_key = nvs_sys.get_secret("api_key", "")
        mqtt_user = nvs_sys.get_secret("mqtt_username", "")
        mqtt_password = nvs_sys.get_secret("mqtt_password", "")

        cfg.set_device_id(device_id)
        cfg.set_wifi_enabled(wifi_enabled)
        cfg.set_wifi_ssid(wifi_ssid)
        cfg.set_wifi_password(wifi_password)
        cfg.set_mqtt_server(mqtt_server)
        cfg.set_ntp_server_ip(ntp_server)
        cfg.set_mqtt_user(mqtt_user)
        cfg.set_mqtt_password(mqtt_password)
        cfg.set_api_key(api_key)
        cfg.set_hw_version(hw_version)
        cfg.set_sw_version(sw_version)
        cfg.set_serial_number(serial_number)

        self.device_id = str(device_id)
        self.topic_status = "devices/{}/status".format(self.device_id).encode()

    def get_client(self):
        return self.client

    def set_client(self, client):
        self.client = client

    def set_ip_address(self, ip_address):
        self.ip_address = ip_address

    def _get_base_payload(self):
        return {
            "device_id": self.device_id,
            "ip_address": self.ip_address,
            "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms) // 1000,
            "rssi": self.wifi.get_rssi() if self.wifi else None,
            "sw_version": cfg.get_sw_version(),
            "hw_version": cfg.get_hw_version(),
        }

    def mask_secret(self, value):
        if not value:
            return ""
        if len(value) <= 2:
            return "*" * len(value)
        return value[0] + ("*" * (len(value) - 2)) + value[-1]

    def restart_connections(self):
        c = self.get_client()
        if c is not None:
            try:
                c.disconnect()
            except (OSError, Exception):
                pass
            self.set_client(None)

        if self.wifi is not None:
            try:
                self.wifi.disconnect()
            except (OSError, Exception):
                pass
            try:
                self.wifi.deinit()
            except (OSError, Exception):
                pass

        self.wifi = None

    def mqtt_initialization(self):
        if self.client is not None:
            return self.client

        mqtt_user = cfg.get_mqtt_user()
        mqtt_password = cfg.get_mqtt_password()

        user = mqtt_user.encode("utf-8") if mqtt_user else None
        password = mqtt_password.encode("utf-8") if mqtt_password else None

        self.client = MQTTClient(
            client_id=str(self.device_id).encode("utf-8"),
            server=cfg.get_mqtt_server(),
            port=cfg.get_mqtt_port(),
            user=user,
            password=password,
            keepalive=cfg.get_mqtt_keepalive(),
        )

        self.client.set_callback(self.message_mqtt)
        self.client.connect()
        self.client.subscribe("devices/{}/cmd".format(self.device_id).encode("utf-8"))
        return self.client

    def send_status(self, result, typeData="STATUS", info=None, timestamp=None):
        payload = {
            "result": result,
            "info": info,
            "type": typeData,
            "timestamp": time.time() if timestamp is None else timestamp,
        }
        if self.client is None:
            raise Exception("MQTT client is not connected")
        self.client.publish(self.topic_status, ujson.dumps(payload).encode())

    def _find_channel(self, channels, name):
        for ch_id, ch_name in channels:
            if ch_name == name:
                return ch_id
        return None

    def message_mqtt(self, topic, msg):
        try:
            data = ujson.loads(msg)
            cmd = data.get("cmd")
            params = data.get("params", {})

            if cmd == "PING":
                self.send_status(
                    "ALIVE",
                    "STATUS",
                    self._get_base_payload(),
                )
            elif cmd == "RESET":
                self.send_status(
                    "RESET",
                    "STATUS",
                    self._get_base_payload(),
                )
                time.sleep(0.1)
                self.stm32.req_reset()
            elif cmd == "SYNC_TIME":
                dt = self.set_time()
                sync_time = "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
                    dt[0], dt[1], dt[2], dt[4], dt[5], dt[6]
                )
                date = self.stm32.req_rtc_read()
                self.send_status(
                    "TIME_SYNCED",
                    "STATUS",
                    {
                        "sync_time": sync_time,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_RTC":
                date = self.stm32.req_rtc_read()
                self.send_status(
                    "RTC_READ",
                    "STATUS",
                    {
                        "rtc_time": date,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_INPUTS":
                inputs = {}
                for channel_id, channel_name in self.input_channels:
                    value = self.stm32.req_get_input_states(channel_id)
                    inputs[channel_name] = value
                self.send_status("INPUTS_READ", "STATUS", inputs)
            elif cmd == "SET_OUTPUT":
                channel_id = self._find_channel(
                    self.output_channels, params.get("channel")
                )
                if channel_id is None:
                    raise Exception(
                        "Unknown output channel: {}".format(params.get("channel"))
                    )
                self.stm32.req_set_output(channel_id, params.get("value", 0))
                self.send_status(
                    "OUTPUT_SET",
                    "STATUS",
                    {
                        "channel": params.get("channel"),
                        "value": params.get("value", 0),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "SET_PWM":
                channel_id = self._find_channel(
                    self.pwm_channels, params.get("channel")
                )
                if channel_id is None:
                    raise Exception(
                        "Unknown PWM channel: {}".format(params.get("channel"))
                    )
                self.stm32.req_set_pwm(channel_id, params.get("duty_cycle", 0))
                self.send_status(
                    "PWM_SET",
                    "STATUS",
                    {
                        "channel": params.get("channel"),
                        "duty_cycle": params.get("duty_cycle", 0),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "RGB":
                self.stm32.req_set_rgb(
                    params.get("r", 0),
                    params.get("g", 0),
                    params.get("b", 0),
                    params.get("brightness", 100),
                )
                self.send_status(
                    "RGB_SET",
                    "STATUS",
                    {
                        "r": params.get("r", 0),
                        "g": params.get("g", 0),
                        "b": params.get("b", 0),
                        "brightness": params.get("brightness", 100),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "BUZZER":
                self.stm32.req_set_buzzer(
                    params.get("freq", 1000), params.get("volume", 100)
                )
                self.send_status(
                    "BUZZER_SET",
                    "STATUS",
                    {
                        "freq": params.get("freq", 1000),
                        "volume": params.get("volume", 100),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_INA":
                bus_voltage, shunt_voltage, current, power, id = (
                    self.stm32.req_ina_read()
                )
                self.send_status(
                    "INA_READ",
                    "STATUS",
                    {
                        "bus_voltage": bus_voltage,
                        "shunt_voltage": shunt_voltage,
                        "current": current,
                        "power": power,
                        "id": id,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_SHT40":
                t, h = self.stm32.req_sht40()
                self.send_status(
                    "SHT40_READ",
                    "STATUS",
                    {
                        "temperature": t,
                        "humidity": h,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_BME280":
                tb, hb, pb = self.stm32.req_bme280()
                self.send_status(
                    "BME280_READ",
                    "STATUS",
                    {
                        "temperature": tb,
                        "humidity": hb,
                        "pressure": pb,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_SENSOR":
                sensor_data = self._read_sensor_data()
                self.send_status("SENSOR_READ", "STATUS", sensor_data)

            elif cmd == "READ_RADAR":
                radar_data = self.stm32.req_radar()
                self.send_status(
                    "RADAR_READ",
                    "STATUS",
                    {
                        "x": radar_data[0],
                        "y": radar_data[1],
                        "distance": radar_data[2],
                        "speed": radar_data[3],
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "LCD_CLEAR":
                self.stm32.req_lcd_clear()
                self.send_status(
                    "LCD_CLEARED",
                    "STATUS",
                    {
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "LCD_BACKLIGHT":
                self.stm32.req_lcd_set_backlight(params.get("state", 0))
                self.send_status(
                    "LCD_BACKLIGHT_SET",
                    "STATUS",
                    {
                        "state": params.get("state", 0),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "WRITE_FRAM":
                ok = self.stm32.req_fram_write(
                    params.get("address", 0), bytes(params.get("data", []))
                )
                self.send_status(
                    "FRAM_WRITE",
                    "STATUS",
                    {
                        "address": params.get("address", 0),
                        "data": params.get("data", []),
                        "ok": ok,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_FRAM":
                data = self.stm32.req_fram_read(
                    params.get("address", 0), params.get("length", 0)
                )
                self.send_status(
                    "FRAM_READ",
                    "STATUS",
                    {
                        "address": params.get("address", 0),
                        "data": list(data) if data is not None else None,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_FLAGS":
                flags = self.stm32.req_fram_read_flags()
                flags["WIFI_ENABLED"] = self.status_data.get("wifi_enabled", 0)
                self.send_status(
                    "FLAGS_READ",
                    "STATUS",
                    {
                        "flags": flags,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "WRITE_FLAGS":

                flags_data = params.get("flags", 0)
                wifi_flag = params.get("wifi_enabled", 0)

                if wifi_flag not in (0, 1):
                    raise Exception("Invalid wifi_enabled value: {}".format(wifi_flag))

                self.status_data["wifi_enabled"] = wifi_flag
                self.save_data()
                self.apply_config()

                if isinstance(flags_data, str):
                    flags_byte = int(flags_data, 0)
                else:
                    flags_byte = int(flags_data)

                if flags_byte < 0 or flags_byte > 255:
                    raise Exception("Invalid flags value: {}".format(flags_data))
                ok = self.stm32.req_fram_write_flags(flags_byte)
                self.send_status(
                    "FLAGS_WRITE",
                    "STATUS",
                    {
                        "flags": flags_byte,
                        "ok": ok,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_CONFIG":
                cfg_data = self.stm32.read_fram_config()
                cfg_data["serial_number"] = self.status_data.get("serial_number", "")
                cfg_data["wifi_ssid"] = self.status_data.get("wifi_ssid", "")
                cfg_data["wifi_enabled"] = self.status_data.get("wifi_enabled", 0)
                cfg_data["wifi_password"] = self.mask_secret(
                    nvs_sys.get_secret("wifi_password", "")
                )
                cfg_data["mqtt_server"] = self.status_data.get("mqtt_server", "")
                cfg_data["ntp_server"] = self.status_data.get("ntp_server", "")
                cfg_data["mqtt_user"] = nvs_sys.get_secret("mqtt_username", "")
                cfg_data["mqtt_password"] = self.mask_secret(
                    nvs_sys.get_secret("mqtt_password", "")
                )
                cfg_data["api_key"] = self.mask_secret(
                    nvs_sys.get_secret("api_key", "")
                )
                cfg_data["hw_version"] = cfg.get_hw_version()
                cfg_data["sw_version"] = cfg.get_sw_version()
                self.send_status("CONFIG_READ", "STATUS", cfg_data)
            elif cmd == "WRITE_API_KEY":
                api_key = params.get("api_key", "")
                ok = 0
                if api_key:
                    ok = 1 if nvs_sys.set_secret("api_key", api_key) else 0
                    if ok:
                        self.apply_config()
                self.send_status(
                    "API_KEY_WRITE",
                    "STATUS",
                    {
                        "ok": ok,
                        "api_key": self.mask_secret(api_key),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "WRITE_WIFI_ENABLED":
                state = int(params.get("state", 0))
                self.status_data["wifi_enabled"] = 1 if state else 0
                self.save_data()
                self.apply_config()
                self.send_status(
                    "WIFI_ENABLED_WRITE",
                    "STATUS",
                    {
                        "ok": 1,
                        "wifi_enabled": self.status_data["wifi_enabled"],
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
                self.pending_restart = True
            elif cmd == "WRITE_IDS":
                ok = self.stm32.write_fram_config_field(
                    "device_id", params.get("device_id", "")
                )

                if ok:
                    self.apply_config()

                self.send_status(
                    "IDS_CONFIG_WRITE",
                    "STATUS",
                    {
                        "ok": ok,
                        "device_id": self.device_id,
                    },
                )

                if ok:
                    self.pending_restart = True
            elif cmd == "WRITE_WIFI":
                ssid = params.get("wifi_ssid", "")
                password = params.get("wifi_password", "")

                ok = 0
                if ssid and password:
                    ok_pw = nvs_sys.set_secret("wifi_password", password)
                    if ok_pw:
                        self.status_data["wifi_ssid"] = ssid
                        self.save_data()
                        self.apply_config()
                        ok = 1

                self.send_status(
                    "WIFI_CONFIG_WRITE",
                    "STATUS",
                    {
                        "ok": ok,
                        "wifi_ssid": ssid,
                        "wifi_password": self.mask_secret(password),
                    },
                )

                if ok:
                    self.pending_restart = True
            elif cmd == "WRITE_SERVERS":
                mqtt_server = params.get("mqtt_server", "")
                ntp_server = params.get("ntp_server", "")

                ok = 0
                if mqtt_server and ntp_server:
                    self.status_data["mqtt_server"] = mqtt_server
                    self.status_data["ntp_server"] = ntp_server
                    self.save_data()
                    self.apply_config()
                    ok = 1

                self.send_status(
                    "SERVERS_CONFIG_WRITE",
                    "STATUS",
                    {
                        "ok": ok,
                        "mqtt_server": mqtt_server,
                        "ntp_server": ntp_server,
                    },
                )

                if ok:
                    self.pending_restart = True
            elif cmd == "WRITE_MQTT_CREDENTIALS":

                mqtt_user = params.get("mqtt_user", "")
                mqtt_password = params.get("mqtt_password", "")

                ok = 0
                if mqtt_user and mqtt_password:
                    ok_user = nvs_sys.set_secret("mqtt_username", mqtt_user)
                    ok_pw = nvs_sys.set_secret("mqtt_password", mqtt_password)
                    if ok_user and ok_pw:
                        self.apply_config()
                        ok = 1

                self.send_status(
                    "MQTT_CREDENTIALS_CONFIG_WRITE",
                    "STATUS",
                    {
                        "ok": ok,
                        "mqtt_user": mqtt_user,
                        "mqtt_password": self.mask_secret(mqtt_password),
                    },
                )

                if ok:
                    self.pending_restart = True
            elif cmd == "WRITE_FLASH":

                ok = self.stm32.req_flash_write(
                    params.get("address", 0), bytes(params.get("data", []))
                )
                self.send_status(
                    "FLASH_WRITE",
                    "STATUS",
                    {
                        "address": params.get("address", 0),
                        "data": params.get("data", []),
                        "ok": ok,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_FLASH":
                data = self.stm32.req_flash_read(
                    params.get("address", 0), params.get("length", 0)
                )
                self.send_status(
                    "FLASH_READ",
                    "STATUS",
                    {
                        "address": params.get("address", 0),
                        "data": list(data) if data is not None else None,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_LOG_COUNT":
                cnt = self.stm32.req_log_count()
                self.send_status(
                    "LOG_COUNT",
                    "STATUS",
                    {
                        "count": cnt,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "READ_LOG":
                index = int(params.get("index", 0))
                newest_first = 1 if params.get("newest_first", 0) else 0

                rec = self.stm32.req_log_read(index, newest_first)
                if rec is None:
                    raise Exception("Failed to read log record")

                self.send_status(
                    "LOG_READ",
                    "STATUS",
                    {
                        "index": index,
                        "newest_first": newest_first,
                        "record": rec,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "WRITE_LOG":

                timestamp = int(params.get("timestamp", 0))
                temperature_x100 = int(params.get("temperature_x100", 0))
                humidity_x100 = int(params.get("humidity_x100", 0))
                pressure_pa = int(params.get("pressure_pa", 0))

                ok = self.stm32.req_log_append(
                    timestamp,
                    temperature_x100,
                    humidity_x100,
                    pressure_pa,
                )

                self.send_status(
                    "LOG_WRITE",
                    "STATUS",
                    {
                        "ok": ok,
                        "timestamp": timestamp,
                        "temperature_x100": temperature_x100,
                        "humidity_x100": humidity_x100,
                        "pressure_pa": pressure_pa,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )

            elif cmd == "CLEAR_LOG":

                ok = self.stm32.req_log_clear()

                self.send_status(
                    "LOG_CLEARED",
                    "STATUS",
                    {
                        "ok": ok,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "GET_STATUS":
                fw, hw = self.stm32.req_fw_hw_version()
                build_data = self.stm32.req_build_date()
                prod_date = self.stm32.req_prod_date()
                self.send_status(
                    "STATUS_READ",
                    "STATUS",
                    {
                        "controller_sw": fw,
                        "controller_hw": hw,
                        "controller_build_date": build_data,
                        "controller_prod_date": prod_date,
                        "sw_version": cfg.get_sw_version(),
                        "hw_version": cfg.get_hw_version(),
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                        "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms)
                        // 1000,
                        "rssi": self.wifi.get_rssi() if self.wifi else None,
                    },
                )
            elif cmd == "GET_INFO":
                self.send_status(
                    "INFO_READ",
                    "STATUS",
                    {
                        "device_id": self.device_id,
                        "serial_number": self.status_data.get("serial_number", ""),
                        "hw_version": cfg.get_hw_version(),
                        "sw_version": cfg.get_sw_version(),
                        "ip_address": self.ip_address,
                        "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms)
                        // 1000,
                    },
                )
            elif cmd == "GET_OUTPUT_CHANNELS":
                channels = [name for _, name in self.output_channels]
                self.send_status(
                    "OUTPUT_CHANNELS",
                    "STATUS",
                    {"channels": channels, "device_id": self.device_id},
                )
            elif cmd == "GET_PWM_CHANNELS":
                channels = [name for _, name in self.pwm_channels]
                self.send_status(
                    "PWM_CHANNELS",
                    "STATUS",
                    {"channels": channels, "device_id": self.device_id},
                )
            elif cmd == "GET_INPUT_CHANNELS":
                channels = [name for _, name in self.input_channels]
                self.send_status(
                    "INPUT_CHANNELS",
                    "STATUS",
                    {"channels": channels, "device_id": self.device_id},
                )
            elif cmd == "GET_TIME":
                date = self.stm32.req_rtc_read()
                self.send_status(
                    "TIME_READ",
                    "STATUS",
                    {
                        "rtc_time": date,
                        "device_id": self.device_id,
                        "ip_address": self.ip_address,
                    },
                )
            elif cmd == "GET_CONFIG":
                cfg_data = self.stm32.read_fram_config()
                cfg_data["serial_number"] = self.status_data.get("serial_number", "")
                cfg_data["wifi_ssid"] = self.status_data.get("wifi_ssid", "")
                cfg_data["wifi_enabled"] = self.status_data.get("wifi_enabled", 0)
                cfg_data["wifi_password"] = self.mask_secret(
                    nvs_sys.get_secret("wifi_password", "")
                )
                cfg_data["mqtt_server"] = self.status_data.get("mqtt_server", "")
                cfg_data["ntp_server"] = self.status_data.get("ntp_server", "")
                cfg_data["mqtt_user"] = nvs_sys.get_secret("mqtt_username", "")
                cfg_data["mqtt_password"] = self.mask_secret(
                    nvs_sys.get_secret("mqtt_password", "")
                )
                cfg_data["api_key"] = self.mask_secret(
                    nvs_sys.get_secret("api_key", "")
                )
                cfg_data["hw_version"] = cfg.get_hw_version()
                cfg_data["sw_version"] = cfg.get_sw_version()
                self.send_status("CONFIG_READ", "STATUS", cfg_data)
            else:
                raise Exception("Unknown cmd: {}".format(cmd))
        except Exception as e:
            self.error_management("CMD", str(e))

    def load_data(self, filename="status.json"):
        try:
            with open(filename, "r") as f:
                self.status_data = ujson.load(f)
                if "pending_errors" not in self.status_data or not isinstance(
                    self.status_data["pending_errors"], list
                ):
                    self.status_data["pending_errors"] = []
        except Exception:
            self.status_data = {
                "device_id": "",
                "ip_address": "",
                "wifi_ssid": "",
                "mqtt_server": "",
                "ntp_server": "",
                "wifi_enabled": 0,
                "pending_errors": [],
            }

    def save_data(self, filename="status.json"):
        tmp = filename + ".tmp"
        with open(tmp, "w") as f:
            ujson.dump(self.status_data, f)
        os.rename(tmp, filename)

    def error_dict(self, source, msg):
        return {
            "timestamp": time.time(),
            "source": source,
            "msg": str(msg),
            "device_id": self.device_id,
        }

    def error_queue(self, payload, max_len=50):
        self.status_data["pending_errors"].append(payload)
        if len(self.status_data["pending_errors"]) > max_len:
            self.status_data["pending_errors"] = self.status_data["pending_errors"][
                -max_len:
            ]
        try:
            self.save_data()
        except Exception as e:
            self.errors["SAVE_STATUS"] = e

    def error_management(self, source, msg):
        payload = self.error_dict(source, msg)
        if self.client is None:
            self.error_queue(payload)
            return
        try:
            self.send_status("ERROR", "ERROR", payload)
        except Exception as e:
            self.error_queue(payload)
            self.error_queue(self.error_dict("SEND_STATUS", e))

    def send_errors(self):
        if self.client is None:
            return
        if not self.status_data.get("pending_errors"):
            return
        try:
            self.send_status(
                "ERRORS_FLUSH",
                "ERROR",
                {
                    "items": self.status_data["pending_errors"],
                    "device_id": self.device_id,
                    "ip_address": self.ip_address,
                },
            )
            self.status_data["pending_errors"] = []
            self.save_data()
        except Exception as e:
            self.errors["SEND_ERRORS"] = e

    def set_time(self):
        dt = self.rtc.datetime()
        wd = (dt[3] % 7) + 1
        self.stm32.req_rtc_write(dt[0] - 2000, dt[1], dt[2], wd, dt[4], dt[5], dt[6])
        return dt

    def set_ext_time(self):
        dt = self.rtc.datetime()
        wd = (dt[3] % 7) + 1
        self.stm32.req_ext_rtc_write(
            dt[0] - 2000, dt[1], dt[2], wd, dt[4], dt[5], dt[6]
        )
        return dt

    def status_led(self, state: int):
        if state:
            self.led.on()
        else:
            self.led.off()

    def _read_sensor_data(self):
        data = {
            "ip_address": self.ip_address,
            "device_id": self.device_id,
            "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms) // 1000,
            "rssi": self.wifi.get_rssi() if self.wifi else None,
            "sw_version": cfg.get_sw_version(),
            "hw_version": cfg.get_hw_version(),
            "api_key": nvs_sys.get_secret("api_key", ""),
        }
        if self.stm32 is None:
            return data

        if cfg.get_bme280_present():
            try:
                tb, hb, pb = self.stm32.req_bme280()
                if not (tb == 0.0 and hb == 0.0 and pb == 0.0):
                    data["sensor_readings"] = [
                        {"parameter_key": "temperature", "value": tb},
                        {"parameter_key": "humidity", "value": hb},
                        {"parameter_key": "atmPressure", "value": pb},
                    ]
                    data["bme280_error"] = 0
                else:
                    data["bme280_error"] = 1
            except Exception as e:
                self.error_management("BME280_READ", str(e))
                data["bme280_error"] = 1
        elif cfg.get_sht40_present():
            try:
                t, h = self.stm32.req_sht40()
                if not (t == 0.0 and h == 0.0):
                    data["sensor_readings"] = [
                        {"parameter_key": "temperature", "value": t},
                        {"parameter_key": "humidity", "value": h},
                    ]
                    data["sht40_error"] = 0
                else:
                    data["sht40_error"] = 1
            except Exception as e:
                self.error_management("SHT40_READ", str(e))
                data["sht40_error"] = 1
        if cfg.get_ext_rtc_present():
            try:
                date = self.stm32.req_rtc_read()
                if date:
                    data["rtc"] = date
            except Exception as e:
                self.error_management("RTC_READ", str(e))
        else:
            try:
                data["rtc"] = time.localtime()
            except Exception as e:
                self.error_management("INTERNAL TIME", str(e))
        if cfg.get_adc_present():
            try:
                v0, v1 = self.stm32.req_adc()
                v0_voltage = self.stm32.adc_to_voltage(v0)
                data["battery_voltage"] = round(self.stm32.vadc_to_vin(v0_voltage), 3)
            except Exception as e:
                self.error_management("ADC_READ", str(e))
        return data

    def read_data(self):
        t0 = time.time()

        self.stm32_gpio.value(1)
        self.led.on()

        try:
            if self.stm32 is None:
                raise Exception("STM32 not initialized")

            if self.stm32.req_get_input_states(0x03):
                self.set_time()

                data = self._read_sensor_data()
                data["irq_dropped"] = self.irq_dropped

                try:
                    serial = self.stm32.req_serial()
                    fw, hw = self.stm32.req_fw_hw_version()
                    data["controller_serial"] = serial
                    data["controller_sw"] = fw
                    data["controller_hw"] = hw
                    data["stm32_error"] = 0
                except Exception as e:
                    self.error_management("CONTROLLER_INFO", str(e))
                    data["stm32_error"] = 1

                if self.client is not None:
                    try:
                        self.send_status("DATA", "DATA", data, timestamp=t0)
                    except Exception as e:
                        self.error_management("SEND_DATA", str(e))
            else:
                data = {
                    "device_id": self.device_id,
                    "ip_address": self.ip_address,
                    "irq_dropped": self.irq_dropped,
                    "stm32_error": 1,
                }
                if self.client is not None:
                    try:
                        self.send_status("DATA", "DATA", data, timestamp=t0)
                    except Exception as e:
                        self.error_management("SEND_DATA", str(e))
        except Exception as e:
            self.error_management("READ_DATA", str(e))
        finally:
            self.stm32_gpio.value(0)
            self.led.off()

    def read_usb(self, line):
        try:
            data = [x.strip() for x in line.split(",")]
            cmd = data[0].upper()

            if cmd == "PING":
                print(
                    "STATUS:ALIVE,{},{}".format(
                        cfg.get_sw_version(),
                        cfg.get_hw_version(),
                    )
                )

            elif cmd == "GET_OUTPUT_CHANNELS":
                channels = [name for _, name in self.output_channels]
                print("OK:OUTPUT_CHANNELS,{}".format(channels))

            elif cmd == "GET_PWM_CHANNELS":
                channels = [name for _, name in self.pwm_channels]
                print("OK:PWM_CHANNELS,{}".format(channels))

            elif cmd == "GET_INPUT_CHANNELS":
                channels = [name for _, name in self.input_channels]
                print("OK:INPUT_CHANNELS,{}".format(channels))

            elif cmd == "GET_STATUS":
                fw, hw = self.stm32.req_fw_hw_version()
                build_data = self.stm32.req_build_date()
                prod_date = self.stm32.req_prod_date()
                status = {
                    "controller_sw": fw,
                    "controller_hw": hw,
                    "controller_build_date": build_data,
                    "controller_prod_date": prod_date,
                    "sw_version": cfg.get_sw_version(),
                    "hw_version": cfg.get_hw_version(),
                    "device_id": self.status_data.get("device_id", ""),
                    "ip_address": self.ip_address,
                    "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms)
                    // 1000,
                    "rssi": self.wifi.get_rssi() if self.wifi else None,
                }
                print("OK:STATUS,{}".format(status))

            elif cmd == "GET_INFO":
                info = {
                    "device_id": self.status_data.get("device_id", ""),
                    "serial_number": self.status_data.get("serial_number", ""),
                    "hw_version": cfg.get_hw_version(),
                    "sw_version": cfg.get_sw_version(),
                    "ip_address": self.ip_address,
                    "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms)
                    // 1000,
                }
                print("OK:GET_INFO,{}".format(info))

            elif cmd == "GET_COMMANDS":
                commands = [
                    "PING",
                    "GET_STATUS",
                    "GET_INFO",
                    "GET_COMMANDS",
                    "GET_OUTPUT_CHANNELS",
                    "GET_PWM_CHANNELS",
                    "GET_INPUT_CHANNELS",
                    "RESET",
                    "SYNC_TIME",
                    "READ_RTC",
                    "GET_TIME",
                    "READ_INPUTS",
                    "SET_OUTPUT",
                    "SET_PWM",
                    "RGB",
                    "BUZZER",
                    "READ_INA",
                    "READ_SHT40",
                    "READ_BME280",
                    "READ_SENSOR",
                    "LCD_CLEAR",
                    "LCD_BACKLIGHT",
                    "WRITE_FLASH",
                    "READ_FLASH",
                    "WRITE_FRAM",
                    "READ_FRAM",
                    "READ_FLAGS",
                    "WRITE_FLAGS",
                    "READ_CONFIG",
                    "GET_CONFIG",
                    "WRITE_API_KEY",
                    "WRITE_WIFI_ENABLED",
                    "WRITE_IDS",
                    "WRITE_WIFI",
                    "WRITE_SERVERS",
                    "WRITE_MQTT_CREDENTIALS",
                    "READ_LOG_COUNT",
                    "READ_LOG",
                    "WRITE_LOG",
                    "CLEAR_LOG",
                ]
                print("OK:GET_COMMANDS,{}".format(commands))

            elif cmd == "RESET":
                print("OK:RESET")
                time.sleep(0.1)
                self.stm32.req_reset()

            elif cmd == "SYNC_TIME":
                dt = self.set_time()
                sync_time = "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
                    dt[0], dt[1], dt[2], dt[4], dt[5], dt[6]
                )
                date = self.stm32.req_rtc_read()
                print("OK:SYNC_TIME,{},{}".format(sync_time, date))

            elif cmd == "READ_RTC":
                date = self.stm32.req_rtc_read()
                print("OK:READ_RTC,{}".format(date))

            elif cmd == "READ_INPUTS":
                inputs = {}
                for channel_id, channel_name in self.input_channels:
                    value = self.stm32.req_get_input_states(channel_id)
                    inputs[channel_name] = value
                print("OK:READ_INPUTS,{}".format(inputs))

            elif cmd == "SET_OUTPUT":
                if len(data) < 3:
                    print("ERR:SET_OUTPUT_FORMAT")
                    return

                channel_name = data[1]
                value = int(data[2])

                channel_id = self._find_channel(self.output_channels, channel_name)
                if channel_id is None:
                    print("ERR:UNKNOWN_OUTPUT_CHANNEL,{}".format(channel_name))
                    return

                self.stm32.req_set_output(channel_id, value)
                print("OK:SET_OUTPUT,{},{}".format(channel_name, value))

            elif cmd == "SET_PWM":
                if len(data) < 3:
                    print("ERR:SET_PWM_FORMAT")
                    return

                channel_name = data[1]
                duty_cycle = int(data[2])

                channel_id = self._find_channel(self.pwm_channels, channel_name)
                if channel_id is None:
                    print("ERR:UNKNOWN_PWM_CHANNEL,{}".format(channel_name))
                    return

                self.stm32.req_set_pwm(channel_id, duty_cycle)
                print("OK:SET_PWM,{},{}".format(channel_name, duty_cycle))

            elif cmd == "RGB":
                if len(data) < 5:
                    print("ERR:RGB_FORMAT")
                    return

                r = int(data[1])
                g = int(data[2])
                b = int(data[3])
                brightness = int(data[4])

                self.stm32.req_set_rgb(r, g, b, brightness)
                print("OK:RGB,{},{},{},{}".format(r, g, b, brightness))

            elif cmd == "BUZZER":
                if len(data) < 3:
                    print("ERR:BUZZER_FORMAT")
                    return

                freq = int(data[1])
                volume = int(data[2])

                self.stm32.req_set_buzzer(freq, volume)
                print("OK:BUZZER,{},{}".format(freq, volume))

            elif cmd == "READ_INA":
                bus_voltage, shunt_voltage, current, power, id = (
                    self.stm32.req_ina_read()
                )
                print(
                    "OK:READ_INA,{},{},{},{},{}".format(
                        bus_voltage, shunt_voltage, current, power, id
                    )
                )

            elif cmd == "READ_SHT40":
                t, h = self.stm32.req_sht40()
                print("OK:READ_SHT40,{},{}".format(t, h))

            elif cmd == "READ_BME280":
                tb, hb, pb = self.stm32.req_bme280()
                print("OK:READ_BME280,{},{},{}".format(tb, hb, pb))

            elif cmd == "READ_SENSOR":
                sensor_data = self._read_sensor_data()
                print("OK:READ_SENSOR,{}".format(sensor_data))

            elif cmd == "READ_RADAR":
                radar_data = self.stm32.req_radar()
                print("OK:READ_RADAR,{}".format(radar_data))

            elif cmd == "LCD_CLEAR":
                self.stm32.req_lcd_clear()
                print("OK:LCD_CLEAR")

            elif cmd == "LCD_BACKLIGHT":
                if len(data) < 2:
                    print("ERR:LCD_BACKLIGHT_FORMAT")
                    return
                state = int(data[1])
                self.stm32.req_lcd_set_backlight(state)
                print("OK:LCD_BACKLIGHT,{}".format(state))

            elif cmd == "WRITE_FLASH":
                if len(data) < 3:
                    print("ERR:WRITE_FLASH_FORMAT")
                    return
                address = int(data[1], 0)
                bytes_data = bytes(int(x, 0) for x in data[2:])
                ok = self.stm32.req_flash_write(address, bytes_data)
                print("OK:WRITE_FLASH,{},{}".format(address, ok))

            elif cmd == "READ_FLASH":
                if len(data) < 3:
                    print("ERR:READ_FLASH_FORMAT")
                    return
                address = int(data[1], 0)
                length = int(data[2], 0)
                bytes_data = self.stm32.req_flash_read(address, length)
                if bytes_data is not None:
                    print("OK:READ_FLASH,{},{}".format(address, list(bytes_data)))
                else:
                    print("ERR:READ_FLASH,{},NO_DATA".format(address))

            elif cmd == "WRITE_FRAM":

                if len(data) < 4:
                    print("ERR:WRITE_FRAM_FORMAT")
                    return

                address = int(data[1], 0)
                bytes_data = bytes(int(x, 0) for x in data[2:])

                ok = self.stm32.req_fram_write(address, bytes_data)
                print("OK:WRITE_FRAM,{},{}".format(address, ok))

            elif cmd == "READ_FRAM":

                if len(data) < 3:
                    print("ERR:READ_FRAM_FORMAT")
                    return

                address = int(data[1], 0)
                length = int(data[2], 0)

                bytes_data = self.stm32.req_fram_read(address, length)
                if bytes_data is not None:
                    list_data = list(bytes_data)
                    print("OK:READ_FRAM,{},{}".format(address, list_data))
                else:
                    print("ERR:READ_FRAM,{},NO_DATA".format(address))

            elif cmd == "READ_FLAGS":
                flags = self.stm32.req_fram_read_flags()
                flags["WIFI_ENABLED"] = self.status_data.get("wifi_enabled", 0)
                print("OK:READ_FLAGS,{}".format(flags))

            elif cmd == "WRITE_FLAGS":

                if len(data) < 3:
                    print("ERR:WRITE_FLAGS_FORMAT")
                    return

                flags_byte = int(data[1], 0)
                wifi_flag = int(data[2], 0)
                if flags_byte < 0 or flags_byte > 255:
                    print("ERR:WRITE_FLAGS_VALUE")
                    return

                if wifi_flag not in (0, 1):
                    print("ERR:WRITE_FLAGS_WIFI_VALUE")
                    return

                self.status_data["wifi_enabled"] = wifi_flag
                self.save_data()
                self.apply_config()

                ok = self.stm32.req_fram_write_flags(flags_byte)
                print("OK:WRITE_FLAGS,{},{}".format(flags_byte, ok))

            elif cmd == "READ_CONFIG":
                cfg_data = self.stm32.read_fram_config()
                cfg_data["serial_number"] = self.status_data.get("serial_number", "")
                cfg_data["wifi_ssid"] = self.status_data.get("wifi_ssid", "")
                cfg_data["wifi_enabled"] = self.status_data.get("wifi_enabled", 0)
                cfg_data["wifi_password"] = self.mask_secret(
                    nvs_sys.get_secret("wifi_password", "")
                )
                cfg_data["mqtt_server"] = self.status_data.get("mqtt_server", "")
                cfg_data["ntp_server"] = self.status_data.get("ntp_server", "")
                cfg_data["mqtt_user"] = nvs_sys.get_secret("mqtt_username", "")
                cfg_data["mqtt_password"] = self.mask_secret(
                    nvs_sys.get_secret("mqtt_password", "")
                )
                cfg_data["api_key"] = self.mask_secret(
                    nvs_sys.get_secret("api_key", "")
                )
                cfg_data["hw_version"] = cfg.get_hw_version()
                cfg_data["sw_version"] = cfg.get_sw_version()
                print("OK:READ_CONFIG,{}".format(cfg_data))

            elif cmd == "WRITE_API_KEY":
                if len(data) < 2:
                    print("ERR:WRITE_API_KEY_FORMAT")
                    return
                api_key = data[1]
                ok = 1 if nvs_sys.set_secret("api_key", api_key) else 0
                if ok:
                    self.apply_config()
                print("OK:WRITE_API_KEY,{},{}".format(self.mask_secret(api_key), ok))

            elif cmd == "WRITE_WIFI_ENABLED":
                if len(data) < 2:
                    print("ERR:WRITE_WIFI_ENABLED_FORMAT")
                    return
                state = int(data[1])
                self.status_data["wifi_enabled"] = 1 if state else 0
                self.save_data()
                self.apply_config()
                self.pending_restart = True
                print(
                    "OK:WRITE_WIFI_ENABLED,{}".format(self.status_data["wifi_enabled"])
                )

            elif cmd == "WRITE_IDS":
                if len(data) < 2:
                    print("ERR:WRITE_IDS_FORMAT")
                    return

                device_id = data[1]

                ok = self.stm32.write_fram_config_field("device_id", device_id)

                if ok:
                    self.apply_config()
                    self.restart_connections()

                print("OK:WRITE_IDS,{},{}".format(device_id, ok))

            elif cmd == "WRITE_WIFI":
                if len(data) < 3:
                    print("ERR:WRITE_WIFI_FORMAT")
                    return

                ssid = data[1]
                password = data[2]

                ok = 0
                ok_pw = nvs_sys.set_secret("wifi_password", password)
                if ok_pw:
                    self.status_data["wifi_ssid"] = ssid
                    self.save_data()
                    self.apply_config()
                    self.restart_connections()
                    ok = 1

                password = self.mask_secret(password)
                print("OK:WRITE_WIFI,{},{},{}".format(ssid, password, ok))

            elif cmd == "WRITE_SERVERS":

                if len(data) < 3:
                    print("ERR:WRITE_SERVERS_FORMAT")
                    return

                mqtt_server = data[1]
                ntp_server = data[2]

                ok = 0
                if mqtt_server and ntp_server:
                    self.status_data["mqtt_server"] = mqtt_server
                    self.status_data["ntp_server"] = ntp_server
                    self.save_data()
                    self.apply_config()
                    self.restart_connections()
                    ok = 1

                print("OK:WRITE_SERVERS,{},{},{}".format(mqtt_server, ntp_server, ok))

            elif cmd == "WRITE_MQTT_CREDENTIALS":

                if len(data) < 3:
                    print("ERR:WRITE_MQTT_CREDENTIALS_FORMAT")
                    return

                mqtt_user = data[1]
                mqtt_password = data[2]

                ok = 0
                ok_user = nvs_sys.set_secret("mqtt_username", mqtt_user)
                ok_pw = nvs_sys.set_secret("mqtt_password", mqtt_password)
                if ok_user and ok_pw:
                    self.apply_config()
                    self.restart_connections()
                    ok = 1

                mqtt_password_masked = self.mask_secret(mqtt_password)
                print(
                    "OK:WRITE_MQTT_CREDENTIALS,{},{},{}".format(
                        mqtt_user, mqtt_password_masked, ok
                    )
                )

            elif cmd == "READ_LOG_COUNT":
                cnt = self.stm32.req_log_count()
                print("OK:READ_LOG_COUNT,{}".format(cnt))

            elif cmd == "READ_LOG":
                if len(data) < 2:
                    print("ERR:READ_LOG_FORMAT")
                    return

                index = int(data[1], 0)
                newest_first = int(data[2], 0) if len(data) >= 3 else 0

                rec = self.stm32.req_log_read(index, newest_first)
                if rec is None:
                    print("ERR:READ_LOG_FAILED")
                    return

                print("OK:READ_LOG,{}".format(rec))

            elif cmd == "WRITE_LOG":

                if len(data) < 5:
                    print("ERR:WRITE_LOG_FORMAT")
                    return

                timestamp = int(data[1], 0)
                temperature_x100 = int(data[2], 0)
                humidity_x100 = int(data[3], 0)
                pressure_pa = int(data[4], 0)

                ok = self.stm32.req_log_append(
                    timestamp,
                    temperature_x100,
                    humidity_x100,
                    pressure_pa,
                )

                print(
                    "OK:WRITE_LOG,{},{},{},{},{}".format(
                        timestamp, temperature_x100, humidity_x100, pressure_pa, ok
                    )
                )

            elif cmd == "CLEAR_LOG":
                ok = self.stm32.req_log_clear()
                print("OK:CLEAR_LOG,{}".format(ok))

            elif cmd == "GET_CONFIG":
                cfg_data = self.stm32.read_fram_config()
                cfg_data["serial_number"] = self.status_data.get("serial_number", "")
                cfg_data["wifi_ssid"] = self.status_data.get("wifi_ssid", "")
                cfg_data["wifi_enabled"] = self.status_data.get("wifi_enabled", 0)
                cfg_data["wifi_password"] = self.mask_secret(
                    nvs_sys.get_secret("wifi_password", "")
                )
                cfg_data["mqtt_server"] = self.status_data.get("mqtt_server", "")
                cfg_data["ntp_server"] = self.status_data.get("ntp_server", "")
                cfg_data["mqtt_user"] = nvs_sys.get_secret("mqtt_username", "")
                cfg_data["mqtt_password"] = self.mask_secret(
                    nvs_sys.get_secret("mqtt_password", "")
                )
                cfg_data["api_key"] = self.mask_secret(
                    nvs_sys.get_secret("api_key", "")
                )
                cfg_data["hw_version"] = cfg.get_hw_version()
                cfg_data["sw_version"] = cfg.get_sw_version()
                print("OK:GET_CONFIG,{}".format(cfg_data))

            elif cmd == "GET_TIME":
                date = self.stm32.req_rtc_read()
                print("OK:GET_TIME,{}".format(date))

            else:
                print("ERR:UNKNOWN_CMD,{}".format(cmd))

        except Exception as e:
            print("ERR:READ_USB,{}".format(e))
            self.error_management("READ_USB", str(e))
