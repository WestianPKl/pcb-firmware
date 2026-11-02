#ifndef LSM6DS3_H
#define LSM6DS3_H

#include <stdint.h>

/* ============================================================
 * LSM6DS3TR-C
 * I2C ADDRESS / DEVICE ID
 * ============================================================ */

#define LSM6DS3_I2C_ADDR                   0x6AU
#define LSM6DS3_WHO_AM_I_VALUE             0x6AU


/* ============================================================
 * FUNCTION CONFIGURATION / SYNCHRONIZATION
 * ============================================================ */

#define LSM6DS3_REG_FUNC_CFG_ACCESS        0x01U
#define LSM6DS3_REG_SENSOR_SYNC_TIME_FRAME 0x04U
#define LSM6DS3_REG_SENSOR_SYNC_RES_RATIO  0x05U


/* ============================================================
 * FIFO CONFIGURATION
 * ============================================================ */

#define LSM6DS3_REG_FIFO_CTRL1             0x06U
#define LSM6DS3_REG_FIFO_CTRL2             0x07U
#define LSM6DS3_REG_FIFO_CTRL3             0x08U
#define LSM6DS3_REG_FIFO_CTRL4             0x09U
#define LSM6DS3_REG_FIFO_CTRL5             0x0AU


/* ============================================================
 * INTERRUPTS / DEVICE IDENTIFICATION
 * ============================================================ */

#define LSM6DS3_REG_DRDY_PULSE_CFG_G       0x0BU
#define LSM6DS3_REG_INT1_CTRL              0x0DU
#define LSM6DS3_REG_INT2_CTRL              0x0EU
#define LSM6DS3_REG_WHO_AM_I               0x0FU


/* ============================================================
 * ACCELEROMETER / GYROSCOPE CONFIGURATION
 * ============================================================ */

#define LSM6DS3_REG_CTRL1_XL               0x10U
#define LSM6DS3_REG_CTRL2_G                0x11U
#define LSM6DS3_REG_CTRL3_C                0x12U
#define LSM6DS3_REG_CTRL4_C                0x13U
#define LSM6DS3_REG_CTRL5_C                0x14U
#define LSM6DS3_REG_CTRL6_C                0x15U
#define LSM6DS3_REG_CTRL7_G                0x16U
#define LSM6DS3_REG_CTRL8_XL               0x17U
#define LSM6DS3_REG_CTRL9_XL               0x18U
#define LSM6DS3_REG_CTRL10_C               0x19U


/* ============================================================
 * SENSOR HUB / EVENT STATUS
 * ============================================================ */

#define LSM6DS3_REG_MASTER_CONFIG          0x1AU
#define LSM6DS3_REG_WAKE_UP_SRC            0x1BU
#define LSM6DS3_REG_TAP_SRC                0x1CU
#define LSM6DS3_REG_D6D_SRC                0x1DU
#define LSM6DS3_REG_STATUS_REG             0x1EU


/* ============================================================
 * TEMPERATURE OUTPUT
 * ============================================================ */

#define LSM6DS3_REG_OUT_TEMP_L             0x20U
#define LSM6DS3_REG_OUT_TEMP_H             0x21U


/* ============================================================
 * GYROSCOPE OUTPUT
 * ============================================================ */

#define LSM6DS3_REG_OUTX_L_G               0x22U
#define LSM6DS3_REG_OUTX_H_G               0x23U
#define LSM6DS3_REG_OUTY_L_G               0x24U
#define LSM6DS3_REG_OUTY_H_G               0x25U
#define LSM6DS3_REG_OUTZ_L_G               0x26U
#define LSM6DS3_REG_OUTZ_H_G               0x27U


/* ============================================================
 * ACCELEROMETER OUTPUT
 * ============================================================ */

#define LSM6DS3_REG_OUTX_L_XL              0x28U
#define LSM6DS3_REG_OUTX_H_XL              0x29U
#define LSM6DS3_REG_OUTY_L_XL              0x2AU
#define LSM6DS3_REG_OUTY_H_XL              0x2BU
#define LSM6DS3_REG_OUTZ_L_XL              0x2CU
#define LSM6DS3_REG_OUTZ_H_XL              0x2DU


/* ============================================================
 * SENSOR HUB OUTPUT 1-12
 * ============================================================ */

#define LSM6DS3_REG_SENSORHUB1_REG         0x2EU
#define LSM6DS3_REG_SENSORHUB2_REG         0x2FU
#define LSM6DS3_REG_SENSORHUB3_REG         0x30U
#define LSM6DS3_REG_SENSORHUB4_REG         0x31U
#define LSM6DS3_REG_SENSORHUB5_REG         0x32U
#define LSM6DS3_REG_SENSORHUB6_REG         0x33U
#define LSM6DS3_REG_SENSORHUB7_REG         0x34U
#define LSM6DS3_REG_SENSORHUB8_REG         0x35U
#define LSM6DS3_REG_SENSORHUB9_REG         0x36U
#define LSM6DS3_REG_SENSORHUB10_REG        0x37U
#define LSM6DS3_REG_SENSORHUB11_REG        0x38U
#define LSM6DS3_REG_SENSORHUB12_REG        0x39U


/* ============================================================
 * FIFO STATUS / FIFO DATA
 * ============================================================ */

#define LSM6DS3_REG_FIFO_STATUS1           0x3AU
#define LSM6DS3_REG_FIFO_STATUS2           0x3BU
#define LSM6DS3_REG_FIFO_STATUS3           0x3CU
#define LSM6DS3_REG_FIFO_STATUS4           0x3DU

#define LSM6DS3_REG_FIFO_DATA_OUT_L        0x3EU
#define LSM6DS3_REG_FIFO_DATA_OUT_H        0x3FU


/* ============================================================
 * TIMESTAMP
 * ============================================================ */

#define LSM6DS3_REG_TIMESTAMP0_REG         0x40U
#define LSM6DS3_REG_TIMESTAMP1_REG         0x41U
#define LSM6DS3_REG_TIMESTAMP2_REG         0x42U


/* ============================================================
 * PEDOMETER / STEP COUNTER
 * ============================================================ */

#define LSM6DS3_REG_STEP_TIMESTAMP_L       0x49U
#define LSM6DS3_REG_STEP_TIMESTAMP_H       0x4AU
#define LSM6DS3_REG_STEP_COUNTER_L         0x4BU
#define LSM6DS3_REG_STEP_COUNTER_H         0x4CU


/* ============================================================
 * SENSOR HUB OUTPUT 13-18
 * ============================================================ */

#define LSM6DS3_REG_SENSORHUB13_REG        0x4DU
#define LSM6DS3_REG_SENSORHUB14_REG        0x4EU
#define LSM6DS3_REG_SENSORHUB15_REG        0x4FU
#define LSM6DS3_REG_SENSORHUB16_REG        0x50U
#define LSM6DS3_REG_SENSORHUB17_REG        0x51U
#define LSM6DS3_REG_SENSORHUB18_REG        0x52U


/* ============================================================
 * EMBEDDED FUNCTIONS / EVENT DETECTION
 * ============================================================ */

#define LSM6DS3_REG_FUNC_SRC1              0x53U
#define LSM6DS3_REG_FUNC_SRC2              0x54U
#define LSM6DS3_REG_WRIST_TILT_IA          0x55U

#define LSM6DS3_REG_TAP_CFG                0x58U
#define LSM6DS3_REG_TAP_THS_6D             0x59U
#define LSM6DS3_REG_INT_DUR2               0x5AU

#define LSM6DS3_REG_WAKE_UP_THS            0x5BU
#define LSM6DS3_REG_WAKE_UP_DUR            0x5CU
#define LSM6DS3_REG_FREE_FALL              0x5DU

#define LSM6DS3_REG_MD1_CFG                0x5EU
#define LSM6DS3_REG_MD2_CFG                0x5FU


/* ============================================================
 * SENSOR HUB COMMAND / SYNC STATUS
 * ============================================================ */

#define LSM6DS3_REG_MASTER_CMD_CODE        0x60U
#define LSM6DS3_REG_SENS_SYNC_SPI_ERROR_CODE 0x61U


/* ============================================================
 * EXTERNAL MAGNETOMETER RAW DATA
 * ============================================================ */

#define LSM6DS3_REG_OUT_MAG_RAW_X_L        0x66U
#define LSM6DS3_REG_OUT_MAG_RAW_X_H        0x67U
#define LSM6DS3_REG_OUT_MAG_RAW_Y_L        0x68U
#define LSM6DS3_REG_OUT_MAG_RAW_Y_H        0x69U
#define LSM6DS3_REG_OUT_MAG_RAW_Z_L        0x6AU
#define LSM6DS3_REG_OUT_MAG_RAW_Z_H        0x6BU


/* ============================================================
 * USER ACCELEROMETER OFFSET
 * ============================================================ */

#define LSM6DS3_REG_X_OFS_USR              0x73U
#define LSM6DS3_REG_Y_OFS_USR              0x74U
#define LSM6DS3_REG_Z_OFS_USR              0x75U

typedef struct
{
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;

    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

} lsm6ds3_data_t;

uint8_t lsm6d63_whoami(void);
uint8_t lsm6d63_init(void);
uint8_t lsm6ds3_read_data(lsm6ds3_data_t *data);

#endif // LSM6DS3_H