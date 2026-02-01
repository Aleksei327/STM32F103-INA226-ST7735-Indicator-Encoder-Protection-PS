/**
 * encoder.c
 * Энкодер через EXTI прерывания на CLK пине
 * Простое и надёжное решение БЕЗ таймера
 */

#include "encoder.h"

extern TIM_HandleTypeDef htim2;

// Счётчик энкодера
static volatile int32_t encoder_count = 0;
static int32_t last_encoder_count = 0;

// Для защиты от дребезга
static volatile uint32_t last_interrupt_time = 0;
#define DEBOUNCE_TIME_MS 15

/**
 * Инициализация энкодера через EXTI прерывания
 */
void Encoder_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Включаем тактирование
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();  // Для EXTI

    // ==================== ЭНКОДЕР ====================
    // PA0 (CLK) - с прерыванием на оба фронта
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // Прерывание на оба фронта
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA1 (DT) - обычный вход
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Кнопка сброса (PA 2)

    GPIO_InitStruct.Pin = RESET_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(RESET_BTN_PORT, &GPIO_InitStruct);

    // Настройка приоритета и включение прерывания
        HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);


    // Пищалка


    // ==================== НАСТРОЙКА ПРЕРЫВАНИЯ ====================
    // EXTI Line 0 для PA0
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/**
 * Обработчик прерывания EXTI0 (PA0 - CLK энкодера)
 * ВАЖНО: Эту функцию нужно добавить в stm32f1xx_it.c!
 */
void EXTI0_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET) {
        // Очищаем флаг прерывания
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);

        // Защита от дребезга
        uint32_t current_time = HAL_GetTick();
        if ((current_time - last_interrupt_time) < DEBOUNCE_TIME_MS) {
            return;
        }

        last_interrupt_time = current_time;


        for(volatile int i = 0; i < 2000; i++) {
                    __asm("nop");
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) {
                     // Если DT высокий -> значит крутим в одну сторону
                     encoder_count--;
                } else {
                     // Если DT низкий -> значит крутим в другую
                     encoder_count++;
                }

        // Читаем состояние пинов
       /// uint8_t clk = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
       /// uint8_t dt = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);

        // Определяем направление
        // Если CLK и DT в одинаковом состоянии - против часовой
        // Если разные - по часовой
        ///if (clk == dt) {
        ///    encoder_count--;  // Против часовой
        ///} else {
        ///    encoder_count++;  // По часовой
        ///}
    }
}

/**
 * Получить изменение счётчика
 */
int16_t Encoder_GetDelta(void) {
    // Временно отключаем прерывания для атомарного чтения
    __disable_irq();
    int32_t current = encoder_count;
    __enable_irq();

    int16_t delta = (int16_t)(current - last_encoder_count);
    last_encoder_count = current;

    return delta;
}

/**
 * Сброс счётчика
 */
void Encoder_Reset(void) {
    __disable_irq();
    encoder_count = 0;
    last_encoder_count = 0;
    __enable_irq();
}

/**
 * Проверка нажатия кнопки
 */
uint8_t ResetButton_IsPressed(void) {
    static uint8_t last_state = GPIO_PIN_SET;
    static uint32_t last_press_time = 0;

    uint8_t current_state = HAL_GPIO_ReadPin(RESET_BTN_PORT, RESET_BTN_PIN);

    if (current_state == GPIO_PIN_RESET && last_state == GPIO_PIN_SET) {
        if ((HAL_GetTick() - last_press_time) > 50) {
            last_press_time = HAL_GetTick();
            last_state = current_state;
            return 1;
        }
    }

    last_state = current_state;
    return 0;
}

/**
 * Управление пищалкой
 */
void Buzzer_On(void) {
    // Включаем ШИМ на 4 канале - пищалка начинает орать
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
}

void Buzzer_Off(void) {
    // Выключаем ШИМ - тишина
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
}
