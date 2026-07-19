![20260201_201156](https://github.com/user-attachments/assets/993bd9a1-72f7-42b8-ba96-80e2ae9ea129)

# Laboratory Power Supply Meter and Protection

[Русский](README.md) | [English](README_EN.md)

**Project author: Aleksei SUBBOTIN**

Copyright © 2026 Aleksei SUBBOTIN.

This project is a low-cost, high-accuracy voltage/current meter and active
protection controller for a laboratory power supply. It uses an STM32F103C8T6
Blue Pill, INA226, 160x128 ST7735 TFT, rotary encoder, and passive buzzer.

On the hardware for which this project was developed, communication with the
INA226 through the STM32 hardware I2C peripheral could hang. The firmware
works around that issue with a complete software I2C implementation
(bit-banging) on `PB8` and `PB9`.

Only the I2C communication is implemented in software. Voltage/current
conversion, shunt-threshold comparison, and the physical `ALERT` output are
handled in hardware by the INA226.

Project video: [YouTube](https://youtu.be/KYVpQovXies)

## Features

- voltage, current, power, accumulated capacity, and current-limit display;
- rotary-encoder current-limit adjustment;
- INA226 hardware shunt over-limit detection (`SOL`);
- active-low, latched (`LEN`) physical `ALERT` output;
- `ALERT!` screen and intermittent audible warning;
- manual reset with the encoder push button;
- automatic re-trigger if the overload remains after reset;
- software I2C workaround for the hardware-I2C lockup;
- ready-to-flash HEX and BIN files.

The assembled unit has been tested to keep protection and the physical
`ALERT` output active until the reset button is pressed. After reset, the
INA226 checks the current again and immediately re-triggers if the overload
is still present.

## Firmware settings

| Setting | Value |
|---|---:|
| Initial current limit | 0.15 A |
| Minimum / maximum limit | 0.01 A / 2.35 A |
| Normal / fast encoder step | 0.01 A / 0.1 A |
| Shunt resistance | 0.033 ohm |
| Current resolution | 0.001 A |
| INA226 7-bit address | `0x40` |
| Address format used by the code | `0x80` |
| INA226 Configuration register | `0x484F` |
| Voltage reading correction | +0.015 V |
| Encoder debounce | 15 ms |
| Passive buzzer frequency | approximately 2.5 kHz |

`0x484F` selects 128-sample averaging, 204 us bus conversion, 204 us shunt
conversion, and continuous shunt-plus-bus measurement. A complete averaged
cycle takes approximately 52.2 ms. Other values such as `0x4127`, `0x4527`,
or `0x4927` are alternative conversion-time and averaging settings, not
newer firmware versions.

## Blue Pill pin map

| Blue Pill | Function | Connected signal |
|---|---|---|
| `PA0` | EXTI0 input | encoder `CLK` |
| `PA1` | digital input | encoder `DT` |
| `PA2` | active-low reset input | encoder `SW` |
| `PA3` | TIM2_CH4, approximately 2.5 kHz PWM | passive buzzer or driver |
| `PA5` | SPI1_SCK | ST7735 `SCL`/`SCK`/`CLK` |
| `PA7` | SPI1_MOSI | ST7735 `SDA`/`MOSI`/`DIN` |
| `PB0` | digital output | ST7735 `RES`/`RST` |
| `PB1` | digital output | ST7735 `DC`/`A0`/`RS` |
| `PB8` | software I2C SCL | INA226 `SCL` |
| `PB9` | software I2C SDA | INA226 `SDA` |
| `PB10` | digital output | ST7735 `CS`/`SS` |
| `3.3V` | logic supply | display, INA226, encoder |
| `GND` | common ground | all modules and protection circuit |

Although CubeMX assigns hardware I2C1 to `PB6`/`PB7`, the working INA226 code
does not use those pins.

## ST7735 wiring

| ST7735 signal | Blue Pill | Purpose |
|---|---|---|
| `VCC` | `3.3V` | display supply |
| `GND` | `GND` | common ground |
| `SCL`, `SCK`, `CLK` | `PA5` | SPI1 clock |
| `SDA`, `MOSI`, `DIN` | `PA7` | SPI1 data from STM32 |
| `RES`, `RST`, `RESET` | `PB0` | hardware reset |
| `DC`, `A0`, `RS` | `PB1` | command/data selection |
| `CS`, `SS` | `PB10` | chip select |
| `LED`, `BL`, `BLK` | `3.3V` | backlight |
| `MISO`, `SDO` | not connected | display data is not read |

Use 3.3 V for the module supply and logic. Add a series resistor to the
backlight if the display module does not already include one.

## Encoder wiring

| Encoder signal | Blue Pill | Purpose |
|---|---|---|
| `+`, `VCC` | `3.3V` | module supply |
| `GND` | `GND` | common ground |
| `CLK` | `PA0` | EXTI0 encoder pulses |
| `DT` | `PA1` | rotation direction |
| `SW` | `PA2` | protection reset button |

The inputs use internal pull-ups. The push button must connect `PA2` to
`GND` when pressed.

## INA226 wiring

### INA226 VSSOP-10 pins

| Pin | Name | Connection |
|---:|---|---|
| 1 | `A1` | `GND` for address `0x40` |
| 2 | `A0` | `GND` for address `0x40` |
| 3 | `ALERT` | 10 kohm pull-up to 3.3 V and external cutoff input |
| 4 | `SDA` | Blue Pill `PB9` |
| 5 | `SCL` | Blue Pill `PB8` |
| 6 | `VS` | Blue Pill `3.3V` |
| 7 | `GND` | common `GND` |
| 8 | `VBUS` | measured bus, normally the load side of the shunt |
| 9 | `IN-` | load side of the shunt |
| 10 | `IN+` | source side of the shunt |

### Typical INA226 module

| Module signal | Connection |
|---|---|
| `VCC`, `VS` | Blue Pill `3.3V` |
| `GND` | common `GND` |
| `SCL` | Blue Pill `PB8` |
| `SDA` | Blue Pill `PB9` |
| `A0`, `A1` | `GND` for address `0x40` |
| `ALERT`, `AL` | 10 kohm pull-up to 3.3 V and external cutoff circuit |
| `IN+`, `VIN+` | source side of the shunt |
| `IN-`, `VIN-` | load side of the shunt |
| `VBUS` | load side (`IN-`) if not already connected on the module |

The INA226 `VS` supply accepts only 2.7-5.5 V and must not be connected to
the power supply's high-voltage rail. `VBUS` is the measurement input and is
rated for bus voltages up to 36 V.

`SCL`, `SDA`, and `ALERT` are open-drain signals and require pull-ups to
3.3 V. Many INA226 modules already contain I2C pull-up resistors.

## ALERT and reset behavior

1. The STM32 calculates `threshold voltage = current limit * 0.033 ohm`.
2. It programs the INA226 Alert Limit register and enables `SOL + LEN`.
3. The INA226 compares the shunt voltage against the threshold in hardware.
4. An overload sets `AFF` and drives the open-drain `ALERT` output low.
5. The STM32 detects `AFF` over software I2C, displays `ALERT!`, and starts
   the buzzer.
6. Pressing the encoder button on `PA2` calls the reset routine and
   re-enables `SOL + LEN`.
7. Normal current resumes operation; a continuing overload re-asserts the
   alert on the next INA226 conversion.

The INA226 does not provide a separate "clear ALERT" I2C command. With
`LEN = 1`, reading the Mask/Enable register (`0x06`) acknowledges the event
and clears the latch according to the Texas Instruments documentation. This
is what "software reset of ALERT" means in this firmware.

The physical `ALERT` pin is not connected to a separate Blue Pill GPIO in the
current design. The MCU reads `AFF` through I2C, while `ALERT` feeds an
external hardware cutoff circuit. Do not connect it directly to a power
MOSFET gate or a high-voltage node; use a suitable driver, optocoupler,
comparator, and/or hardware latch.

## Buzzer

`PA3` generates approximately 2.5 kHz PWM for a passive buzzer. A small
piezo sounder may be connected between `PA3` and `GND`. Use a transistor
driver for higher-current or electromagnetic sounders.

## Ready-to-flash firmware

The [`firmware`](firmware) directory contains:

- [`LBP_Indicator_v1.1.hex`](firmware/LBP_Indicator_v1.1.hex) for ST-LINK or
  STM32CubeProgrammer;
- [`LBP_Indicator_v1.1.bin`](firmware/LBP_Indicator_v1.1.bin), flash address
  `0x08000000`;
- a HEX-only archive;
- a complete forum package with source, PDF, and firmware;
- SHA-256 checksums.

The complete Russian wiring and operation manual is available as
[`LBP_Indicator_RU.pdf`](docs/LBP_Indicator_RU.pdf).

## Main source files

- `Core/Src/main.c`: main loop, UI, and software alert state;
- `Core/Src/INA226.c`: measurements and INA226 alert setup;
- `Core/Src/soft_i2c.c`: software I2C on `PB8`/`PB9`;
- `Core/Src/encoder.c`: encoder, push button, and buzzer;
- `Core/Src/st7735.c`: display driver;
- `Core/Src/spi.c`: display SPI1 configuration;
- `LBP_Indicator.ioc`: STM32CubeMX configuration.

## Links

- [Project video on YouTube](https://youtu.be/KYVpQovXies)
- [GitHub repository](https://github.com/Aleksei327/STM32F103-INA226-ST7735-Indicator-Encoder-Protection-PS)
- [Texas Instruments INA226 documentation](https://www.ti.com/product/INA226)
