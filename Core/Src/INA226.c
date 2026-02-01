#include "INA226.h"
#include "soft_i2c.h"

#define CURRENT_LSB 0.001f

// Внутренняя функция записи (теперь публичная для Alert)
void INA226_WriteReg(uint8_t reg, uint16_t value) {
    uint8_t buf[2];
    buf[0] = (value >> 8) & 0xFF;
    buf[1] = value & 0xFF;
    Soft_I2C_WriteReg(INA226_ADDR, reg, buf, 2);
}

uint16_t INA226_ReadReg(uint8_t reg) {
    uint8_t buf[2] = {0, 0};

    if (Soft_I2C_ReadReg(INA226_ADDR, reg, buf, 2) != 0) {
        return 0xDEAD;
    }

    return ((uint16_t)buf[0] << 8) | buf[1];
}

void INA226_Init(I2C_HandleTypeDef *hi2c) {
    (void)hi2c;

    if (Soft_I2C_IsDeviceReady(INA226_ADDR) != 0) {
        return;
    }

    HAL_Delay(10);

    // Сброс
    INA226_WriteReg(INA226_REG_CONFIG, 0x8000);
    HAL_Delay(10);

    // Калибровка
    uint16_t cal = (uint16_t)(0.00512f / (CURRENT_LSB * INA226_SHUNT_OHMS));
    INA226_WriteReg(INA226_REG_CALIBRATION, cal);
    HAL_Delay(5);

    // Конфигурация
    INA226_WriteReg(INA226_REG_CONFIG, 0x4127);
    //INA226_WriteReg(INA226_REG_CONFIG, 0x4527);
    HAL_Delay(10);
}

float INA226_ReadBusVoltage(void) {
    uint16_t raw = INA226_ReadReg(INA226_REG_BUSVOLT);
    if (raw == 0xDEAD || raw == 0xFFFF) return 0.0f;
    return raw * 0.00125f;
}

float INA226_ReadCurrent(void) {
    int16_t raw = (int16_t)INA226_ReadReg(INA226_REG_CURRENT);
    if (raw == (int16_t)0xDEAD || raw == (int16_t)0xFFFF) return 0.0f;
    return (float)raw * CURRENT_LSB;
}

float INA226_ReadPower(void) {
    uint16_t raw = INA226_ReadReg(INA226_REG_POWER);
    if (raw == 0xDEAD || raw == 0xFFFF) return 0.0f;
    return (float)raw * CURRENT_LSB * 25.0f;
}

float INA226_ReadShuntVoltage(void) {
    int16_t raw = (int16_t)INA226_ReadReg(INA226_REG_SHUNTVOLT);
    if (raw == (int16_t)0xDEAD) return 0.0f;
    return (float)raw * 0.0000025f;
}

// ==================== ФУНКЦИИ ALERT ====================

// Установить лимит по току (в Амперах)
/*void INA226_SetCurrentLimit(float current_amps) {
    // Переводим ток в значение регистра
    // Current Register = Current / Current_LSB
    int16_t limit_raw = (int16_t)(current_amps / CURRENT_LSB/1.7f);

    // Записываем в Alert Limit регистр
    INA226_WriteReg(INA226_REG_ALERT_LIMIT, (uint16_t)limit_raw);
}
*/

void INA226_SetCurrentLimit(float current_amps) {
    // 1. Считаем, какое напряжение упадет на шунте при таком токе.
    // U = I * R
    float limit_voltage = current_amps * INA226_SHUNT_OHMS;

    // 2. Переводим напряжение в формат регистра INA226.
    // Регистр Alert Limit хранит напряжение с шагом 2.5 мкВ.
    // Значение = Напряжение / 0.0000025

    int16_t limit_raw = (int16_t)(limit_voltage / 0.0000025f);

    // 3. Пишем в регистр
    INA226_WriteReg(INA226_REG_ALERT_LIMIT, (uint16_t)limit_raw);
}

// Получить текущий лимит
float INA226_GetCurrentLimit(void) {
    int16_t limit_raw = (int16_t)INA226_ReadReg(INA226_REG_ALERT_LIMIT);
    return (float)limit_raw * CURRENT_LSB;
}

// Включить Alert по превышению тока
void INA226_EnableCurrentAlert(void) {
    // Конфигурация Mask/Enable:
    // - POL (bit 11) = Power Over-Limit
    // - AFF (bit 4) = Alert Function Flag
    // - APOL (bit 1) = 0 (Active LOW) или 1 (Active HIGH)
    // - LEN (bit 0) = 1 (Latch enable - требует сброса)

    uint16_t mask = INA226_MASK_SOL |  // Power Over-Limit alert
                    INA226_MASK_LEN;    // Latch enable

    INA226_WriteReg(INA226_REG_MASK_ENABLE, mask);
}

// Отключить Alert
void INA226_DisableAlert(void) {
    INA226_WriteReg(INA226_REG_MASK_ENABLE, 0x0000);
}

// Проверить сработал ли Alert
uint8_t INA226_IsAlertTriggered(void) {
    uint16_t mask = INA226_ReadReg(INA226_REG_MASK_ENABLE);

    // Проверяем Alert Function Flag (bit 4)
    return (mask & INA226_MASK_AFF) ? 1 : 0;
}

// Сбросить Alert (чтение Mask/Enable сбрасывает флаг при LEN=1)
void INA226_ClearAlert(void) {
    // Просто читаем регистр - это сбрасывает latched alert
    volatile uint16_t temp = INA226_ReadReg(INA226_REG_MASK_ENABLE);
    (void)temp;
}
