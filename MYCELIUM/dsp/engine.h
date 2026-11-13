#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 void *(*create)(const char*);
 void (*destroy)(void*);
 void (*midi)(void*,const uint8_t*,int);
 void (*set_param)(void*,const char*,const char*);
 int (*get_param)(void*,const char*,char*,int);
 void (*render)(void*,int16_t*,int);
 void (*process)(void*,const int16_t*,int16_t*,int);
 void (*song_pos)(void*,double,double,int,int);
 /* Appended methods: preserve the original slots above. */
 int (*set_sample_rate)(void*,double);
 void (*process_float)(void*,const float*,float*,int);
 /* Optional v0.9 per-frame stems for oscilloscope: input, textures, reverb. */
 void (*process_float_taps)(void*,const float*,float*,float*,float*,float*,int);
} mpc_engine_t;
const mpc_engine_t* mpc_engine(void);
#ifdef __cplusplus
}
#endif
