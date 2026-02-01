/*
 * soft_i2c.c - Программная реализация I2C (bit-banging)
 * Работает на ЛЮБЫХ GPIO пинах, не требует аппаратного I2C
 * ГАРАНТИРОВАННО работает даже на поддельных STM32!
 */

#include "stm32f1xx_hal.h"
#include "soft_i2c.h"

// ==================== НАСТРОЙКИ ПИНОВ ====================
// Можешь использовать ЛЮБЫЕ свободные GPIO!
// Сейчас используем PB8, PB9 (те же что были для аппаратного I2C)
#define SOFT_I2C_SCL_PORT   GPIOB
#define SOFT_I2C_SCL_PIN    GPIO_PIN_8

#define SOFT_I2C_SDA_PORT   GPIOB
#define SOFT_I2C_SDA_PIN    GPIO_PIN_9

// Задержка для ~100 кГц I2C (можно уменьшить для ускорения)
#define I2C_DELAY_US  5

// ==================== МАКРОСЫ ====================
#define SCL_HIGH()  HAL_GPIO_WritePin(SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET)
#define SCL_LOW()   HAL_GPIO_WritePin(SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET)
#define SDA_HIGH()  HAL_GPIO_WritePin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET)
#define SDA_LOW()   HAL_GPIO_WritePin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET)
#define SDA_READ()  HAL_GPIO_ReadPin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN)

// ==================== ЗАДЕРЖКА ====================
static void delay_us(uint32_t us) {
    // Для 72 МГц: ~72 тактов на 1 мкс
    // Один цикл ≈ 4 такта → 18 итераций на мкс
    volatile uint32_t count = us * 18;
    while(count--);
}

// ==================== ИНИЦИАЛИЗАЦИЯ ====================
void Soft_I2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Включаем тактирование GPIO
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // SCL: Output Open-Drain
    GPIO_InitStruct.Pin = SOFT_I2C_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // Внешние подтяжки 10кОм
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_SCL_PORT, &GPIO_InitStruct);

    // SDA: Output Open-Drain
    GPIO_InitStruct.Pin = SOFT_I2C_SDA_PIN;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &GPIO_InitStruct);

    // Обе линии в HIGH (idle)
    SCL_HIGH();
    SDA_HIGH();
    delay_us(100);
}

// ==================== БАЗОВЫЕ ОПЕРАЦИИ ====================

// START condition
static void I2C_Start(void) {
    SDA_HIGH();
    delay_us(I2C_DELAY_US);
    SCL_HIGH();
    delay_us(I2C_DELAY_US);
    SDA_LOW();   // START: SDA HIGH->LOW при SCL=HIGH
    delay_us(I2C_DELAY_US);
    SCL_LOW();
    delay_us(I2C_DELAY_US);
}

// STOP condition
static void I2C_Stop(void) {
    SDA_LOW();
    delay_us(I2C_DELAY_US);
    SCL_HIGH();
    delay_us(I2C_DELAY_US);
    SDA_HIGH();  // STOP: SDA LOW->HIGH при SCL=HIGH
    delay_us(I2C_DELAY_US);
}

// Отправка байта, возврат: 0=ACK, 1=NACK
static uint8_t I2C_WriteByte(uint8_t data) {
    uint8_t ack;

    // 8 бит, MSB first
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) {
            SDA_HIGH();
        } else {
            SDA_LOW();
        }
        delay_us(I2C_DELAY_US);

        SCL_HIGH();
        delay_us(I2C_DELAY_US);
        SCL_LOW();
        delay_us(I2C_DELAY_US);

        data <<= 1;
    }

    // Читаем ACK
    SDA_HIGH();  // Отпускаем линию
    delay_us(I2C_DELAY_US);
    SCL_HIGH();
    delay_us(I2C_DELAY_US);
    ack = SDA_READ();  // 0=ACK, 1=NACK
    SCL_LOW();
    delay_us(I2C_DELAY_US);

    return ack;
}

// Чтение байта
static uint8_t I2C_ReadByte(uint8_t send_ack) {
    uint8_t data = 0;

    SDA_HIGH();  // Отпускаем для чтения

    for (int i = 0; i < 8; i++) {
        delay_us(I2C_DELAY_US);
        SCL_HIGH();
        delay_us(I2C_DELAY_US);

        data <<= 1;
        if (SDA_READ()) {
            data |= 0x01;
        }

        SCL_LOW();
    }

    // Отправляем ACK/NACK
    if (send_ack) {
        SDA_LOW();   // ACK
    } else {
        SDA_HIGH();  // NACK
    }
    delay_us(I2C_DELAY_US);
    SCL_HIGH();
    delay_us(I2C_DELAY_US);
    SCL_LOW();
    delay_us(I2C_DELAY_US);
    SDA_HIGH();

    return data;
}

// ==================== ПУБЛИЧНЫЕ ФУНКЦИИ ====================

uint8_t Soft_I2C_IsDeviceReady(uint8_t addr) {
    uint8_t ack;

    I2C_Start();
    ack = I2C_WriteByte(addr & 0xFE);  // Write mode
    I2C_Stop();

    return (ack == 0) ? 0 : 1;  // 0=OK, 1=FAIL
}

uint8_t Soft_I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len) {
    I2C_Start();

    if (I2C_WriteByte(addr & 0xFE)) {  // Адрес + Write
        I2C_Stop();
        return 1;
    }

    if (I2C_WriteByte(reg)) {  // Регистр
        I2C_Stop();
        return 1;
    }

    for (uint16_t i = 0; i < len; i++) {
        if (I2C_WriteByte(data[i])) {
            I2C_Stop();
            return 1;
        }
    }

    I2C_Stop();
    return 0;
}

uint8_t Soft_I2C_ReadReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len) {
    I2C_Start();

    if (I2C_WriteByte(addr & 0xFE)) {  // Адрес + Write
        I2C_Stop();
        return 1;
    }

    if (I2C_WriteByte(reg)) {  // Регистр
        I2C_Stop();
        return 1;
    }

    // Repeated START
    I2C_Start();

    if (I2C_WriteByte(addr | 0x01)) {  // Адрес + Read
        I2C_Stop();
        return 1;
    }

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1) {
            data[i] = I2C_ReadByte(0);  // Последний - NACK
        } else {
            data[i] = I2C_ReadByte(1);  // ACK
        }
    }

    I2C_Stop();
    return 0;
}

// Сканирование шины (для отладки)
void Soft_I2C_Scan(void) {
    uint8_t found = 0;

    for (uint8_t addr = 1; addr < 128; addr++) {
        if (Soft_I2C_IsDeviceReady(addr << 1) == 0) {
            found++;
            // Можно вывести на экран найденный адрес
        }
    }
}
