
![20260201_201156](https://github.com/user-attachments/assets/993bd9a1-72f7-42b8-ba96-80e2ae9ea129)
STM32F103-INA226-LBP-Active-Protection-System (Advanced Version)
An intelligent measurement and active hardware protection system for Laboratory Power Supplies (LPS) based on STM32F103C8T6 (Blue Pill), INA226 high-side current sensor, and ST7735 TFT display.
This project is an evolution of my previous "Indicator only" project. It now features an interactive current limit control and a high-speed hardware emergency cutoff.
________________________________________
🚀 Key Features & Functionality
1.	High-Speed Hardware Cutoff (Active Protection):
o	Uses the INA226 ALERT pin. Unlike software-based comparison (which is slow), the INA226 chip monitors current at the hardware level and triggers the ALERT signal near-instantaneously.
o	Latch Mode: Once triggered, the protection stays active (latched) until the user manually resets it via the encoder button.
2.	Software I2C (Bit-Banging):
o	A custom-built I2C implementation. It provides extreme reliability, avoids standard STM32 hardware I2C "freezing" issues, and allows using any GPIO pins for SDA/SCL.
3.	Rotary Encoder Interface:
o	Real-time current limit adjustment with 10mA precision.
o	Intelligent debouncing and non-blocking operation.
4.	Visual & Audio Alarm System:
o	Flashing "ALERT!" screen with current limit info.
o	Synchronized intermittent beep using hardware PWM (TIM2) to ensure smooth operation without lagging the UI.
________________________________________
🛠 Technical Configuration & Customization
1. INA226 Sensor Settings
•	Shunt Resistor: Configured in INA226.c via #define INA226_SHUNT_OHMS 0.033f. Change this to match your resistor (e.g., 0.01 or 0.1).
•	Alert Limits: The hardware limit is calculated based on shunt voltage. The register step is fixed at 2.5 μV.
•	I2C Address: Default is 0x80 (A0 and A1 connected to GND). Change in INA226.h if needed.
2. Rotary Encoder (Mechanical & Code)
Mechanical encoders can be tricky. Here is how to tune them:
•	Direction Fix: If values decrease when turning clockwise:
o	Hardware fix: Swap the outer wires (CLK and DT).
o	Software fix: Change ++ to -- logic in the EXTI0_IRQHandler within encoder.c.
•	Debouncing: If the encoder "skips" steps, increase #define DEBOUNCE_TIME_MS (default is 15ms) to 25-30ms in encoder.c.
•	Pull-ups: Internal GPIO_PULLUP is enabled, but for noisy environments, adding external 4.7k - 10k resistors to +3.3V is highly recommended.
3. User Interface & Limits (ST7735)
•	Adjustment Bounds: Configured in main.c:
o	Max Limit: 3.0A
o	Min Limit: 0.01A
o	Step: 0.01A (10mA).
•	Display Logic: The screen refresh is optimized to only update values when they change, preventing SPI bus congestion.
________________________________________
🔌 Hardware Pinout
Peripheral	STM32 Pin	Description
Display (SPI1)	PA5, PA7	SCK and MOSI
Display (Control)	PA2, PA3, PA4	CS, DC, RST
INA226 (Soft I2C)	PB6, PB7	SCL and SDA
Rotary Encoder	PA0, PA1	CLK (Interrupt) and DT
Reset Button	PA2	Encoder's push button
Buzzer	PA3	PWM Output (TIM2_CH4)
________________________________________
⚠️ Critical Assembly Notes
The ALERT pin on the INA226 is an "Open-Drain" type. You MUST use a 10k Ohm pull-up resistor between the ALERT pin and +3.3V. For a complete protection circuit, connect this pin to an LM393 comparator or a similar logic circuit to control your power supply's output pass transistors.

