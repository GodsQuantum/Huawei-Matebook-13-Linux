#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "../../driver/goodix51a0/fastbrief/sigfm.h"
#define W 80
#define H 64
#define N (W*H)
#define VIEWS 24
#define LOOPS 20
static void fill(uint8_t *p, unsigned seed) {
  for (int y=0;y<H;y++)
    for (int x=0;x<W;x++) {
      unsigned xx=(unsigned)x, yy=(unsigned)y;
      p[y*W+x]=(uint8_t)((xx*17u+yy*29u+((xx^yy)*11u)+((xx*yy+seed*97u)%251u))&255u);
    }
}
static double now_ms(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec*1000.0+(double)t.tv_nsec/1e6;
}
static int scan(SigfmImgInfo *p, SigfmImgInfo **views, int shortcut) {
  int best=0;
  for(int i=0;i<VIEWS;i++) {
    int s=sigfm_match_score(p,views[i]);
    if(s>best) best=s;
    if(shortcut&&s>=7) return s;
  }
  return best;
}
int main(void) {
  uint8_t a[N];
  fill(a,1);
  SigfmImgInfo *probe=sigfm_extract(a,W,H),*views[VIEWS];
  if(!probe || sigfm_keypoints_count(probe)<25) return 2;
  for(int i=0;i<VIEWS;i++) views[i]=sigfm_copy_info(probe);
  if(!views[0])return 3;
  volatile int sink=0;
  int base=scan(probe,views,0),early=scan(probe,views,1);
  if(base<7 || early<7) {fprintf(stderr,"Unusable synthetic match base=%d early=%d\n",base,early);return 4;}
  double t0=now_ms();
  for(int j=0;j<LOOPS;j++)sink+=scan(probe,views,0);
  double t1=now_ms();
  for(int j=0;j<LOOPS;j++)sink+=scan(probe,views,1);
  double t2=now_ms();
  printf("SYNTHETIC_CPU_ONLY views=%d loops=%d features=%d score_full=%d score_early=%d full_ms_per_verify=%.3f early_ms_per_verify=%.3f saved_ms=%.3f (best-case identical first view, not real-user speed) sink=%d\n",
         VIEWS,LOOPS,sigfm_keypoints_count(probe),base,early,(t1-t0)/LOOPS,(t2-t1)/LOOPS,((t1-t0)-(t2-t1))/LOOPS,(int)sink);
  for(int i=0;i<VIEWS;i++)sigfm_free_info(views[i]);
  sigfm_free_info(probe);
  return 0;
}
