#include "main.h"
#include "gyro.h"
#include "cmsis_os.h"
#include <stdio.h>

extern SPI_HandleTypeDef  hspi5;
extern UART_HandleTypeDef huart1;

#define CS_LOW()   HAL_GPIO_WritePin(NCS_MEMS_SPI_GPIO_Port, NCS_MEMS_SPI_Pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(NCS_MEMS_SPI_GPIO_Port, NCS_MEMS_SPI_Pin, GPIO_PIN_SET)

/* The LCD shares SPI5. Keep it off the bus. */
#define LCD_DESELECT()  do {                                     \
    HAL_GPIO_WritePin(CSX_GPIO_Port, CSX_Pin, GPIO_PIN_SET);     \
    HAL_GPIO_WritePin(RDX_GPIO_Port, RDX_Pin, GPIO_PIN_SET);     \
  } while (0)

static gyro_raw_t bias = { 0, 0, 0 };

#if GYRO_DEBUG_BYTES
static void gyro_print(const char *s, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)s, len, 100);
}
#endif

uint8_t gyro_read_reg(uint8_t reg)
{
  uint8_t tx[2] = { reg | GYRO_READ, 0xFF };
  uint8_t rx[2] = { 0 };

  CS_LOW();
  HAL_SPI_TransmitReceive(&hspi5, tx, rx, 2, 100);
  CS_HIGH();

  return rx[1];
}

void gyro_write_reg(uint8_t reg, uint8_t value)
{
  uint8_t tx[2] = { reg & ~GYRO_READ, value };

  CS_LOW();
  HAL_SPI_Transmit(&hspi5, tx, 2, 100);
  CS_HIGH();
}

bool gyro_init(void)
{
  LCD_DESELECT();
  CS_HIGH();

  uint8_t id = gyro_read_reg(GYRO_WHO_AM_I);

  if (id != GYRO_ID_L3GD20 &&
      id != GYRO_ID_I3G4250D &&
      id != GYRO_ID_L3GD20H)
  {
    return false;
  }

  gyro_write_reg(GYRO_CTRL_REG4, GYRO_CTRL4_BDU | GYRO_CTRL4_245DPS);
  gyro_write_reg(GYRO_CTRL_REG1, GYRO_CTRL1_ENABLE);

  /* Confirm the configuration actually landed */
  if (gyro_read_reg(GYRO_CTRL_REG4) != (GYRO_CTRL4_BDU | GYRO_CTRL4_245DPS))
  {
    return false;
  }

  return true;
}

void gyro_read_raw(gyro_raw_t *out)
{
  uint8_t tx[7] = { 0 };
  uint8_t rx[7] = { 0 };

  tx[0] = GYRO_OUT_X_L | GYRO_READ | GYRO_AUTOINC;

  CS_LOW();
  HAL_SPI_TransmitReceive(&hspi5, tx, rx, 7, 100);
  CS_HIGH();

#if GYRO_DEBUG_BYTES
  {
    char dbg[64];
    int n = snprintf(dbg, sizeof(dbg),
                     "  raw %02X %02X  %02X %02X  %02X %02X\r\n",
                     rx[1], rx[2], rx[3], rx[4], rx[5], rx[6]);
    gyro_print(dbg, n);
  }
#endif

  out->x = (int16_t)((rx[2] << 8) | rx[1]);
  out->y = (int16_t)((rx[4] << 8) | rx[3]);
  out->z = (int16_t)((rx[6] << 8) | rx[5]);
}

void gyro_calibrate(uint16_t samples)
{
  int32_t sx = 0, sy = 0, sz = 0;
  gyro_raw_t g;

  if (samples == 0) return;

  bias.x = bias.y = bias.z = 0;

  for (uint16_t i = 0; i < samples; i++)
  {
    gyro_read_raw(&g);
    sx += g.x;
    sy += g.y;
    sz += g.z;
    osDelay(11);
  }

  bias.x = (int16_t)(sx / samples);
  bias.y = (int16_t)(sy / samples);
  bias.z = (int16_t)(sz / samples);
}

void gyro_read_dps(float *x, float *y, float *z)
{
  gyro_raw_t g;
  gyro_read_raw(&g);

  *x = (g.x - bias.x) * GYRO_SENS_245DPS;
  *y = (g.y - bias.y) * GYRO_SENS_245DPS;
  *z = (g.z - bias.z) * GYRO_SENS_245DPS;
}
