#ifndef GYRO_H
#define GYRO_H

#include <stdint.h>
#include <stdbool.h>

/* Set to 1 to dump raw SPI bytes over UART while debugging */
#define GYRO_DEBUG_BYTES  0

/* SPI address byte flags */
#define GYRO_READ         0x80
#define GYRO_AUTOINC      0x40

/* Register map */
#define GYRO_WHO_AM_I     0x0F
#define GYRO_CTRL_REG1    0x20
#define GYRO_CTRL_REG2    0x21
#define GYRO_CTRL_REG3    0x22
#define GYRO_CTRL_REG4    0x23
#define GYRO_CTRL_REG5    0x24
#define GYRO_STATUS_REG   0x27
#define GYRO_OUT_X_L      0x28

/* Expected WHO_AM_I values */
#define GYRO_ID_L3GD20    0xD4
#define GYRO_ID_I3G4250D  0xD3
#define GYRO_ID_L3GD20H   0xD7

/* CTRL_REG1: normal mode, all three axes enabled */
#define GYRO_CTRL1_ENABLE 0x0F

/* CTRL_REG4: block data update + full scale +/- 245 dps */
#define GYRO_CTRL4_BDU    0x80
#define GYRO_CTRL4_245DPS 0x00
#define GYRO_SENS_245DPS  0.00875f   /* dps per LSB */

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} gyro_raw_t;

uint8_t gyro_read_reg(uint8_t reg);
void    gyro_write_reg(uint8_t reg, uint8_t value);
bool    gyro_init(void);
void    gyro_read_raw(gyro_raw_t *out);
void    gyro_calibrate(uint16_t samples);
void    gyro_read_dps(float *x, float *y, float *z);

#endif /* GYRO_H */
