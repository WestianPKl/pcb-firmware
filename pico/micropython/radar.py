class RadarParser:
    def __init__(self, config):
        self.config = config

    def _u16(self, data, pos):
        return data[pos] | (data[pos + 1] << 8)

    def _radar_signed(self, raw):
        if raw & 0x8000:
            return raw - 0x8000
        return -raw

    def _isqrt(self, n):
        x = n
        y = (x + 1) // 2
        while y < x:
            x = y
            y = (x + n // x) // 2
        return x

    def parse_target(self, frame, index):
        pos = 4 + index * 8

        raw_x = self._u16(frame, pos)
        raw_y = self._u16(frame, pos + 2)
        raw_speed = self._u16(frame, pos + 4)
        resolution = self._u16(frame, pos + 6)

        if raw_x == 0 and raw_y == 0 and raw_speed == 0 and resolution == 0:
            return None

        x = self._radar_signed(raw_x)
        y = self._radar_signed(raw_y)
        speed = self._radar_signed(raw_speed)
        distance = self._isqrt(x * x + y * y)

        return x, y, speed, distance, resolution
