/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  * of the I2C instances and Bus Recovery procedure.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN 1 */
/**
 * @brief ПОЛНАЯ очистка и восстановление шины I2C
 */
void I2C1_FullReset(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. ПОЛНОЕ отключение всего что связано с I2C
    __HAL_RCC_I2C1_CLK_DISABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    HAL_Delay(10);
    __HAL_RCC_I2C1_RELEASE_RESET();
    HAL_Delay(10);

    // 2. Включаем GPIO
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    // 3. Сбрасываем пины в обычный GPIO
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

    // 4. SCL как Output Push-Pull
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 5. SDA как Input
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 6. Устанавливаем SCL в HIGH
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(5);

    // 7. Генерируем тактовые импульсы для освобождения шины
    for (int i = 0; i < 20; i++) {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_SET) {
            break; // SDA освободилась
        }
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(5);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(5);
    }

    // 8. Генерируем STOP condition
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_Delay(5);

    // 9. Сбрасываем пины обратно
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);
}

/**
 * @brief Ручная инициализация I2C - УЛУЧШЕННАЯ версия
 */
void I2C1_ManualInit(void)
{
    // 1. Включаем тактирование
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // 2. Remap I2C1 на PB8/PB9
    AFIO->MAPR |= AFIO_MAPR_I2C1_REMAP;

    // 3. Настройка GPIO PB8, PB9 как Alternate Function Open-Drain
    // PB8 (биты 0-3 в CRH): MODE=11 (50MHz), CNF=11 (AF OD)
    GPIOB->CRH &= ~(0xF << 0);
    GPIOB->CRH |= (0xF << 0);

    // PB9 (биты 4-7 в CRH): MODE=11 (50MHz), CNF=11 (AF OD)
    GPIOB->CRH &= ~(0xF << 4);
    GPIOB->CRH |= (0xF << 4);

    // Включаем внутренние подтяжки (помогает с 10кОм внешними)
    GPIOB->ODR |= (GPIO_PIN_8 | GPIO_PIN_9);

    // 4. Полная очистка регистра CR1
    I2C1->CR1 = 0x0000;
    HAL_Delay(5);

    // 5. Сброс I2C периферии
    I2C1->CR1 = I2C_CR1_SWRST;
    HAL_Delay(5);

    // 6. КРИТИЧНО: Полная очистка CR1, убираем SWRST!
    I2C1->CR1 = 0x0000;
    HAL_Delay(10);

    // 7. Настройка I2C для 100 кГц
    // Предполагаем PCLK1 = 36 МГц (SYSCLK=72МГц, APB1 делитель /2)
    I2C1->CR2 = 36;  // FREQ = 36 МГц
    I2C1->CCR = 180; // CCR = 36000000 / (2 * 100000) = 180
    I2C1->TRISE = 37; // TRISE = 36 + 1

    // 8. Включение I2C БЕЗ ACK (ACK включим позже при необходимости)
    I2C1->CR1 = I2C_CR1_PE;

    // 9. Задержка для стабилизации
    HAL_Delay(50);

    // 10. Проверяем что CR1 содержит ТОЛЬКО PE (0x0001)
    if ((I2C1->CR1 & 0xFFFE) != 0) {
        // Есть лишние биты - очищаем
        I2C1->CR1 = I2C_CR1_PE;
        HAL_Delay(10);
    }
}

/**
 * @brief Проверка и принудительное включение I2C если выключился
 */
void I2C1_EnsureEnabled(void)
{
    uint16_t cr1_val = I2C1->CR1;

    // Проверяем флаг SWRST - если установлен, это КРИТИЧЕСКАЯ ошибка
    if (cr1_val & I2C_CR1_SWRST) {
        // SWRST активен - полностью сбрасываем
        I2C1->CR1 = 0x0000;
        HAL_Delay(10);
        I2C1->CR1 = I2C_CR1_PE;
        HAL_Delay(10);
        return;
    }

    // Если периферия выключена - включаем
    if ((cr1_val & I2C_CR1_PE) == 0) {
        I2C1->CR1 = I2C_CR1_PE;
        HAL_Delay(1);
    }

    // Сбрасываем флаги ошибок в SR1
    volatile uint16_t temp = I2C1->SR1;
    temp = I2C1->SR2;
    (void)temp;
}
/* USER CODE END 1 */

/* I2C1 init function */
void MX_I2C1_Init(void)
{
  /* USER CODE BEGIN I2C1_Init 0 */
  // Полный сброс шины
  I2C1_FullReset();
  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */
  // Ручная инициализация
  I2C1_ManualInit();

  // Заполняем структуру HAL для совместимости
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 50000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  hi2c1.State = HAL_I2C_STATE_READY;

  // Проверяем что периферия включена
  I2C1_EnsureEnabled();
  /* USER CODE END I2C1_Init 1 */

  /* USER CODE BEGIN I2C1_Init 2 */
  /* USER CODE END I2C1_Init 2 */
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  /* Эта функция не вызывается при ручной инициализации */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    __HAL_AFIO_REMAP_I2C1_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    //GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
  if(i2cHandle->Instance==I2C1)
  {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);
  }
}
