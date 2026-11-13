/* 0.9.7 regression: no silent output, independent clean pass-through,
   genuine old-voice tape speed changes and new-voice 1x anchors. */
#include "../dsp/mycelium.c"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <math.h>

static float runAudio(State*s,int frames,float amplitude){
 float energy=0;float input[2*128],output[2*128];int start=0;
 while(start<frames){
  int count=frames-start;if(count>128)count=128;
  for(int k=0;k<count;k++){
   float v=amplitude*sinf(2.f*PI*(float)(start+k)*220.f/(float)s->sr);
   input[k*2]=v;input[k*2+1]=v;
  }
  processFloat(s,input,output,count);
  for(int k=0;k<count*2;k++){assert(isfinite(output[k]));energy+=fabsf(output[k]);}
  start+=count;
 }
 return energy;
}

int main(void){
 State*s=create(NULL);assert(s);assert(configureRate(s,48000));
 /* The previous min/max arrays were TWO ELEMENTS SHORT. In particular master
    output and tape switch could be clamped incorrectly, silencing the plugin. */
 assert(PARAMS==98);
 assert(maxv[MASTER_LEVEL]==1.5f && maxv[TAPE_ENABLED]==1.f);
 assert(minv[TIME_BEND]==.125f && maxv[TIME_BEND]==2.f);
 setParam(s,"master_level","1");assert(s->p[MASTER_LEVEL]==1.f);
 setParam(s,"tape_enabled","1");assert(s->p[TAPE_ENABLED]==1.f);
 float audible=runAudio(s,48000/2,.45f);
 printf("PASS default sound energy %.3f\n",audible);
 assert(audible>100.f);
 destroy(s);

 /* Fully wet but no effective grain/loop output must still permit live input
    during warm-up / after the texture has stopped. */
 s=create(NULL);assert(s);assert(configureRate(s,48000));
 setParam(s,"mix","1");setParam(s,"grain_level","0");
 setParam(s,"loop_level","0");setParam(s,"rev_mix","0");
 float backstop=runAudio(s,48000/2,.45f);
 printf("PASS fully wet silent-engine fallback %.3f\n",backstop);
 assert(backstop>1000.f);
 /* Respect the explicit clean-volume mute with no pre-existing reverb tail. */
 destroy(s);s=create(NULL);assert(s);assert(configureRate(s,48000));
 setParam(s,"mix","0");setParam(s,"rev_mix","0");s->currentReverbMix=0;
 setParam(s,"dry_level","0");
 float muted=runAudio(s,4800,.4f);
 assert(muted<.001f);
 destroy(s);

 s=create(NULL);assert(s);assert(configureRate(s,48000));
 setParam(s,"rev_mix","0");setParam(s,"duration","0.1");
 /* Explicitly seed old grains and microloops: verify phase and pitch drift. */
 s->inputSeen=0;
 s->filled=(size_t)(s->sr*2);s->write=s->filled;
 for(size_t k=0;k<s->filled;k++){
  float v=.2f*sinf(2.f*PI*220.f*k/(float)s->sr);
  s->capture[k*2]=s->capture[k*2+1]=v;
 }
 Grain*g=&s->grains[0];memset(g,0,sizeof(*g));
 g->active=1;g->len=480000;g->regionLen=s->filled;g->regionStart=0;
 g->step=1;g->timeAnchor=1;g->gain=.1f;
 MicroLoop*l=&s->loops[0];memset(l,0,sizeof(*l));
 l->active=1;l->len=480000;l->regionLen=s->filled;l->regionStart=0;
 l->step=1;l->timeAnchor=1;l->volume=.1f;l->feedback=.9f;
 l->phraseCount=1;l->phraseOffsets[0]=0;
 setParam(s,"time_bend","0.25");
 runAudio(s,48000,.0f);
 /* One second of wall time, but old voices travel only about 0.25-0.35 s. */
 assert(g->active && l->active);
 printf("PASS old voices grain phase=%.0f loop phase=%.0f\n",g->playAge,l->playAge);
 assert(g->playAge>48000*.23f && g->playAge<48000*.36f);
 assert(l->playAge>48000*.23f && l->playAge<48000*.36f);
 float previously=g->playAge;
 runAudio(s,4800,.0f);
 assert(g->playAge-previously<4800*.28f);
 /* A newly spawned grain/loop anchors to the bent speed and thus begins 1x. */
 spawnGrain(s,0,s->filled);spawnLoop(s,0,s->filled);
 Grain*newGrain=&s->grains[1];MicroLoop*newLoop=&s->loops[1];
 assert(newGrain->active && newLoop->active);
 assert(fabsf(newGrain->timeAnchor-s->currentTimeBend)<.0001f);
 assert(fabsf(newLoop->timeAnchor-s->currentTimeBend)<.0001f);
 float grainAge=newGrain->playAge, loopAge=newLoop->playAge;
 runAudio(s,128,.0f);
 assert(newGrain->playAge-grainAge>125.f);
 assert(newLoop->playAge-loopAge>125.f);
 printf("PASS fresh notes remain 1x over slowed grain and loop layers\n");
 destroy(s);
 puts("ALL V0.9.7 REGRESSIONS PASSED");
 return 0;
}
