/*
 * This file is part of the 1Bitsy lcd graphics library.
 *
 * Copyright (C) 2017 Bob Miller <kbob@jogger-egg.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

// own header
#include "lcd.h"

// C and POSIX headers
#include <assert.h>

// External Library headers
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

// Current Library headers
#include "gfx-pixslice.h"
#include "gpio.h"
#include "intr.h"
#include "systick.h"


// --  Dimensions  --  --  --  --  --  --  --  --  --  --  --  --  --  -

#define RAM_BASE 0x20000000
#define RAM_SIZE (128 << 10)    /* 128 Kbytes */


// --  Pixslice  -  --  --  --  --  --  --  --  --  --  --  --  --  --  -

#define PIXSLICE_COUNT          2

typedef enum pixslice_state {
    TS_CLEARED,
    TS_DRAWING,
    TS_SEND_WAIT,
    TS_SENDING,
    TS_CLEAR_WAIT,
    TS_CLEARING,
} pixslice_state;

typedef struct pixslice_impl {
    gfx_pixslice            slice; // must be first.
    volatile pixslice_state state;
    void                  *buffer;
} pixslice_impl;

static gfx_rgb565 pixslice_buffers[PIXSLICE_COUNT][LCD_MAX_SLICE_PIXELS]
                                  __attribute__((section(".sram1")));
static pixslice_impl pixslices[PIXSLICE_COUNT];

static volatile gfx_rgb565 bg_color = 0x0000;

static inline size_t pixslice_size_bytes(const gfx_pixslice *slice)
{
    return slice->h * slice->w * sizeof *slice->pixels;
}


// --  Pin Mapping  -  --  --  --  --  --  --  --  --  --  --  --  --  -

#define LCD_CSX_PORT    GPIOC
#define LCD_CSX_PIN     GPIO3
#define LCD_RESX_PORT   GPIOC
#define LCD_RESX_PIN    GPIO2
#define LCD_DCX_PORT    GPIOC
#define LCD_DCX_PIN     GPIO6
#define LCD_WRX_PORT    GPIOB
#define LCD_WRX_PIN     GPIO1
#define LCD_RDX_PORT    GPIOB
#define LCD_RDX_PIN     GPIO0
#define LCD_DATA_PORT   GPIOB
#define LCD_DATA_PINS ((GPIO15 << 1) - GPIO8)
#define LCD_BL_PORT     GPIOB
#define LCD_BL_PIN      GPIO4

static const gpio_pin lcd_gpio_pins[] = {

    {                           // CSX (chip select)
        .gp_port  = LCD_CSX_PORT,
        .gp_pin   = LCD_CSX_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 1,
    },
    {                           // RESX (reset)
        .gp_port  = LCD_RESX_PORT,
        .gp_pin   = LCD_RESX_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 1,
    },
    {                           // DCX (data/command)
        .gp_port  = LCD_DCX_PORT,
        .gp_pin   = LCD_DCX_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
    },
    {                           // WRX (World Rallycross)
        .gp_port  = LCD_WRX_PORT,
        .gp_pin   = LCD_WRX_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 1,
    },
    {                           // RDX (read enable)
        .gp_port  = LCD_RDX_PORT,
        .gp_pin   = LCD_RDX_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 1,
    },
    {                           // DATA[0..7] (data pins)
        .gp_port  = LCD_DATA_PORT,
        .gp_pin   = LCD_DATA_PINS,
        .gp_mode  = GPIO_MODE_OUTPUT,
    },
    {                           // BL (backlight control)
        .gp_port  = LCD_BL_PORT,
        .gp_pin   = LCD_BL_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_pupd  = GPIO_PUPD_NONE,
        .gp_level = 1,
    }
};
        
static const size_t lcd_gpio_pin_count = (&lcd_gpio_pins)[1] - lcd_gpio_pins;


// -- ILI9341 Definitions  --  --  --  --  --  --  --  --  --  --  --  -

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0D
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR    0x30
#define ILI9341_MADCTL   0x36
#define ILI9341_VSCRSADD 0x37
#define ILI9341_PIXFMT   0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1
// #define ILI9341_PWCTR6  0xFC
#define ILI9341_IFCTL   0xF6

/* The first bunch of commands is undocumented. It turns out this is what ILItek provided
 * and is necessary even though we do not know what it does.
 * The relevant link for this this forum entry:
 * https://forums.adafruit.com/viewtopic.php?f=47&t=63229
 */
static const uint8_t ILI9341_init_commands[] = {
    4,  0xEF, 0x03, 0x80, 0x02,
    4,  0xCF, 0x00, 0XC1, 0X30,
    5,  0xED, 0x64, 0x03, 0X12, 0X81,
    4,  0xE8, 0x85, 0x00, 0x78,
    6,  0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,
    2,  0xF7, 0x20,
    3,  0xEA, 0x00, 0x00,
    /* GVDD level, VOCM & grayscale.
     * Param 1: 0x23 -> 3.8V
     */
    2,  ILI9341_PWCTR1, 0x23,
    /* Step-up circuit factor.
     * Param 1: 0x10 -> -VCI x 4
     */
    2,  ILI9341_PWCTR2, 0x10,
    /* VCOMH & VOML voltage.
     * Param 1: 0x3e -> 3.45V 0x28 -> -1.5
     */
    3,  ILI9341_VMCTR1, 0x3e, 0x28,
    /* VCOM offset.
     * Param 1: 0x86 -> VCOMH=VMH+6 VCOML=VMH+6
     */
    2,  ILI9341_VMCTR2, 0x86,

    /* Set endianess to LSB */
    4,  ILI9341_IFCTL, 0x00, 0x00, 0x20, 

    /* Memory Access Control: Vertical mode.
     * Memory to LCD data order.
     * D7: 0 -> Top to Bottom
     * D6: 1 -> Right to Left
     * D5: 0 -> Normal Mode
     * D4: 0 -> LCD Refresh Top to Bottom
     * D3: 1 -> BGR color order
     * D2: 0 -> LCD Refresh Left to Right
     * D1-0: 0 -> Switching between Segment outputs and RAM
     */
    /*2,  ILI9341_MADCTL, 0x48,*/

    /* Memory Access Control: Landscape mode.
     * Memory to LCD data order.
     * D7: 1 -> Bottom to Top
     * D6: 1 -> Right to Left
     * D5: 1 -> Reverse Mode
     * D4: 0 -> LCD Refresh Top to Bottom
     * D3: 1 -> BGR color order
     * D2: 0 -> LCD Refresh Left to Right
     * D1-0: 0 -> Switching between Segment outputs and RAM
     */
    2,  ILI9341_MADCTL, 0xE8,

    /* XXX: datasheet says this command takes 2 params?
     * Param 2?: 0x55 -> RGB & MCU iface fmt 16bit.
     */
    2,  ILI9341_PIXFMT, 0x55,
    /* Frame Rate Ctrl:
     * Param 1: 0x00 -> div ratio fsoc/1, 0x18 -> Frame Rate 79fps
     */
    3,  ILI9341_FRMCTR1, 0x00, 0x18,
    /* Display Function Control:
     * Param 1: 0x08
     * PTG: Set the scan mode in non-dislay area: Interval scan
     * PT: Determine source/VCOM output in a non-display area in the partial
     *     display mode:
     *  Source output on non-display area
     *   Positive polarity: V63
     *   Negative polarity: V0
     *  VCOM output on non-display area
     *   Positive polarity: VCOML
     *   Negative polarity: VCOMH
     * Param 2: 0x82
     * REV: Set display to Normally white.
     * GS: Gate output scan direction: G1 -> G320
     * SS: Source output scan direction: S1 -> S720
     * SM: Set gate driver arrangement: Even & Odd number parallel
     * Param 3: 0x27
     * Set the number of lines to drive in 8 line intervals.
     * NL: 320 lines
     * Param 4: skipped
     */
    4,  ILI9341_DFUNCTR, 0x08, 0x82, 0x27,
    /* Gamma Function Disable. Undocumented register. */
    2,  0xF2, 0x00,
    /* Gamma curve select
     * Param 1: 0x01
     * GC: Select gamma curve 1 (G2.2)
     */
    2,  ILI9341_GAMMASET, 0x01,
    /* Set Positive Gamma Correction
     * Values adjust the gray scale voltage to adjust the gamma characteristics
     * of the TFT panel.
     */
    16, ILI9341_GMCTRP1, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
                         0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03,
                         0x0E, 0x09, 0x00,
     /* Set Negative Gamma Correction
      * Inverse of the above? The datasheet is not very verbose on how this
      * works. :(
      */
    16, ILI9341_GMCTRN1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
                         0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C,
                         0x31, 0x36, 0x0F,
    0
};

// --  Bit Banging  -  --  --  --  --  --  --  --  --  --  --  --  --  -

// Send 8 bits of command or data.  Deselect the ILI chip if done.
static inline void bang8(uint8_t data, bool cmd, bool done)
{
    if (cmd) {
        gpio_clear(LCD_CSX_PORT, LCD_CSX_PIN);
        gpio_clear(LCD_DCX_PORT, LCD_DCX_PIN);
    }
    gpio_clear(LCD_WRX_PORT, LCD_WRX_PIN);
    #if LCD_DATA_PINS == 0x00FF
        ((volatile uint8_t *)&GPIO_ODR(LCD_DATA_PORT))[0] = data;
    #elif LCD_DATA_PINS == 0xFF00
        ((volatile uint8_t *)&GPIO_ODR(LCD_DATA_PORT))[1] = data;
    #else
        #error "data pins must be byte aligned."
    #endif
    gpio_set(LCD_WRX_PORT, LCD_WRX_PIN);
    if (cmd)
        gpio_set(LCD_DCX_PORT, LCD_DCX_PIN);
    if (done)
        gpio_set(LCD_CSX_PORT, LCD_CSX_PIN);
}

// 16 bits big-endian.
static inline void bang16(uint16_t data, bool done)
{
    bang8(data >> 8, false, false);
    bang8(data & 0xFF, false, done);
}


// --  Clear DMA   --  --  --  --  --  --  --  --  --  --  --  --  --  -

static volatile bool clear_dma_busy;

static void init_clear_dma(void)
{
    rcc_periph_clock_enable(RCC_DMA2);
    nvic_enable_irq(NVIC_DMA2_STREAM7_IRQ);
}

static void start_clear_dma(const pixslice_impl *impl)
{
    uintptr_t base = (uintptr_t)impl->buffer;
    size_t size = LCD_MAX_SLICE_BYTES;
    assert(RAM_BASE <= base);
    assert(base + size <= RAM_BASE + RAM_SIZE);
    assert(size >= 16);
    assert(!(size & 0xF));      // multiple of 16 bytes

    // Fill the first 16 bytes with the pixel.  Then
    // DMA will duplicate that through the buffer.
    uint32_t *p = impl->buffer;
    const int pburst = 4;

    uint32_t pix_twice = bg_color << 16 | bg_color;

    for (int i = 0; i < pburst; i++)
        *p++ = pix_twice;

    DMA2_S7CR &= ~DMA_SxCR_EN;
    while (DMA2_S7CR & DMA_SxCR_EN)
        continue;

    DMA2_S7PAR  = (void *)base;
    DMA2_S7M0AR = p;
    DMA2_S7NDTR = (size - 16) / 4;
    DMA2_S7FCR  = (DMA_SxFCR_FEIE          |
                   DMA_SxFCR_DMDIS         |
                   DMA_SxFCR_FTH_4_4_FULL);
    DMA2_S7CR = (DMA_SxCR_CHSEL_0          |
                   DMA_SxCR_MBURST_INCR4   |
                   DMA_SxCR_PBURST_INCR4   |
                  !DMA_SxCR_DBM            |
                   DMA_SxCR_PL_LOW         |
                  !DMA_SxCR_PINCOS         |
                   DMA_SxCR_MSIZE_32BIT    |
                   DMA_SxCR_PSIZE_32BIT    |
                   DMA_SxCR_MINC           |
                  !DMA_SxCR_PINC           |
                  !DMA_SxCR_CIRC           |
                   DMA_SxCR_DIR_MEM_TO_MEM |
                  !DMA_SxCR_PFCTRL         |
                   DMA_SxCR_TCIE           |
                  !DMA_SxCR_HTIE           |
                   DMA_SxCR_TEIE           |
                   DMA_SxCR_DMEIE          |
                   DMA_SxCR_EN);
}

void dma2_stream7_isr(void)
{
    const uint32_t ERR_BITS = DMA_HISR_TEIF7 | DMA_HISR_DMEIF7 | DMA_HISR_FEIF7;
    const uint32_t CLEAR_BITS = DMA_HISR_TCIF7 | DMA_HISR_HTIF7 | ERR_BITS;
    uint32_t dma2_hisr = DMA2_HISR;
    DMA2_HIFCR = dma2_hisr & CLEAR_BITS;
    assert((dma2_hisr & ERR_BITS) == 0);

    assert(dma2_hisr & DMA_HISR_TCIF7);

    DMA2_S7CR  = 0;
    DMA2_HIFCR = CLEAR_BITS;

    clear_dma_busy = false;
    for (size_t i = 0; i < PIXSLICE_COUNT; i++) {
        pixslice_impl *impl = &pixslices[i];
        if (impl->state == TS_CLEARING) {
            impl->state = TS_CLEARED;
        } else if (impl->state == TS_CLEAR_WAIT && !clear_dma_busy) {
            impl->state = TS_CLEARING;
            clear_dma_busy = true;
            start_clear_dma(impl);
        }
    }
}

static void clear_pixslice(gfx_pixslice *slice)
{
    pixslice_impl *impl = (pixslice_impl *)slice;
    bool busy = false;
    WITH_INTERRUPTS_MASKED {
        busy = clear_dma_busy;
        if (busy) {
            impl->state = TS_CLEAR_WAIT;
        } else {
            clear_dma_busy = true;
            impl->state = TS_CLEARING;
        }
    }
    if (!busy)
        start_clear_dma(impl);
}


// --  Video DMA   --  --  --  --  --  --  --  --  --  --  --  --  --  -

static volatile bool video_dma_busy;

static void start_video_dma(const pixslice_impl *impl)
{
    const gfx_pixslice *slice = &impl->slice;
    // Configure DMA.
    {
        DMA2_S1CR &= ~DMA_SxCR_EN;
        while (DMA2_S1CR & DMA_SxCR_EN)
            continue;

        void *par;
        #if LCD_DATA_PINS == 0x00FF
            par = (void *)&GPIO_ODR(LCD_DATA_PORT);
        #elif LCD_DATA_PINS == 0xFF00
            par = (uint8_t *)&GPIO_ODR(LCD_DATA_PORT) + 1;
        #else
            #error "data pins must be byte aligned."
        #endif

        DMA2_S1PAR  = par;
        DMA2_S1M0AR = impl->buffer;
        DMA2_S1NDTR = pixslice_size_bytes(slice);
        DMA2_S1FCR  = (DMA_SxFCR_FEIE                 |
                       DMA_SxFCR_DMDIS                |
                       DMA_SxFCR_FTH_4_4_FULL);
        DMA2_S1CR   = (DMA_SxCR_CHSEL_7               |
                       DMA_SxCR_MBURST_INCR4          |
                       DMA_SxCR_PBURST_SINGLE         |
                      !DMA_SxCR_CT                    |
                      !DMA_SxCR_DBM                   |
                       DMA_SxCR_PL_VERY_HIGH          |
                      !DMA_SxCR_PINCOS                |
                       DMA_SxCR_MSIZE_32BIT           |
                       DMA_SxCR_PSIZE_8BIT            |
                       DMA_SxCR_MINC                  |
                      !DMA_SxCR_PINC                  |
                       DMA_SxCR_CIRC                  |
                       DMA_SxCR_DIR_MEM_TO_PERIPHERAL |
                      !DMA_SxCR_PFCTRL                |
                       DMA_SxCR_TCIE                  |
                      !DMA_SxCR_HTIE                  |
                       DMA_SxCR_TEIE                  |
                       DMA_SxCR_DMEIE                 |
                       DMA_SxCR_EN);
    }

    // Configure Timer.
    {
        TIM8_CR1    = 0;
        TIM8_CR1    = (TIM_CR1_CKD_CK_INT             |
                      !TIM_CR1_ARPE                   |
                       TIM_CR1_CMS_EDGE               |
                       TIM_CR1_DIR_UP                 |
                      !TIM_CR1_OPM                    |
                      !TIM_CR1_URS                    |
                      !TIM_CR1_UDIS                   |
                      !TIM_CR1_CEN);
        TIM8_CR2    = (
                      !TIM_CR2_TI1S                   |
                       TIM_CR2_MMS_RESET              |
                       TIM_CR2_CCDS);
        TIM8_SMCR   = 0;
        TIM8_DIER   = (
                      !TIM_DIER_TDE                   |
                      !TIM_DIER_CC4DE                 |
                      !TIM_DIER_CC3DE                 |
                      !TIM_DIER_CC2DE                 |
                      !TIM_DIER_CC1DE                 |
                       TIM_DIER_UDE                   |
                      !TIM_DIER_TIE                   |
                      !TIM_DIER_CC4IE                 |
                      !TIM_DIER_CC3IE                 |
                      !TIM_DIER_CC2IE                 |
                      !TIM_DIER_CC1IE                 |
                      !TIM_DIER_UIE);
        TIM8_SR     = 0;
        TIM8_EGR    = TIM_EGR_UG;
        TIM8_CCMR1  = 0;
        TIM8_CCMR2  = (TIM_CCMR2_CC3S_OUT             |
                       TIM_CCMR2_OC3M_PWM2);
        TIM8_CCER   = (
                      !TIM_CCER_CC4P                  |
                      !TIM_CCER_CC4E                  |
                      !TIM_CCER_CC3NP                 |
                       TIM_CCER_CC3NE                 |
                      !TIM_CCER_CC3P                  |
                      !TIM_CCER_CC3E                  |
                      !TIM_CCER_CC2NP                 |
                      !TIM_CCER_CC2NE                 |
                      !TIM_CCER_CC2P                  |
                      !TIM_CCER_CC2E                  |
                      !TIM_CCER_CC1NP                 |
                      !TIM_CCER_CC1NE                 |
                      !TIM_CCER_CC1P                  |
                      !TIM_CCER_CC1E);
        TIM8_CNT    = 0;
        TIM8_PSC    = 0;
        // TIM8_ARR    = 34/2;
        // TIM8_CCR3   = 17/2;
        //TIM8_ARR    = 17 + 2;
        //TIM8_CCR3   = 12 + 4;
        TIM8_ARR    = 11 + 2 + 10;
        TIM8_CCR3   = 5 + 4 + 10;
        TIM8_BDTR   = TIM_BDTR_MOE | TIM_BDTR_OSSR;
        // TIM8_DCR    = 0;
        // TIM8_DMAR   = 0;
    }

    // Bit-bang the ILI9341 RAM address range.
    bang8(ILI9341_CASET, true, false);
    bang16(slice->x, false);
    bang16(slice->x + slice->w - 1, false);

    bang8(ILI9341_PASET, true, false);
    bang16(slice->y, false);
    bang16(slice->y + slice->h - 1, false);

    // Bit-bang the command word.
    bang8(ILI9341_RAMWR, true, false);

    // Switch the LCD_WRX pin to timer control.
    gpio_set_af(LCD_WRX_PORT, GPIO_AF3, LCD_WRX_PIN);
    gpio_mode_setup(LCD_WRX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, LCD_WRX_PIN);

    // Start the timer.
    TIM8_CR1 |= TIM_CR1_CEN;
}

volatile uint32_t dma2_lisr_save;
volatile uint32_t success_count;
volatile uint32_t error_count;

void dma2_stream1_isr(void)
{
    const uint32_t ERR_BITS = DMA_LISR_TEIF1 | DMA_LISR_DMEIF1 | DMA_LISR_FEIF1;
    const uint32_t CLEAR_BITS = DMA_LISR_TCIF1 | DMA_LISR_HTIF1 | ERR_BITS;
    uint32_t dma2_lisr = DMA2_LISR;
    DMA2_LIFCR = dma2_lisr & CLEAR_BITS;
    if (dma2_lisr & ERR_BITS) {
        dma2_lisr_save = dma2_lisr;
        error_count++;
    } else {
        success_count++;
    }
    // assert((dma2_lisr & ERR_BITS) == 0);

    if (dma2_lisr & DMA_LISR_TCIF1) {
        // Transfer done.
        //  - Deselect ILI9341.
        //  - Switch LCD_WRX and LCD_RDX pins back to GPIO mode.
        //  - Stop the timer.
        //  - Stop the DMA.
        gpio_set(LCD_CSX_PORT, LCD_CSX_PIN);
        gpio_mode_setup(LCD_WRX_PORT,
                        GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                        LCD_WRX_PIN);
        gpio_mode_setup(LCD_RDX_PORT,
                        GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                        LCD_RDX_PIN);

        // DMA2_S1CR &= ~DMA_SxCR_EN;
        // while (DMA2_S1CR & DMA_SxCR_EN)
        //     continue;

        TIM8_CR1   = 0;
        DMA2_S1CR  = 0;
        while (DMA2_S1CR & DMA_SxCR_EN)
            continue;
        DMA2_LIFCR = CLEAR_BITS;

        video_dma_busy = false;
        for (size_t i = 0; i < PIXSLICE_COUNT; i++) {
            pixslice_impl *impl = &pixslices[i];
            if (impl->state == TS_SENDING) {
                clear_pixslice(&impl->slice);
            } else if (impl->state == TS_SEND_WAIT && !video_dma_busy) {
                impl->state = TS_SENDING;
                video_dma_busy = true;
                start_video_dma(impl);
            }
        }
    }
}

static void init_video_dma(void)
{
    // RCC
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);

    // GPIO
    gpio_init_pins(lcd_gpio_pins, lcd_gpio_pin_count);
        
    // TIMER
    rcc_periph_clock_enable(RCC_TIM8);

    // DMA: DMA controller 2, stream 1, channel 7.
    rcc_periph_clock_enable(RCC_DMA2);
    nvic_enable_irq(NVIC_DMA2_STREAM1_IRQ);

    // Initialize ILI9341.
    {
        // toggle RST low to reset
        gpio_set(LCD_RESX_PORT, LCD_RESX_PIN);
        delay_msec(5);
        gpio_clear(LCD_RESX_PORT, LCD_RESX_PIN);
        delay_msec(20);
        gpio_set(LCD_RESX_PORT, LCD_RESX_PIN);
        delay_msec(150);

        // Send commands to ILI9341.
        const uint8_t *addr = ILI9341_init_commands;
        while (1) {
            uint8_t count = *addr++;
            if (count-- == 0)
                break;
            bang8(*addr++, true, false);
            while (count-- > 0) {
                bang8(*addr++, false, false);
            }
        }
        bang8(ILI9341_SLPOUT, true, true); // Exit sleep.

        delay_msec(120);                
        bang8(ILI9341_DISPON, true, true);
    }
}


// --  Pixslice DMA buffers --  --  --  --  --  --  --  --  --  --  --  -

static void init_pixslices()
{
    for (size_t i = 0; i < PIXSLICE_COUNT; i++) {
        pixslice_impl *impl = pixslices + i;
        impl->buffer = pixslice_buffers[i];
        clear_pixslice(&impl->slice);
    }
}

// --  Backlight control API   --  --  --  --  --  --  --  --  --  --  -

void lcd_bl_on(void)
{
    /* Enable backlight. */
    gpio_set(LCD_BL_PORT, LCD_BL_PIN);
}

void lcd_bl_off(void)
{
    /* Disable backlight. */
    gpio_clear(LCD_BL_PORT, LCD_BL_PIN);
}

void lcd_bl_toggle(void)
{
    /* Toggle the backlight state. */
    gpio_toggle(LCD_BL_PORT, LCD_BL_PIN);
}

void lcd_bl_init(void)
{
    /* Nothing to be done. */
}

// --  Facade API  --  --  --  --  --  --  --  --  --  --  --  --  --  -

void lcd_init(void)
{
    init_video_dma();
    init_clear_dma();
    init_pixslices();
    lcd_bl_init();
}

gfx_pixslice *lcd_alloc_pixslice(int x, int y, size_t w, size_t h)
{
    pixslice_impl *impl = NULL;
    while (!impl) {
        for (size_t i = 0; i < PIXSLICE_COUNT && !impl; i++) {
            WITH_INTERRUPTS_MASKED {
                if (pixslices[i].state == TS_CLEARED) {
                    impl = &pixslices[i];
                    impl->state = TS_DRAWING;
                }
            }
        }
    }
    gfx_init_pixslice(&impl->slice, impl->buffer, x, y, w, h, w);
    return &impl->slice;
}

void lcd_send_pixslice(gfx_pixslice *slice)
{
    pixslice_impl *impl = (pixslice_impl *)slice;
    bool busy = false;
    WITH_INTERRUPTS_MASKED {
        busy = video_dma_busy;
        if (busy) {
            impl->state = TS_SEND_WAIT;
        } else {
            video_dma_busy = true;
            impl->state = TS_SENDING;
        }
    }
    if (!busy)
        start_video_dma(impl);
}

void lcd_set_bg_color(gfx_rgb565 color, bool immediate)
{
    if (immediate) {
        bool needs_clearing[PIXSLICE_COUNT] = { false, false };
        WITH_INTERRUPTS_MASKED {
            bg_color = color;
            for (size_t i = 0; i < PIXSLICE_COUNT; i++) {
                if (pixslices[i].state == TS_CLEARED) {
                    pixslices[i].state = TS_DRAWING;
                    needs_clearing[i] = true;
                } else
                    needs_clearing[i] = false;
            }
        }
        for (size_t i = 0; i < PIXSLICE_COUNT; i++)
            if (needs_clearing[i])
                clear_pixslice(&pixslices[i].slice);
    } else {
        bg_color = color;
    }
}

gfx_rgb565 lcd_bg_color(void)
{
    return bg_color;
}

// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
