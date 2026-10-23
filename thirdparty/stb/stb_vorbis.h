/*
 * Header-only include for the vendored stb_vorbis implementation.
 *
 * The implementation is built separately from thirdparty/stb/stb_vorbis.c;
 * this wrapper only exposes its declarations to C++ translation units.
 */
#ifndef OGGSOUND_STB_VORBIS_H
#define OGGSOUND_STB_VORBIS_H

/* OggSound only decodes Vorbis data from memory. */
#ifndef STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_STDIO
#endif
#ifndef STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_PUSHDATA_API
#endif

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

#endif /* OGGSOUND_STB_VORBIS_H */
