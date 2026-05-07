# Migration Guide (Industrial Refactor)

## Overview

This project moved from a monolithic driver style to:

- `common/` shared foundations
- `hal/` peripheral transaction layer
- `drivers/` device-level logic with object-oriented C structs

## API Mapping

### SSD1306

- Old:
  - `ssd1306_oled_init()`
  - `ssd1306_oled_putc()`
  - `ssd1306_oled_refresh()`
- New:
  - `ssd1306_t *dev = ssd1306_default();`
  - `ssd1306_init(dev)`
  - `ssd1306_putc(dev, ch)`
  - `ssd1306_flush(dev)`

Compatibility wrappers are still provided in `ssd1306_oled.h`.

### USART1

- Old:
  - `usart1_try_read_byte()`
  - `usart1_try_read_string()`
- New additions:
  - `usart1_set_line_policy()` to select `CR/LF/CRLF`
  - internal RX path now uses reusable `ring_buffer_t`

## New Build Options

- `DEBUG_LOG` (ON/OFF)
- `ASSERT_LEVEL` (`0..2`)
- `OLED_REFRESH_MODE` (`AUTO|FULL|REGION`, reserved for strategy switching)

## Suggested Migration Steps for New Modules

1. put register-level transaction flow in `hal/`
2. put protocol/device logic in `drivers/`
3. expose only typed status returns (`stm_status_t`)
4. keep optional compatibility wrappers while migrating `app/`
