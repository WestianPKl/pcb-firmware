from controller_client import ControllerClient
import time

RELAY_SLOT = 3
COMM_SLOT = 2
OUTPUT_SLOT = 1

HOST = "192.168.18.200"
PORT = 5001

client = ControllerClient(HOST, PORT)
client.connect()

# response = client.ping()
# print("Ping:", response)

# response = client.get_controller_info()
# print("Controller:", response)

# response = client.get_cards_info()
# print("Cards:", response)

# response = client.identify_card(3)
# print("Identify:", response)

# response = client.get_power_card_info()
# print("Power Card Info:", response)

# response = client.get_adc_data()
# print("ADC Data:", response)

# response = client.get_ina_data()
# print("INA Data:", response)

# response = client.get_relay(RELAY_SLOT, 0)
# print("Relay:", response)
# time.sleep(0.5)

# response = client.set_relay(RELAY_SLOT, 0, 1)
# print("Set relay:", response)
# time.sleep(0.5)

# response = client.get_relay(RELAY_SLOT, 0)
# print("Relay:", response)
# time.sleep(0.5)

# response = client.set_relay(RELAY_SLOT, 0, 0)
# print("Set relay:", response)
# time.sleep(0.5)

# response = client.get_relay(RELAY_SLOT, 0)
# print("Relay:", response)
# time.sleep(0.5)


# response = client.get_digital_output(OUTPUT_SLOT, 0)
# print("Digital Output:", response)
# time.sleep(0.5)

# response = client.set_digital_output(OUTPUT_SLOT, 0, 1)
# print("Set Digital Output:", response)
# time.sleep(0.5)

# response = client.get_digital_output(OUTPUT_SLOT, 0)
# print("Digital Output:", response)
# time.sleep(0.5)

# response = client.set_digital_output(OUTPUT_SLOT, 0, 0)
# print("Set Digital Output:", response)
# time.sleep(0.5)

# response = client.get_digital_output(OUTPUT_SLOT, 0)
# print("Digital Output:", response)
# time.sleep(0.5)

# response = client.get_pwm(OUTPUT_SLOT, 0)
# print("PWM:", response)
# time.sleep(0.5)

# response = client.set_pwm(OUTPUT_SLOT, 0, 100)
# print("Set PWM:", response)
# time.sleep(0.5)

# response = client.get_pwm(OUTPUT_SLOT, 0)
# print("PWM:", response)
# time.sleep(0.5)

# response = client.set_pwm(OUTPUT_SLOT, 0, 50)
# print("Set PWM:", response)
# time.sleep(0.5)

# response = client.get_pwm(OUTPUT_SLOT, 0)
# print("PWM:", response)
# time.sleep(0.5)

# response = client.set_pwm(OUTPUT_SLOT, 0, 25)
# print("Set PWM:", response)
# time.sleep(0.5)

# response = client.get_pwm(OUTPUT_SLOT, 0)
# print("PWM:", response)
# time.sleep(0.5)

# response = client.set_pwm(OUTPUT_SLOT, 0, 0)
# print("Set PWM:", response)
# time.sleep(0.5)

# response = client.get_pwm(OUTPUT_SLOT, 0)
# print("PWM:", response)
# time.sleep(0.5)

# relays = range(0, 12)

# for relay in relays:
#     response = client.get_relay(RELAY_SLOT, relay)
#     print("Relay:", response)
#     time.sleep(0.5)

#     response = client.set_relay(RELAY_SLOT, relay, 1)
#     print("Set relay:", response)
#     time.sleep(0.5)

#     response = client.get_relay(RELAY_SLOT, relay)
#     print("Relay:", response)
#     time.sleep(0.5)

#     response = client.set_relay(RELAY_SLOT, relay, 0)
#     print("Set relay:", response)
#     time.sleep(0.5)

#     response = client.get_relay(RELAY_SLOT, relay)
#     print("Relay:", response)
#     time.sleep(0.5)

response = client.i2c_config(COMM_SLOT, 100000, 0)
print("I2C Config:", response)

for addr in (0x76, 0x77):
    response = client.i2c_reg_read(COMM_SLOT, addr, 0xD0, 1)

    print(f"BME280 0x{addr:02X} CHIP_ID:", response)

    if response["status"] == 0x40 and response["length"] == 1:
        chip_id = response["data"][0]

        print(f"CHIP ID: 0x{chip_id:02X}")

        if chip_id == 0x60:
            print(f"BME280 FOUND at 0x{addr:02X}")

response = client.spi_config(COMM_SLOT, 1500000, 0, 0, 0, 8)

response = client.spi_transfer_byte(COMM_SLOT, 0xA5)
print("SPI Byte:", response)

response = client.spi_transfer_word(
    COMM_SLOT,
    bytes(
        [
            0x11,
            0x22,
            0x33,
            0x44,
            0xAA,
            0xBB,
            0xCC,
        ]
    ),
)
print("SPI Word:", response)

if response["status"] == 0x40:
    print("SPI Data:", response["data"].hex(" "))

response = client.uart_config(COMM_SLOT, 115200, 8, 0, 1)
print("UART Config:", response)

response = client.uart_write(COMM_SLOT, b"Hello MicroDAQ\r\n")
print("UART Write:", response)

time.sleep(0.1)

response = client.uart_read(COMM_SLOT)
print("UART Read:", response)

if response["status"] == 0x40:
    print("UART Data:", response["data"])

response = client.rs232_config(COMM_SLOT, 115200, 8, 0, 1)
print("RS232 Config:", response)

response = client.rs232_write(COMM_SLOT, b"Hello MicroDAQ\r\n")
print("RS232 Write:", response)

time.sleep(0.1)

response = client.rs232_read(COMM_SLOT)
print("RS232 Read:", response)

if response["status"] == 0x40:
    print("RS232 Data:", response["data"])

response = client.rs485_config(COMM_SLOT, 115200, 8, 0, 1)
print("RS485 Config:", response)

# response = client.rs485_write(COMM_SLOT, b"Hello MicroDAQ\r\n")
# print("RS485 Write:", response)

# time.sleep(0.1)

# response = client.rs485_read(COMM_SLOT)
# print("RS485 Read:", response)

# if response["status"] == 0x40:
#     print("RS485 Data:", response["data"])

response = client.can_config(
    COMM_SLOT,
    nominal_bitrate=1,
    data_bitrate=1,
    fd_enabled=1,
    brs_enabled=0,
    mode=2,
    can_termination=0,
)
print("Config:", response)

response = client.can_std_send(COMM_SLOT, 0x123, b"\x11\x22\x33\x44")
print("STD TX:", response)

response = client.can_receive(COMM_SLOT)
print("STD RX:", response)

response = client.can_fd_send(COMM_SLOT, 0x124, b"\x55\x66\x77\x88")
print("FD TX:", response)

response = client.can_receive(COMM_SLOT)
print("FD RX:", response)

response = client.multipart_test(2)
print(response)
data = response["payload"]
expected = bytes(range(0x11, 0x81))
assert len(data) == 113
assert data == expected
print("MULTIPART PASS:", len(data), "bytes")

client.close()
