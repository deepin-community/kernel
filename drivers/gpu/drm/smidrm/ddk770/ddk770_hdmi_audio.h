#ifndef  _DDK770_HDMI_AUDIO_H_
#define  _DDK770_HDMI_AUDIO_H_

#include "ddk770_mode.h"

#define SAMPLE_RATE 48
#define SAMPLE_SIZE 24
#define WORD_LENGTH 24

typedef struct audio_n_computation {
	u32 pixel_clock;
	u32 n;
}audio_n_computation_t;

int audio_Initialize(hdmi_index index);
void audio_Configure(hdmi_index index, u32 pixel_clock, u32 samplefreq);
void audio_i2s_configure(hdmi_index index, int sampleSize);

void fc_audio_mute(hdmi_index index);
void fc_audio_unmute(hdmi_index index);

#endif