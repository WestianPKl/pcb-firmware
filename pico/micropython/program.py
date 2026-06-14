import machine, time, ujson, os
from machine import Pin, I2C, PWM, Timer, ADC, UART
from mqtt_simple import MQTTClient
from config import get_config
from nvs_system import NVSSystem
from wireless import WiFi
from rtc_clock import RTC_Clock, apply_timezone_offset
from i2c_lcd import I2cLcd
from sht30 import SHT30
from sht40 import SHT40
from radar import RadarParser
from bme280_i2c import BME280

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
        self.pending_read = False
        self.pending_restart = False
        self.irq_dropped = 0
        self.last_irq_ms = 0
        self.errors = {}
        self.wifi = None
        self.uart1 = None
        self.i2c = None
        self.lcd = None
        self.radar = None
        self.clock = None
        self.sensor = None
        self.leds = []
        self.buzzer_pwm = None
        self.relays = []
        self.switches = []
        self._timer = None
        self._lcd_timer = None
        self.pending_lcd_update = False
        self._last_sensor_data = {}
        self.adc = None
        self.start_ticks_ms = time.ticks_ms()
        self._last_error_save_ms = 0
        self.buffer = b""

    def _irq_status(self, pin):
        now = time.ticks_ms()
        if self.pending_read:
            self.irq_dropped += 1
            return
        if time.ticks_diff(now, self.last_irq_ms) < cfg.get_debounce_irq_ms():
            return
        self.last_irq_ms = now
        self.pending_read = True

    def set_wifi(self, wifi):
        self.wifi = wifi

    def _timer_callback(self, t):
        self.pending_read = True

    def _lcd_timer_callback(self, t):
        self.pending_lcd_update = True

    def init_hardware(self):
        try:
            i2c_id, sda, scl, freq = cfg.get_i2c_config()
            self.i2c = I2C(i2c_id, scl=Pin(scl), sda=Pin(sda), freq=freq)
        except Exception as e:
            self.errors["I2C_INIT"] = str(e)

        try:
            self.uart1 = UART(
                1,
                baudrate=256000,
                bits=8,
                parity=None,
                stop=1,
                tx=4,
                rx=5,
            )
            self.radar = RadarParser(cfg)
        except Exception as e:
            self.errors["UART_INIT"] = str(e)

        try:
            if self.i2c is not None:
                lcd_addr, rows, cols = cfg.get_lcd_config()
                self.lcd = I2cLcd(self.i2c, lcd_addr, rows, cols)
                self.lcd.move_to(0, 0)
                self.lcd.putstr("Logger ready")
                self.lcd.move_to(0, 1)
                self.lcd.putstr("Initializing...")

        except Exception as e:
            self.errors["LCD_INIT"] = str(e)

        try:
            if self.i2c is not None and cfg.is_clock_enabled():
                self.clock = RTC_Clock(self.i2c)
        except Exception as e:
            self.errors["RTC_INIT"] = str(e)

        try:
            if self.i2c is not None:
                sht_type = cfg.get_sht_type()
                if sht_type == 30:
                    self.sensor = SHT30(self.i2c)
                elif sht_type == 40:
                    self.sensor = SHT40(self.i2c)
                else:
                    self.sensor = BME280(self.i2c)
        except Exception as e:
            self.errors["SENSOR_INIT"] = str(e)

        try:
            self.leds = [PWM(Pin(p)) for p in cfg.get_led_pins()]
            for led in self.leds:
                led.freq(1000)
                led.duty_u16(0)
        except Exception as e:
            self.errors["LED_INIT"] = str(e)

        if cfg.is_relay_version():
            try:
                self.relays = [Pin(p, Pin.OUT, value=0) for p in cfg.get_relay_pins()]
            except Exception as e:
                self.errors["RELAY_INIT"] = str(e)
        else:
            try:
                self.buzzer_pwm = PWM(Pin(cfg.get_buzzer_pin()))
                self.buzzer_pwm.duty_u16(0)
            except Exception as e:
                self.errors["BUZZER_INIT"] = str(e)

        try:
            self.switches = [Pin(p, Pin.IN, Pin.PULL_UP) for p in cfg.get_switch_pins()]
        except Exception as e:
            self.errors["SWITCH_INIT"] = str(e)

        try:
            self.adc = ADC(Pin(26, Pin.IN))
        except Exception as e:
            self.errors["ADC_INIT"] = str(e)

        try:
            self._lcd_timer = Timer()
            self._lcd_timer.init(
                mode=Timer.PERIODIC,
                period=1000,
                callback=self._lcd_timer_callback,
            )
        except Exception as e:
            self.errors["LCD_TIMER_INIT"] = str(e)

        post_time = cfg.get_post_time()
        if post_time > 0:
            try:
                self._timer = Timer()
                self._timer.init(
                    mode=Timer.PERIODIC,
                    period=post_time * 1000,
                    callback=self._timer_callback,
                )
            except Exception as e:
                self.errors["TIMER_INIT"] = str(e)

    def apply_config(self):
        hw_version = self.status_data.get("hw_version", "")
        sw_version = self.status_data.get("sw_version", "")
        serial_number = self.status_data.get("serial_number", "")
        device_id = self.status_data.get("device_id", "")
        wifi_enabled = self.status_data.get("wifi_enabled", 0)
        wifi_ssid = self.status_data.get("wifi_ssid", "")
        wifi_password = nvs_sys.get_secret("wifi_password", "")
        ntp_server = self.status_data.get("ntp_server", "")
        mqtt_server = self.status_data.get("mqtt_server", "")
        mqtt_user = nvs_sys.get_secret("mqtt_username", "")
        mqtt_password = nvs_sys.get_secret("mqtt_password", "")
        server_ip = self.status_data.get("server_ip", "")
        server_port = self.status_data.get("server_port", 0)
        post_time = self.status_data.get("post_time", 0)
        temperature_enabled = self.status_data.get("temperature", 0)
        humidity_enabled = self.status_data.get("humidity", 0)
        pressure_enabled = self.status_data.get("pressure", 0)
        sht_type = self.status_data.get("sht", 0)
        clock_enabled = self.status_data.get("clock", 0)
        logging_enabled = self.status_data.get("logging_enabled", 0)
        set_time_enabled = self.status_data.get("set_time", 0)
        api_key = nvs_sys.get_secret("api_key", "")

        cfg.set_hw_version(hw_version)
        cfg.set_sw_version(sw_version)
        cfg.set_serial_number(serial_number)
        cfg.set_device_id(device_id)
        cfg.set_wifi_enabled(wifi_enabled)
        cfg.set_wifi_config(wifi_ssid, wifi_password)
        cfg.set_ntp_server_ip(ntp_server)
        cfg.set_mqtt_config(
            mqtt_server,
            cfg.get_mqtt_port(),
            mqtt_user,
            mqtt_password,
            cfg.get_mqtt_keepalive(),
        )
        cfg.set_server_config(server_ip, server_port)
        cfg.set_post_time(post_time)
        cfg.set_temperature_enabled(temperature_enabled)
        cfg.set_humidity_enabled(humidity_enabled)
        cfg.set_pressure_enabled(pressure_enabled)
        cfg.set_sht_type(sht_type)
        cfg.set_clock_enabled(clock_enabled)
        cfg.set_logging_enabled(logging_enabled)
        cfg.set_set_time_enabled(set_time_enabled)
        cfg.set_api_key(api_key)

        self.device_id = str(device_id)
        self.topic_status = "devices/{}/status".format(self.device_id).encode()

    def get_client(self):
        return self.client

    def set_client(self, client):
        self.client = client

    def set_ip_address(self, ip_address):
        self.ip_address = ip_address

    def queue_runtime_errors(self):
        if not self.errors:
            return
        errors = self.errors
        self.errors = {}
        for source, msg in errors.items():
            self.error_queue(self.error_dict(source, msg))

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
                self.send_status("ALIVE", "STATUS", self._get_base_payload())

            elif cmd == "RESET":
                self.send_status("RESET", "STATUS", self._get_base_payload())
                time.sleep(0.1)
                machine.reset()

            elif cmd == "SYNC_TIME":
                dt = self.set_time()
                sync_time = None
                if dt:
                    sync_time = "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
                        dt[0], dt[1], dt[2], dt[4], dt[5], dt[6]
                    )
                self.send_status(
                    "TIME_SYNCED",
                    "STATUS",
                    {
                        "sync_time": sync_time,
                        "ip_address": self.ip_address,
                        "device_id": self.device_id,
                    },
                )

            elif cmd == "READ_RTC":
                rtc_time = self.clock.read_time() if self.clock else None
                self.send_status(
                    "RTC_READ",
                    "STATUS",
                    {
                        "rtc_time": rtc_time,
                        "ip_address": self.ip_address,
                        "device_id": self.device_id,
                    },
                )

            elif cmd == "READ_RADAR":
                radar_data = self._read_radar_data()
                self.send_status("RADAR_READ", "STATUS", radar_data)

            elif cmd == "READ_SENSOR":
                sensor_data = self._read_sensor_data()
                self.send_status("SENSOR_READ", "STATUS", sensor_data)

            elif cmd == "READ_ADC":
                if self.adc is not None:
                    batt_raw = self.adc.read_u16()
                    batt_v = round(batt_raw * (3.3 / 65535) * 2.0, 3)
                    self.send_status(
                        "ADC_READ",
                        "STATUS",
                        {"battery_voltage": batt_v, "raw": batt_raw},
                    )
                else:
                    self.send_status(
                        "ADC_READ", "STATUS", {"error": "ADC not initialized"}
                    )

            elif cmd == "SET_LED":
                idx = params.get("index", 0)
                duty = min(max(int(params.get("duty", 0)), 0), 65535)
                if 0 <= idx < len(self.leds):
                    self.leds[idx].duty_u16(duty)
                self.send_status("LED_SET", "STATUS", {"index": idx, "duty": duty})

            elif cmd == "SET_BUZZER":
                freq = max(int(params.get("freq", 0)), 0)
                duty = min(max(int(params.get("duty", 0)), 0), 65535)
                if self.buzzer_pwm is not None:
                    if freq > 0 and duty > 0:
                        self.buzzer_pwm.freq(freq)
                        self.buzzer_pwm.duty_u16(duty)
                    else:
                        self.buzzer_pwm.duty_u16(0)
                self.send_status("BUZZER_SET", "STATUS", {"freq": freq, "duty": duty})

            elif cmd == "SET_RELAY":
                idx = params.get("index", 0)
                state = params.get("state", 0)
                if 0 <= idx < len(self.relays):
                    self.relays[idx].value(1 if state else 0)
                self.send_status("RELAY_SET", "STATUS", {"index": idx, "state": state})

            elif cmd == "READ_SWITCH":
                states = [sw.value() for sw in self.switches]
                self.send_status("SWITCH_READ", "STATUS", {"states": states})

            elif cmd == "LCD_CLEAR":
                if self.lcd is not None:
                    self.lcd.clear()
                self.send_status(
                    "LCD_CLEARED", "STATUS", {"ip_address": self.ip_address}
                )

            elif cmd == "LCD_BACKLIGHT":
                state = params.get("state", 0)
                if self.lcd is not None:
                    if state:
                        self.lcd.backlight_on()
                    else:
                        self.lcd.backlight_off()
                self.send_status("LCD_BACKLIGHT_SET", "STATUS", {"state": state})

            elif cmd == "LCD_WRITE":
                row = int(params.get("row", 0))
                col = int(params.get("col", 0))
                text = str(params.get("text", ""))
                if self.lcd is not None:
                    max_len = max(self.lcd.num_columns - col, 0)
                    self.lcd.move_to(col, row)
                    self.lcd.putstr(text[:max_len])
                self.send_status(
                    "LCD_WRITTEN", "STATUS", {"row": row, "col": col, "text": text}
                )

            elif cmd == "READ_CONFIG":
                cfg_data = {
                    "device_id": self.status_data.get("device_id", ""),
                    "hw_version": cfg.get_hw_version(),
                    "sw_version": cfg.get_sw_version(),
                    "wifi_ssid": self.status_data.get("wifi_ssid", ""),
                    "wifi_password": self.mask_secret(
                        nvs_sys.get_secret("wifi_password")
                    ),
                    "mqtt_server": self.status_data.get("mqtt_server", ""),
                    "ntp_server": self.status_data.get("ntp_server", ""),
                    "mqtt_user": nvs_sys.get_secret("mqtt_username"),
                    "mqtt_password": self.mask_secret(
                        nvs_sys.get_secret("mqtt_password")
                    ),
                    "api_key": self.mask_secret(nvs_sys.get_secret("api_key")),
                    "post_time": self.status_data.get("post_time", 0),
                    "wifi_enabled": self.status_data.get("wifi_enabled", 0),
                }
                self.send_status("CONFIG_READ", "STATUS", cfg_data)

            elif cmd == "WRITE_ID":
                device_id = str(params.get("device_id", ""))
                ok = 0
                if device_id:
                    self.status_data["device_id"] = device_id
                    self.save_data()
                    self.apply_config()
                    ok = 1
                self.send_status(
                    "ID_CONFIG_WRITE", "STATUS", {"ok": ok, "device_id": device_id}
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
                        self.status_data["wifi_enabled"] = int(
                            params.get("wifi_enabled", 1)
                        )
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
                    {"ok": ok, "mqtt_server": mqtt_server, "ntp_server": ntp_server},
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

            elif cmd == "WRITE_WIFI_ENABLED":
                state = int(params.get("state", 0))
                self.status_data["wifi_enabled"] = 1 if state else 0
                self.save_data()
                self.apply_config()
                self.send_status(
                    "WIFI_ENABLED_WRITE", "STATUS", {"ok": 1, "wifi_enabled": state}
                )
                self.pending_restart = True

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
                "hw_version": "",
                "sw_version": "",
                "serial_number": "",
                "device_id": "",
                "wifi_enabled": 0,
                "wifi_ssid": "",
                "mqtt_server": "",
                "ip_address": "",
                "ntp_server": "",
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
        }

    def error_queue(self, payload, max_len=50):
        if "pending_errors" not in self.status_data or not isinstance(
            self.status_data.get("pending_errors"), list
        ):
            self.status_data["pending_errors"] = []

        src = payload.get("source")
        msg = payload.get("msg")
        for existing in self.status_data["pending_errors"]:
            if existing.get("source") == src and existing.get("msg") == msg:
                return
        self.status_data["pending_errors"].append(payload)
        if len(self.status_data["pending_errors"]) > max_len:
            self.status_data["pending_errors"] = self.status_data["pending_errors"][
                -max_len:
            ]
        now = time.ticks_ms()
        if time.ticks_diff(now, self._last_error_save_ms) >= 30000:
            self._last_error_save_ms = now
            try:
                self.save_data()
            except Exception as e:
                self.errors["SAVE_STATUS"] = str(e)

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
        self.queue_runtime_errors()
        if self.client is None:
            return
        if not self.status_data.get("pending_errors"):
            return
        batch_size = 10
        while self.status_data["pending_errors"]:
            batch = self.status_data["pending_errors"][:batch_size]
            try:
                self.send_status(
                    "ERRORS_FLUSH",
                    "ERROR",
                    {
                        "items": batch,
                        "ip_address": self.ip_address,
                    },
                )
                self.status_data["pending_errors"] = self.status_data["pending_errors"][
                    batch_size:
                ]
            except Exception as e:
                self.errors["SEND_ERRORS"] = str(e)
                break
        try:
            self.save_data()
        except Exception as e:
            self.errors["SAVE_STATUS"] = str(e)

    def set_time(self):
        try:
            if self.clock is not None and cfg.is_set_time_enabled():
                self.clock.set_time_system()
            return machine.RTC().datetime()
        except Exception as e:
            self.error_management("SET_TIME", str(e))
            return None

    def _read_radar_data(self):
        data = {}
        if self.uart1 is None or self.radar is None:
            return data
        if self.uart1.any():
            raw = self.uart1.read()
            if raw:
                self.buffer += raw

        while len(self.buffer) >= cfg.get_frame_len():
            start = self.buffer.find(cfg.get_frame_header())

            if start < 0:
                self.buffer = b""
                break

            self.buffer = self.buffer[start:]

            if len(self.buffer) < cfg.get_frame_len():
                break

            frame = self.buffer[: cfg.get_frame_len()]

            if not frame.endswith(cfg.get_frame_end()):
                self.buffer = self.buffer[1:]
                continue

            data["radar"] = []
            for i in range(3):
                target = self.radar.parse_target(frame, i)

                if target:
                    x, y, speed, distance, resolution = target
                    data["radar"].append(
                        {
                            "target": i + 1,
                            "x": x,
                            "y": y,
                            "distance": distance,
                            "speed": speed,
                            "resolution": resolution,
                        }
                    )

            self.buffer = self.buffer[cfg.get_frame_len() :]
        return data

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

        if self.sensor is not None:
            data["sensor_readings"] = []
            try:
                if cfg.is_temperature_enabled():

                    data["sensor_readings"].append(
                        {
                            "parameter_key": "temperature",
                            "value": round(self.sensor.temperature(), 2),
                        }
                    )
                if cfg.is_humidity_enabled():
                    data["sensor_readings"].append(
                        {
                            "parameter_key": "humidity",
                            "value": round(self.sensor.relative_humidity(), 2),
                        }
                    )
                if cfg.is_pressure_enabled():
                    data["sensor_readings"].append(
                        {
                            "parameter_key": "atmPressure",
                            "value": round(self.sensor.atm_pressure(), 2),
                        }
                    )
            except Exception as e:
                self.error_management("SENSOR_READ", str(e))
        if self.clock is not None and cfg.is_clock_enabled():
            try:
                rtc_data = self.clock.read_time()
                if rtc_data is not None:
                    tz_offset = self.clock.get_timezone_offset()
                    data["rtc"] = apply_timezone_offset(rtc_data, tz_offset)
                else:
                    data["rtc"] = rtc_data
            except Exception as e:
                self.error_management("RTC_READ", str(e))
        if self.adc is not None:
            try:
                batt_raw = self.adc.read_u16()
                data["battery_voltage"] = round(batt_raw * (3.3 / 65535) * 2.0, 3)
            except Exception as e:
                self.error_management("ADC_READ", str(e))
        return data

    def _update_lcd(self, data):
        if self.lcd is None:
            return
        try:
            sensor_readings = data.get("sensor_readings", [])
            t = next(
                (
                    r["value"]
                    for r in sensor_readings
                    if r.get("parameter_key") == "temperature"
                ),
                None,
            )
            h = next(
                (
                    r["value"]
                    for r in sensor_readings
                    if r.get("parameter_key") == "humidity"
                ),
                None,
            )
            if t is not None and h is not None:
                self.lcd.move_to(0, 0)
                self.lcd.putstr("T/H: {}C {}%".format(round(t, 1), round(h, 1)))
            rtc_data = data.get("rtc")
            if rtc_data is not None and self.clock is not None:
                sep = ":" if time.time() % 2 == 0 else " "
                self.lcd.move_to(0, 1)
                self.lcd.putstr(
                    "{:04d}-{:02d}-{:02d} {:02d}{}{:02d}".format(
                        rtc_data[0],
                        rtc_data[1],
                        rtc_data[2],
                        rtc_data[3],
                        sep,
                        rtc_data[4],
                    )
                )
        except Exception as e:
            self.error_management("LCD_UPDATE", str(e))

    def lcd_tick(self):
        self.pending_lcd_update = False
        self._update_lcd(self._last_sensor_data)

    def read_data(self):
        t0 = time.time()
        data = self._read_sensor_data()
        data["irq_dropped"] = self.irq_dropped
        self._last_sensor_data = data
        self._update_lcd(data)
        if self.client is not None:
            try:
                self.send_status("DATA", "DATA", data, timestamp=t0)
            except Exception as e:
                self.error_management("SEND_DATA", str(e))

    def read_usb(self, line):
        try:
            line = line.strip()
            if not line:
                return

            data = [x.strip() for x in line.split(",")]
            cmd = data[0].upper()

            if cmd == "PING":
                print(
                    "STATUS:ALIVE,{},{}".format(
                        self.status_data.get("sw_version", ""),
                        self.status_data.get("hw_version", ""),
                    )
                )

            elif cmd == "GET_STATUS":
                sensor_data = self._read_sensor_data()
                status = {
                    "ip_address": self.ip_address,
                    "uptime": time.ticks_diff(time.ticks_ms(), self.start_ticks_ms)
                    // 1000,
                    "rssi": self.wifi.get_rssi() if self.wifi else None,
                    "sw_version": cfg.get_sw_version(),
                    "hw_version": cfg.get_hw_version(),
                }
                status.update(sensor_data)
                print("OK:STATUS,{}".format(status))

            elif cmd == "RESET":
                print("OK:RESET")
                time.sleep(0.1)
                machine.reset()

            elif cmd == "SYNC_TIME":
                dt = self.set_time()
                if dt:
                    sync_time = "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z".format(
                        dt[0], dt[1], dt[2], dt[4], dt[5], dt[6]
                    )
                    print("OK:SYNC_TIME,{}".format(sync_time))
                else:
                    print("ERR:SYNC_TIME_FAILED")

            elif cmd == "READ_RTC":
                rtc_time = self.clock.read_time() if self.clock else None
                print("OK:READ_RTC,{}".format(rtc_time))

            elif cmd == "READ_SENSOR":
                sensor_data = self._read_sensor_data()
                print("OK:READ_SENSOR,{}".format(sensor_data))

            elif cmd == "READ_RADAR":
                radar_data = self._read_radar_data()
                print("OK:READ_RADAR,{}".format(radar_data))

            elif cmd == "READ_ADC":
                if self.adc is not None:
                    batt_raw = self.adc.read_u16()
                    batt_v = round(batt_raw * (3.3 / 65535) * 2.0, 3)
                    print("OK:READ_ADC,{},{}".format(batt_v, batt_raw))
                else:
                    print("ERR:READ_ADC_NOT_INITIALIZED")

            elif cmd == "SET_LED":
                if len(data) < 3:
                    print("ERR:SET_LED_FORMAT")
                    return
                idx = int(data[1])
                duty = int(data[2])
                if 0 <= idx < len(self.leds):
                    self.leds[idx].duty_u16(duty)
                    print("OK:SET_LED,{},{}".format(idx, duty))
                else:
                    print("ERR:SET_LED_INDEX")

            elif cmd == "SET_BUZZER":
                if len(data) < 3:
                    print("ERR:SET_BUZZER_FORMAT")
                    return
                freq = int(data[1])
                duty = int(data[2])
                if self.buzzer_pwm is not None:
                    if freq > 0 and duty > 0:
                        self.buzzer_pwm.freq(freq)
                        self.buzzer_pwm.duty_u16(duty)
                    else:
                        self.buzzer_pwm.duty_u16(0)
                    print("OK:SET_BUZZER,{},{}".format(freq, duty))
                else:
                    print("ERR:NO_BUZZER")

            elif cmd == "SET_RELAY":
                if len(data) < 3:
                    print("ERR:SET_RELAY_FORMAT")
                    return
                idx = int(data[1])
                state = int(data[2])
                if 0 <= idx < len(self.relays):
                    self.relays[idx].value(1 if state else 0)
                    print("OK:SET_RELAY,{},{}".format(idx, state))
                else:
                    print("ERR:SET_RELAY_INDEX")

            elif cmd == "READ_SWITCH":
                states = [sw.value() for sw in self.switches]
                print("OK:READ_SWITCH,{}".format(states))

            elif cmd == "LCD_CLEAR":
                if self.lcd is not None:
                    self.lcd.clear()
                print("OK:LCD_CLEAR")

            elif cmd == "LCD_WRITE":
                if len(data) < 4:
                    print("ERR:LCD_WRITE_FORMAT")
                    return
                row = int(data[1])
                col = int(data[2])
                text = data[3]
                if self.lcd is not None:
                    self.lcd.move_to(col, row)
                    self.lcd.putstr(text)
                print("OK:LCD_WRITE,{},{},{}".format(row, col, text))

            elif cmd == "READ_CONFIG":
                cfg_data = {
                    "device_id": self.status_data.get("device_id", ""),
                    "hw_version": cfg.get_hw_version(),
                    "sw_version": cfg.get_sw_version(),
                    "wifi_ssid": self.status_data.get("wifi_ssid", ""),
                    "wifi_password": self.mask_secret(
                        nvs_sys.get_secret("wifi_password")
                    ),
                    "mqtt_server": self.status_data.get("mqtt_server", ""),
                    "ntp_server": self.status_data.get("ntp_server", ""),
                    "mqtt_user": nvs_sys.get_secret("mqtt_username"),
                    "mqtt_password": self.mask_secret(
                        nvs_sys.get_secret("mqtt_password")
                    ),
                    "api_key": self.mask_secret(nvs_sys.get_secret("api_key")),
                    "post_time": self.status_data.get("post_time", 0),
                    "wifi_enabled": self.status_data.get("wifi_enabled", 0),
                }
                print("OK:READ_CONFIG,{}".format(cfg_data))

            elif cmd == "WRITE_ID":
                if len(data) < 2:
                    print("ERR:WRITE_ID_FORMAT")
                    return
                device_id = data[1]
                self.status_data["device_id"] = device_id
                self.save_data()
                self.apply_config()
                self.pending_restart = True
                print("OK:WRITE_ID,{}".format(device_id))

            elif cmd == "WRITE_POST_TIME":
                if len(data) < 2:
                    print("ERR:WRITE_POST_TIME_FORMAT")
                    return
                post_time = int(data[1])
                self.status_data["post_time"] = post_time
                self.save_data()
                self.apply_config()
                self.pending_restart = True
                print("OK:WRITE_POST_TIME,{}".format(post_time))

            elif cmd == "WRITE_WIFI":
                if len(data) < 3:
                    print("ERR:WRITE_WIFI_FORMAT")
                    return
                ssid = data[1]
                password = data[2]
                enabled = int(data[3]) if len(data) > 3 else 1
                ok = 0
                ok_pw = nvs_sys.set_secret("wifi_password", password)
                if ok_pw:
                    self.status_data["wifi_ssid"] = ssid
                    self.status_data["wifi_enabled"] = 1 if enabled else 0
                    self.save_data()
                    self.apply_config()
                    self.pending_restart = True
                    ok = 1
                print(
                    "OK:WRITE_WIFI,{},{},{}".format(
                        ssid, self.mask_secret(password), ok
                    )
                )

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
                    self.pending_restart = True
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
                    self.pending_restart = True
                    ok = 1
                print(
                    "OK:WRITE_MQTT_CREDENTIALS,{},{},{}".format(
                        mqtt_user, self.mask_secret(mqtt_password), ok
                    )
                )

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

            else:
                print("ERR:UNKNOWN_CMD,{}".format(cmd))

        except Exception as e:
            print("ERR:READ_USB,{}".format(e))
            self.error_management("READ_USB", str(e))
