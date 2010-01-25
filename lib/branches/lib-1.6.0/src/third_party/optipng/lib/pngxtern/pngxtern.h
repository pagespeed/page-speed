/*
 * pngxtern.h - external file format processing for libpng.
 *
 * Copyright (C) 2003-2008 Cosmin Truta.
 * This software is distributed under the same licensing and warranty terms
 * as libpng.
 */


#ifndef PNGXTERN_H
#define PNGXTERN_H

#include "png.h"
#ifdef PNGX_INTERNAL
#include <stdio.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct GIFInput;

/* GIF */
int pngx_sig_is_gif
   PNGARG((const png_bytep sig, png_size_t sig_size,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));
int pngx_read_gif
   PNGARG((png_structp png_ptr, png_infop info_ptr, struct GIFInput *stream));

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* PNGXTERN_H */
