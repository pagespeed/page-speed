/*
 * pngxset.c - libpng extension: additional image info storage.
 * Copyright (C) 2001-2008 Cosmin Truta.
 *
 * This module contains functions proposed for addition to libpng.
 */

#include "pngx.h"


/* Fortunately, direct access to the info structure is safe,
 * because the position of the fields accessed here is fixed
 * (i.e. not dependent on any libpng configuration macros).
 */


void PNGAPI
pngx_set_compression_method(png_structp png_ptr, png_infop info_ptr,
   int compression_method)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;
   if (compression_method != PNG_COMPRESSION_TYPE_BASE)
      png_error(png_ptr, "Unknown compression method");
   info_ptr->compression_type = (png_byte)compression_method;
}

void PNGAPI
pngx_set_filter_method(png_structp png_ptr, png_infop info_ptr,
   int filter_method)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;
   if (filter_method != PNG_FILTER_TYPE_BASE)
      png_error(png_ptr, "Unknown filter method");
   info_ptr->filter_type = (png_byte)filter_method;
}

void PNGAPI
pngx_set_interlace_method(png_structp png_ptr, png_infop info_ptr,
   int interlace_method)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;
   if (interlace_method < 0 || interlace_method >= PNG_INTERLACE_LAST)
      png_error(png_ptr, "Unknown interlace method");
   info_ptr->interlace_type = (png_byte)interlace_method;
}
