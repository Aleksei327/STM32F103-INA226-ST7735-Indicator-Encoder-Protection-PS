/**
 * encoder.h
 * Энкодер через EXTI прерывания (БЕЗ таймера!)
 * Простое и надёжное решение
 */

#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

// ==================== НАСТРОЙКИ ПИНОВ ====================
// ЭНКОДЕР
// PA0 - CLK (с прерыванием)
// PA1 - DT (обычный вход)

// КНОПКА СБРОСА ALERT
#define RESET_BTN_PORT      GPIOA
#define RESET_BTN_PIN       GPIO_PIN_2

// ПИЩАЛКА (Buzzer)
#define BUZZER_PORT         GPIOA
//#define BUZZER_PIN          GPIO_PIN_3

// ==================== ФУНКЦИИ ====================

// Инициализация энкодера через EXTI прерывания
void Encoder_Init(void);

// Получить изменение счётчика энкодера с последнего вызова
// Возврат: количество "щелчков" (положительное/отрицательное/0)
int16_t Encoder_GetDelta(void);

// Сбросить счётчик энкодера
void Encoder_Reset(void);

// Проверка нажатия кнопки сброса Alert
uint8_t ResetButton_IsPressed(void);

// Управление пищалкой
//void Buzzer_On(void);
//void Buzzer_Off(void);
//void Buzzer_Beep(uint16_t duration_ms);

#endif
