import ujson, os


class NVSSystem:
    def __init__(self, filename: str = "secrets.json"):
        self._filename = filename
        self._data = {}
        self._load()

    def _load(self):
        try:
            with open(self._filename, "r") as f:
                self._data = ujson.load(f)
        except Exception:
            self._data = {}

    def _save(self):
        tmp = self._filename + ".tmp"
        with open(tmp, "w") as f:
            ujson.dump(self._data, f)
        os.rename(tmp, self._filename)

    def set_secret(self, key: str, value: str) -> bool:
        try:
            self._data[key] = value
            self._save()
            return True
        except Exception:
            return False

    def get_secret(self, key: str, default: str = "") -> str:
        return self._data.get(key, default)

    def delete_secret(self, key: str) -> bool:
        try:
            if key in self._data:
                del self._data[key]
                self._save()
            return True
        except Exception:
            return False

    def has_secret(self, key: str) -> bool:
        return key in self._data and bool(self._data[key])
