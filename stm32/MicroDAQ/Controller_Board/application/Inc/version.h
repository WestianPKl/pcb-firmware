#ifndef VERSION_H
#define VERSION_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint16_t year;
    uint8_t month;
    uint8_t day;
} fw_version_t;

static const fw_version_t fw_version =
{
    .major = 1U,
    .minor = 0U,
    .patch = 0U,
    .year = 2026U,
    .month = 10U,
    .day = 3U
};

#define BOARD_INFO_STRUCT_VERSION 2U

#define INFO_ADDR       0x081FE000U
#define INFO_END        0x08200000U
#define INFO_SIZE       0x2000U
#define INFO_MAGIC      0x43415244U

#define CARD_TYPE_POWER          0x0001U
#define CARD_TYPE_SWITCH         0x0002U
#define CARD_TYPE_MEASUREMENT    0x0003U
#define CARD_TYPE_COMMUNICATION  0x0004U
#define CARD_TYPE_CONTROLLER     0x0005U
#define CARD_TYPE_OUTPUT         0x0006U

#define CAP_RELAY           (1UL << 0)
#define CAP_VOLTAGE         (1UL << 1)
#define CAP_CURRENT         (1UL << 2)
#define CAP_FREQUENCY       (1UL << 3)
#define CAP_DIGITAL_INPUT   (1UL << 4)
#define CAP_DIGITAL_OUTPUT  (1UL << 5)
#define CAP_PWM             (1UL << 6)
#define CAP_ANALOG_OUTPUT   (1UL << 7)
#define CAP_CAN             (1UL << 8)
#define CAP_CAN_FD          (1UL << 9)
#define CAP_LIN             (1UL << 10)
#define CAP_UART            (1UL << 11)
#define CAP_RS485           (1UL << 12)
#define CAP_RS232           (1UL << 13)
#define CAP_I2C             (1UL << 14)
#define CAP_SPI             (1UL << 15)

typedef struct
{
    uint32_t flags;
    uint8_t relay_count;
    uint8_t voltage_count;
    uint8_t current_count;
    uint8_t frequency_count;
    uint8_t digital_input_count;
    uint8_t digital_output_count;
    uint8_t pwm_count;
    uint8_t analog_output_count;
    uint8_t can_count;
    uint8_t lin_count;
    uint8_t uart_count;
    uint8_t rs485_count;
    uint8_t rs232_count;
    uint8_t i2c_count;
    uint8_t spi_count;
    uint8_t reserved[9];
} card_capabilities_t;

_Static_assert(sizeof(card_capabilities_t) == 28U, "card_capabilities_t size mismatch");

typedef struct __attribute__((packed, aligned(16)))
{
    uint32_t magic;
    uint16_t struct_version;
    uint8_t  hw_major;
    uint8_t  hw_minor;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint32_t serial_number;
    uint16_t card_type;
    uint16_t reserved0;
    card_capabilities_t capabilities;
    uint8_t slot_count;
    uint8_t reserved_board[3];
    uint32_t crc32;
    uint32_t reserved1;
} board_info_t;

_Static_assert(offsetof(board_info_t, capabilities) == 20U, "Invalid capabilities offset");
_Static_assert(sizeof(board_info_t) == 64U, "board_info_t size mismatch");
_Static_assert(offsetof(board_info_t, slot_count) == 48U, "board_info_t slot_count offset mismatch");
_Static_assert(offsetof(board_info_t, crc32) == 52U, "board_info_t crc32 offset mismatch");

#define BOARD_INFO_CRC_LENGTH ((uint32_t)offsetof(board_info_t, crc32))

static inline const board_info_t *board_info_get(void)
{
    return (const board_info_t *)INFO_ADDR;
}

uint8_t board_info_valid(void);

#endif // VERSION_H