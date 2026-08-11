// minimp3 is a header only library, so exactly one translation unit has to
// instantiate it. Doing that here lets the rest of the application stay inline
// in headers: a target only has to add this one file to its build.
//
// MINIMP3_ONLY_MP3 drops the Layer I and Layer II tables, which we have no use
// for and which cost flash. Neither define changes the layout of mp3dec_t, so
// this file and Mp3Decoder.h agree on it regardless.
//
// SIMD is selected by minimp3 from the compiler's own architecture macros, so
// desktop builds get SSE and Cortex-M builds fall through to plain C. Define
// MINIMP3_NO_SIMD for a target where that guess turns out wrong.

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3

#include "../../../ThirdParty/minimp3/minimp3.h"
