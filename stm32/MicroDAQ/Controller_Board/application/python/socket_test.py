import socket

HOST = "192.168.18.200"
PORT = 5001

MAGIC = 0xAA

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


def build_request(sequence, command, payload=b""):
    length = len(payload)

    frame = bytearray()

    frame.append(MAGIC)

    frame.append((length >> 8) & 0xFF)
    frame.append(length & 0xFF)

    frame.append(sequence)
    frame.append(command)

    frame.extend(payload)

    crc = crc16(frame[1:])

    frame.append((crc >> 8) & 0xFF)
    frame.append(crc & 0xFF)

    return bytes(frame)


# request = build_request(sequence=0x01, command=CMD_PING)
# request = build_request(sequence=0x02, command=CMD_IDENTIFY_CARD, payload=bytes([3]))
# request = build_request(sequence=0x02, command=CMD_GET_RELAY, payload=bytes([3, 1]))
request = build_request(sequence=0x02, command=CMD_SET_RELAY, payload=bytes([2, 1, 0]))

print("TX:", request.hex(" ").upper())

with socket.create_connection((HOST, PORT), timeout=2) as s:
    s.sendall(request)

    response = s.recv(1024)

    print("RX:", response.hex(" ").upper())
