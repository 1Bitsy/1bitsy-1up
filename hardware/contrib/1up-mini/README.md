# 1up-mini

This is a board based on 1up v0.2, with horizontal form factor and easier to
solder by hand.

It is not supported by 1bit squared.

## What changed?

It aims to be fully compatible with 1up board. Some of the components are
changed to avoid hard to solder packages:

- Headphone amplifier reference is changed. It consumes more power than the
  one used on the original board.
- Volume control is the same component, with a different package.
- Battery charger and power path is split in two components.
- 0402 footprints are replaced with 0603.
- Touch screen signals are only connected to 6 pin connector.

Board shape is horizontal with symmetric buttons on both sides.

## What to change for the next iteration?

- 0603 and resistor network footprints are a little bit to small for hand
  soldering.
- SD card slot must be changed for something easier to solder.
- Headphone jack could be changed for something more solid.
- Hole for pogo pin is too small. Add a large pad so that it can be replaced
  with a cable.
- Switch are hard to source. Also the one I ordered are not so good.

## Assembly

Some tricky points:

- You will need solder paste, flux and hot air to solder the SD card slot.
- Surface under the LCD should be flat. Cut the pin used to mount the 1bitsy
  before soldering. Use isolating tape to avoid short circuits (there is a
  metal shield under the LCD).
- You can replace the pogo pin with a cable.

## BOM

| Reference              | Package    | Qty. | Designation     | Mouser/URL           |
| ---------------------- | ---------- | ---- | --------------- | -------------------- |
| C[19,26,27,29,30]      | 0603       | 5    | 100 nF          |                      |
| C[20,21,24,25,31..33]  | 0603       | 7    | 1 µF            |                      |
| C[22,23]               | 0603       | 2    | 2.2 µF          |                      |
| C28                    | 0805       | 1    | 10 µF           |                      |
| CON1                   | SCHA4B0419 | 1    | Micro SD        | 688-SCHA4B0419       |
| D2                     | 0603       | 1    | Yellow LED      |                      |
| J3                     | Jack       | 1    | JACK_3C1S       | 490-SJ2-35853BSMT-TR |
| P1                     | PogoPin    | 1    | PogoPin         | 575-906115 or cable  |
| P[2..4]                |            | 3    | Battery conn.   |                      |
| Q1                     | SOT-23     | 1    | NMOS            | 726-BSS806NH6327XT   |
| R11                    | 0603       | 1    | 49 kΩ           |                      |
| R[12..15]              | 4x0603     | 4    | 10 kΩ           | 667-EXB-38V103JV     |
| R[17,18]               | 0603       | 2    | 2.7 kΩ          |                      |
| R[20..23]              | 0603       | 4    | 10 Ω            |                      |
| R24                    | 0603       | 1    | 1 kΩ            |                      |
| R6                     | 0603       | 1    | 4.7 kΩ          |                      |
| R[7,10,16,19,25,26,30] | 0603       | 7    | 10 kΩ           |                      |
| R[8,9]                 | 0603       | 2    | 50 Ω            |                      |
| SW[12,13]              | Switch     | 2    | Side Switch     | 653-B3U-3000P        |
| SW14                   | Switch     | 1    | SPDT Switch     | 611-PCM12SMTR        |
| SW[2..11]              | Switch     | 1    | Switch          | [aliexpress][]       |
| U10                    | TSSOP-8    | 1    | TPS2111A        | 595-TPS2111APWR      |
| U2                     | 1bitsy     | 1    | 1bitsy          |                      |
| U3                     | TSSOP-14   | 1    | MCP4661-103E/ST | 579-MCP4661-103E/ST  |
| U4                     | TSSOP-14   | 1    | TPA6139A2-PW    | 595-TPA6139A2PWR     |
| U[5,6]                 | TSSOP-16   | 2    | 74HC165         | 771-74HC165PW-T      |
| U7                     | SOT-23-6   | 1    | BQ21040         | 595-BQ21040DBVR      |
| U8                     | ER-CON50HT | 1    | 50 pin conn.    | sold with LCD        |
| U9                     | ER-CON06HB | 1    | 6 pin conn.     | sold with LCD        |
| LCD                    |            | 1    | LCD             | [buydisplay][]       |

[buydisplay]:
  http://www.buydisplay.com/default/serial-spi-2-8-tft-lcd-module-display-320x240-optional-touch-screen
[aliexpress]:
  https://www.aliexpress.com/store/product/Original-new-100-Japan-conductive-rubber-head-waterproof-touch-switch-8-8-5-silent-key-micro/828867_32724415032.html
