/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "st7735.h"
#include <stdio.h>
#include <stdlib.h>   // ← ДОБАВЬ ЭТУ СТРОКУ для функции abs()
#include "ina226.h"
#include "soft_i2c.h"
#include "encoder.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

TIM_HandleTypeDef htim2; //переменная таймера



// Текущий лимит тока (настраивается энкодером)
static float current_limit = 1.000f;  // По умолчанию 1.000A

// Флаг срабатывания Alert
static uint8_t alert_triggered = 0;

// Для мигания экрана при Alert
static uint32_t last_blink_time = 0;
static uint8_t blink_state = 0;
static uint32_t last_beep_time = 0; //переменная пищалки

// Для прерывистой пищалки
//static uint32_t last_beep_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UpdateDisplay(float v_val, float i_val, float power, float capacity_ah);
void HandleAlertIndication(void);
void MX_TIM2_Init(void);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // Инициализация дисплея
  ST7735_Init();

  // Инициализация программного I2C
  Soft_I2C_Init();

  // Инициализация энкодера и кнопки
  Encoder_Init();

  // === ТЕСТ ПИЩАЛКИ ===
    // Если при включении будет ПИИИИП - значит таймер настроен верно,
    // и проблема только в логике Alert.
    Buzzer_On();        // Включить ШИМ
    HAL_Delay(500);     // Ждать полсекунды
    Buzzer_Off();       // Выключить ШИМ
    // ====================

  // Приветственный экран
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_WriteString(10, 50, "LBP Indicator", Font_11x18, ST7735_WHITE, ST7735_BLACK);
  ST7735_WriteString(20, 75, "v1.1", Font_7x10, ST7735_GREEN, ST7735_BLACK);
  HAL_Delay(1500);

  // Проверка INA226
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_WriteString(5, 10, "Init INA226...", Font_7x10, ST7735_CYAN, ST7735_BLACK);

  if (Soft_I2C_IsDeviceReady(INA226_ADDR) == 0) {
      ST7735_WriteString(5, 25, "INA226 OK!", Font_7x10, ST7735_GREEN, ST7735_BLACK);
  } else {
      ST7735_WriteString(5, 25, "INA226 FAIL!", Font_7x10, ST7735_RED, ST7735_BLACK);
      while(1); // Останавливаемся если INA226 не найдена
  }

  HAL_Delay(500);

  // Инициализация INA226
  INA226_Init(NULL);

  // Настройка Alert
  INA226_SetCurrentLimit(current_limit);
  INA226_EnableCurrentAlert();

  ST7735_WriteString(5, 40, "Alert enabled", Font_7x10, ST7735_YELLOW, ST7735_BLACK);
  char msg[32];
  sprintf(msg, "Limit: %.2fA", current_limit);
  ST7735_WriteString(5, 55, msg, Font_7x10, ST7735_YELLOW, ST7735_BLACK);
  HAL_Delay(500);

  // Очищаем экран для основного режима
  ST7735_FillScreen(ST7735_BLACK);

  // Переменные для измерений
  float v_val = 0.0f;
  float i_val = 0.0f;
  float power = 0.0f;
  float capacity_ah = 0.0f;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // ==================== ОБРАБОТКА ЭНКОДЕРА ====================
	  int16_t encoder_delta = Encoder_GetDelta();

	  if (encoder_delta != 0) {
	      // Адаптивный шаг в зависимости от скорости вращения
	      float step;

	      if (abs(encoder_delta) > 6) {
	          // Быстрое вращение - шаг 0.1A
	          step = 0.1f;
	      } else {
	          // Медленное вращение - шаг 0.01A
	          step = 0.01f;
	      }

	      current_limit += encoder_delta * step;

	      // Ограничиваем диапазон
	      if (current_limit < 0.01f) current_limit = 0.01f;
	      if (current_limit > 2.35f) current_limit = 2.35f;

	      // Применяем новый лимит
	      INA226_SetCurrentLimit(current_limit);
	  }

    // ==================== КНОПКА СБРОСА ALERT ====================
    if (ResetButton_IsPressed()) {
      INA226_ClearAlert();
      alert_triggered = 0;
     Buzzer_Off();  // Выключаем пищалку

      // Восстанавливаем нормальный фон
      ST7735_FillScreen(ST7735_BLACK);
    }

    // ==================== ПРОВЕРКА ALERT ====================
    if (INA226_IsAlertTriggered() && !alert_triggered) {
      alert_triggered = 1;
    }




    // ==================== ЧТЕНИЕ ДАННЫХ ====================
    v_val = INA226_ReadBusVoltage();
    i_val = INA226_ReadCurrent();
    power = INA226_ReadPower();

    // Интегрирование тока для расчёта ёмкости
    capacity_ah += (i_val * 0.1f) / 3600.0f;  // 100мс опрос







    // ==================== ОБНОВЛЕНИЕ ЭКРАНА ====================
    if (alert_triggered) {
      HandleAlertIndication();
    } else {
      // Нормальный режим - чёрный фон
      UpdateDisplay(v_val, i_val, power, capacity_ah);
    }


    HAL_Delay(1);  // Обновление 10 раз в секунду

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* USER CODE BEGIN 4 */

/**
 * Обновление дисплея в нормальном режиме
 */
void UpdateDisplay(float v_val, float i_val, float power, float capacity_ah) {
  char str[40];

  // ==================== НАПРЯЖЕНИЕ ====================
  // Позиция: верх экрана
  sprintf(str, "%6.3f V", v_val);
  ST7735_WriteString(15, 10, str, Font_16x26, ST7735_RED, ST7735_BLACK);

  // ==================== ТОК ====================
  // Позиция: под напряжением
  sprintf(str, "%6.3f A", i_val);
  ST7735_WriteString(15, 40, str, Font_16x26, ST7735_BLUE, ST7735_BLACK);

  // ==================== ЛИМИТ ТОКА (регулируется энкодером) ====================
  // Позиция: ниже тока
  ST7735_WriteString(10, 72, "Limit:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
  sprintf(str, "%.3fA", current_limit);
  ST7735_WriteString(78, 72, str, Font_11x18, ST7735_WHITE, ST7735_BLACK);

  // ==================== МОЩНОСТЬ И ЁМКОСТЬ ====================
  // Позиция: внизу экрана
  sprintf(str, "%.2fW %.3fAh", power, capacity_ah);
  ST7735_WriteString(10, 100, str, Font_11x18, ST7735_GREEN, ST7735_BLACK);
}

/**
 * Индикация срабатывания Alert
 */
void HandleAlertIndication(void) {
  uint32_t current_tick = HAL_GetTick();

  // ==================== ЦИКЛ МИГАНИЯ И ПИСКА (500мс) ====================
  if (current_tick - last_blink_time > 500) {
    last_blink_time = current_tick;
    blink_state = !blink_state;

    if (blink_state) {
      // ЭКРАН ЗАГОРЕЛСЯ -> ВКЛЮЧАЕМ ЗВУК
      Buzzer_On();
      uint16_t bg_color = 0xFCE0; // Розовый
      ST7735_FillScreen(bg_color);
      ST7735_WriteString(25, 50, "ALERT!", Font_16x26, ST7735_RED, bg_color);

      char str[40];
      sprintf(str, "Limit: %.2fA", current_limit);
      ST7735_WriteString(20, 90, str, Font_11x18, ST7735_WHITE, bg_color);
      ST7735_WriteString(10, 110, "Press to reset", Font_7x10, ST7735_YELLOW, bg_color);
    }
    else {
      // ЭКРАН ПОГАС (ЧЕРНЫЙ) -> ВЫКЛЮЧАЕМ ЗВУК
      Buzzer_Off();
      ST7735_FillScreen(ST7735_BLACK);
      // На черном фоне текст можно не писать, чтобы быстрее мерцало
    }
  }
}

/* USER CODE END 4 */

/**
  * @brief System Clock Configuration
  * @retval None
  */

void MX_TIM2_Init(void) {
    //TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Включаем такты
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 2. Настраиваем PA3. ВАЖНО: Режим AF_PP (Альтернативная функция)
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. Настройка таймера (для пассивной пищалки ~2.5 кГц)
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 71; // 72МГц / 72 = 1 МГц (1 мкс тики)
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 400;   // 400 мкс = 2500 Гц
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim2);

    // 4. Настройка канала 4 (PA3)
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 200; // 50% скважность (громкость макс)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4);

    // Запуск основного счетчика (но PWM пока молчит)
    HAL_TIM_Base_Start(&htim2);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
