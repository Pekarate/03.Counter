# Counter Firmware

Firmware for a counter device built on the Nuvoton MS51 series MCU and a VCNL4040 proximity sensor.

## Overview

This project counts object presence using the VCNL4040 proximity sensor, shows the result on a segmented LCD, and enters power-down mode when the system stays inactive for too long.

Current firmware behavior:

- MCU: Nuvoton `MS51xB9xE`
- Main sensor: `VCNL4040` over I2C
- Display: 3-byte segmented LCD driven directly from GPIO
- UART debug: `19200` baud
- System clock setup in firmware: HIRC `16 MHz`, `CKDIV = 1` for an `8 MHz` operating clock
- Sensor sampling period: `300 ms`
- Object confirmation time: about `1.5 s`
- Auto sleep timeout: `30 minutes` without detected object activity
- Manual sleep: hold the button for about `1.5 s`

## Main Behavior

### Startup

At boot the firmware:

- configures GPIO, UART, timer, I2C, and LCD
- starts the inactivity timeout immediately
- initializes the VCNL4040 sensor
- runs a calibration routine if no valid threshold is stored

### Counting Logic

The firmware reads proximity data periodically and compares it against `DETECT_THRESHOLD`.

- If the signal stays above threshold long enough, an object is considered present.
- When an object becomes visible, the displayed counter is incremented.
- When the object leaves, the visibility state is cleared.
- If the count ends in an odd half-step state, a delayed correction can reduce the count after `20 s`.

Key timing constants are defined in [src/main.c](/f:/01.Freelance/01.Quang/03.Counter/src/main.c).

### Sleep / Power-Down

There are two relevant power behaviors:

1. `Power-down shutdown`

- Triggered when no object activity is detected for `30 minutes`
- Triggered when the button is held for about `1.5 s`
- Implemented through `set_PCON_PD`

2. `Loop idle mode`

- There is optional `IDLE` support in the main loop behind `CONTROL_COM_ENABLE`
- In the current source this flag is commented out, so the firmware does not enter per-loop `IDLE`

## Important Files

- [src/main.c](/f:/01.Freelance/01.Quang/03.Counter/src/main.c): application flow, counting, LCD updates, button handling, sleep logic
- [src/vcnl4040.c](/f:/01.Freelance/01.Quang/03.Counter/src/vcnl4040.c): VCNL4040 configuration and proximity reads
- [src/usr_i2c.c](/f:/01.Freelance/01.Quang/03.Counter/src/usr_i2c.c): low-level I2C access
- [src/htim.c](/f:/01.Freelance/01.Quang/03.Counter/src/htim.c): system tick based on Timer 3
- [Keil/counter.uvproj](/f:/01.Freelance/01.Quang/03.Counter/Keil/counter.uvproj): Keil uVision project

## Build

### Toolchain

- Keil uVision for `MCS-51`
- Target device: `MS51xB9xE`

### Build Steps

1. Open [Keil/counter.uvproj](/f:/01.Freelance/01.Quang/03.Counter/Keil/counter.uvproj) in Keil uVision.
2. Select target `Counter`.
3. Build the project.
4. The Keil project is configured to generate `.hex` and run `Hex2Bin.exe` after build.

## Notes

- EEPROM read/write helpers are currently stubbed in the firmware, so threshold persistence depends on future implementation of those functions.
- `CONTROL_COM_ENABLE` is currently disabled in [src/main.c](/f:/01.Freelance/01.Quang/03.Counter/src/main.c#L14).
- The inactivity timeout now starts at boot, so the device also sleeps after `30 minutes` even if no object has ever been detected.
