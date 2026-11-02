import socket
from protocol import Protocol


class ControllerClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.socket = None
        self.sequence = 0

    def connect(self):
        if self.socket is None:
            self.socket = socket.create_connection((self.host, self.port), timeout=2)

    def close(self):
        if self.socket:
            self.socket.close()
            self.socket = None

    def send(self, data):
        if self.socket:
            self.socket.sendall(data)

    def _recv_exact(self, size):
        data = bytearray()

        while len(data) < size:
            chunk = self.socket.recv(size - len(data))

            if not chunk:
                raise ConnectionError("Connection closed by controller")

            data.extend(chunk)

        return bytes(data)

    def receive_response(self):
        header = self._recv_exact(3)

        if header[0] != Protocol.MAGIC:
            raise ValueError("Invalid response magic")

        length = (header[1] << 8) | header[2]

        rest = self._recv_exact(5 + length)

        return header + rest

    def _exchange(self, request):
        request_sequence = request[3]
        self.send(request)
        response = self.receive_response()
        parsed = Protocol.parse_response(response)
        if parsed["sequence"] != request_sequence:
            raise ValueError("Response sequence mismatch")
        return parsed

    def _next_sequence(self):
        self.sequence = (self.sequence + 1) & 0xFF
        return self.sequence

    def ping(self):
        sequence = self._next_sequence()
        request = Protocol.build_request(sequence, Protocol.CMD_PING)
        return self._exchange(request)

    def get_controller_info(self):
        sequence = self._next_sequence()
        request = Protocol.build_request(sequence, Protocol.CMD_GET_CONTROLLER_INFO)
        return self._exchange(request)

    def get_cards_info(self):
        sequence = self._next_sequence()
        request = Protocol.build_request(sequence, Protocol.CMD_GET_CARDS_INFO)
        return self._exchange(request)

    def identify_card(self, slot):
        sequence = self._next_sequence()
        payload = bytes([slot])
        request = Protocol.build_request(sequence, Protocol.CMD_IDENTIFY_CARD, payload)
        return self._exchange(request)

    def get_relay(self, slot, relay):
        sequence = self._next_sequence()
        payload = bytes([slot, relay])
        request = Protocol.build_request(sequence, Protocol.CMD_GET_RELAY, payload)
        return self._exchange(request)

    def set_relay(self, slot, relay, state):
        sequence = self._next_sequence()
        payload = bytes([slot, relay, state])
        request = Protocol.build_request(sequence, Protocol.CMD_SET_RELAY, payload)
        return self._exchange(request)

    def get_digital_output(self, slot, output):
        sequence = self._next_sequence()
        payload = bytes([slot, output])
        request = Protocol.build_request(
            sequence, Protocol.CMD_GET_DIGITAL_OUTPUT, payload
        )
        return self._exchange(request)

    def set_digital_output(self, slot, output, state):
        sequence = self._next_sequence()
        payload = bytes([slot, output, state])
        request = Protocol.build_request(
            sequence, Protocol.CMD_SET_DIGITAL_OUTPUT, payload
        )
        return self._exchange(request)

    def get_pwm(self, slot, pwm):
        sequence = self._next_sequence()
        payload = bytes([slot, pwm])
        request = Protocol.build_request(sequence, Protocol.CMD_GET_PWM, payload)
        return self._exchange(request)

    def set_pwm(self, slot, pwm, duty_cycle):
        sequence = self._next_sequence()
        payload = bytes([slot, pwm, duty_cycle])
        request = Protocol.build_request(sequence, Protocol.CMD_SET_PWM, payload)
        return self._exchange(request)

    def get_power_card_info(self):
        sequence = self._next_sequence()
        request = Protocol.build_request(sequence, Protocol.CMD_GET_POWER_CARD_INFO)
        return self._exchange(request)

    def get_adc_data(self):
        sequence = self._next_sequence()
        request = Protocol.build_request(sequence, Protocol.CMD_GET_ADC_DATA)
        return self._exchange(request)

    def get_ina_data(self):
        sequence = self._next_sequence()
        request = Protocol.build_request(sequence, Protocol.CMD_GET_INA_DATA)
        return self._exchange(request)

    def i2c_config(self, slot, clock_frequency, addressing_mode):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(clock_frequency).to_bytes(4, "big"))
        payload.append(addressing_mode)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_I2C_CONFIG,
            payload,
        )

        return self._exchange(request)

    def i2c_write(self, slot, addr, data):
        sequence = self._next_sequence()

        data = bytes(data)

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(addr).to_bytes(2, "big"))
        payload.extend(len(data).to_bytes(2, "big"))
        payload.extend(data)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_I2C_WRITE,
            payload,
        )

        return self._exchange(request)

    def i2c_read(self, slot, addr, length):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(addr).to_bytes(2, "big"))
        payload.extend(int(length).to_bytes(2, "big"))

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_I2C_READ,
            payload,
        )

        return self._exchange(request)

    def i2c_reg_write(self, slot, addr, reg, data):
        sequence = self._next_sequence()

        data = bytes(data)

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(addr).to_bytes(2, "big"))
        payload.append(reg)
        payload.extend(len(data).to_bytes(2, "big"))
        payload.extend(data)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_I2C_REG_WRITE,
            payload,
        )

        return self._exchange(request)

    def i2c_reg_read(self, slot, addr, reg, length):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(addr).to_bytes(2, "big"))
        payload.append(reg)
        payload.extend(int(length).to_bytes(2, "big"))

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_I2C_REG_READ,
            payload,
        )

        return self._exchange(request)

    def spi_config(
        self,
        slot,
        frequency,
        cpol,
        cpha,
        bit_order,
        data_size,
    ):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(frequency).to_bytes(4, "big"))
        payload.append(cpol)
        payload.append(cpha)
        payload.append(bit_order)
        payload.append(data_size)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_SPI_CONFIG,
            payload,
        )

        return self._exchange(request)

    def spi_transfer_byte(self, slot, tx):
        sequence = self._next_sequence()

        payload = bytes(
            [
                slot,
                tx,
            ]
        )

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_SPI_TRANSFER_BYTE,
            payload,
        )

        return self._exchange(request)

    def spi_transfer_word(self, slot, data):
        sequence = self._next_sequence()

        data = bytes(data)

        payload = bytearray()
        payload.append(slot)
        payload.extend(len(data).to_bytes(2, "big"))
        payload.extend(data)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_SPI_TRANSFER_WORD,
            payload,
        )

        return self._exchange(request)

    def uart_config(
        self,
        slot,
        baudrate,
        data_bits,
        parity,
        stop_bits,
    ):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(baudrate).to_bytes(4, "big"))
        payload.append(data_bits)
        payload.append(parity)
        payload.append(stop_bits)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_UART_CONFIG,
            payload,
        )

        return self._exchange(request)

    def uart_write(self, slot, data):
        sequence = self._next_sequence()

        if isinstance(data, str):
            data = data.encode()

        data = bytes(data)

        payload = bytearray()
        payload.append(slot)
        payload.extend(len(data).to_bytes(2, "big"))
        payload.extend(data)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_UART_WRITE,
            payload,
        )

        return self._exchange(request)

    def uart_read(self, slot):
        sequence = self._next_sequence()

        payload = bytes([slot])

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_UART_READ,
            payload,
        )

        return self._exchange(request)

    def rs232_config(
        self,
        slot,
        baudrate,
        data_bits,
        parity,
        stop_bits,
    ):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(baudrate).to_bytes(4, "big"))
        payload.append(data_bits)
        payload.append(parity)
        payload.append(stop_bits)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_RS232_CONFIG,
            payload,
        )

        return self._exchange(request)

    def rs232_write(self, slot, data):
        sequence = self._next_sequence()

        if isinstance(data, str):
            data = data.encode()

        data = bytes(data)

        payload = bytearray()
        payload.append(slot)
        payload.extend(len(data).to_bytes(2, "big"))
        payload.extend(data)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_RS232_WRITE,
            payload,
        )

        return self._exchange(request)

    def rs232_read(self, slot):
        sequence = self._next_sequence()

        payload = bytes([slot])

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_RS232_READ,
            payload,
        )

        return self._exchange(request)

    def rs485_config(
        self,
        slot,
        baudrate,
        data_bits,
        parity,
        stop_bits,
    ):
        sequence = self._next_sequence()

        payload = bytearray()
        payload.append(slot)
        payload.extend(int(baudrate).to_bytes(4, "big"))
        payload.append(data_bits)
        payload.append(parity)
        payload.append(stop_bits)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_RS485_CONFIG,
            payload,
        )

        return self._exchange(request)

    def rs485_write(self, slot, data):
        sequence = self._next_sequence()

        if isinstance(data, str):
            data = data.encode()

        data = bytes(data)

        payload = bytearray()
        payload.append(slot)
        payload.extend(len(data).to_bytes(2, "big"))
        payload.extend(data)

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_RS485_WRITE,
            payload,
        )

        return self._exchange(request)

    def rs485_read(self, slot):
        sequence = self._next_sequence()

        payload = bytes([slot])

        request = Protocol.build_request(
            sequence,
            Protocol.CMD_RS485_READ,
            payload,
        )

        return self._exchange(request)

    def can_config(
        self,
        slot,
        nominal_bitrate=1,
        data_bitrate=1,
        fd_enabled=0,
        brs_enabled=0,
        mode=2,
        can_termination=0,
    ):
        if nominal_bitrate not in (0, 1):
            raise ValueError("nominal_bitrate: 0=125 kbps, 1=500 kbps")
        if data_bitrate not in (0, 1):
            raise ValueError("data_bitrate: 0=1 Mbps, 1=2 Mbps")
        if mode not in (0, 1, 2):
            raise ValueError("mode: 0=normal, 1=internal loopback, 2=external loopback")
        if brs_enabled and not fd_enabled:
            raise ValueError("BRS requires CAN FD")

        payload = bytearray([slot])
        payload.extend(int(nominal_bitrate).to_bytes(4, "big"))
        payload.extend(int(data_bitrate).to_bytes(4, "big"))
        payload.extend(
            [
                int(bool(fd_enabled)),
                int(bool(brs_enabled)),
                mode,
                int(bool(can_termination)),
            ]
        )

        request = Protocol.build_request(
            self._next_sequence(), Protocol.CMD_CAN_CONFIG, payload
        )
        return self._exchange(request)

    def can_std_send(self, slot, can_id, data=b""):
        if not 0 <= can_id <= 0x7FF:
            raise ValueError("CAN STD ID must be in range 0x000..0x7FF")

        data = bytes(data)
        if len(data) > 8:
            raise ValueError("Classic CAN supports at most 8 data bytes")

        can_frame = int(can_id).to_bytes(4, "big") + data
        payload = bytes([slot]) + len(can_frame).to_bytes(2, "big") + can_frame
        request = Protocol.build_request(
            self._next_sequence(), Protocol.CMD_CAN_STD_SEND, payload
        )
        return self._exchange(request)

    def can_fd_send(self, slot, can_id, data=b""):
        if not 0 <= can_id <= 0x7FF:
            raise ValueError("Currently CAN FD send supports 11-bit IDs only")

        data = bytes(data)
        allowed_sizes = set(range(9)) | {12, 16, 20, 24, 32, 48}
        if len(data) not in allowed_sizes:
            raise ValueError(
                "CAN FD length must match a valid DLC; currently supported: "
                "0..8, 12, 16, 20, 24, 32 or 48 bytes"
            )

        can_frame = int(can_id).to_bytes(4, "big") + data
        payload = bytes([slot]) + len(can_frame).to_bytes(2, "big") + can_frame
        request = Protocol.build_request(
            self._next_sequence(), Protocol.CMD_CAN_FD_SEND, payload
        )
        return self._exchange(request)

    def can_receive(self, slot):
        request = Protocol.build_request(
            self._next_sequence(), Protocol.CMD_CAN_RECEIVE, bytes([slot])
        )
        return self._exchange(request)

    def multipart_test(self, slot):
        request = Protocol.build_request(
            self._next_sequence(), Protocol.CMD_MULTIPART_TEST, bytes([slot])
        )

        return self._exchange(request)
