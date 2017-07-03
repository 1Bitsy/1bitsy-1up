# README

This repository/directory contains code for the 1Bitsy 1UP retro inspired
handheld game console. Expect it to morph and change as the time goes by. Feel
free to rip out the code you need for your 1bitsy projects. But respect the
licenses of the files.

1UP is a retro inspired handheld game console subproject of the 1Bitsy dev
board project.

For more information visit https://hackaday.io/project/25632-1bitsy-1up

The 1Bitsy is an open hardware stm32F415 based small microcontroller development
board.

For more information visit http://1bitsy.org

The libopencm3 project aims to create an open-source firmware library for
various ARM Cortex-M3 microcontrollers.

For more information visit http://libopencm3.org

## Usage

You will have to fetch the libopencm3 submodule by running:

    git submodule init
    git submodule update

You compile the needed part of the library and the project firmware by invoking
"make" in the toplevel directory.

Executing "make flash" will try to use arm-none-eabi-gdb to connect ta a Black
Magic Probe and upload the firmware to your target.

## Notes

If you are using an older GCC compiler. (pre V5) You may need to provide the C
standard to the compiler. Otherwise the compiler might complain. To solve the
issue you can invoke the `make` command in the top level directory by setting the
`CFLAGS` variable:
```
CFLAGS=-std=c99 make
```

If your own project is using some specific extensions to the C standard you can
add the necessary `CFLAGS` variable setting to your project Makefile in the src
subdirectory.

## Contributions

Pull requests simplifying improving modularizing and all the polish possible
are very welcome! Join the 1Bitsy 1UP community at:
https://gitter.im/1bitsy/1bitsy-1up
