#include "lsm6d63.h"
#include "i2c.h"
#include <stddef.h>

/* CTRL3_C */
#define LSM6DS3_CTRL3_SW_RESET_Pos   0U
#define LSM6DS3_CTRL3_IF_INC_Pos     2U
#define LSM6DS3_CTRL3_BDU_Pos        6U

#define LSM6DS3_CTRL3_SW_RESET_Msk   (1U << LSM6DS3_CTRL3_SW_RESET_Pos)
#define LSM6DS3_CTRL3_IF_INC_Msk     (1U << LSM6DS3_CTRL3_IF_INC_Pos)
#define LSM6DS3_CTRL3_BDU_Msk        (1U << LSM6DS3_CTRL3_BDU_Pos)

/* CTRL1_XL */
#define LSM6DS3_XL_ODR_104HZ    (4U << 4U)
#define LSM6DS3_XL_FS_2G        (0U << 2U)

/* CTRL2_G */
#define LSM6DS3_G_ODR_104HZ     (4U << 4U)
#define LSM6DS3_G_FS_245DPS    (0U << 2U)

#define LSM6DS3_STATUS_XLDA_Msk  (1U << 0U)
#define LSM6DS3_STATUS_GDA_Msk   (1U << 1U)

#define LSM6DS3_ACCEL_SCALE_G    0.000061f
#define LSM6DS3_GYRO_SCALE_DPS   0.00875f

uint8_t lsm6d63_whoami(void)
{
    uint8_t whoami = 0U;
    if (i2c1_read_reg(LSM6DS3_I2C_ADDR, LSM6DS3_REG_WHO_AM_I, &whoami, 1U) != 1U)
    {
        whoami = 0U;
    }
    return whoami;
}

uint8_t lsm6d63_init(void)
{
    if (lsm6d63_whoami() != LSM6DS3_WHO_AM_I_VALUE)
    {
        return 0U;
    }

    uint8_t ctrl3 = LSM6DS3_CTRL3_BDU_Msk | LSM6DS3_CTRL3_IF_INC_Msk;

    if (i2c1_write_reg(LSM6DS3_I2C_ADDR, LSM6DS3_REG_CTRL3_C, &ctrl3, 1U) != 1U)
    {
        return 0U;
    }

    uint8_t ctrl1 = LSM6DS3_XL_ODR_104HZ | LSM6DS3_XL_FS_2G;

    if (i2c1_write_reg(LSM6DS3_I2C_ADDR, LSM6DS3_REG_CTRL1_XL, &ctrl1, 1U) != 1U)
    {
        return 0U;
    }

    uint8_t ctrl2 = LSM6DS3_G_ODR_104HZ | LSM6DS3_G_FS_245DPS;

    if (i2c1_write_reg(LSM6DS3_I2C_ADDR, LSM6DS3_REG_CTRL2_G, &ctrl2, 1U) != 1U)
    {
        return 0U;
    }

    return 1U;
}

static int16_t lsm6ds3_to_int16(const uint8_t *buf)
{
    uint16_t value = (uint16_t)buf[0] |
                     ((uint16_t)buf[1] << 8U);

    return (int16_t)value;
}

uint8_t lsm6ds3_read_data(lsm6ds3_data_t *data)
{
    if (data == NULL)
    {
        return 0U;
    }

    uint8_t status = 0U;
    if (i2c1_read_reg( LSM6DS3_I2C_ADDR, LSM6DS3_REG_STATUS_REG, &status, 1U) != 1U)
    {
        return 0U;
    }

    if ((status & (LSM6DS3_STATUS_XLDA_Msk | LSM6DS3_STATUS_GDA_Msk)) != (LSM6DS3_STATUS_XLDA_Msk | LSM6DS3_STATUS_GDA_Msk))
    {
        return 2U;
    }

    uint8_t raw[12];

    if (i2c1_read_reg(LSM6DS3_I2C_ADDR, LSM6DS3_REG_OUTX_L_G, raw, 12U) != 1U)
    {
        return 0U;
    }

    int16_t gx = lsm6ds3_to_int16(&raw[0]);
    int16_t gy = lsm6ds3_to_int16(&raw[2]);
    int16_t gz = lsm6ds3_to_int16(&raw[4]);

    int16_t ax = lsm6ds3_to_int16(&raw[6]);
    int16_t ay = lsm6ds3_to_int16(&raw[8]);
    int16_t az = lsm6ds3_to_int16(&raw[10]);

    data->gyro_x_dps = (float)gx * LSM6DS3_GYRO_SCALE_DPS;
    data->gyro_y_dps = (float)gy * LSM6DS3_GYRO_SCALE_DPS;
    data->gyro_z_dps = (float)gz * LSM6DS3_GYRO_SCALE_DPS;

    data->accel_x_g = (float)ax * LSM6DS3_ACCEL_SCALE_G;
    data->accel_y_g = (float)ay * LSM6DS3_ACCEL_SCALE_G;
    data->accel_z_g = (float)az * LSM6DS3_ACCEL_SCALE_G;

    return 1U;
}