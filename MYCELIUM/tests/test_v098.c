/* MYCELIUM 0.9.8 regression tests: six styles, six new-note modes,
   actual parallel-voice copies, phrase-level restart, and dry-audio safety. */
#include "../dsp/mycelium.c"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void runSilence(State*s,int frames){
 float input[256]={0},out[256];
 while(frames>0){int count=frames>128?128:frames;
  processFloat(s,input,out,count);
  for(int k=0;k<count*2;++k)assert(isfinite(out[k]));
  frames-=count;
 }
}
int main(void){
 State*s=create(NULL);assert(s && configureRate(s,48000));
 assert(PARAMS==98);
 assert(fabsf(maxv[TIME_BEND]-2.f)<.001f);
 assert(fabsf(maxv[TRANSITION_TIME]-30.f)<.001f);
 setParam(s,"time_bend","0.5");s->currentTimeBend=.5f;
 setParam(s,"time_depth","1");
 setParam(s,"time_mode","0");
 assert(fabsf(timeRatio(s,1.f,0.f)-.5f)<.001f);
 assert(fabsf(timeRatio(s,.5f,0.f)-1.f)<.001f);
 setParam(s,"time_depth","0");
 assert(fabsf(timeRatio(s,1.f,0.f)-1.f)<.001f);
 setParam(s,"time_depth","1");
 setParam(s,"new_notes_mode","0");
 assert(fabsf(newTimeAnchor(s)-.5f)<.001f);  /* fresh starts 1x */
 setParam(s,"new_notes_mode","1");
 assert(fabsf(newTimeAnchor(s)-1.f)<.001f); /* join starts bent */
 setParam(s,"new_notes_mode","2");
 assert(fabsf(newTimeAnchor(s)-.5f)<.001f); /* melt from 1x */
 setParam(s,"time_mode","3");setParam(s,"time_motion","1");
 s->timeMotionPhase=.25;
 float rippleHigh=timeRatio(s,1.f,96000.f);
 s->timeMotionPhase=.75;
 float rippleLow=timeRatio(s,1.f,96000.f);
 assert(rippleHigh>rippleLow);
 setParam(s,"time_mode","4");s->timeMotionPhase=.15;
 assert(isfinite(timeRatio(s,1.f,0.f)));
 setParam(s,"time_mode","5");
 assert(timeRatio(s,1.f,0.f)<.1f); /* almost stopped */
 setParam(s,"time_mode","0");

 /* Real parallel mode requires a NEW clean-rate voice AND a bent copy. */
 s->filled=96000;s->write=s->filled;
 setParam(s,"new_notes_mode","3");
 spawnGrain(s,0,s->filled);spawnLoop(s,0,s->filled);
 assert(s->grains[0].active && s->grains[1].active);
 assert(s->loops[0].active && s->loops[1].active);
 assert(fabsf(s->grains[0].timeAnchor-.5f)<.001f);
 assert(fabsf(s->grains[1].timeAnchor-1.f)<.001f);
 assert(fabsf(s->loops[0].timeAnchor-.5f)<.001f);
 assert(fabsf(s->loops[1].timeAnchor-1.f)<.001f);
 resetVoices(s);
 setParam(s,"new_notes_mode","5");
 float first=newTimeAnchor(s),second=newTimeAnchor(s);
 assert(first!=second); /* alternating independent fragments */
 setParam(s,"new_notes_mode","4");
 setParam(s,"transition_time","5");
 s->timeRestartAge=0.f;
 assert(fabsf(timeRatio(s,1.f,0.f)-1.f)<.001f);
 s->timeRestartAge=48000.f*5.f;
 assert(fabsf(timeRatio(s,1.f,0.f)-.5f)<.001f);
 /* A newly recorded phrase starts unbent, but the underlying knob stays put. */
 s->timeRestartAge=0.f;
 assert(fabsf(s->p[TIME_BEND]-.5f)<.001f);

 /* Output remains finite while toggling all available TIME modes. */
 setParam(s,"new_notes_mode","0");
 for(int mode=0;mode<6;++mode){
  char n[16];snprintf(n,sizeof n,"%d",mode);setParam(s,"time_mode",n);
  runSilence(s,256);
 }
 destroy(s);
 puts("PASS v0.9.8: time styles, six policies, real parallel copies, restart, finite output");
 return 0;
}
