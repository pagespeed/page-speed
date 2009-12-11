/*
 * pngxio.c - libpng extension: I/O state query.
 *
 * Copyright (C) 2001-2008 Cosmin Truta.
 * This software is distributed under the same licensing and warranty terms
 * as libpng.
 *
 * NOTE:
 * The functionality provided in this module has "graduated", and is now
 * part of libpng-1.4.  The original code is used here as a back-port, for
 * compatibility with libpng-1.2 and earlier.  However, it has limitations:
 * is thread-unsafe and only supports one reading and one writing png_ptr.
 * (The libpng-1.4 code is much simpler and does not have these limitations,
 * due to the presence of io_state inside png_struct.)
 *
 * For more info, see pngx.h.
 */

#define PNG_INTERNAL
#include "png.h"
#include "pngx.h"


#if PNG_LIBPNG_VER < 10400


/* Here comes the kludge... */

static png_structp
pngx_priv_read_ptr, pngx_priv_write_ptr;

static png_rw_ptr
pngx_priv_read_fn, pngx_priv_write_fn;

static int
pngx_priv_read_io_state, pngx_priv_write_io_state;

static png_byte
pngx_priv_read_crt_chunk_hdr[8], pngx_priv_write_crt_chunk_hdr[8];

static unsigned int
pngx_priv_read_crt_chunk_hdr_len, pngx_priv_write_crt_chunk_hdr_len;

static png_uint_32
pngx_priv_read_crt_len, pngx_priv_write_crt_len;

static const char *pngx_priv_errmsg =
   "Internal PNGXIO error: incorrect use of the pngx_ functions";


/* It's better to avoid direct access to the libpng internal structures,
 * considering that optipng.c doesn't currently use pngx_get_io_chunk_name().
 */
#define PNGXIO_NO_CHUNK_NAME


/* Update io_state and call the user-supplied read/write functions. */
void /* PRIVATE */
pngx_priv_read_write(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_rw_ptr io_data_fn;
   int *io_state_ptr, io_state_op;
   png_bytep io_crt_chunk_hdr;
   unsigned int *io_crt_chunk_hdr_len_ptr;
   png_uint_32 *io_crt_len_ptr;

   if (png_ptr == pngx_priv_read_ptr)
   {
      io_data_fn = pngx_priv_read_fn;
      io_state_ptr = &pngx_priv_read_io_state;
      io_state_op = PNGX_IO_READING;
      io_crt_chunk_hdr = pngx_priv_read_crt_chunk_hdr;
      io_crt_chunk_hdr_len_ptr = &pngx_priv_read_crt_chunk_hdr_len;
      io_crt_len_ptr = &pngx_priv_read_crt_len;
   }
   else if (png_ptr == pngx_priv_write_ptr)
   {
      io_data_fn = pngx_priv_write_fn;
      io_state_ptr = &pngx_priv_write_io_state;
      io_state_op = PNGX_IO_WRITING;
      io_crt_chunk_hdr = pngx_priv_write_crt_chunk_hdr;
      io_crt_chunk_hdr_len_ptr = &pngx_priv_write_crt_chunk_hdr_len;
      io_crt_len_ptr = &pngx_priv_write_crt_len;
   }
   else
   {
      png_error(png_ptr, pngx_priv_errmsg);
      return;
   }

   switch (*io_state_ptr & PNGX_IO_MASK_LOC)
   {
   case PNGX_IO_SIGNATURE:
      /* The signature must be serialized in a single I/O session.
       * (This limitation is imposed for simplicity reasons.)
       */
      PNGX_ASSERT(length <= 8);
      io_data_fn(png_ptr, data, length);
      *io_state_ptr = io_state_op | PNGX_IO_CHUNK_HDR;
      *io_crt_chunk_hdr_len_ptr = 0;
      return;
   case PNGX_IO_CHUNK_HDR:
      /* The chunk header may be serialized in multiple I/O sessions.
       * (For performance reasons, libpng should do it in a single session.)
       */
      PNGX_ASSERT(length + *io_crt_chunk_hdr_len_ptr <= 8);
      if (io_state_op == PNGX_IO_READING)
      {
         if (*io_crt_chunk_hdr_len_ptr == 0)
            io_data_fn(png_ptr, io_crt_chunk_hdr, 8);
         memcpy(data, io_crt_chunk_hdr + *io_crt_chunk_hdr_len_ptr, length);
         *io_crt_chunk_hdr_len_ptr += length;
         if (*io_crt_chunk_hdr_len_ptr < 8)
            return;
         *io_crt_len_ptr = png_get_uint_32(io_crt_chunk_hdr);
#ifndef PNGXIO_NO_CHUNK_NAME
         memcpy(png_ptr->chunk_name, io_crt_chunk_hdr + 4, 4);
#endif
      }
      else  /* io_state_op == PNGX_IO_WRITING */
      {
         memcpy(io_crt_chunk_hdr + *io_crt_chunk_hdr_len_ptr, data, length);
         *io_crt_chunk_hdr_len_ptr += length;
         if (*io_crt_chunk_hdr_len_ptr < 8)
            return;
         *io_crt_len_ptr = png_get_uint_32(io_crt_chunk_hdr);
#ifndef PNGXIO_NO_CHUNK_NAME
         memcpy(png_ptr->chunk_name, io_crt_chunk_hdr + 4, 4);
#endif
         io_data_fn(png_ptr, io_crt_chunk_hdr, 8);
      }
      *io_crt_chunk_hdr_len_ptr = 0;
      *io_state_ptr = io_state_op | PNGX_IO_CHUNK_DATA;
      return;
   case PNGX_IO_CHUNK_DATA:
      /* Chunk data may be serialized in multiple I/O sessions. */
      if (length == 0)
         return;
      if (*io_crt_len_ptr > 0)
      {
         PNGX_ASSERT(length <= *io_crt_len_ptr);
         io_data_fn(png_ptr, data, length);
         if ((*io_crt_len_ptr -= length) == 0)
            *io_state_ptr = io_state_op | PNGX_IO_CHUNK_CRC;
         return;
      }
      /* ... else go to the next case. */
   case PNGX_IO_CHUNK_CRC:
      /* The CRC must be serialized in a single I/O session.
       * (libpng already complies to this.)
       */
      PNGX_ASSERT(length == 4);
      io_data_fn(png_ptr, data, 4);
      *io_state_ptr = io_state_op | PNGX_IO_CHUNK_HDR;
      return;
   }
}


/* In libpng-1.4, the implementation of this function simply retrieves
 * png_ptr->io_state.
 */
png_uint_32 PNGAPI
pngx_get_io_state(png_structp png_ptr)
{
   /* CAUTION: ugly code. */
   /* This mess will disappear when io_state is added to png_ptr. */
   if (png_ptr == pngx_priv_read_ptr)
      return pngx_priv_read_io_state;
   else if (png_ptr == pngx_priv_write_ptr)
      return pngx_priv_write_io_state;
   png_error(png_ptr, pngx_priv_errmsg);
   /* Never get here... */
   return PNGX_IO_NONE;
}

/* In libpng-1.4, the implementation of this function simply retrieves
 * png_ptr->chunk_name.
 */
png_bytep PNGAPI
pngx_get_io_chunk_name(png_structp png_ptr)
{
#ifndef PNGXIO_NO_CHUNK_NAME
   return png_ptr->chunk_name;
#else
   png_error(png_ptr,
      "[internal error] pngx_get_io_chunk_name() is not implemented");
   return NULL;
#endif
}


/* The following functions are wrapped around the libpng-supplied ones,
 * in order to enable the new libpng-1.4 functionality (implemented above)
 * in the older libpng versions.
 */
void PNGAPI
pngx_set_read_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr read_data_fn)
{
   pngx_priv_read_ptr = png_ptr;
   pngx_priv_write_ptr = NULL;
   pngx_priv_read_fn = read_data_fn;
   png_set_read_fn(png_ptr, io_ptr, pngx_priv_read_write);
   pngx_priv_read_io_state = PNGX_IO_READING | PNGX_IO_SIGNATURE;
}

void PNGAPI
pngx_set_write_fn(png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
   pngx_priv_write_ptr = png_ptr;
   pngx_priv_read_ptr = NULL;
   pngx_priv_write_fn = write_data_fn;
   png_set_write_fn(png_ptr, io_ptr, pngx_priv_read_write, output_flush_fn);
   pngx_priv_write_io_state = PNGX_IO_WRITING | PNGX_IO_SIGNATURE;
}

void PNGAPI
pngx_write_sig(png_structp png_ptr)
{
#if 0  /* png_write_sig is not exported from libpng-1.2. */
   png_write_sig(png_ptr);
   /* TODO: Add png_write_sig to the list of libpng-1.2 exports.
    * This would complement well the group png_write_chunk{_start,_data,_end}.
    */
#else
   static png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
   pngx_priv_read_write(png_ptr, png_signature, 8);
   /* TODO: Take png_ptr->sig_bytes into account. */
#endif
}


#endif /* PNG_LIBPNG_VER < 10400 */
