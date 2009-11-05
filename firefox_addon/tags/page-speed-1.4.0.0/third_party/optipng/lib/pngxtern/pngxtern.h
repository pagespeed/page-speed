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


/*
 * Read the contents of an image file into the libpng structures.
 * The currently recognized file formats are:
 * PNG (standalone), PNG (datastream), BMP, GIF, PNM and TIFF.
 *
 * The function reads either the first or the most relevant image,
 * depending on the format.  For example, embedded thumbnails, if
 * present, are skipped.
 *
 * On success, the function returns the number of images contained
 * by the image file (which can be greater than 1 for formats like
 * GIF or TIFF).  If the function finds more than one image but does
 * not perform a complete image count, it returns an upper bound.
 * The function stores the short and/or the long format name
 * (e.g. "PPM", "Portable Pixmap") into the given name buffers,
 * if they are non-null.
 *
 * If the function fails to detect a known format, it rewinds the
 * file stream stored in io_ptr and returns 0.
 *
 * If the given format name buffers are non-null, but not large
 * enough, the function returns -1.  The calling application can
 * retry the call after enlarging these buffers.
 *
 * On other errors (e.g. read error or decoding error), the function
 * issues a png_error().
 *
 * This function requires io_ptr to be a fseek-able FILE*.
 * It does not work with generic I/O routines.
 */
extern PNG_EXPORT(int, pngx_read_image)
   PNGARG((png_structp png_ptr, png_infop info_ptr,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));


/*
 * Private format decoding helpers.
 */
#ifdef PNGX_INTERNAL

/* BMP */
int pngx_sig_is_bmp
   PNGARG((const png_bytep sig, png_size_t sig_size,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));
int pngx_read_bmp
   PNGARG((png_structp png_ptr, png_infop info_ptr, FILE *stream));

/* GIF */
int pngx_sig_is_gif
   PNGARG((const png_bytep sig, png_size_t sig_size,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));
int pngx_read_gif
   PNGARG((png_structp png_ptr, png_infop info_ptr, FILE *stream));

/* JPEG */
int pngx_sig_is_jpeg
   PNGARG((const png_bytep sig, png_size_t sig_size,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));
int pngx_read_jpeg
   PNGARG((png_structp png_ptr, png_infop info_ptr, FILE *stream));

/* PNM */
int pngx_sig_is_pnm
   PNGARG((const png_bytep sig, png_size_t sig_size,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));
int pngx_read_pnm
   PNGARG((png_structp png_ptr, png_infop info_ptr, FILE *stream));

/* TIFF */
int pngx_sig_is_tiff
   PNGARG((const png_bytep sig, png_size_t sig_size,
           png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
           png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size));
int pngx_read_tiff
   PNGARG((png_structp png_ptr, png_infop info_ptr, FILE *stream));

#endif /* PNGX_INTERNAL */


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* PNGXTERN_H */
