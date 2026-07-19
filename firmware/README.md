# Готовая прошивка / Ready-to-flash firmware

**Автор проекта: Aleksei SUBBOTIN**

## Файлы

- `LBP_Indicator_v1.1.hex` — прошивка Intel HEX для ST-LINK и
  STM32CubeProgrammer;
- `LBP_Indicator_v1.1.bin` — бинарная прошивка, адрес записи `0x08000000`;
- `LBP_Indicator_v1.1_HEX.zip` — минимальный архив с HEX и инструкцией;
- `LBP_Indicator_v1.1_forum.zip` — полный архив для публикации на форуме:
  исходники, прошивка, PDF и текстовое описание;
- `SHA256SUMS.txt` — контрольные суммы файлов.

Целевая плата: STM32F103C8T6 Blue Pill.

Перед записью проверьте правильность выбранного микроконтроллера и соединений
ST-LINK: `SWDIO`, `SWCLK`, `GND`, `3.3V`. После программирования выполните
сброс платы.

## Files

- `LBP_Indicator_v1.1.hex`: Intel HEX image for ST-LINK and
  STM32CubeProgrammer;
- `LBP_Indicator_v1.1.bin`: binary image, flash address `0x08000000`;
- `LBP_Indicator_v1.1_HEX.zip`: minimal HEX distribution package;
- `LBP_Indicator_v1.1_forum.zip`: complete source, firmware, PDF, and text
  package for forum publication;
- `SHA256SUMS.txt`: SHA-256 file checksums.

Target board: STM32F103C8T6 Blue Pill.
