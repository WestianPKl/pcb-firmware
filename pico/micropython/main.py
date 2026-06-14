import time, ntptime, sys, uselect, machine
from wireless import WiFi
from program import Program
from config import get_config

cfg = get_config()

USB_MAX_LINE = 256


def _disconnect_mqtt(program):
    c = program.get_client()
    if c is not None:
        try:
            c.disconnect()
        except (OSError, Exception):
            pass
        program.set_client(None)


def _connect_wifi(program, wifi, source="WIFI"):
    if wifi is None:
        program.set_ip_address(None)
        return None

    try:
        try:
            wifi.connect(timeout=cfg.get_wifi_connect_timeout())
        except TypeError:
            # Compatibility with WiFi.connect() implementations without timeout arg.
            wifi.connect()

        ip_address = wifi.get_ip()
        if ip_address is None:
            raise Exception("No IP address after WiFi connect")

        program.set_ip_address(ip_address)
        return ip_address
    except (OSError, Exception) as e:
        program.set_ip_address(None)
        program.error_management(source, "Connect failed: {}".format(e))
        return None


def _sync_ntp(program):
    try:
        ntp_host = cfg.get_ntp_server_ip()
        if ntp_host:
            ntptime.host = ntp_host
        ntptime.settime()
        program.set_time()
        return True
    except Exception as e:
        program.error_management("NTP", "settime failed: {}".format(e))
        return False


def _read_usb_line(poll, usb_buf, program):
    events = poll.poll(50)
    if not events:
        return

    ch = sys.stdin.read(1)
    if ch in ("\r", "\n"):
        if usb_buf:
            line = "".join(usb_buf).strip()
            usb_buf[:] = []
            if line:
                program.read_usb(line)
        return

    if ch:
        if len(usb_buf) < USB_MAX_LINE:
            usb_buf.append(ch)
        else:
            usb_buf[:] = []
            print("ERR:USB_LINE_TOO_LONG")


def main():
    program = Program()

    time.sleep(2)

    program.apply_config()
    program.init_hardware()

    wifi = None
    ip_address = None
    ntp_done_after_wifi = False

    wifi_reconnect_ms = cfg.get_wifi_reconnect_ms()
    mqtt_reconnect_ms = cfg.get_mqtt_reconnect_ms()
    last_wifi_try = time.ticks_ms()
    last_mqtt_try = time.ticks_ms()

    device_id = cfg.get_device_id()

    if not device_id or device_id == 0:
        print("WARN:CONFIG:Invalid device ID in configuration")

    if cfg.is_wifi_enabled() and (cfg.get_ssid() and cfg.get_password()):
        wifi = WiFi(cfg.get_ssid(), cfg.get_password())
        program.set_wifi(wifi)
        ip_address = _connect_wifi(program, wifi, "WIFI")
    else:
        program.set_wifi(None)

    if ip_address is not None:
        ntp_done_after_wifi = _sync_ntp(program)
        try:
            program.mqtt_initialization()
        except Exception as e:
            program.error_management(
                "MQTT",
                "MQTT client initialization failed: {}".format(e),
            )
            program.set_client(None)
    elif cfg.is_wifi_enabled() and (cfg.get_ssid() and cfg.get_password()):
        program.error_management("WIFI", "Wifi could not be connected")
        program.set_client(None)

    if (
        program.get_client() is not None
        and cfg.is_wifi_enabled()
        and ip_address is not None
    ):
        program.send_errors()
        try:
            program.send_status(
                "START",
                "STATUS",
                {
                    "device_id": device_id,
                    "ip_address": ip_address,
                    "uptime": time.ticks_diff(time.ticks_ms(), program.start_ticks_ms)
                    // 1000,
                    "rssi": wifi.get_rssi() if wifi else None,
                    "sw_version": cfg.get_sw_version(),
                    "hw_version": cfg.get_hw_version(),
                },
            )
        except Exception as e:
            program.error_management("MQTT", "Failed to send status: {}".format(e))

    poll = uselect.poll()
    poll.register(sys.stdin, uselect.POLLIN)

    usb_buf = []
    while True:
        now = time.ticks_ms()
        _read_usb_line(poll, usb_buf, program)

        if program.pending_restart:
            program.pending_restart = False
            program.restart_connections()

            wifi = None
            ip_address = None
            program.set_ip_address(None)
            ntp_done_after_wifi = False
            last_wifi_try = time.ticks_ms()
            last_mqtt_try = time.ticks_ms()

            if cfg.is_wifi_enabled() and (cfg.get_ssid() and cfg.get_password()):
                wifi = WiFi(cfg.get_ssid(), cfg.get_password())
                program.set_wifi(wifi)
                ip_address = _connect_wifi(program, wifi, "WIFI")
            else:
                program.set_wifi(None)

            time.sleep(0.2)
            continue

        if program.pending_lcd_update:
            program.lcd_tick()

        if program.pending_read:
            program.pending_read = False
            program.read_data()

        if not cfg.is_wifi_enabled():
            _disconnect_mqtt(program)
            ip_address = None
            program.set_ip_address(None)
            time.sleep(cfg.get_poll_interval_ms() / 1000.0)
            continue

        if cfg.get_ssid() == "" or cfg.get_password() == "":
            _disconnect_mqtt(program)
            ip_address = None
            program.set_ip_address(None)
            time.sleep(cfg.get_no_wifi_sleep_ms() / 1000.0)
            continue

        if wifi is None:
            wifi = WiFi(cfg.get_ssid(), cfg.get_password())
            program.set_wifi(wifi)

        wifi_connected = False
        try:
            wifi_connected = wifi.is_connected() and wifi.get_ip() is not None
        except (OSError, Exception) as e:
            program.error_management("WIFI", "Status check failed: {}".format(e))

        if not wifi_connected:
            ntp_done_after_wifi = False
            _disconnect_mqtt(program)
            ip_address = None
            program.set_ip_address(None)

            if time.ticks_diff(now, last_wifi_try) >= wifi_reconnect_ms:
                last_wifi_try = now
                ip_address = _connect_wifi(program, wifi, "WIFI")

            time.sleep(cfg.get_no_wifi_sleep_ms() / 1000.0)
            continue

        current_ip = wifi.get_ip()
        if current_ip != ip_address:
            ip_address = current_ip
            program.set_ip_address(ip_address)
            ntp_done_after_wifi = False

        if not ntp_done_after_wifi:
            ntp_done_after_wifi = _sync_ntp(program)

        if program.get_client() is None and ip_address is not None:
            if time.ticks_diff(now, last_mqtt_try) >= mqtt_reconnect_ms:
                last_mqtt_try = now
                try:
                    program.mqtt_initialization()
                    program.send_errors()
                    program.send_status(
                        "RECONNECTED",
                        "STATUS",
                        {"ip_address": ip_address},
                    )
                except Exception as e:
                    program.error_management("MQTT", "Reconnect failed: {}".format(e))
                    program.set_client(None)

        c = program.get_client()
        if c is not None and ip_address is not None:
            try:
                c.check_msg()
            except (OSError, Exception) as e:
                program.error_management("MQTT", "MQTT check_msg failed: {}".format(e))
                _disconnect_mqtt(program)

        time.sleep(cfg.get_poll_interval_ms() / 1000.0)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("Fatal error: {}".format(e))
        time.sleep(5)
        machine.reset()
