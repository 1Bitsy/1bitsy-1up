#ifndef AUDIO_included
#define AUDIO_included

#include <stddef.h>
#include <stdint.h>


// Terminology:
//   A `sample` is a single 8 or 12 bit PCM value.
//   A `frame` is a set of samples that should be emitted at the same instant.
//      (one sample for mono, two samples for stereo).
//   The sample rate is actually the number of frames per second, not samples.
//
// 12 bit samples are right-aligned: they come from the least
// significant 12 bits of each 16 bit word.
//
// N.B., stereo mode causes less memory contention than mono.
// So stereo may allow a higher video frame rate.


typedef enum audio_channel_count {
    ACC_MONO   = 1,
    ACC_STEREO = 2,
} audio_channel_count;

typedef enum audio_sample_depth {
    ASD_8BIT   = 1,
    ASD_12BIT  = 2,
} audio_sample_depth;


// Audio callback fills buffer with new frames.
//
// Type of buffer and number of samples depends on how audio was
// initialized.
//
// Stereo channels are interleaved: even samples are left channel, odd
// samples are right channel.
//
//    sample_depth    channel_count    bytes_per_frame
//          8               1                 1
//          8               2                 2
//         12               1                 2
//         12               2                 4


typedef void audio_callback_fn(void *frames_out, size_t frame_count);

#define DEFINE_AUDIO_BUFFER(name, frames, channels, depth) \
    static uint8_t name[2 * (frames) * (channels) * (depth)] \
    __attribute__((section(".sram2")));

// Pass NULL for buffer to reuse existing buffer.
extern void                audio_init              (uint32_t sample_rate,
                                                    audio_channel_count,
                                                    audio_sample_depth,
                                                    void *buffer,
                                                    size_t byte_count);

extern uint32_t            audio_get_sample_rate   (void);
extern audio_channel_count audio_get_channel_count (void);
extern audio_sample_depth  audio_get_sample_depth  (void);
extern void               *audio_get_sample_buffer (void);
extern size_t              audio_get_frame_bytes   (void);
extern size_t              audio_get_frame_count   (void);
extern size_t              audio_get_byte_count    (void);

/* returns previous callback. */
extern audio_callback_fn  *audio_register_callback (audio_callback_fn *);

extern void                audio_start             (void);
extern void                audio_stop              (void);

#endif /* !AUDIO_included */
