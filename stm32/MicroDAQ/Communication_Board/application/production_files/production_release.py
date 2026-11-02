import struct
import zlib

CARD_TYPE_POWER = 0x0001
CARD_TYPE_SWITCH = 0x0002
CARD_TYPE_MEASUREMENT = 0x0003
CARD_TYPE_COMMUNICATION = 0x0004
CARD_TYPE_CONTROLLER = 0x0005
CARD_TYPE_OUTPUT = 0x0006

CAP_RELAY = 1 << 0
CAP_VOLTAGE = 1 << 1
CAP_CURRENT = 1 << 2
CAP_FREQUENCY = 1 << 3
CAP_DIGITAL_INPUT = 1 << 4
CAP_DIGITAL_OUTPUT = 1 << 5
CAP_PWM = 1 << 6
CAP_ANALOG_OUTPUT = 1 << 7
CAP_CAN = 1 << 8
CAP_CAN_FD = 1 << 9
CAP_LIN = 1 << 10
CAP_UART = 1 << 11
CAP_RS485 = 1 << 12
CAP_RS232 = 1 << 13
CAP_I2C = 1 << 14
CAP_SPI = 1 << 15

INFO_ADDR = 0x0803E000
INFO_MAGIC = 0x43415244
INFO_SIZE = 2048

struct_version = 2

hw_major = 1
hw_minor = 0

year = 2026
month = 10
day = 28

serial_number = 0x12365668
card_type = CARD_TYPE_COMMUNICATION

reserved0 = 0xFFFF
reserved1 = 0xFFFFFFFF

# COMMUNICATION CARD HW 1.0
cap_flags = (
    CAP_CAN
    | CAP_CAN_FD
    | CAP_LIN
    | CAP_UART
    | CAP_RS485
    | CAP_RS232
    | CAP_I2C
    | CAP_SPI
)

relay_count = 0
voltage_count = 0
current_count = 0
frequency_count = 0

digital_input_count = 0
digital_output_count = 0
pwm_count = 0
analog_output_count = 0

can_count = 1
lin_count = 1
uart_count = 1
rs485_count = 1
rs232_count = 1
i2c_count = 1
spi_count = 1

reserved = [0xFF] * 9

slot_count = 0
reserved_board = [0xFF] * 3


def crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF


header = struct.pack(
    "<IHBBHBBIHH",
    INFO_MAGIC,
    struct_version,
    hw_major,
    hw_minor,
    year,
    month,
    day,
    serial_number,
    card_type,
    reserved0,
)

assert len(header) == 20

capabilities = struct.pack(
    "<I15B9B",
    cap_flags,
    relay_count,
    voltage_count,
    current_count,
    frequency_count,
    digital_input_count,
    digital_output_count,
    pwm_count,
    analog_output_count,
    can_count,
    lin_count,
    uart_count,
    rs485_count,
    rs232_count,
    i2c_count,
    spi_count,
    *reserved,
)

assert len(capabilities) == 28

board_extension = struct.pack(
    "<B3B",
    slot_count,
    *reserved_board,
)

assert len(board_extension) == 4

crc_data = header + capabilities + board_extension

assert len(crc_data) == 52

crc32_value = crc32(crc_data)


board_info = crc_data + struct.pack("<I", crc32_value) + struct.pack("<I", reserved1)

assert len(board_info) == 60

blob = bytearray([0xFF] * INFO_SIZE)

blob[0 : len(board_info)] = board_info


with open("info_page.bin", "wb") as f:
    f.write(blob)


print(f"Magic:        0x{INFO_MAGIC:08X}")
print(f"Struct ver.:  {struct_version}")
print(f"Serial:       0x{serial_number:08X}")
print(f"Type:         0x{card_type:04X}")
print(f"Capabilities: 0x{cap_flags:08X}")
print(f"Relays:       {relay_count}")
print(f"Slots:        {slot_count}")
print(f"CRC32:        0x{crc32_value:08X}")
print(f"CRC length:   {len(crc_data)} B")
print(f"Data size:    {len(board_info)} B")
print("C struct:     64 B including 4 B alignment padding")
print(f"Wrote info_page.bin ({len(blob)} B)")
