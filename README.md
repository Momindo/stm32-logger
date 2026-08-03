# stm32-logger

A FreeRTOS multi-sensor logger for the STM32F429I-DISC1, built as a
learning project covering RTOS design, driver development from datasheets,
and git branching workflow.

## Hardware

- STM32F429I-DISC1 discovery board (Cortex-M4F @ 168 MHz)
- On-board I3G4250D 3-axis gyroscope on SPI5
- On-board 2.4" ILI9341 TFT (also on SPI5)
- 8 MB external SDRAM on FMC
- USART1 to the ST-LINK virtual COM port @ 115200 8N1

## Building

Open in STM32CubeIDE and build. Requires:

- **Use float with printf from newlib-nano** enabled under
  Project Properties > C/C++ Build > Settings > MCU Settings

## Progress

| Tag | Milestone |
|---|---|
| v0.0.1 | Blink |
| v0.0.2 | Debounced button controls blink rate |
| v0.1.0 | FreeRTOS scaffold: blink and heartbeat tasks |
| v0.2.0 | I3G4250D gyroscope driver over SPI5 |
| v0.3.0 | Sample queue pipeline with two producers |

## Architecture

Producer tasks sample at their own rates and push fixed-size `sample_t`
records into a single queue. One logger task drains the queue and writes
CSV to the UART. Producers know nothing about the transport; the logger
knows nothing about the sensors.

| Task | Priority | Rate | Role |
|---|---|---|---|
| gyroTask | AboveNormal | 50 Hz | Producer, SRC_GYRO |
| defaultTask | Normal | 1 kHz poll | LED blink and button |
| sysTask | BelowNormal | 1 Hz | Producer, SRC_SYSTEM |
| heartbeatTask | BelowNormal | 1 Hz | Statistics line |
| loggerTask | Low | on demand | Consumer, writes UART |

## Output format

Comma-separated. Lines beginning with `#` are statistics and can be skipped
by a parser.

```
<timestamp_ms>,gyro,<seq>,<x>,<y>,<z>
<timestamp_ms>,sys,<seq>,<uptime_s>,<free_heap_kb>
# up=<s> q=<free slots> dropG=<n> dropS=<n> | gyro L=<n> | log L=<n>
```

Gyroscope values are **raw counts**, not degrees per second. Multiply by
0.00875 to convert. Zero-rate bias is measured at startup but deliberately
not subtracted on-device, so the logged data stays a faithful record.

## Design notes

**Producers never block.** `osMessageQueuePut` is called with a zero
timeout. If the queue is full the sample is discarded and a per-source
counter incremented. A producer that waited for queue space would miss its
own deadline, and every timestamp after that would be wrong. Dropped data
is recoverable; a distorted timebase is not.

**Every record carries a sequence number.** Gaps in `seq` reveal exactly
how many samples were lost and where.

**No dynamic allocation after startup.** All tasks, stacks and the queue
are allocated before the scheduler starts, so `free_heap` is constant.
Records travel by value with no pointers, which is what makes this
possible.

**One task owns each bus.** SPI5 has no mutex yet, so only `gyroTask`
issues SPI transactions. Concurrent access from a second task corrupts
transfers.

## Known issues and board quirks

**HSE does not lock on this board.** HSERDY never sets in either crystal
or bypass mode. Running on HSI instead: PLL M=16 N=336 P=2 Q=7, giving
168 MHz SYSCLK and a 48 MHz USB clock. HSI is +/-1% accurate, which is
out of spec for USB — must be resolved before the USB CDC phase.

**SPI5 pins need VERY_HIGH output speed.** CubeMX's board-selector default
of `GPIO_SPEED_FREQ_LOW` limits slew rate to roughly 2 MHz. At the 5.25 MHz
SPI clock this corrupts bits intermittently — long burst reads fail while
short transactions mostly survive. Set PF7/PF8/PF9 to Very High.

**The LCD shares SPI5.** CSX (PC2) and RDX (PD12) are active low and CubeMX
initialises them to RESET, leaving the display selected on the bus. Both
must be driven high before using the gyroscope.

**CubeMX cannot size the queue.** Its Item Size field only accepts
primitive types and silently reverts anything else to `uint16_t`. The
sample queue is therefore created by hand in `USER CODE BEGIN RTOS_QUEUES`
with `sizeof(sample_t)`. A `_Static_assert` in `sample.h` catches a
mismatch at compile time.

**HAL timebase must not be SysTick.** FreeRTOS owns SysTick; HAL is
configured to use TIM6.

## Gyroscope notes

WHO_AM_I returns 0xD3 (I3G4250D). Most STM32F429 examples assume the older
L3GD20, which returns 0xD4. Register maps are otherwise compatible.

BDU (CTRL_REG4 bit 7) is enabled so the output register pair is held stable
across a burst read.