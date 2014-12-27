
#define VERSION "1.3.1"
#define FLAC__INTEGER_ONLY_LIBRARY 1
//#define FLAC__OVERFLOW_DETECT 1
#define HAVE_BSWAP32 1
#define HAVE_BSWAP16 1

#include "flac/all.h"
#include "flac/lpc.c"
#include "flac/format.c"
#include "flac/float.c"
#include "flac/fixed.c"
#include "flac/md5.c"
#include "flac/crc.c"
#include "flac/bitreader.c"
#include "flac/bitmath.c"
#include "flac/memory.c"
#include "flac/stream_decoder.c"
#include "flac/cpu.c"

