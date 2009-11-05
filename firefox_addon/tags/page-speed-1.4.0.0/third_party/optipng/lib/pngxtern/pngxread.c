/*
 * pngxread.c - libpng external I/O: read utility functions.
 * Copyright (C) 2001-2008 Cosmin Truta.
 */

#define PNGX_INTERNAL
#include "pngx.h"
#include "pngxtern.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char pngx_png_fmt_name[] =
   "PNG";
static const char pngx_png_fmt_desc[] =
   "Portable Network Graphics";
static const char pngx_png_datastream_fmt_name[] =
   "PNG datastream";
static const char pngx_png_datastream_fmt_desc[] =
   "Portable Network Graphics embedded datastream";


static int
pngx_sig_is_png(png_structp png_ptr,
                const png_bytep sig, png_size_t sig_size,
                png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
                png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size)
{
   /* The signature of this function differs from the other pngx_sig_is_X()
    * functions.  For efficiency, the PNG signature bytes are handled here.
    */

   static const png_byte png_file_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
   static const png_byte mng_file_sig[8] = {138, 77, 78, 71, 13, 10, 26, 10};
   static const png_byte png_ihdr_sig[8] = {0, 0, 0, 13, 73, 72, 68, 82};

   const char *fmt;
   int has_png_sig;

   /* Since png_read_png() fails rather abruptly with png_error(),
    * spend a little more effort to ensure that the format is indeed PNG.
    * Among other things, look for the presence of IHDR.
    */
   if (sig_size <= 25 + 18)  /* size of (IHDR + IDAT) > (12+13) + (12+6) */
      return -1;
   has_png_sig = (memcmp(sig, png_file_sig, 8) == 0);
   if (memcmp(sig + (has_png_sig ? 8 : 0), png_ihdr_sig, 8) != 0)
   {
      /* This is not valid PNG: get as much information as possible. */
      if (memcmp(sig, png_file_sig, 4) == 0 && (sig[4] == 10 || sig[4] == 13))
         png_error(png_ptr,
            "PNG file appears to be corrupted by text file conversions");
      else if (memcmp(sig, mng_file_sig, 8) == 0)
         png_error(png_ptr, "MNG decoding is not supported");
      /* JNG is handled by the pngxrjpg module. */
      return 0;  /* not PNG */
   }

   /* Store the format name. */
   if (fmt_name_buf != NULL)
   {
      fmt = (has_png_sig ? pngx_png_fmt_name : pngx_png_datastream_fmt_name);
      PNGX_ASSERT(fmt_name_buf_size > strlen(fmt));
      strcpy(fmt_name_buf, fmt);
   }
   if (fmt_desc_buf != NULL)
   {
      fmt = (has_png_sig ? pngx_png_fmt_desc : pngx_png_datastream_fmt_desc);
      PNGX_ASSERT(fmt_desc_buf_size > strlen(fmt));
      strcpy(fmt_desc_buf, fmt);
   }
   return 1;  /* PNG, really! */
}


int PNGAPI
pngx_read_image(png_structp png_ptr, png_infop info_ptr,
                png_charp fmt_name_buf, png_size_t fmt_name_buf_size,
                png_charp fmt_desc_buf, png_size_t fmt_desc_buf_size)
{
   png_byte sig[128];
   png_size_t num;
   int (*read_fn)(png_structp, png_infop, FILE *);
   FILE *stream;
   fpos_t fpos;
   int result;

   /* Precondition. */
#ifdef PNG_FLAG_MALLOC_NULL_MEM_OK
   PNGX_ASSERT_MSG(!(png_ptr->flags & PNG_FLAG_MALLOC_NULL_MEM_OK),
      "pngxtern requires a safe allocator");
#endif

   /* Check the format name buffers. */
   /* Ensure that the longest short name ("PNG datastream") and
    * the longest long name ("Portable Network Graphics embedded datastream")
    * will fit.
    */
   if ((fmt_name_buf != NULL &&
        fmt_name_buf_size < sizeof(pngx_png_datastream_fmt_name)) ||
       (fmt_desc_buf != NULL &&
        fmt_desc_buf_size < sizeof(pngx_png_datastream_fmt_desc)))
      return -1;  /* invalid parameters */

   /* Read the signature bytes. */
   stream = (FILE *)png_get_io_ptr(png_ptr);
   PNGX_ASSERT(stream != NULL);
   if (fgetpos(stream, &fpos) != 0)
      png_error(png_ptr, "Can't ftell in input file stream");
   num = fread(sig, 1, sizeof(sig), stream);
   if (fsetpos(stream, &fpos) != 0)
      png_error(png_ptr, "Can't fseek in input file stream");

   /* Try the PNG format first. */
   if (pngx_sig_is_png(png_ptr, sig, num,
       fmt_name_buf, fmt_name_buf_size, fmt_desc_buf, fmt_desc_buf_size) > 0)
   {
      png_read_png(png_ptr, info_ptr, 0, NULL);
      if (getc(stream) != EOF)
      {
         png_warning(png_ptr, "Extraneous data found after IEND");
         fseek(stream, 0, SEEK_END);
      }
      return 1;
   }

   /* Check the signature bytes against other known image formats. */
   if (pngx_sig_is_bmp(sig, num,
       fmt_name_buf, fmt_name_buf_size, fmt_desc_buf, fmt_desc_buf_size) > 0)
      read_fn = pngx_read_bmp;
   else if (pngx_sig_is_gif(sig, num,
       fmt_name_buf, fmt_name_buf_size, fmt_desc_buf, fmt_desc_buf_size) > 0)
      read_fn = pngx_read_gif;
   else if (pngx_sig_is_jpeg(sig, num,
       fmt_name_buf, fmt_name_buf_size, fmt_desc_buf, fmt_desc_buf_size) > 0)
      read_fn = pngx_read_jpeg;
   else if (pngx_sig_is_pnm(sig, num,
       fmt_name_buf, fmt_name_buf_size, fmt_desc_buf, fmt_desc_buf_size) > 0)
      read_fn = pngx_read_pnm;
   else if (pngx_sig_is_tiff(sig, num,
       fmt_name_buf, fmt_name_buf_size, fmt_desc_buf, fmt_desc_buf_size) > 0)
      read_fn = pngx_read_tiff;
   else
      return 0;  /* not a known image format */

   /* Read the image. */
   result = read_fn(png_ptr, info_ptr, stream);
   /* Signature checking may give false positives; reading can still fail. */
   if (result <= 0)  /* this isn't the format we thought it was */
      if (fsetpos(stream, &fpos) != 0)
         png_error(png_ptr, "Can't fseek in input file stream");
   return result;
}
