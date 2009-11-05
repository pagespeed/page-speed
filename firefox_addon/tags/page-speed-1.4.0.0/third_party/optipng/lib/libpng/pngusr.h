/*
 * pngusr.h -- private configuration settings for OptiPNG.
 *
 * This file is #included by png.h.
 */


#ifndef PNGUSR_H
#define PNGUSR_H


/*
 * Track down memory leaks, if using MS Visual C++.
 */
#if defined(_DEBUG) && defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
#endif


/*
 * Redefine internal libpng constants.
 */
#define PNG_ZBUF_SIZE       16384
#define PNG_USER_WIDTH_MAX  0x7fffffffL
#define PNG_USER_HEIGHT_MAX 0x7fffffffL


/*
 * Remove libpng features that are not needed by OptiPNG.
 */
#define PNG_NO_ASSEMBLER_CODE
#define PNG_NO_ERROR_NUMBERS
#define PNG_NO_FLOATING_POINT_SUPPORTED
#define PNG_NO_LEGACY_SUPPORTED
#define PNG_NO_MNG_FEATURES
#define PNG_NO_PROGRESSIVE_READ
#define PNG_NO_SETJMP_SUPPORTED
#define PNG_NO_SET_USER_LIMITS
#define PNG_NO_READ_TRANSFORMS
#define PNG_NO_WRITE_TRANSFORMS
#define PNG_NO_USER_MEM
#define PNG_NO_ZALLOC_ZERO


#endif	/* PNGUSR_H */
