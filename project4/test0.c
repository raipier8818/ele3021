#include "types.h"
#include "stat.h"
#include "user.h"

int 
main(int argc, char* argv[])
{
  printf(1, "[Test 0] default\n");

  int numfp = countfp();
  int numvp = countvp();
  int numpp = countpp();
  int numptp = countptp();

  sbrk(4096);

  int numfpa = countfp();
  int numvpa = countvp();
  int numppa = countpp();
  int numptpa = countptp();
  
  printf(1, "fp: %d %d\n", numfp, numfpa);
  printf(1, "vp: %d %d\n", numvp, numvpa);
  printf(1, "pp: %d %d\n", numpp, numppa);
  printf(1, "ptp: %d %d\n", numptp, numptpa);

  if((numvp == numpp) && (numvpa == numppa) && (numfp - numfpa == 1))
    printf(1, "[Test 0] pass\n\n");
  else
    printf(1, "[Test 0] fail\n\n");

  exit();
}

