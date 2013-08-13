#include "rfb.h"

int main(int argc, char **argv)
{
  rfb_servinfo_t *sinfop;
  int err;
  int i;
  int mapsz;
  int seed = 0;
  uint32_t colors[] = COLORS;
  
  sinfop = rfb_run(0, 0, NULL, "rfb");
  mapsz = sinfop->infop.fbwidth * sinfop->infop.fbheight;


  while(1)
    for(i = 0; i < mapsz; ++i);

  rfb_kill(sinfop);

  return 0;
}
