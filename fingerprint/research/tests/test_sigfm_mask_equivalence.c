#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../driver/goodix51a0/fastbrief/sigfm.h"

#define W 80
#define H 64
#define N (W * H)

static void
fill_img(uint8_t *p, unsigned seed, int sx, int sy)
{
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      {
        unsigned xx = (unsigned)((x + sx + W) % W);
        unsigned yy = (unsigned)((y + sy + H) % H);
        unsigned v = xx * 17u + yy * 29u + ((xx ^ yy) * 11u)
                     + ((xx * yy + seed * 97u) % 251u);
        p[y * W + x] = (uint8_t)(v & 0xffu);
      }
}

int
main(void)
{
  uint8_t a[N], b[N];
  for (unsigned k = 1; k <= 12; k++)
    {
      fill_img(a, k, 0, 0);
      fill_img(b, k, (int)(k % 5), (int)(k % 3));
      SigfmImgInfo *fa = sigfm_extract(a, W, H);
      SigfmImgInfo *fb = sigfm_extract(b, W, H);
      if (!fa || !fb)
        return 2;

      int nk = sigfm_keypoints_count(fa);
      uint8_t *mask = calloc((size_t)(nk > 0 ? nk : 1), 1);
      if (!mask)
        return 3;

      int baseline = sigfm_match_score(fa, fb);
      int masked = sigfm_match_score_mask(fa, fb, mask);
      int marked = 0;
      for (int i = 0; i < nk; i++)
        marked += mask[i] ? 1 : 0;

      if (baseline != masked)
        {
          fprintf(stderr, "score mismatch case=%u baseline=%d masked=%d\n",
                  k, baseline, masked);
          return 4;
        }
      if (marked < masked)
        {
          fprintf(stderr, "mask under-count case=%u score=%d marked=%d\n",
                  k, masked, marked);
          return 5;
        }

      free(mask);
      sigfm_free_info(fa);
      sigfm_free_info(fb);
    }

  puts("test_sigfm_mask_equivalence: OK");
  return 0;
}
