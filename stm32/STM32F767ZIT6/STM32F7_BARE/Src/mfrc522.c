#include "mfrc522.h"
#include "spi.h"
#include "dma.h"
#include <stdio.h>

#define MFRC522_XFER_MAX_DATA 32U

static void mfrc522_read_registers(uint8_t reg, uint8_t *data, uint32_t size)
{
    if (!data || size == 0U) return;
    if (size > MFRC522_XFER_MAX_DATA) return;

    uint32_t len = size + 1U;
    uint8_t txbuf[1U + MFRC522_XFER_MAX_DATA];
    uint8_t rxbuf[1U + MFRC522_XFER_MAX_DATA];

    txbuf[0] = ((reg << 1) & 0x7E) | 0x80;
    for (uint32_t i = 1; i < len; i++) txbuf[i] = 0x00U;

    spi1_cs_low();
    spi1_dma_xfer(txbuf, rxbuf, len);
    spi1_cs_high();

    for (uint32_t i = 0; i < size; i++) data[i] = rxbuf[i + 1U];
}

static void mfrc522_write_register(uint8_t reg, uint8_t value)
{
    uint8_t txbuf[2];
    uint8_t rxbuf[2];

    txbuf[0] = (reg << 1) & 0x7E;
    txbuf[1] = value;

    spi1_cs_low();
    spi1_dma_xfer(txbuf, rxbuf, 2U);
    spi1_cs_high();

    (void)rxbuf;
}

static int mfrc522_transceive(uint8_t *send, uint8_t send_len,
                              uint8_t *recv, uint8_t *recv_len,
                              uint8_t bitframing)
{
    mfrc522_write_register(0x02, 0x00);
    mfrc522_write_register(0x0A, 0x80);
    mfrc522_write_register(0x01, 0x00);
    mfrc522_write_register(0x04, 0x7F);

    for (uint8_t i = 0; i < send_len; i++)
        mfrc522_write_register(0x09, send[i]);

    mfrc522_write_register(0x01, 0x0C);
    mfrc522_write_register(0x0D, bitframing | 0x80);

    uint8_t irq;
    uint32_t timeout = 100000;
    do {
        mfrc522_read_registers(0x04, &irq, 1);
        if (irq & 0x01) { mfrc522_write_register(0x0D, 0x00); return -3; }
        if (--timeout == 0) { mfrc522_write_register(0x0D, 0x00); return -1; }
    } while (!(irq & 0x30));

    mfrc522_write_register(0x0D, 0x00);

    uint8_t err;
    mfrc522_read_registers(0x06, &err, 1);
    if (err & 0x13) return -2;

    uint8_t n;
    mfrc522_read_registers(0x0A, &n, 1);
    if (n == 0) return -4;

    if (n > *recv_len) n = *recv_len;
    for (uint8_t i = 0; i < n; i++)
        mfrc522_read_registers(0x09, &recv[i], 1);
    *recv_len = n;

    return 0;
}

void mfrc522_version(uint8_t *version)
{
    if (!version) return;
    mfrc522_read_registers(0x37, version, 1);
}

void mfrc522_check(uint8_t *status)
{
    mfrc522_write_register(0x24, 0x26);
    mfrc522_read_registers(0x24, status, 1);
}

void mfrc522_init(void)
{
    mfrc522_write_register(0x01, 0x0F);
    uint8_t cmd;
    uint32_t timeout = 100000;
    do {
        mfrc522_read_registers(0x01, &cmd, 1);
    } while ((cmd & 0x10) && (--timeout != 0));

    mfrc522_write_register(0x2A, 0x8D);
    mfrc522_write_register(0x2B, 0x3E);
    mfrc522_write_register(0x2D, 0x1E);
    mfrc522_write_register(0x2C, 0x00);
    mfrc522_write_register(0x15, 0x40);
    mfrc522_write_register(0x11, 0x3D);
    mfrc522_write_register(0x26, 0x70);

    uint8_t tx;
    mfrc522_read_registers(0x14, &tx, 1);
    if (!(tx & 0x03)) {
        mfrc522_write_register(0x14, tx | 0x03);
    }
}

void mfrc522_antenna_check(uint8_t *tx)
{
    mfrc522_read_registers(0x14, tx, 1);
}

int mfrc522_check_atqa(uint8_t *atqa, uint8_t *atqa_len)
{
    uint8_t reqa = 0x26;
    mfrc522_write_register(0x0D, 0x07);
    int r = mfrc522_transceive(&reqa, 1, atqa, atqa_len, 0x07);
    return r;
}

static int mfrc522_anticoll(uint8_t *uid, uint8_t *uid_len)
{
    uint8_t anticoll[2] = {0x93, 0x20};
    int r = mfrc522_transceive(anticoll, 2, uid, uid_len, 0x00);
    printf("anticoll: r=%d, len=%d\n", r, *uid_len);
    return r;
}

int mfrc522_read_id(uint8_t *uid, uint8_t *uid_len)
{
    uint8_t atqa[2];
    uint8_t atqa_len = sizeof(atqa);
    int ra = mfrc522_check_atqa(atqa, &atqa_len);
    printf("ATQA: r=%d, len=%d, [%02X %02X]\n", ra, atqa_len, atqa[0], atqa[1]);
    if (ra != 0) return -1;

    uint8_t buf[5];
    uint8_t len = sizeof(buf);
    if (mfrc522_anticoll(buf, &len) != 0) return -2;
    if (len != 5) return -3;

    uint8_t bcc = buf[0]^buf[1]^buf[2]^buf[3];
    if (bcc != buf[4]) return -4;

    for (int i=0;i<4;i++) uid[i]=buf[i];
    *uid_len=4;
    printf("UID: %02X %02X %02X %02X (BCC OK)\n", uid[0],uid[1],uid[2],uid[3]);
    return 0;
}