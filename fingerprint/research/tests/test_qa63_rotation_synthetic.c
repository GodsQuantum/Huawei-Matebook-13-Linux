#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../driver/goodix51a0/fastbrief/sigfm.h"

#define W 80
#define H 64
#define N (W * H)

static uint32_t step(uint32_t *state)
{
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

static SigfmImgInfo *synthetic_print(uint32_t seed)
{
  uint8_t image[N];
  uint32_t state = seed;
  for (int i=0; i<N; i++)
    image[i] = (uint8_t) (step(&state) & 0xff);
  SigfmImgInfo *features = sigfm_extract(image, W, H);
  return features;
}
int main(void)
{
  SigfmImgInfo *registered = synthetic_print(0x51a0u);
  SigfmImgInfo *impostor = synthetic_print(0x2170u);
  assert(registered && impostor);
  int correct = sigfm_match_score(registered,registered);
  int wrong = sigfm_match_score(impostor,registered);
  SigfmImgInfo *r18 = sigfm_rotate_extract(registered,18.0);
  SigfmImgInfo *n18 = sigfm_rotate_extract(registered,-18.0);
  assert(r18 && n18);
  int rotated18 = sigfm_match_score(r18,registered);
  int rotatedn18 = sigfm_match_score(n18,registered);
  SigfmImgInfo *recovered = sigfm_rotate_extract(r18,-18.0);
  int recovered_score = recovered ? sigfm_match_score(recovered,registered) : -1;
  SigfmImgInfo *invalid = sigfm_rotate_extract(registered,21.0);
  assert(!invalid);
  printf("synthetic genuine=%d wrong=%d rot+18=%d rot-18=%d recovered=%d\n",correct,wrong,rotated18,rotatedn18,recovered_score);
  assert(correct >= 7);
  assert(wrong < 7);
  assert(rotated18 >= 0 && rotatedn18 >= 0);
  assert(recovered_score >= 7);
  if (recovered) sigfm_free_info(recovered);
  sigfm_free_info(r18);sigfm_free_info(n18);
  sigfm_free_info(impostor);sigfm_free_info(registered);
  return 0;
}
