[![Join the chat at https://gitter.im/1bitsy/1bitsy-1up](https://badges.gitter.im/1bitsy/1bitsy-1up.svg)](https://gitter.im/1bitsy/1bitsy-1up?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

This repository contains the board and code used on the 1Bitsy 1UP.

## Intro

1Bitsy 1UP is a retro inspired handheld game console, the design is based
around the following __semi arbitrary constraints and requirements.__

* Formfactor: PCB the size of the inside of a GB DMG (the gray one) This will
  hopefully eventually allow us to put the 1UP pcb inside a modified GB DMG
  case, either original or new molds sold by several online stores.
* Main CPU: All code should be able to execute on a 1Bitsy that is fitted with
  an STM32F415RGT6. That particular chip (due to the package size) does not
  contain external ram support or LCD display engine. Making LCD drive little
  bit more tricky. It is a nice constraint that makes coding for the platform
  bit more of a challenge in the style of classic conloles and 8bit computers
  because it does not have enough memory for a full framebuffer. ("racing the
  beam" FTW)
* Main CPU Mounting: The board contains the 1Bitsy compatible low profile
  socket using [SL-115-TT-19](https://www.samtec.com/products/sl-115-tt-19)
  and [BBL-115-T-E](https://www.samtec.com/products/bbl-115-t-e) Samtec
  connectors as well as one MillMax PogoPin spring loaded SMD connector.
* LCD: The LCD is a 240x320 TFT display with capacitive touch, connected
  through the 8bit parallel interface to the 1Bitsy. This allows us to push the
  data speed to the display quite high and reach good framerates that would not
  be possible through the SPI interface. The capacitive touch screen adds an
  interesting aspect of control that was not available on classic consoles
  prior to DS. (That was also resistive not capacitive)
* Audio: We are connecting the STM32F4 built in DAC lines for audio generation.
  This is not as slick and good as an i2s part but will likely make things
  "interesting".
* Controls: The gamepad consists of the classic D-Pad, ABXY, Select, Start set
  of buttons. Because we have the capacitive touch no other analog controls are
  considered necessary.
* Storage: To simplify storing games and data in large quantities we have
  connected a microSD card over the SDIO interface to the STM32, allowing for
  reasonabely fast load and save times and maybe even some other interesting
  hacks.
* Expansion: The platform will receive future expansions and improvements that
  will strive to remain transparent. Meaning all code written for the first
  version of the platform should work either out of the box or only with very
  minor modifications.

The display used is a TFT LCD with I2C CapTouch and ILI9341 driver.

__The most basic design consists of:__
* 1Bitsy STM32F415RGT6 (168MHz, 192kb RAM, 1MB Flash)
* 240x320 2.8" TFT with capacitive touch and PWM backlight control
* D-Pad, ABXY, Start, Select buttons
* DAC audio out to headphones. (speakers optional)
* SDCard connected over SDIO interface

Second tier addons for quality of life:
* Lipo battery and charger built in
* Speaker audio

__Future "transparent" expansions:__
These expansions will contribute to new and interesting hacks possible on the
platform, they are not considered forward compatible but backward compatible.
This means that we do not anticipate the expansions to exist from the beginning
of the platform life and the software written at that time will not support the
future expansions, but the old software has to function as is or only with very
minor changes on the new device that contains the new expansions.

## Hacking

The board design is in the hardware subdirectory.
