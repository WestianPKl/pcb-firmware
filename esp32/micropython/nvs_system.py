import esp32


class NVSSystem:
    def __init__(self, namespace: str = "secrets"):
        self.namespace = namespace

    def _nvs(self):
        return esp32.NVS(self.namespace)

    def set_secret(self, key: str, value: str) -> bool:
        try:
            nvs = self._nvs()
            data = value.encode("utf-8")
            nvs.set_blob(key, data)
            nvs.commit()
            return True
        except (OSError, ValueError):
            return False

    def get_secret(self, key: str, default: str = "") -> str:
        try:
            nvs = self._nvs()

            size = nvs.get_blob(key, bytearray())
            if size <= 0:
                return default

            buf = bytearray(size)
            nvs.get_blob(key, buf)
            return buf.decode("utf-8")

        except (OSError, ValueError, UnicodeError):
            return default

    def delete_secret(self, key: str) -> bool:
        try:
            nvs = self._nvs()
            nvs.erase_key(key)
            nvs.commit()
            return True
        except OSError:
            return False

    def has_secret(self, key: str) -> bool:
        try:
            nvs = self._nvs()
            size = nvs.get_blob(key, bytearray())
            return size > 0
        except OSError:
            return False
