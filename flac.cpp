
#define VERSION "1.3.1"
#define FLAC__INTEGER_ONLY_LIBRARY 1
//#define FLAC__OVERFLOW_DETECT 1
#define HAVE_BSWAP32 1
#define HAVE_BSWAP16 1

#include "FLAC/all.h"
#include "FLAC/lpc.c"
#include "FLAC/format.c"
#include "FLAC/float.c"
#include "FLAC/fixed.c"
#include "FLAC/md5.c"
#include "FLAC/crc.c"
#include "FLAC/bitreader.c"
#include "FLAC/bitmath.c"
#include "FLAC/memory.c"
#include "FLAC/stream_decoder.c"
#include "FLAC/cpu.c"

