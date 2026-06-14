import network
import time


class WiFi:
    def __init__(self, ssid, password):
        self.ssid = ssid
        self.password = password
        self.wlan = network.WLAN(network.STA_IF)
        self.wlan.active(True)

    def connect(self, timeout=30):
        if self.wlan is None:
            self.wlan = network.WLAN(network.STA_IF)
            self.wlan.active(True)

        if self.wlan.isconnected():
            return self.wlan.ifconfig()

        status = self.wlan.status()

        if status == network.STAT_CONNECTING:
            start = time.ticks_ms()
            while not self.wlan.isconnected():
                if time.ticks_diff(time.ticks_ms(), start) > timeout * 1000:
                    raise Exception(
                        "WiFi timeout while connecting, status={}".format(
                            self.wlan.status()
                        )
                    )
                time.sleep(0.2)
            return self.wlan.ifconfig()

        self.wlan.active(True)
        self.wlan.connect(self.ssid, self.password)

        start = time.ticks_ms()
        while not self.wlan.isconnected():
            if time.ticks_diff(time.ticks_ms(), start) > timeout * 1000:
                st = self.wlan.status()
                raise Exception("WiFi timeout, status={}".format(st))
            time.sleep(0.2)

        return self.wlan.ifconfig()

    def disconnect(self):
        if self.wlan is not None:
            try:
                self.wlan.disconnect()
            except Exception:
                pass

    def is_connected(self):
        return self.wlan is not None and self.wlan.isconnected()

    def get_ip(self):
        if self.wlan is not None and self.wlan.isconnected():
            return self.wlan.ifconfig()[0]
        return None

    def get_rssi(self):
        if self.wlan is not None and self.wlan.isconnected():
            return self.wlan.status("rssi")
        return None

    def reconfigure(self, ssid, password):
        self.ssid = ssid
        self.password = password

    def deinit(self, deactivate=True):
        if self.wlan is not None:
            self.wlan.active(False)
            self.wlan = None
