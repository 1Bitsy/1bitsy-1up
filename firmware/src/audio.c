#include "audio.h"

#include <libopencm3/cm3/nvic.h>

#include <libopencm3/stm32/dac.h>
#ifndef DAC_SR                  // XXX DAC_SR is missing?
    #define DAC_SR MMIO32(DAC_BASE + 0x34)
    #define DAC_SR_DMAUDR2 (1 << 29)
    #define DAC_SR_DMAUDR1 (1 << 13)
#endif
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

// The Reference Manual says TIM6 and TIM7 are for the DACs.  We only
// need one, so we use TIM6.
//
//     20.1 TIM6&TIM7 introduction
//
//     They may be used as generic timers for time-base generation but
//     they are also specifically used to drive the digial-to-analog
//     converter (DAC).  In fact, the timers are internally connected
//     to the DAC and are able to drive it through their trigger outputs.
//
//     [Reference manual, Document RM0090]

static uint32_t            sample_rate;
static audio_channel_count channel_count;
static audio_sample_depth  sample_depth;
static void               *sample_buffer;
static size_t              buffer_bytes;

static audio_callback_fn  *registered_callback;


// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -


uint32_t audio_get_sample_rate(void)
{
    return sample_rate;
}

audio_channel_count audio_get_channel_count(void)
{
    return channel_count;
}

audio_sample_depth audio_get_sample_depth(void)
{
    return sample_depth;
}

void *audio_get_sample_buffer(void)
{
    return sample_buffer;
}

size_t audio_get_frame_bytes(void)
{
    return channel_count * sample_depth;
}

size_t audio_get_frame_count(void)
{
    return buffer_bytes / audio_get_frame_bytes();
}

size_t audio_get_byte_count(void)
{
    return buffer_bytes;
}


// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -


static void init_timer(void)
{
    // XXX This happens to work but doesn't take into account all
    // XXX the ways the clocks could be configured.
    // XXX RCC_DCKCFGR.TIMPRE description:
    // XXX
    // XXX     0: If the APB prescaler (PPRE1, PPRE2 in the RCC_CFGR
    // XXX        register) is configured to a division factor of 1,
    // XXX        TIMxCLK = PCLKx.  Otherwise, the timer clock
    // XXX        frequencies are set to twice the frequency of the
    // XXX        APB domain to which the timers are connected:
    // XXX        TIMxCLK = 2xPCLKx.
    // XXX     1: If the APB prescaler (PPRE1, PPRE2 in the RCC_CFGR
    // XXX        register) is configured to a division factor of 1,
    // XXX        2, or 4, TIMxCLK = HCLK.  Otherwise, the timer clock
    // XXX        frequencies are set to four times to the frequency
    // XXX        [sic] of the APB domain to which the timers are
    // XXX        connected: TIMxCLK = 4xPCLKx.

    uint32_t period = (2 * rcc_apb1_frequency + sample_rate / 2) / sample_rate;

    /* Enable TIM6 clock. */
    rcc_periph_clock_enable(RCC_TIM6);
    rcc_periph_reset_pulse(RST_TIM6);
    /* Timer global mode: - No divider, Alignment edge, Direction up */
    timer_set_mode(TIM6, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_disable_preload(TIM6);
    timer_continuous_mode(TIM6);
    timer_update_on_overflow(TIM6);
    timer_set_period(TIM6, period);
    timer_set_master_mode(TIM6, TIM_CR2_MMS_UPDATE);
}

static void start_timer(void)
{
    timer_enable_counter(TIM6);
}

static void stop_timer(void)
{
    timer_disable_counter(TIM6);
}


// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -


static void init_dma(void)
{
    uint32_t mono_msize = 0, mono_psize = 0;
    uint32_t stereo_msize = 0, stereo_psize = 0;
    uint32_t cr_msize = 0, cr_psize = 0;
    volatile void *dhr1a = 0, *dhr2a = 0, *dhrda = 0;

    if (sample_depth == ASD_8BIT) {
        mono_msize   = DMA_SxCR_MSIZE_8BIT;
        mono_psize   = DMA_SxCR_PSIZE_8BIT;
        stereo_msize = DMA_SxCR_MSIZE_16BIT;
        stereo_psize = DMA_SxCR_PSIZE_16BIT;
        dhr1a        = &DAC_DHR8R1;
        dhr2a        = &DAC_DHR8R2;
        dhrda        = &DAC_DHR8RD;
    } else if (sample_depth == ASD_12BIT) {
        mono_msize   = DMA_SxCR_MSIZE_16BIT;
        mono_psize   = DMA_SxCR_PSIZE_16BIT;
        stereo_msize = DMA_SxCR_MSIZE_32BIT;
        stereo_psize = DMA_SxCR_PSIZE_32BIT;
        dhr1a        = &DAC_DHR12R1;
        dhr2a        = &DAC_DHR12R2;
        dhrda        = &DAC_DHR12RD;
    }

    if (channel_count == ACC_STEREO) {
        dhr1a    = dhrda;
        cr_msize = stereo_msize;
        cr_psize = stereo_psize;
    } else if (channel_count == ACC_MONO) {
        cr_msize = mono_msize;
        cr_psize = mono_psize;
    }

    /* DAC channel 1 uses DMA controller 1 Stream 5 Channel 7. */
    /* Enable DMA1 clock and IRQ */
    rcc_periph_clock_enable(RCC_DMA1);

    /* Setup Stream5 Channel7 for DAC1 (mono) or both DACs (stereo) */
    nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
    dma_stream_reset(DMA1, DMA_STREAM5);
    dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_LOW);
    dma_set_memory_size(DMA1, DMA_STREAM5, cr_msize);
    dma_set_peripheral_size(DMA1, DMA_STREAM5, cr_psize);
    dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
    dma_enable_circular_mode(DMA1, DMA_STREAM5);
    dma_set_transfer_mode(DMA1, DMA_STREAM5, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t) dhr1a);
    dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t) sample_buffer);
    dma_set_number_of_data(DMA1, DMA_STREAM5, audio_get_frame_count());
    dma_enable_half_transfer_interrupt(DMA1, DMA_STREAM5);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
    dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_7);
    dma_enable_stream(DMA1, DMA_STREAM5);

    if (channel_count == ACC_MONO) {
        /* Setup Stream6 Channel7 for DAC2 */
        dma_stream_reset(DMA1, DMA_STREAM6);
        dma_set_priority(DMA1, DMA_STREAM6, DMA_SxCR_PL_LOW);
        dma_set_memory_size(DMA1, DMA_STREAM6, cr_msize);
        dma_set_peripheral_size(DMA1, DMA_STREAM6, cr_psize);
        dma_enable_memory_increment_mode(DMA1, DMA_STREAM6);
        dma_enable_circular_mode(DMA1, DMA_STREAM6);
        dma_set_transfer_mode(DMA1, DMA_STREAM6,
                              DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
        dma_set_peripheral_address(DMA1, DMA_STREAM6, (uint32_t) dhr2a);
        dma_set_memory_address(DMA1, DMA_STREAM6, (uint32_t)sample_buffer);
        dma_set_number_of_data(DMA1, DMA_STREAM6, audio_get_frame_count());
        dma_channel_select(DMA1, DMA_STREAM6, DMA_SxCR_CHSEL_7);
        dma_enable_stream(DMA1, DMA_STREAM6);
    }
}

static void stop_dma(void)
{
    dma_disable_stream(DMA1, DMA_STREAM5);
    dma_disable_stream(DMA1, DMA_STREAM6);
}

void dma1_stream5_isr(void)
{
    size_t half_frames = audio_get_frame_count() / 2;
    if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_HTIF)) {
        dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_HTIF);
        (*registered_callback)(sample_buffer, half_frames);
    }
    if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
        dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
        (*registered_callback)(sample_buffer + buffer_bytes / 2, half_frames);
    }
}


// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -


static void init_dac()
{
    uint32_t mamp1 = 0, mamp2 = 0;
    if (sample_depth == ASD_12BIT) {
        mamp1 = DAC_CR_MAMP1_1;
        mamp2 = DAC_CR_MAMP2_1;
    } else if (sample_depth == ASD_8BIT) {
        // Avoid quantization noise with a high masking noise.
        mamp1 = DAC_CR_MAMP1_5;
        mamp2 = DAC_CR_MAMP2_5;
    }

    /* Enable the DAC clock on APB1 */
    rcc_periph_clock_enable(RCC_DAC);

    /* Setup the DAC channel 1 with timer 6 as trigger source. */
    dac_trigger_enable(CHANNEL_1);
    dac_set_trigger_source(DAC_CR_TSEL1_T6);
    dac_dma_enable(CHANNEL_1);
    dac_enable(CHANNEL_1);
    dac_set_waveform_generation(DAC_CR_WAVE1_NOISE);
    dac_set_waveform_characteristics(mamp1);

    /* Setup the DAC channel 2 with timer 6 as trigger source. */
    dac_trigger_enable(CHANNEL_2);
    dac_set_trigger_source(DAC_CR_TSEL2_T6);
    dac_dma_enable(CHANNEL_2);
    dac_enable(CHANNEL_2);
    dac_set_waveform_generation(DAC_CR_WAVE2_NOISE);
    dac_set_waveform_characteristics(mamp2);
}

static void stop_dac(void)
{
    dac_load_data_buffer_dual(0, 0, RIGHT12);
    // Don't actually stop the DAC.  Leave it running but stop feeding
    // it new samples.  Otherwise it will probably click.
}


void audio_init(uint32_t            Fs,
                audio_channel_count nchan,
                audio_sample_depth  depth,
                void               *buffer,
                size_t              byte_count)
{
    audio_stop();

    sample_rate = Fs;
    channel_count = nchan;
    sample_depth = depth;
    if (buffer) {
        sample_buffer = buffer;
        buffer_bytes = byte_count;
    }
}

audio_callback_fn *audio_register_callback(audio_callback_fn *new)
{
    audio_callback_fn *prev = registered_callback;
    registered_callback = new;
    return prev;
}

void audio_start(void)
{
    init_timer();
    init_dma();
    init_dac();
    start_timer();
}

void audio_stop(void)
{
    stop_dma();
    stop_dac();
    stop_timer();
}
