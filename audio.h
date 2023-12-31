#ifndef _AUDIO_H
#define _AUDIO_H

typedef enum {
  AUDIO__SUCCESS_CHIME = 0,
  AUDIO__FAILURE_CHIME,

  NUM_AUDIO_TYPES,
} audio_type_t;

void audio_open(void);
void audio_play(audio_type_t audio_type);
void audio_stop(void);

#endif

