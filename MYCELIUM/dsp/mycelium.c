/* MYCELIUM 0.9.8, native-float stereo audio engine. MIT licensed.
   Original work, not a clone or reverse engineering of any commercial pedal.
   In the Windows VST3, audio runs at the host sample rate with no PCM conversion.
*/
#include "engine.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846f
#ifndef MAX_GRAINS
#define MAX_GRAINS 32 /* Live 3 balanced profile, lower voice count than desktop */
#endif
#ifndef MAX_LOOPS
#define MAX_LOOPS 4  /* Live 3 balanced profile */
#endif
#define FDN 8
#define PARAMS 98
#define BUFFER_SECONDS 60
#define MAX_PHRASE_STEPS 8
#define MAX_ONSETS 128
#define CLIP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))

enum { BUFFER,DURATION,FREEZE,THRESHOLD,DECAY,REGEN,
 GRAIN,DENSITY,SPRAY,REVERSE,WIDTH,GRAIN_NOTE,GRAIN_VAR,GRAIN_QUANT,
 LOOP_LEVEL,LOOP_RATE,LOOP_LENGTH,LOOP_FEEDBACK,LOOP_REVERSE,LOOP_JITTER,LOOP_NOTE,LOOP_VAR,LOOP_QUANT,
 PITCH_MODE,ROOT,
 SOURCE_HP,SOURCE_LP,GRAIN_HP,GRAIN_LP,LOOP_HP,LOOP_LP,REV_HP,REV_LP,
 REV_MIX,REV_TIME,REV_SIZE,REV_PREDELAY,REV_DAMP,REV_WIDTH,
 MIX,WET_GAIN,TEMPO,SOFT_CLIP,
 POOL_ORIGINAL,POOL_OCT_UP,POOL_OCT_DOWN,POOL_FIFTH_UP,POOL_FIFTH_DOWN,
 GRAIN_POOL,GRAIN_ALGORITHM,LOOP_POOL,LOOP_ALGORITHM,
 CLEAN_SEND,TEXTURE_SEND,GRAIN_ADVENTURE,LOOP_ADVENTURE,PHRASE_STYLE,PHRASE_DENSITY,PHRASE_VARIATION,
 CHORUS_ON,CHORUS_RATE,CHORUS_DEPTH,CHORUS_MIX,FLANGER_ON,FLANGER_RATE,FLANGER_DEPTH,FLANGER_FEEDBACK,FLANGER_MIX,
 DRIVE_ON,DRIVE_AMOUNT,DRIVE_MIX,SMART_MEMORY,NEW_PHRASE_GAP,
 DRY_LEVEL,GRAIN_LEVEL,LOOP_MIX_LEVEL,REVERB_LEVEL,TEXTURE_SUSTAIN,REVERB_CHARACTER,
 INPUT_TAPE_MIX,TEXTURE_TAPE_MIX,TAPE_WOW,TAPE_FLUTTER,TAPE_AGE,TAPE_SATURATION,TAPE_TONE,TAPE_STEREO,TAPE_OUTPUT,PHRASE_BIAS,MASTER_LEVEL,TAPE_ENABLED,TIME_BEND,TIME_MODE,TIME_MOTION,TIME_RATE,NEW_NOTES_MODE,TRANSITION_TIME,TIME_DEPTH };

static const char* keys[PARAMS]={
 "buffer","duration","freeze","threshold","decay","regen",
 "grain","density","spray","reverse","width","grain_note","grain_var","grain_quant",
 "loop_level","loop_rate","loop_length","loop_feedback","loop_reverse","loop_jitter","loop_note","loop_var","loop_quant",
 "pitch_mode","root",
 "source_hp","source_lp","grain_hp","grain_lp","loop_hp","loop_lp","rev_hp","rev_lp",
 "rev_mix","rev_time","rev_size","rev_predelay","rev_damp","rev_width",
 "mix","wet_gain","tempo","soft_clip",
 "pool_original","pool_oct_up","pool_oct_down","pool_fifth_up","pool_fifth_down",
 "grain_pool","grain_algorithm","loop_pool","loop_algorithm",
 "clean_send","texture_send","grain_adventure","loop_adventure","phrase_style","phrase_density","phrase_variation",
 "chorus_on","chorus_rate","chorus_depth","chorus_mix","flanger_on","flanger_rate","flanger_depth","flanger_feedback","flanger_mix",
 "drive_on","drive_amount","drive_mix","smart_memory","new_phrase_gap",
 "dry_level","grain_level","loop_mix_level","reverb_level","texture_sustain","reverb_character",
 "input_tape_mix","texture_tape_mix","tape_wow","tape_flutter","tape_age","tape_saturation","tape_tone","tape_stereo","tape_output","phrase_bias","master_level","tape_enabled","time_bend","time_mode","time_motion","time_rate","new_notes_mode","transition_time","time_depth"};
static const float minv[PARAMS]={
 0.5f, 0.0f, 0.0f, 0.0001f, 0.0f, 0.0f, 80.0f, 0.5f,
 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
 0.0f, 20.0f, 500.0f, 20.0f, 500.0f, 20.0f, 500.0f, 20.0f,
 500.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
 0.0f, 40.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
 0.0f, 0.0f, 0.0f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f,
 0.02f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
 1.0f, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0.125f, 0, 0, 0.01f, 0, 0, 0
};
static const float maxv[PARAMS]={
 60.0f, 180.0f, 1.0f, 0.05f, 1.0f, 0.85f, 5000.0f, 32.0f,
 1.0f, 1.0f, 1.0f, 10.0f, 3.0f, 1.0f, 1.0f, 8.0f,
 8000.0f, 0.98f, 1.0f, 1.0f, 10.0f, 3.0f, 1.0f, 6.0f,
 11.0f, 1200.0f, 20000.0f, 2000.0f, 20000.0f, 2000.0f, 20000.0f, 1200.0f,
 20000.0f, 1.0f, 60.0f, 2.3f, 500.0f, 1.0f, 1.0f, 1.0f,
 2.0f, 240.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
 1.0f, 5.0f, 1.0f, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f,
 5.0f, 1.0f, 1.0f, 1.0f, 2.0f, 1.0f, 1.0f, 1.0f,
 3.0f, 1.0f, 0.85f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
 12.0f, 1.5f, 1.5f, 1.5f, 1.5f, 1, 3, 1,
 1, 1, 1, 1, 1, 1, 1, 2,
 1, 1.5f, 1, 2.0f, 5, 1, 4, 5, 30, 1
};
static const float defaults[PARAMS]={
 12.0f, 0.0f, 0.0f, 0.004f, 0.0f, 0.0f, 1800.0f, 6.0f,
 0.35f, 0.14f, 0.9f, 5.0f, 1.0f, 1.0f, 0.52f, 3.0f,
 1600.0f, 0.75f, 0.1f, 0.12f, 5.0f, 1.0f, 1.0f, 2.0f,
 0.0f, 20.0f, 20000.0f, 40.0f, 14000.0f, 30.0f, 14000.0f, 120.0f,
 11500.0f, 0.6f, 22.0f, 1.4f, 90.0f, 0.55f, 1.0f, 0.78f,
 0.85f, 110.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
 1.0f, 0.0f, 1.0f, 1.0f, 0.35f, 0.65f, 0.35f, 0.35f,
 0.0f, 0.75f, 0.55f, 0.0f, 0.22f, 0.55f, 0.4f, 0.0f,
 0.12f, 0.65f, 0.32f, 0.35f, 0.0f, 0.32f, 0.55f, 1.0f,
 2.5f, 1, .8f, .8f, 1, .75f, 2, 0,
 0, .18f, .15f, .12f, .2f, .75f, .65f, 1,
 .35f, 1, 1, 1.0f, 0, .25f, .15f, 0, 6, 1
};
static const int pitchOffsets[11]={-24,-19,-12,-7,-5,0,5,7,12,19,24};
static const int scaleMasks[7]={0,0xfff,0xab5,0x5ad,0x295,0x6ad,0x555};
static const float beatDivs[9]={4,2,1,.5f,.25f,.125f,.0625f,.75f,1.f/3.f};
static const float fdnTimes[FDN]={.053f,.067f,.079f,.097f,.113f,.131f,.151f,.173f};

/* Four independent, stereo, first-order HP and LP pairs. */
typedef struct {float lp[2],hpMemory[2],lpCoeff,hpCoeff;} Band;
typedef struct {
 float pos,step,pan,gain; float playAge,timeAnchor,timeLife; size_t regionStart, regionLen; int age,len,active;
} Grain;
typedef struct {
 float pos,step,pan,volume,feedback;float playAge,timeAnchor,timeLife;
 size_t regionStart,regionLen;
 int age,len,active;
 int phraseCount,phraseOffsets[MAX_PHRASE_STEPS],phraseMode;
} MicroLoop;
/* Each engine has an independent note-selection sequence. No allocations in audio thread. */
typedef struct {
 int mask, cursor, direction, last, bag[5], bagSize, bagPos;
} PitchSequence;
typedef struct {
 double sr, bpm, ppq;
 int playing,freeze,inputSeen,silenceCount;
 uint64_t quietCaptureSamples;
 uint64_t samplesSinceInput;
 uint64_t memoryRefreshes;
 float p[PARAMS];
 float *capture;size_t capacity,write,filled,freezeStart,freezeLen;
 float *preL,*preR;size_t preCapacity,preWrite;
 float *tape[4];size_t tapeCapacity,tapeWrite;
 float tapeToneState[2][2], tapeDC[2][2];
 double tapeWowPhase,tapeFlutterPhase;
 uint64_t captureSamples,onsets[MAX_ONSETS];int onsetCount,onsetWrite;
 float transientEnvelope;uint64_t lastOnsetSample;
 float *fdn[FDN];size_t fdnCapacity,fdnWrite;
 float fdnDamp[FDN],fdnGain[FDN],fdnRead[FDN];
 float currentSize,currentReverbMix,currentMix,currentWetGain,currentTimeBend,textureReady;
 double timeMotionPhase; uint32_t timeVoiceCounter;
 float timeRestartAge; /* wall-clock samples since latest phrase-start reset */
 int reverbCounter;
 Band sourceFilter,grainFilter,loopFilter,reverbFilter;
 Grain grains[MAX_GRAINS];MicroLoop loops[MAX_LOOPS];
 PitchSequence grainPitchSequence,loopPitchSequence;
 float grainClock,loopClock,inputMeter,outputMeter;
 int activeGrains,activeLoops;uint32_t rng;
} State;

static float limit(float v,float lo,float hi){return CLIP(v,lo,hi);}
static size_t wrapindex(long long i,size_t n){long long z=i%(long long)n;return (size_t)(z<0?z+(long long)n:z);}
static int mod12(int n){int k=n%12;return k<0?k+12:k;}
static float rand01(State*s){uint32_t x=s->rng;x^=x<<13;x^=x>>17;x^=x<<5;s->rng=x;return (x&0xffffff)/16777216.f;}
static float semisRatio(float s){return exp2f(s/12.f);}
static float quantize(float semis,int mode,int root){
 if(mode==0)return semis;
 int mask=scaleMasks[CLIP(mode,0,6)];int c=(int)roundf(semis),best=c;
 float bestDistance=1e9f;
 for(int i=c-10;i<=c+10;i++)if(mask&(1<<mod12(i-root))){float d=fabsf(semis-i);if(d<bestDistance){bestDistance=d;best=i;}}
 return (float)best;
}
static float pitchFor(State*s,int baseParam,int varParam,int quantParam){
 float offset=(float)pitchOffsets[CLIP((int)roundf(s->p[baseParam]),0,10)];
 int variation=CLIP((int)roundf(s->p[varParam]),0,3);
 float chance=rand01(s);
 if(variation>0&&chance>.55f){
  float direction=rand01(s)<.5f?-1.f:1.f;
  if(variation==1)offset+=direction*7.f;               /* fifth */
  else if(variation==2)offset+=direction*12.f;          /* octave */
  else offset+=direction*(rand01(s)<.5f?7.f:12.f); /* either */
 }
 if(s->p[quantParam]>=.5f)offset=quantize(offset,(int)roundf(s->p[PITCH_MODE]),(int)roundf(s->p[ROOT]));
 return limit(offset,-36,36);
}
/* PITCH POOL: only the five user-selected intervals are used.
   Sorted by pitch so a cycle/wander is easy to hear: -12, -7, 0, +7, +12.
   Scale quantization intentionally does not affect these fixed perfect intervals. */
static const int poolSemitones[5]={-12,-7,0,7,12};
static const int poolToggleParameters[5]={POOL_OCT_DOWN,POOL_FIFTH_DOWN,POOL_ORIGINAL,POOL_FIFTH_UP,POOL_OCT_UP};
static int activePitchMask(State*s){
 int mask=0;
 for(int i=0;i<5;i++)if(s->p[poolToggleParameters[i]]>=.5f)mask|=1<<i;
 return mask?mask:1<<2; /* safe fallback to original if all switches are off */
}
static int selectPoolPitch(State*s,PitchSequence*seq,int algo){
 int mask=activePitchMask(s);
 if(seq->mask!=mask){
  seq->mask=mask;seq->cursor=0;seq->direction=1;seq->last=-1;
  seq->bagSize=0;seq->bagPos=0;
 }
 int notes[5],n=0;
 for(int i=0;i<5;i++)if(mask&(1<<i))notes[n++]=i;
 int picked=notes[0];
 switch(CLIP(algo,0,5)){
  case 0: /* any enabled note, uniformly random */
   picked=notes[CLIP((int)(rand01(s)*n),0,n-1)];break;
  case 1: /* round robin through all enabled notes */
   if(seq->cursor>=n)seq->cursor=0;
   picked=notes[seq->cursor];seq->cursor=(seq->cursor+1)%n;break;
  case 2: /* ping-pong in pitch order, without doubling end notes */
   if(seq->cursor>=n)seq->cursor=0;
   picked=notes[seq->cursor];
   if(n>1){
    seq->cursor+=seq->direction;
    if(seq->cursor>=n){seq->cursor=n-2;seq->direction=-1;}
    if(seq->cursor<0){seq->cursor=1;seq->direction=1;}
   }break;
  case 3: /* Fisher-Yates bag; every enabled pitch appears once per bag */
   if(seq->bagPos>=seq->bagSize){
    seq->bagSize=n;seq->bagPos=0;
    for(int i=0;i<n;i++)seq->bag[i]=notes[i];
    for(int i=n-1;i>0;i--){
     int j=CLIP((int)(rand01(s)*(i+1)),0,i);
     int t=seq->bag[i];seq->bag[i]=seq->bag[j];seq->bag[j]=t;
    }
   }
   picked=seq->bag[seq->bagPos++];break;
  case 4: { /* original pitch approximately 55%, other enabled notes equally */
   if(mask&(1<<2)){
    if(n==1||rand01(s)<.55f){picked=2;break;}
    int other[4],count=0;
    for(int i=0;i<n;i++)if(notes[i]!=2)other[count++]=notes[i];
    picked=other[CLIP((int)(rand01(s)*count),0,count-1)];
   }else picked=notes[CLIP((int)(rand01(s)*n),0,n-1)];
   break;
  }
  case 5: { /* short pitch-to-pitch steps with an occasional held note */
   int index=0;
   if(seq->last>=0){
    for(int i=0;i<n;i++)if(notes[i]==seq->last){index=i;break;}
   } else { /* first event starts at the original if it is enabled */
    for(int i=0;i<n;i++)if(notes[i]==2){index=i;break;}
   }
   float r=rand01(s);
   if(r>.23f&&n>1){
    int step=r<.615f?-1:1;
    index=CLIP(index+step,0,n-1);
   }
   picked=notes[index];break;
  }
 }
 seq->last=picked;
 return poolSemitones[picked];
}
static void setBand(Band *b,float highpass,float lowpass,double rate){
 highpass=limit(highpass,15,(float)rate*.40f);
 lowpass=limit(lowpass,200,(float)rate*.47f);
 b->lpCoeff=expf(-2*PI*lowpass/(float)rate);
 b->hpCoeff=expf(-2*PI*highpass/(float)rate);
}
static void updateFilters(State*s){
 setBand(&s->sourceFilter,s->p[SOURCE_HP],s->p[SOURCE_LP],s->sr);
 setBand(&s->grainFilter,s->p[GRAIN_HP],s->p[GRAIN_LP],s->sr);
 setBand(&s->loopFilter,s->p[LOOP_HP],s->p[LOOP_LP],s->sr);
 setBand(&s->reverbFilter,s->p[REV_HP],s->p[REV_LP],s->sr);
}
static float bandSample(Band*b,int ch,float x){
 float low=(1.f-b->lpCoeff)*x+b->lpCoeff*b->lp[ch];b->lp[ch]=low;
 float belowHP=(1.f-b->hpCoeff)*low+b->hpCoeff*b->hpMemory[ch];b->hpMemory[ch]=belowHP;
 return low-belowHP;
}
static void resetVoices(State*s){memset(s->grains,0,sizeof s->grains);memset(s->loops,0,sizeof s->loops);s->grainClock=0;s->loopClock=0;}
static void freeBuffers(State*s){
 free(s->capture);s->capture=NULL;free(s->preL);free(s->preR);s->preL=s->preR=NULL;
 for(int ch=0;ch<4;ch++){free(s->tape[ch]);s->tape[ch]=NULL;}
 for(int i=0;i<FDN;i++){free(s->fdn[i]);s->fdn[i]=NULL;}
}
static int configureRate(void *ptr,double sampleRate){
 State*s=(State*)ptr;if(!s)return 0;
 sampleRate=CLIP(sampleRate,22050.,192000.);
 size_t samples=(size_t)(sampleRate*BUFFER_SECONDS)+16;
 size_t pre=(size_t)(sampleRate*.60)+16;
 /* Long Ambient reflections at max Room Size need up to 0.557 s.
    Keep at least 0.75 s so the feedback read index can NEVER go negative. */
 size_t delay=(size_t)(sampleRate*.75)+32;
 float *capture=(float*)calloc(samples*2,sizeof(float));
 float *preL=(float*)calloc(pre,sizeof(float)),*preR=(float*)calloc(pre,sizeof(float));
 float *fdn[FDN]={0};
 float *tape[4]={0};
 size_t tapeCapacity=(size_t)(sampleRate*.15)+24;
 for(int ch=0;ch<4;ch++)tape[ch]=(float*)calloc(tapeCapacity,sizeof(float));
 for(int i=0;i<FDN;i++)fdn[i]=(float*)calloc(delay,sizeof(float));
 int good=capture&&preL&&preR&&tape[0]&&tape[1]&&tape[2]&&tape[3];
 for(int i=0;i<FDN;i++)if(!fdn[i])good=0;
 if(!good){free(capture);free(preL);free(preR);for(int ch=0;ch<4;ch++)free(tape[ch]);for(int i=0;i<FDN;i++)free(fdn[i]);return 0;}
 freeBuffers(s);s->capture=capture;s->preL=preL;s->preR=preR;
 for(int i=0;i<FDN;i++)s->fdn[i]=fdn[i];
 for(int ch=0;ch<4;ch++)s->tape[ch]=tape[ch];
 s->tapeCapacity=tapeCapacity;s->tapeWrite=0;s->tapeWowPhase=0;s->tapeFlutterPhase=0;
 memset(s->tapeToneState,0,sizeof s->tapeToneState);memset(s->tapeDC,0,sizeof s->tapeDC);
 s->captureSamples=0;s->onsetCount=0;s->onsetWrite=0;s->transientEnvelope=0;s->lastOnsetSample=0;
 s->sr=sampleRate;s->capacity=samples;s->preCapacity=pre;s->fdnCapacity=delay;
 s->write=s->filled=s->freezeStart=s->freezeLen=s->preWrite=s->fdnWrite=0;
 s->freeze=0;s->inputSeen=0;s->silenceCount=0;s->samplesSinceInput=0;s->quietCaptureSamples=0;s->memoryRefreshes=0;
 s->currentSize=s->p[REV_SIZE];s->currentReverbMix=s->p[REV_MIX];
 s->currentMix=s->p[MIX];s->currentWetGain=s->p[WET_GAIN];
 s->currentTimeBend=s->p[TIME_BEND];s->timeMotionPhase=0;s->timeVoiceCounter=0;s->timeRestartAge=1e12f;s->textureReady=0.f;s->reverbCounter=0;
 s->inputMeter=s->outputMeter=0;
 memset(&s->sourceFilter,0,sizeof(Band));memset(&s->grainFilter,0,sizeof(Band));
 memset(&s->loopFilter,0,sizeof(Band));memset(&s->reverbFilter,0,sizeof(Band));
 memset(s->fdnDamp,0,sizeof(s->fdnDamp));memset(s->fdnGain,0,sizeof(s->fdnGain));
 memset(&s->grainPitchSequence,0,sizeof(s->grainPitchSequence));
 memset(&s->loopPitchSequence,0,sizeof(s->loopPitchSequence));
 resetVoices(s);updateFilters(s);return 1;
}
static void* create(const char *unused){
 (void)unused;State*s=(State*)calloc(1,sizeof(State));if(!s)return NULL;
 memcpy(s->p,defaults,sizeof defaults);s->rng=0x739ba31fu;s->bpm=110.;
 if(!configureRate(s,44100.)){free(s);return NULL;}return s;
}
static void destroy(void *ptr){State*s=(State*)ptr;if(!s)return;freeBuffers(s);free(s);}
static void midi(void*ptr,const uint8_t*msg,int length){(void)ptr;(void)msg;(void)length;}
static void freezeNow(State*s){
 size_t wanted=(size_t)(s->p[BUFFER]*s->sr);
 if(wanted>s->filled)wanted=s->filled;
 s->freezeLen=wanted;s->freezeStart=wrapindex((long long)s->write-(long long)wanted,s->capacity);
 /* Capture stops writing until unfreeze, so this immutable ring segment IS the snapshot.
    No huge memcpy and no audio-thread allocation or pause. */
}
static void setParam(void*ptr,const char*key,const char*value){
 State*s=(State*)ptr;if(!s||!key||!value)return;
 for(int i=0;i<PARAMS;i++)if(strcmp(key,keys[i])==0){
  char*end=NULL;float v=strtof(value,&end);if(end==value||!isfinite(v))return;
  v=limit(v,minv[i],maxv[i]);
  if(i==FREEZE){int freeze=v>=.5f;if(freeze&&!s->freeze)freezeNow(s);s->freeze=freeze;s->p[i]=freeze?1.f:0.f;}
  else s->p[i]=v;
  if(i>=SOURCE_HP&&i<=REV_LP)updateFilters(s);
  return;
 }
}
/* Official SDK bridge: direct indexed parameter writes, with no string operations.
   Called on audio thread at block boundaries ONLY, to preserve the single-thread
   ownership of the non-atomic DSP state. All exposed values are clamped. */
static void mycelium_set_by_index(void *ptr,int index,float v){
 State *s=(State*)ptr;
 if(!s||index<0||index>=PARAMS||!isfinite(v))return;
 v=limit(v,minv[index],maxv[index]);
 if(index==FREEZE){
   int f=v>=.5f;
   if(f&&!s->freeze)freezeNow(s);
   s->freeze=f;
   s->p[index]=f?1.f:0.f;
 }else s->p[index]=v;
 if(index>=SOURCE_HP&&index<=REV_LP)updateFilters(s);
}
/* Reset on transport resume or state load. No allocation, I/O or locks.
   The captured audio itself is logically cleared without touching its 21 MB
   ring; feedback buffers are cleared to avoid hearing stale reverb. */
static void mycelium_reset_realtime(void *ptr){
 State*s=(State*)ptr;
 if(!s)return;
 s->write=0;s->filled=0;s->freezeStart=0;s->freezeLen=0;
 s->captureSamples=0;s->onsetCount=0;s->onsetWrite=0;
 s->inputSeen=0;s->silenceCount=0;s->samplesSinceInput=0;s->quietCaptureSamples=0;
 s->lastOnsetSample=0;s->transientEnvelope=0;
 s->grainClock=0;s->loopClock=0;s->reverbCounter=0;
 s->preWrite=0;s->fdnWrite=0;s->tapeWrite=0;
 s->textureReady=0.f;s->currentMix=s->p[MIX];s->currentWetGain=s->p[WET_GAIN];
 s->currentTimeBend=s->p[TIME_BEND];s->timeRestartAge=1e12f;
 s->inputMeter=0;s->outputMeter=0;
 memset(&s->sourceFilter.lp,0,sizeof(s->sourceFilter.lp));
 memset(&s->sourceFilter.hpMemory,0,sizeof(s->sourceFilter.hpMemory));
 memset(s->grainFilter.lp,0,sizeof(s->grainFilter.lp));
 memset(s->grainFilter.hpMemory,0,sizeof(s->grainFilter.hpMemory));
 memset(s->loopFilter.lp,0,sizeof(s->loopFilter.lp));
 memset(s->loopFilter.hpMemory,0,sizeof(s->loopFilter.hpMemory));
 memset(s->reverbFilter.lp,0,sizeof(s->reverbFilter.lp));
 memset(s->reverbFilter.hpMemory,0,sizeof(s->reverbFilter.hpMemory));
 memset(s->fdnDamp,0,sizeof(s->fdnDamp));
 memset(s->tapeToneState,0,sizeof(s->tapeToneState));
 memset(s->tapeDC,0,sizeof(s->tapeDC));
 memset(&s->grainPitchSequence,0,sizeof(s->grainPitchSequence));
 memset(&s->loopPitchSequence,0,sizeof(s->loopPitchSequence));
 for(int i=0;i<FDN;i++)if(s->fdn[i])memset(s->fdn[i],0,s->fdnCapacity*sizeof(float));
 for(int i=0;i<4;i++)if(s->tape[i])memset(s->tape[i],0,s->tapeCapacity*sizeof(float));
 if(s->preL)memset(s->preL,0,s->preCapacity*sizeof(float));
 if(s->preR)memset(s->preR,0,s->preCapacity*sizeof(float));
 resetVoices(s);
}
static int getParam(void*ptr,const char*key,char*dest,int n){
 State*s=(State*)ptr;if(!s||!key||!dest||n<2)return 0;
 if(strcmp(key,"meter_in")==0)return snprintf(dest,n,"%.3f",s->inputMeter);
 if(strcmp(key,"meter_out")==0)return snprintf(dest,n,"%.3f",s->outputMeter);
 if(strcmp(key,"meter_grains")==0)return snprintf(dest,n,"%d",s->activeGrains);
 if(strcmp(key,"meter_loops")==0)return snprintf(dest,n,"%d",s->activeLoops);
 if(strcmp(key,"meter_refreshes")==0)return snprintf(dest,n,"%llu",(unsigned long long)s->memoryRefreshes);
 if(strcmp(key,"meter_capture")==0)return snprintf(dest,n,"%.2f",(s->freeze?s->freezeLen:s->filled)/s->sr);
 for(int i=0;i<PARAMS;i++)if(!strcmp(key,keys[i]))return snprintf(dest,n,"%.4f",s->p[i]);
 return 0;
}
static void sourceRange(State*s,size_t*start,size_t*n){
 if(s->freeze){*start=s->freezeStart;*n=s->freezeLen;return;}
 *n=(size_t)(s->p[BUFFER]*s->sr);if(*n>s->filled)*n=s->filled;
 *start=wrapindex((long long)s->write-(long long)*n,s->capacity);
}
static float readVoice(State*s,size_t start,size_t len,float pos,int ch){
 if(len<2)return 0;
 float p=fmodf(pos,(float)len);if(p<0)p+=(float)len;
 size_t a=(size_t)p;float t=p-(float)a;
 size_t ix=wrapindex((long long)start+(long long)a,s->capacity);
 size_t iy=wrapindex((long long)start+(long long)((a+1)%len),s->capacity);
 return s->capture[ix*2+ch]*(1.f-t)+s->capture[iy*2+ch]*t;
}
/* Collect genuine onset candidates (energy rising faster than the local envelope).
   It is only a rhythm/slice cue. This is NOT automatic musical note recognition. */
static void trackOnset(State*s,float inputLevel){
 float env=s->transientEnvelope;
 float fast=0.07f,slow=0.0010f;
 s->transientEnvelope+=((inputLevel>env)?fast:slow)*(inputLevel-env);
 uint64_t minimum=(uint64_t)(s->sr*.085);
 if(inputLevel>fmaxf(s->p[THRESHOLD]*3.f,.008f) && inputLevel>env*1.85f &&
    s->captureSamples>s->lastOnsetSample+minimum){
  s->onsets[s->onsetWrite]=s->captureSamples;
  s->onsetWrite=(s->onsetWrite+1)%MAX_ONSETS;
  if(s->onsetCount<MAX_ONSETS)s->onsetCount++;
  s->lastOnsetSample=s->captureSamples;
 }
}
/* Offset inside the selected memory. Prefer the nearest detected note attack;
   fall back to the requested position for sustained chords or drones. */
static int nearestOnset(State*s,size_t n,int wanted){
 int best=CLIP(wanted,0,(int)n-1),distance=(int)n;
 for(int i=0;i<s->onsetCount;i++){
  uint64_t t=s->onsets[i],old=s->captureSamples>=t?s->captureSamples-t:UINT64_MAX;
  if(old>n)continue;
  int off=(int)n-(int)old;
  int d=abs(off-wanted);
  if(d<distance){distance=d;best=CLIP(off,0,(int)n-1);}
 }
 return distance<(int)(s->sr*.33)?best:CLIP(wanted,0,(int)n-1);
}
static void makePhrase(State*s,MicroLoop*l,size_t n){
 float adventure=s->p[LOOP_ADVENTURE],v=s->p[PHRASE_VARIATION]*adventure*(1.f-s->p[PHRASE_BIAS]*.5f);
 int style=CLIP((int)roundf(s->p[PHRASE_STYLE]),0,5);
 int base=CLIP((int)n-l->len,0,(int)n-1);
 /* Very low Adventure must ALWAYS give a faithfully repeated contiguous phrase. */
 l->phraseCount=1;l->phraseMode=0;l->phraseOffsets[0]=base;
 if(style==0||adventure<=.015f||rand01(s)>adventure)return;
 l->phraseMode=style;
 int count=style==2?4:8;
 l->phraseCount=count;
 int segment=CLIP(l->len/count,1,l->len);
 for(int i=0;i<count;i++){
  int natural=CLIP(base+i*segment,0,(int)n-1);
  float r=rand01(s),old=rand01(s);
  int target=natural;
  if(style==1){/* Little Pieces: separate little note-like fragments */
   target=(int)(old*(float)CLIP((int)n-segment,1,(int)n));
  } else if(style==2){/* Call / response: first half recognizable, second half replies */
   if(i>=2)target=(int)(old*(float)CLIP((int)n-segment,1,(int)n));
  } else if(style==3){/* Stepping Stones: spaced selections, rests */
   target=(int)((float)(i+1)/(float)(count+1)*((int)n-segment));
  } else if(style==4){/* Rolling Waves: sweep forward and back through the memory */
   float progress=(i<=count/2)?(float)i/(count/2):(float)(count-i)/(count/2);
   target=(int)(progress*(float)CLIP((int)n-segment,1,(int)n));
  } else { /* Happy Accidents */
   target=(int)(old*(float)CLIP((int)n-segment,1,(int)n));
  }
  if(r<v)target=nearestOnset(s,n,target);
  else target=(int)((1.f-v)*natural+v*target);
  l->phraseOffsets[i]=CLIP(target,0,(int)n-1);
  int restAllowed=(style==3||style==5);
  if(restAllowed&&i>0 && rand01(s)<(1.f-s->p[PHRASE_DENSITY])*.85f*adventure)
   l->phraseOffsets[i]=-1;
 }
}
/* A newly opened buffer must NEVER be mistaken for a microscopic oscillator.
   In v0.9.5 a 1.8-second grain could repeatedly wrap around just 128 new
   samples, producing a loud and rather synthetic note after Smart Memory reset.
   Each new grain now reads a finite, continuous passage, never a tiny looping
   segment. We shorten the grain until sufficient source is available. */
/* Each newly recorded piece gets its own time relationship. Existing
   pieces keep their own anchor, allowing new musical material to sit on top. */
static float newTimeAnchor(State*s){
 int mode=(int)roundf(s->p[NEW_NOTES_MODE]);
 uint32_t n=s->timeVoiceCounter++;
 if(mode==1)return 1.f;                 /* join existing speed */
 if(mode==5)return (n&1u)?1.f:fmaxf(s->currentTimeBend,.125f);
 if(mode==3)return fmaxf(s->currentTimeBend,.125f);
 if(mode==4)return fmaxf(s->currentTimeBend,.125f); /* phrase-start ramp, not every grain */
 return fmaxf(s->currentTimeBend,.125f); /* fresh or slowly melt */
}
static float timeRatio(State*s,float anchor,float life){
 float base=limit(s->currentTimeBend/fmaxf(anchor,.125f),.025f,8.f);
 if((int)roundf(s->p[NEW_NOTES_MODE])==4){
  float seconds=s->p[TRANSITION_TIME];
  float ramp=seconds<.001f?1.f:limit(s->timeRestartAge/(float)(s->sr*seconds),0.f,1.f);
  base=1.f+(base-1.f)*ramp;
 }
 int mode=(int)roundf(s->p[TIME_MODE]);
 float depth=s->p[TIME_DEPTH], motion=s->p[TIME_MOTION];
 if(mode==2){ /* progressive cubic ease towards requested rate */
    float t=limit(life/(float)(s->sr*.7),0,1);base=1.f+(base-1.f)*t*t*(3.f-2.f*t);
 } else if(mode==3){
    base*=fmaxf(.15f,1.f+motion*.75f*sinf(2.f*PI*(float)s->timeMotionPhase));
 } else if(mode==4){
    float p=(float)s->timeMotionPhase;
    base*=fmaxf(.15f,1.f+motion*(.37f*sinf(2.f*PI*p)+.23f*sinf(2.f*PI*p*.37f)));
 } else if(mode==5){base=.015f;} /* nearly stopped, no divide by zero */
 return limit(1.f+(base-1.f)*depth,.015f,8.f);
}
static void spawnGrain(State*s,size_t start,size_t n){
 if(n<(size_t)(s->sr*.18))return; /* capture at least 180 ms of fresh audio */
 Grain*g=NULL;for(int i=0;i<MAX_GRAINS;i++)if(!s->grains[i].active){g=&s->grains[i];break;}
 if(!g)return;
 float a=s->p[GRAIN_ADVENTURE];
 float semitones=0;
 if(rand01(s)<powf(a,1.3f)){
  semitones=s->p[GRAIN_POOL]>=.5f
   ?(float)selectPoolPitch(s,&s->grainPitchSequence,(int)roundf(s->p[GRAIN_ALGORITHM]))
   :pitchFor(s,GRAIN_NOTE,GRAIN_VAR,GRAIN_QUANT);
 }
 float step=semisRatio(semitones)*(rand01(s)<s->p[REVERSE]*a?-1.f:1.f);
 int requested=(int)(s->p[GRAIN]*.001f*s->sr);
 requested=CLIP(requested,128,(int)(s->sr*5.0));
 /* Reserve two interpolation samples. A grain may not wrap around the
    captured phrase, even with high pitch shift or reverse playback. */
 const float speed=fmaxf(fabsf(step),.01f);
 int available=(int)floorf(((float)n-3.f)/speed);
 if(available<64)return;
 int length=CLIP(requested,64,available);
 float travel=(float)(length-1)*speed;
 float room=fmaxf(0.f,(float)n-2.f-travel);
 float startPos=room*(1.f-rand01(s)*s->p[SPRAY]*a);
 memset(g,0,sizeof(*g));
 g->active=1;g->regionStart=start;g->regionLen=n;
 g->len=length;g->step=step;g->timeAnchor=newTimeAnchor(s);g->playAge=0.f;g->timeLife=0.f;
 g->pos=(step<0.f?startPos+travel:startPos);
 g->pan=(rand01(s)*2.f-1.f)*s->p[WIDTH]*a;g->gain=.72f;
 if((int)roundf(s->p[NEW_NOTES_MODE])==3){
   for(int k=0;k<MAX_GRAINS;k++)if(!s->grains[k].active){
    Grain*copy=&s->grains[k];*copy=*g;copy->timeAnchor=1.f;
    copy->gain*=.5f;g->gain*=.5f;break;
   }
 }
}
static void spawnLoop(State*s,size_t start,size_t n){
 if(n<128||s->p[LOOP_LEVEL]<.001f)return;
 MicroLoop*l=NULL;
 for(int i=0;i<MAX_LOOPS;i++)if(!s->loops[i].active){l=&s->loops[i];break;}
 if(!l)l=&s->loops[(int)(rand01(s)*MAX_LOOPS)%MAX_LOOPS];
 memset(l,0,sizeof *l);l->active=1;
 l->len=(int)(s->p[LOOP_LENGTH]*.001f*s->sr);l->len=CLIP(l->len,128,(int)(s->sr*8));
 /* Whole Phrase uses the entire selected memory, including musical timing. */
 if((int)roundf(s->p[PHRASE_STYLE])==0)l->len=CLIP((int)n,128,(int)(s->sr*BUFFER_SECONDS));
 l->regionLen=n;l->regionStart=start;l->pos=0;
 float a=s->p[LOOP_ADVENTURE],pitch=0;
 if(rand01(s)<powf(a,1.3f)){
  pitch=s->p[LOOP_POOL]>=.5f
   ?(float)selectPoolPitch(s,&s->loopPitchSequence,(int)roundf(s->p[LOOP_ALGORITHM]))
   :pitchFor(s,LOOP_NOTE,LOOP_VAR,LOOP_QUANT);
 }
 l->step=semisRatio(pitch)*(rand01(s)<s->p[LOOP_REVERSE]*a?-1.f:1.f);
 l->feedback=s->p[LOOP_FEEDBACK];l->volume=.7f;
 l->timeAnchor=newTimeAnchor(s);l->playAge=0.f;l->timeLife=0.f;
 l->pan=(rand01(s)*2-1)*s->p[WIDTH]*a;
 makePhrase(s,l,n);
 if((int)roundf(s->p[NEW_NOTES_MODE])==3){
   for(int k=0;k<MAX_LOOPS;k++)if(!s->loops[k].active){
    MicroLoop*copy=&s->loops[k];*copy=*l;copy->timeAnchor=1.f;
    copy->volume*=.5f;l->volume*=.5f;break;
   }
 }
}
/* Dual-tap tape machine: input and texture have separate wet/dry controls.
   This is an ORIGINAL tape-inspired effect, not modeled from a physical unit.
   The audio stays float; no allocations in the real-time callback. */
static float readDelay(const float*ring,size_t capacity,size_t write,float delay){
 float at=(float)write-delay;
 while(at<0)at+=(float)capacity;
 while(at>=(float)capacity)at-=(float)capacity;
 size_t i=(size_t)at,j=(i+1)%capacity;
 return ring[i]*(1.f-(at-i))+ring[j]*(at-i);
}
static void tapePath(State*s,int path,float *l,float *r,float mix){
 float dry[2]={*l,*r};
 if(mix<.0001f)return;
 float wow=sinf((float)s->tapeWowPhase),flutter=sinf((float)s->tapeFlutterPhase);
 float age=s->p[TAPE_AGE],warmth=s->p[TAPE_SATURATION],tone=s->p[TAPE_TONE];
 float drift=(5.f+11.f*s->p[TAPE_WOW])*wow + (1.5f+5.5f*s->p[TAPE_FLUTTER])*flutter;
 /* Tape transport is read from a delayed recording; offset never goes negative. */
 float delay=(float)(s->sr*.010)+drift*(float)s->sr*.00009f;
 delay=limit(delay,4.f,(float)s->tapeCapacity-3.f);
 float wet[2];
 for(int ch=0;ch<2;ch++){
  float flutterDelay=delay+(ch?2.f:-2.f)*s->p[TAPE_STEREO];
  float moving=readDelay(s->tape[path*2+ch],s->tapeCapacity,s->tapeWrite,flutterDelay);
  float gain=1.f+3.5f*warmth;
  float saturated=tanhf(moving*gain)/fmaxf(tanhf(gain),.001f);
  float cutoff=1500.f+14500.f*tone*(1.f-.6f*age);
  float alpha=1.f-expf(-2.f*PI*cutoff/(float)s->sr);
  s->tapeToneState[path][ch]+=(saturated-s->tapeToneState[path][ch])*alpha;
  /* Very quiet stochastic wear, scaled by input to avoid an endless noise floor. */
  float wear=(rand01(s)-.5f)*age*.004f*fminf(1.f,fabsf(dry[ch])*5.f);
  wet[ch]=(s->tapeToneState[path][ch]+wear)*s->p[TAPE_OUTPUT];
 }
 *l=dry[0]+(wet[0]-dry[0])*mix;
 *r=dry[1]+(wet[1]-dry[1])*mix;
}
static void writeTape(State*s,int path,float left,float right){
 s->tape[path*2][s->tapeWrite]=left;
 s->tape[path*2+1][s->tapeWrite]=right;
}
static void advanceTape(State*s){
 s->tapeWrite=(s->tapeWrite+1)%s->tapeCapacity;
 s->tapeWowPhase+=2.0*PI*(.10+.75*s->p[TAPE_WOW])/s->sr;
 s->tapeFlutterPhase+=2.0*PI*(2.1+8.3*s->p[TAPE_FLUTTER])/s->sr;
 if(s->tapeWowPhase>2.0*PI)s->tapeWowPhase-=2.0*PI;
 if(s->tapeFlutterPhase>2.0*PI)s->tapeFlutterPhase-=2.0*PI;
}
static void runReverb(State*s,float inL,float inR,float*outL,float*outR){
 s->currentSize+=(s->p[REV_SIZE]-s->currentSize)*.00012f;
 s->currentReverbMix+=(s->p[REV_MIX]-s->currentReverbMix)*.0003f;
 int character=(int)roundf(s->p[REVERB_CHARACTER]);
 float damp=limit(s->p[REV_DAMP]+(character==1?.24f:character==0?-.15f:character==3?.12f:0.f),0,1);
 if((s->reverbCounter++&127)==0){
  for(int j=0;j<FDN;j++){
   float sec=fdnTimes[j]*s->currentSize*(character==3?1.4f:character==0?.73f:1.f);
   s->fdnGain[j]=expf(-6.907755f*sec/fmaxf(s->p[REV_TIME],.1f));
  }
 }
 size_t pd=(size_t)(s->p[REV_PREDELAY]*.001f*s->sr);
 if(pd>s->preCapacity-2)pd=s->preCapacity-2;
 s->preL[s->preWrite]=inL;s->preR[s->preWrite]=inR;
 size_t preRead=wrapindex((long long)s->preWrite-(long long)pd,s->preCapacity);
 float pl=s->preL[preRead],pr=s->preR[preRead];
 s->preWrite=(s->preWrite+1)%s->preCapacity;
 float sum=0.f;
 for(int j=0;j<FDN;j++){
  float delay=(float)(fdnTimes[j]*s->currentSize*(character==3?1.4f:character==0?.73f:1.f)*s->sr);
  float rpos=(float)s->fdnWrite-delay;
  if(rpos<0)rpos+=(float)s->fdnCapacity;
  int a=(int)rpos;float frac=rpos-(float)a;
  int b=a+1;if(b>=(int)s->fdnCapacity)b=0;
  float x=s->fdn[j][a]*(1-frac)+s->fdn[j][b]*frac;
  s->fdnDamp[j]+=(x-s->fdnDamp[j])*(.55f-.48f*damp);
  s->fdnRead[j]=s->fdnDamp[j];sum+=s->fdnRead[j];
 }
 /* Energy-preserving Householder feedback matrix, with decorrelated input. */
 for(int j=0;j<FDN;j++){
  float mixed=s->fdnRead[j]-.25f*sum;
  float injected=(j&1?pl:pr)*.12f+(j&2?pr:pl)*.07f*(j&4?-1.f:1.f);
  s->fdn[j][s->fdnWrite]=limit(injected+s->fdnGain[j]*mixed,-6.f,6.f);
 }
 s->fdnWrite=(s->fdnWrite+1)%s->fdnCapacity;
 float a=(s->fdnRead[0]-s->fdnRead[1]+s->fdnRead[2]-s->fdnRead[3]
         +s->fdnRead[4]-s->fdnRead[5]+s->fdnRead[6]-s->fdnRead[7])*.42f;
 float b=(s->fdnRead[0]+s->fdnRead[1]-s->fdnRead[2]-s->fdnRead[3]
         +s->fdnRead[4]+s->fdnRead[5]-s->fdnRead[6]-s->fdnRead[7])*.42f;
 float rl=bandSample(&s->reverbFilter,0,a),rr=bandSample(&s->reverbFilter,1,b);
 float mid=.5f*(rl+rr);float width=s->p[REV_WIDTH];
 *outL=mid+width*(rl-mid);*outR=mid+width*(rr-mid);
}
static void processFloatCore(void*ptr,const float*in,float*out,float*inputTap,float*textureTap,float*reverbTap,int frames){
 State*s=(State*)ptr;if(!s||!out||frames<=0)return;
 float sr=(float)s->sr;
 float bpm=(float)(s->playing&&s->bpm>20?s->bpm:s->p[TEMPO]);
 float loopPeriod=sr*60.f/fmaxf(bpm,20.f)*beatDivs[CLIP((int)roundf(s->p[LOOP_RATE]),0,8)];
 for(int i=0;i<frames;i++){
  float il=in?in[i*2]:0,ir=in?in[i*2+1]:0;
  /* Tape-speed bend acts only on already-generated voices. It never alters
     live input or the clean reverb send. Fresh voices remember the current
     speed as their 1x anchor, so new playing layers over older slow material. */
  s->currentTimeBend+=(s->p[TIME_BEND]-s->currentTimeBend)*.00065f;
  if(s->timeRestartAge<1e12f)s->timeRestartAge+=1.f;
  s->timeMotionPhase+=s->p[TIME_RATE]/s->sr;
  if(s->timeMotionPhase>=1.0)s->timeMotionPhase-=1.0;
  float inputLevel=fmaxf(fabsf(il),fabsf(ir));
  s->inputMeter=s->inputMeter*.997f+inputLevel*.003f;
  if(inputLevel>s->p[THRESHOLD]){
   /* NEW PHRASE DETECTION: after a genuinely quiet gap, a new note must not
      resurrect an old loud performance. Leave memory untouched during the
      silence so existing textures can continue to bloom as requested.
      Freeze explicitly opts out: an intentionally held memory is sacred.
      Reset is O(1) and never allocates/clears megabytes in the audio callback. */
   const uint64_t gap=(uint64_t)(s->p[NEW_PHRASE_GAP]*sr);
   if(s->inputSeen && s->quietCaptureSamples>=gap &&
      (int)roundf(s->p[NEW_NOTES_MODE])==4){
     s->timeRestartAge=0.f;
   }
   if(s->p[SMART_MEMORY]>=.5f && !s->freeze && s->inputSeen &&
      s->quietCaptureSamples>=gap && s->filled>0){
    s->write=0;s->filled=0;
    s->captureSamples=0;s->onsetCount=0;s->onsetWrite=0;
    s->transientEnvelope=0;s->lastOnsetSample=0;
    memset(&s->grainPitchSequence,0,sizeof(s->grainPitchSequence));
    memset(&s->loopPitchSequence,0,sizeof(s->loopPitchSequence));
    resetVoices(s); /* prevents any voice from reading retired audio */
    ++s->memoryRefreshes;
   }
   s->silenceCount=0;s->samplesSinceInput=0;
   s->quietCaptureSamples=0;s->inputSeen=1;
  }else{
   if(s->quietCaptureSamples<UINT64_MAX-1)s->quietCaptureSamples++;
   if(s->silenceCount<(int)(sr*.15f))s->silenceCount++;
  }
  int active=s->inputSeen&&s->silenceCount<(int)(s->sr*.15);
  if(!active&&s->inputSeen&&s->samplesSinceInput<UINT64_MAX-1)s->samplesSinceInput++;
  /* Preserve short pauses between notes in phrases, but stop writing after
     1.2 seconds of silence so a dormant buffer keeps its old material. */
  int capturing=s->inputSeen&&(active||s->quietCaptureSamples<(uint64_t)(s->sr*1.2));
  if(!s->freeze&&capturing){
   s->capture[s->write*2]=bandSample(&s->sourceFilter,0,il);
   s->capture[s->write*2+1]=bandSample(&s->sourceFilter,1,ir);
   s->write=(s->write+1)%s->capacity;if(s->filled<s->capacity)s->filled++;
   s->captureSamples++;trackOnset(s,inputLevel);
  }
  size_t start,n;sourceRange(s,&start,&n);
  int keepSpawning=s->freeze||active||(s->inputSeen&&(s->p[DURATION]==0||s->samplesSinceInput<(uint64_t)(s->p[DURATION]*sr)));
  /* Avoid spawning a tonal 128-sample microloop when a phrase begins. */
  if(keepSpawning&&n>(size_t)(sr*.18f)){
   if((s->grainClock-=1.f)<=0){spawnGrain(s,start,n);s->grainClock+=sr/s->p[DENSITY];}
   if(s->p[LOOP_LEVEL]>.001f&&(s->loopClock-=1.f)<=0){
    if(n>(size_t)(sr*.60f))spawnLoop(s,start,n);
    float nextPeriod=loopPeriod;
    if((int)roundf(s->p[PHRASE_STYLE])==0)nextPeriod=fmaxf(nextPeriod,(float)n);
    s->loopClock+=nextPeriod;
   }
  }
  float gl=0,gr=0,ll=0,lr=0;int grainCount=0,loopCount=0;
  for(int j=0;j<MAX_GRAINS;j++){
   Grain*g=&s->grains[j];if(!g->active)continue;
   if(g->playAge>=g->len){g->active=0;continue;}
   grainCount++;
   float speedRatio=timeRatio(s,g->timeAnchor,g->timeLife);
   if((int)roundf(s->p[NEW_NOTES_MODE])==2){
    float t=limit(g->timeLife/fmaxf(s->p[TRANSITION_TIME]*sr,1.f),0,1);
    speedRatio=1.f+(timeRatio(s,1.f,g->timeLife)-1.f)*t;
   }
   float t=g->playAge/(float)g->len;
   float win=.5f-.5f*cosf(2*PI*t);
   gl+=readVoice(s,g->regionStart,g->regionLen,g->pos,0)*win*g->gain*(1-g->pan*.5f);
   gr+=readVoice(s,g->regionStart,g->regionLen,g->pos,1)*win*g->gain*(1+g->pan*.5f);
   /* Experimental pitch-preserving voice stretch: vary envelope timing,
      not playback pitch. This is not a phase vocoder and may smear attacks. */
   float posRate=((int)roundf(s->p[TIME_MODE])==1)?1.f:speedRatio;
   g->pos+=g->step*posRate;
   g->playAge+=speedRatio;g->timeLife+=1.f;g->age=(int)g->playAge;
  }
  float gn=1.f/sqrtf(fmaxf(1.f,(float)grainCount*.65f));gl*=gn;gr*=gn;
  for(int j=0;j<MAX_LOOPS;j++){
   MicroLoop*l=&s->loops[j];if(!l->active)continue;
   if(l->playAge>=l->len){l->playAge=fmodf(l->playAge,(float)l->len);l->volume*=limit(l->feedback+(1.f-l->feedback)*s->p[TEXTURE_SUSTAIN]*.997f,0.f,.9995f);}
   if(l->volume<.003f){l->active=0;continue;}
   loopCount++;
   int count=CLIP(l->phraseCount,1,MAX_PHRASE_STEPS);
   int segment=CLIP(l->len/count,1,l->len);
   float loopRatio=timeRatio(s,l->timeAnchor,l->timeLife);
   if((int)roundf(s->p[NEW_NOTES_MODE])==2){
    float t=limit(l->timeLife/fmaxf(s->p[TRANSITION_TIME]*sr,1.f),0,1);
    loopRatio=1.f+(timeRatio(s,1.f,l->timeLife)-1.f)*t;
   }
   int ix=CLIP((int)l->playAge/segment,0,count-1);
   float local=l->playAge-ix*segment;
   int offset=l->phraseOffsets[ix];
   if(offset>=0){
    float edge=fminf((float)(sr*.025f),(float)segment*.22f);
    float a=limit(local/fmaxf(edge,1.f),0,1);
    float b=limit((segment-local)/fmaxf(edge,1.f),0,1);
    float window=(.5f-.5f*cosf(PI*a))*(.5f-.5f*cosf(PI*b));
    float wave=1.f;
    if(l->phraseMode==4)wave=.55f+.45f*sinf(PI*(float)ix/(float)count);
    float pos=(float)offset+local*l->step;
    if((int)roundf(s->p[TIME_MODE])==1){
      /* Keep original source pitch by reading at normal step, but reschedule
         phrase sections according to the time clock. Voice-domain approximation. */
      float normLocal=local/fmaxf(loopRatio,.08f);
      pos=(float)offset+fmodf(normLocal,(float)segment)*l->step;
    }
    float amp=l->volume*window*wave*.6f*s->p[LOOP_LEVEL];
    ll+=readVoice(s,l->regionStart,l->regionLen,pos,0)*amp*(1-l->pan*.5f);
    lr+=readVoice(s,l->regionStart,l->regionLen,pos,1)*amp*(1+l->pan*.5f);
   }
   l->playAge+=loopRatio;l->timeLife+=1.f;l->age=(int)l->playAge;
  }
  float ln=1.f/sqrtf(fmaxf(1.f,loopCount*.8f));ll*=ln;lr*=ln;
  gl=bandSample(&s->grainFilter,0,gl);gr=bandSample(&s->grainFilter,1,gr);
  ll=bandSample(&s->loopFilter,0,ll);lr=bandSample(&s->loopFilter,1,lr);
  float wl=gl*s->p[GRAIN_LEVEL]+ll*s->p[LOOP_MIX_LEVEL];
  float wr=gr*s->p[GRAIN_LEVEL]+lr*s->p[LOOP_MIX_LEVEL];
  if(!s->freeze&&!active&&s->p[DECAY]>.0001f){
   float elapsed=(float)(s->samplesSinceInput/s->sr);
   float fade=expf(-elapsed*s->p[DECAY]*.12f*(1.0f-s->p[TEXTURE_SUSTAIN]*.96f));wl*=fade;wr*=fade;
  }
  /* The retired chorus, flanger and drive chains are absent from audio DSP. */
  /* Tape processing is a parallel monitoring/texture layer, never written
     into the pre-FX capture buffer. Both tape mixes are independent. */
  float cleanL=il,cleanR=ir;
  writeTape(s,0,cleanL,cleanR);
  writeTape(s,1,wl,wr);
  if(s->p[TAPE_ENABLED]>=.5f){
   tapePath(s,0,&cleanL,&cleanR,s->p[INPUT_TAPE_MIX]);
   tapePath(s,1,&wl,&wr,s->p[TEXTURE_TAPE_MIX]);
  }
  advanceTape(s);
  float rl,rr;
  runReverb(s,cleanL*s->p[CLEAN_SEND]+wl*s->p[TEXTURE_SEND],
              cleanR*s->p[CLEAN_SEND]+wr*s->p[TEXTURE_SEND],&rl,&rr);
  float revMix=s->currentReverbMix;
  if(!s->freeze&&active&&s->p[REGEN]>.001f&&s->filled>0){
   size_t last=wrapindex((long long)s->write-1,s->capacity);
   float r=s->p[REGEN]*.25f;
   s->capture[last*2]=limit(s->capture[last*2]*(1-r)+wl*r,-1.f,1.f);
   s->capture[last*2+1]=limit(s->capture[last*2+1]*(1-r)+wr*r,-1.f,1.f);
  }
  s->currentWetGain+=(s->p[WET_GAIN]-s->currentWetGain)*.0003f;
  s->currentMix+=(s->p[MIX]-s->currentMix)*.0003f;
  /* When no texture voices are producing audio (especially during the
     180/600ms capture warm-up), automatically allow the clean instrument
     through. This guards against silent-start on fully wet presets.
     Once voices exist, return smoothly to the user-selected dry/wet mix. */
  float readyTarget=(s->p[WET_GAIN]>.001f && ((grainCount>0 && s->p[GRAIN_LEVEL]>.001f) || (loopCount>0 && s->p[LOOP_LEVEL]>.001f && s->p[LOOP_MIX_LEVEL]>.001f)))?1.f:0.f;
  const float readyCoef=readyTarget?.00022f:.0015f;
  s->textureReady+=(readyTarget-s->textureReady)*readyCoef;
  float gain=s->currentWetGain,mix=s->currentMix*s->textureReady;
  /* The Reverb Return is a true SEND RETURN, NOT suppressed by Texture Mix.
     A player can turn Texture Mix to zero and hear clean instrument + reverb.
     'mix' now balances direct sound against generated grains/microloops only. */
  float dryL=cleanL*s->p[DRY_LEVEL]*(1-mix), dryR=cleanR*s->p[DRY_LEVEL]*(1-mix);
  float texL=wl*gain*mix, texR=wr*gain*mix;
  float wetL=rl*revMix*s->p[REVERB_LEVEL], wetR=rr*revMix*s->p[REVERB_LEVEL];
  float ol=(dryL+texL+wetL)*s->p[MASTER_LEVEL];
  float orr=(dryR+texR+wetR)*s->p[MASTER_LEVEL];
  /* Oscilloscope taps expose what each part contributes; no extra DSP. */
  if(inputTap){inputTap[2*i]=il;inputTap[2*i+1]=ir;}
  if(textureTap){textureTap[2*i]=texL;textureTap[2*i+1]=texR;}
  if(reverbTap){reverbTap[2*i]=wetL;reverbTap[2*i+1]=wetR;}
  if(s->p[SOFT_CLIP]>.5f){ol=ol/(1+.20f*fabsf(ol));orr=orr/(1+.20f*fabsf(orr));}
  out[i*2]=limit(ol,-1.f,1.f);out[i*2+1]=limit(orr,-1.f,1.f);
  s->outputMeter=s->outputMeter*.997f+.003f*fmaxf(fabsf(ol),fabsf(orr));
  s->activeGrains=grainCount;s->activeLoops=loopCount;
 }
}
static void processFloat(void*ptr,const float*in,float*out,int frames){
 processFloatCore(ptr,in,out,NULL,NULL,NULL,frames);
}
static void processFloatTaps(void*ptr,const float*in,float*out,
                             float*inputTap,float*textureTap,float*reverbTap,int frames){
 processFloatCore(ptr,in,out,inputTap,textureTap,reverbTap,frames);
}
static int16_t pcm16(float v){return (int16_t)lrintf(limit(v,-1.f,1.f)*32767.f);}
/* MPC 16-bit adapter: batch 128 frames, rather than one DSP call per
   sample. Keep the ARM64 denormal flush local to the render call; restore the
   host FPCR afterwards so this plugin never changes the host's FP state. */
static void process16(void*ptr,const int16_t*in,int16_t*out,int frames){
 if(!out||frames<=0)return;
#if defined(__aarch64__)
 unsigned long long prevFpcr, fpcr;
 __asm__ volatile("mrs %0, fpcr":"=r"(prevFpcr));
 fpcr = prevFpcr | (1ULL<<24); /* FZ: flush subnormals to zero */
 __asm__ volatile("msr fpcr, %0"::"r"(fpcr));
#endif
 while(frames>0){
  const int n=frames>128?128:frames;
  float bufIn[256],bufOut[256];
  for(int i=0;i<n*2;++i)bufIn[i]=in?in[i]/32768.f:0.f;
  processFloat(ptr,bufIn,bufOut,n);
  for(int i=0;i<n*2;++i)out[i]=pcm16(bufOut[i]);
  if(in)in+=n*2;
  out+=n*2;frames-=n;
 }
#if defined(__aarch64__)
 __asm__ volatile("msr fpcr, %0"::"r"(prevFpcr));
#endif
}
static void render(void*ptr,int16_t*out,int frames){(void)ptr;if(out)memset(out,0,(size_t)frames*2*sizeof(int16_t));}
static void songPos(void*ptr,double ppq,double bpm,int playing,int frames){
 (void)frames;State*s=(State*)ptr;if(!s)return;s->ppq=ppq;s->playing=playing;
 if(bpm>20&&bpm<400)s->bpm=bpm;
}
static const mpc_engine_t engine={create,destroy,midi,setParam,getParam,render,process16,songPos,configureRate,processFloat,processFloatTaps};
const mpc_engine_t*mpc_engine(void){return &engine;}
