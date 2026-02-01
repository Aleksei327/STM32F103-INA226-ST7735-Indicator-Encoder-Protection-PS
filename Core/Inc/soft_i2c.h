#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#include <stdint.h>

// Инициализация программного I2C
void Soft_I2C_Init(void);

// Проверка готовности устройства (0=OK, 1=FAIL)
uint8_t Soft_I2C_IsDeviceReady(uint8_t addr);

// Запись в регистр
uint8_t Soft_I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

// Чтение из регистра
uint8_t Soft_I2C_ReadReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

// Сканирование шины (опционально)
void Soft_I2C_Scan(void);

#endif
