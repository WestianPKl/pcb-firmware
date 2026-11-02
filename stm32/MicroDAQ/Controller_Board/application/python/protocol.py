class Protocol:
    MAGIC = 0xAA

    STATUS_OK = 0x40
    STATUS_ERROR = 0x7F
    STATUS_BUSY = 0x41
    STATUS_TIMEOUT = 0x42

    CARD_INFO_SIZE = 47

    CMD_PING = 0x00
    CMD_GET_CONTROLLER_INFO = 0x01
    CMD_GET_CARDS_INFO = 0x10
    CMD_IDENTIFY_CARD = 0x11
    CMD_GET_RELAY = 0x20
    CMD_SET_RELAY = 0x21
    CMD_GET_DIGITAL_OUTPUT = 0x22
    CMD_SET_DIGITAL_OUTPUT = 0x23
    CMD_GET_PWM = 0x24
    CMD_SET_PWM = 0x25
    CMD_GET_POWER_CARD_INFO = 0x30
    CMD_GET_ADC_DATA = 0x31
    CMD_GET_INA_DATA = 0x32
    CMD_I2C_CONFIG = 0x40
    CMD_I2C_WRITE = 0x41
    CMD_I2C_READ = 0x42
    CMD_I2C_REG_WRITE = 0x43
    CMD_I2C_REG_READ = 0x44
    CMD_SPI_CONFIG = 0x45
    CMD_SPI_TRANSFER_BYTE = 0x46
    CMD_SPI_TRANSFER_WORD = 0x47
    CMD_UART_CONFIG = 0x48
    CMD_UART_WRITE = 0x49
    CMD_UART_READ = 0x4A
    CMD_RS232_CONFIG = 0x4B
    CMD_RS232_WRITE = 0x4C
    CMD_RS232_READ = 0x4D
    CMD_RS485_CONFIG = 0x4E
    CMD_RS485_WRITE = 0x4F
    CMD_RS485_READ = 0x50
    CMD_CAN_CONFIG = 0x51
    CMD_CAN_STD_SEND = 0x52
    CMD_CAN_FD_SEND = 0x53
    CMD_CAN_RECEIVE = 0x54
    CMD_MULTIPART_TEST = 0xFF

    @staticmethod
    def crc16(data):
        crc = 0xFFFF

        for byte in data:
            crc ^= byte << 8

            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc <<= 1

                crc &= 0xFFFF

        return crc

    @staticmethod
    def build_request(sequence, command, payload=b""):
        length = len(payload)

        frame = bytearray()

        frame.append(Protocol.MAGIC)

        frame.append((length >> 8) & 0xFF)
        frame.append(length & 0xFF)

        frame.append(sequence)
        frame.append(command)

        frame.extend(payload)

        crc = Protocol.crc16(frame[1:])

        frame.append((crc >> 8) & 0xFF)
        frame.append(crc & 0xFF)

        return bytes(frame)

    @staticmethod
    def parse_response(response):

        if len(response) < 8:
            raise ValueError("Response too short")

        if response[0] != Protocol.MAGIC:
            raise ValueError("Invalid response magic")

        length = int((response[1] << 8) | response[2])
        expected_length = 8 + length

        if len(response) != expected_length:
            raise ValueError("Invalid response frame length")

        received_crc = (response[-2] << 8) | response[-1]
        calculated_crc = Protocol.crc16(response[1:-2])

        if received_crc != calculated_crc:
            raise ValueError("Invalid response CRC")

        sequence = int(response[3])
        command = int(response[4])
        status = int(response[5])
        payload = response[6 : 6 + length]

        if length != len(payload):
            raise ValueError("Invalid response length")

        if command == Protocol.CMD_PING and status == Protocol.STATUS_OK:
            data = payload.hex()

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": data,
            }

        elif (
            command == Protocol.CMD_GET_CONTROLLER_INFO and status == Protocol.STATUS_OK
        ):
            if length != 26:
                raise ValueError("Invalid controller info length")

            card_type = int(payload[0] << 8 | payload[1])
            hw_version = f"{payload[2]}.{payload[3]}"
            production_year = int(payload[4] << 8 | payload[5])
            production_month = int(payload[6])
            production_day = int(payload[7])
            production_date = (
                f"{production_year:04d}-{production_month:02d}-{production_day:02d}"
            )
            serial_number = int.from_bytes(payload[8:12], "big")
            fw_version = f"{payload[12]}.{payload[13]}.{payload[14]}"
            fw_year = int(payload[15] << 8 | payload[16])
            fw_month = int(payload[17])
            fw_day = int(payload[18])
            fw_date = f"{fw_year:04d}-{fw_month:02d}-{fw_day:02d}"
            protocol_version = f"{payload[19]}.{payload[20]}"
            uptime = int.from_bytes(payload[21:25], "big")
            slot_count = int(payload[25])
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "card_type": card_type,
                    "hw_version": hw_version,
                    "production_date": production_date,
                    "serial_number": serial_number,
                    "fw_version": fw_version,
                    "fw_date": fw_date,
                    "protocol_version": protocol_version,
                    "uptime": uptime,
                    "slot_count": slot_count,
                },
            }

        elif command == Protocol.CMD_GET_CARDS_INFO and status == Protocol.STATUS_OK:
            if length % Protocol.CARD_INFO_SIZE != 0:
                raise ValueError("Invalid cards info length")

            card_count = length // Protocol.CARD_INFO_SIZE

            cards = []
            for slot, i in enumerate(
                range(0, len(payload), Protocol.CARD_INFO_SIZE), start=1
            ):
                detected = int(payload[i + 0])
                fault = int(payload[i + 1])
                identified = int(payload[i + 2])
                card_type = int(payload[i + 3] << 8 | payload[i + 4])
                hw_version = f"{payload[i + 5]}.{payload[i + 6]}"
                production_year = int(payload[i + 7] << 8 | payload[i + 8])
                production_month = int(payload[i + 9])
                production_day = int(payload[i + 10])
                production_date = (
                    f"{production_year:04d}-{production_month:02d}-{production_day:02d}"
                )
                serial_number = int.from_bytes(payload[i + 11 : i + 15], "big")
                fw_version = f"{payload[i + 15]}.{payload[i + 16]}.{payload[i + 17]}"
                fw_year = int(payload[i + 18] << 8 | payload[i + 19])
                fw_month = int(payload[i + 20])
                fw_day = int(payload[i + 21])
                fw_date = f"{fw_year:04d}-{fw_month:02d}-{fw_day:02d}"
                protocol_version = f"{payload[i + 22]}.{payload[i + 23]}"
                uptime = int.from_bytes(payload[i + 24 : i + 28], "big")
                capabilities_flags = int.from_bytes(
                    payload[i + 28 : i + 32],
                    "big",
                )
                relay_count = int(payload[i + 32])
                voltage_count = int(payload[i + 33])
                current_count = int(payload[i + 34])
                frequency_count = int(payload[i + 35])
                digital_input_count = int(payload[i + 36])
                digital_output_count = int(payload[i + 37])
                pwm_count = int(payload[i + 38])
                analog_output_count = int(payload[i + 39])
                can_count = int(payload[i + 40])
                lin_count = int(payload[i + 41])
                uart_count = int(payload[i + 42])
                rs485_count = int(payload[i + 43])
                rs232_count = int(payload[i + 44])
                i2c_count = int(payload[i + 45])
                spi_count = int(payload[i + 46])

                cards.append(
                    {
                        "slot": slot,
                        "status": {
                            "detected": bool(detected),
                            "fault": bool(fault),
                            "identified": bool(identified),
                        },
                        "card_type": card_type,
                        "hw_version": hw_version,
                        "production_date": production_date,
                        "serial_number": serial_number,
                        "fw_version": fw_version,
                        "fw_date": fw_date,
                        "protocol_version": protocol_version,
                        "uptime": uptime,
                        "capabilities_flags": capabilities_flags,
                        "relay_count": relay_count,
                        "voltage_count": voltage_count,
                        "current_count": current_count,
                        "frequency_count": frequency_count,
                        "digital_input_count": digital_input_count,
                        "digital_output_count": digital_output_count,
                        "pwm_count": pwm_count,
                        "analog_output_count": analog_output_count,
                        "can_count": can_count,
                        "lin_count": lin_count,
                        "uart_count": uart_count,
                        "rs485_count": rs485_count,
                        "rs232_count": rs232_count,
                        "i2c_count": i2c_count,
                        "spi_count": spi_count,
                    }
                )
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": cards,
            }

        elif command == Protocol.CMD_GET_RELAY and status == Protocol.STATUS_OK:
            if length != 1:
                raise ValueError("Invalid relay response length")

            data = int(payload[0])
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": data,
            }

        elif command == Protocol.CMD_SET_RELAY and status == Protocol.STATUS_OK:
            if length != 3:
                raise ValueError("Invalid set relay response length")

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "slot": int(payload[0]),
                    "relay": int(payload[1]),
                    "state": int(payload[2]),
                },
            }

        elif (
            command == Protocol.CMD_GET_DIGITAL_OUTPUT and status == Protocol.STATUS_OK
        ):
            if length != 1:
                raise ValueError("Invalid digital output response length")

            data = int(payload[0])
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": data,
            }

        elif (
            command == Protocol.CMD_SET_DIGITAL_OUTPUT and status == Protocol.STATUS_OK
        ):
            if length != 3:
                raise ValueError("Invalid set digital output response length")

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "slot": int(payload[0]),
                    "output": int(payload[1]),
                    "state": int(payload[2]),
                },
            }

        elif command == Protocol.CMD_GET_PWM and status == Protocol.STATUS_OK:
            if length != 1:
                raise ValueError("Invalid PWM response length")

            data = int(payload[0])
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": data,
            }

        elif command == Protocol.CMD_SET_PWM and status == Protocol.STATUS_OK:
            if length != 3:
                raise ValueError("Invalid set PWM response length")

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "slot": int(payload[0]),
                    "pwm": int(payload[1]),
                    "duty_cycle": int(payload[2]),
                },
            }

        elif (
            command == Protocol.CMD_GET_POWER_CARD_INFO and status == Protocol.STATUS_OK
        ):
            if length != 47:
                raise ValueError("Invalid power card info response length")

            detected = int(payload[0])
            fault = int(payload[1])
            identified = int(payload[2])
            card_type = int(payload[3] << 8 | payload[4])
            hw_version = f"{payload[5]}.{payload[6]}"
            production_year = int(payload[7] << 8 | payload[8])
            production_month = int(payload[9])
            production_day = int(payload[10])
            production_date = (
                f"{production_year:04d}-{production_month:02d}-{production_day:02d}"
            )
            serial_number = int.from_bytes(payload[11:15], "big")
            fw_version = f"{payload[15]}.{payload[16]}.{payload[17]}"
            fw_year = int(payload[18] << 8 | payload[19])
            fw_month = int(payload[20])
            fw_day = int(payload[21])
            fw_date = f"{fw_year:04d}-{fw_month:02d}-{fw_day:02d}"
            protocol_version = f"{payload[22]}.{payload[23]}"
            uptime = int.from_bytes(payload[24:28], "big")
            capabilities_flags = int.from_bytes(
                payload[28:32],
                "big",
            )
            relay_count = int(payload[32])
            voltage_count = int(payload[33])
            current_count = int(payload[34])
            frequency_count = int(payload[35])
            digital_input_count = int(payload[36])
            digital_output_count = int(payload[37])
            pwm_count = int(payload[38])
            analog_output_count = int(payload[39])
            can_count = int(payload[40])
            lin_count = int(payload[41])
            uart_count = int(payload[42])
            rs485_count = int(payload[43])
            rs232_count = int(payload[44])
            i2c_count = int(payload[45])
            spi_count = int(payload[46])

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "status": {
                        "detected": detected,
                        "fault": fault,
                        "identified": identified,
                    },
                    "card_type": card_type,
                    "hw_version": hw_version,
                    "production_date": production_date,
                    "serial_number": serial_number,
                    "fw_version": fw_version,
                    "fw_date": fw_date,
                    "protocol_version": protocol_version,
                    "uptime": uptime,
                    "capabilities_flags": capabilities_flags,
                    "relay_count": relay_count,
                    "voltage_count": voltage_count,
                    "current_count": current_count,
                    "frequency_count": frequency_count,
                    "digital_input_count": digital_input_count,
                    "digital_output_count": digital_output_count,
                    "pwm_count": pwm_count,
                    "analog_output_count": analog_output_count,
                    "can_count": can_count,
                    "lin_count": lin_count,
                    "uart_count": uart_count,
                    "rs485_count": rs485_count,
                    "rs232_count": rs232_count,
                    "i2c_count": i2c_count,
                    "spi_count": spi_count,
                },
            }

        elif command == Protocol.CMD_GET_ADC_DATA and status == Protocol.STATUS_OK:
            if length != 8:
                raise ValueError("Invalid ADC data response length")

            voltage_in = int.from_bytes(payload[0:2], "big") / 1000.0
            voltage_12v = int.from_bytes(payload[2:4], "big") / 1000.0
            voltage_5v = int.from_bytes(payload[4:6], "big") / 1000.0
            voltage_3v3 = int.from_bytes(payload[6:8], "big") / 1000.0

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "voltage_in_v": voltage_in,
                    "voltage_12v_v": voltage_12v,
                    "voltage_5v_v": voltage_5v,
                    "voltage_3v3_v": voltage_3v3,
                },
            }

        elif command == Protocol.CMD_GET_INA_DATA and status == Protocol.STATUS_OK:
            if length != 16:
                raise ValueError("Invalid INA data response length")

            current_a = (
                int.from_bytes(
                    payload[0:4],
                    "big",
                    signed=True,
                )
                / 1000.0
            )

            voltage_v = (
                int.from_bytes(
                    payload[4:8],
                    "big",
                )
                / 1000.0
            )

            power_w = (
                int.from_bytes(
                    payload[8:12],
                    "big",
                )
                / 1000.0
            )

            shunt_uv = (
                int.from_bytes(
                    payload[12:16],
                    "big",
                    signed=True,
                )
                / 10.0
            )

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": {
                    "current_a": current_a,
                    "voltage_v": voltage_v,
                    "power_w": power_w,
                    "shunt_uv": shunt_uv,
                },
            }

        elif (
            command
            in (
                Protocol.CMD_I2C_CONFIG,
                Protocol.CMD_I2C_WRITE,
                Protocol.CMD_I2C_REG_WRITE,
                Protocol.CMD_SPI_CONFIG,
                Protocol.CMD_UART_CONFIG,
                Protocol.CMD_UART_WRITE,
                Protocol.CMD_RS232_CONFIG,
                Protocol.CMD_RS232_WRITE,
                Protocol.CMD_RS485_CONFIG,
                Protocol.CMD_RS485_WRITE,
                Protocol.CMD_CAN_CONFIG,
                Protocol.CMD_CAN_STD_SEND,
                Protocol.CMD_CAN_FD_SEND,
            )
            and status == Protocol.STATUS_OK
        ):
            if length != 0:
                raise ValueError("Invalid command response length")

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
            }

        elif (
            command
            in (
                Protocol.CMD_I2C_READ,
                Protocol.CMD_I2C_REG_READ,
            )
            and status == Protocol.STATUS_OK
        ):
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": bytes(payload),
            }

        elif command == Protocol.CMD_SPI_TRANSFER_BYTE and status == Protocol.STATUS_OK:
            if length != 1:
                raise ValueError("Invalid SPI byte response length")

            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": int(payload[0]),
            }

        elif command == Protocol.CMD_SPI_TRANSFER_WORD and status == Protocol.STATUS_OK:
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": bytes(payload),
            }

        elif (
            command
            in (
                Protocol.CMD_UART_READ,
                Protocol.CMD_RS232_READ,
                Protocol.CMD_RS485_READ,
            )
            and status == Protocol.STATUS_OK
        ):
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": bytes(payload),
            }

        elif command == Protocol.CMD_CAN_RECEIVE and status == Protocol.STATUS_OK:
            if length == 0:
                decoded = None
            else:
                if length < 6:
                    raise ValueError("Invalid CAN receive metadata length")
                can_id = int.from_bytes(payload[0:4], "big")
                flags = payload[4]
                can_length = payload[5]
                if length != 6 + can_length:
                    raise ValueError("CAN receive data length mismatch")
                decoded = {
                    "id": can_id,
                    "extended": bool(flags & 0x01),
                    "fd": bool(flags & 0x02),
                    "brs": bool(flags & 0x04),
                    "length": can_length,
                    "data": bytes(payload[6:]),
                }
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "data": decoded,
            }

        elif length == 0:
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
            }

        else:
            return {
                "length": length,
                "sequence": sequence,
                "command": command,
                "status": status,
                "payload": payload,
            }
