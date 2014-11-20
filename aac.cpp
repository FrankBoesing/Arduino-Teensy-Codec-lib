
//#define AAC_ENABLE_SBR

#include "common/assembly.h"

#include "aac/aacdec.h"

#ifdef  AAC_ENABLE_SBR
 #include "aac/sbr.h"
 #include "aac/sbr.c"
#endif

#include "aac/aacdec.c"
#include "aac/bitstream.c" 
#include "aac/buffers.c"
#include "aac/noiseless.c"
#include "aac/filefmt.c"
#include "aac/dequant.c"
#include "aac/decelmnt.c"
#include "aac/stproc.c"
#include "aac/pns.c"
#include "aac/tns.c"
#include "aac/dct4.c"
#include "aac/imdct.c"
#include "aac/fft.c"

#include "aac/aactabs.c"
#include "aac/trigtabs.c"

#include "aac/huffman.c"
#include "aac/hufftabs.c"