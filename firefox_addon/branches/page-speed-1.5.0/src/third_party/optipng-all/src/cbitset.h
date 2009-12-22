/**
 ** cbitset.h
 ** Simple C routines for bitset handling.
 **
 ** Copyright (C) 2001-2006 Cosmin Truta.
 **
 ** This software is distributed under the same licensing and warranty
 ** terms as OptiPNG.  Please see the attached LICENSE for more info.
 **/


#ifndef CBITSET_H
#define CBITSET_H

#include <limits.h>
#include <stddef.h>


/**
 * The bitset type.
 **/
typedef int bitset_t;


/**
 * Macros for bitset handling.
 **/
#define BITSET_SIZE     (CHAR_BIT * sizeof(bitset_t) - 1)
#define BITSET_EMPTY    (0)
#define BITSET_FULL     ((1 << BITSET_SIZE) - 1)

#define BITSET_GET(_set, _item)   \
    (((_set) &  (1 << (_item))) != 0)
#define BITSET_RESET(_set, _item) \
    ((_set) &= ~(1 << (_item)))
#define BITSET_SET(_set, _item)   \
    ((_set) |=  (1 << (_item)))

#define BITSET_GET_OVERFLOW(_set)   \
    ((_set) < 0)
#define BITSET_RESET_OVERFLOW(_set) \
    ((_set) &= ~(1 << BITSET_SIZE))
#define BITSET_SET_OVERFLOW(_set)   \
    ((_set) |=  (1 << BITSET_SIZE))


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Counts the number of elements in a bitset.
 * If there is no overflow, the function returns the number of '1' bits.
 * Otherwise, it returns -1.
 **/
int bitset_count(bitset_t set);


/**
 * Converts a string to a bitset value.
 * The leading and trailing space characters are ignored.
 * The function stops at the first character it doesn't recognize
 * (i.e. something different than '0' or '1').
 * If end_ptr is not null, the function sets *end_ptr to point to
 * the character that stopped the scan.
 * If the input is valid, the function returns the value of the
 * converted bitset.  Otherwise, it returns BITSET_EMPTY.
 **/
bitset_t string_to_bitset(const char *str, char **end_ptr);
bitset_t wstring_to_bitset(const wchar_t *str, wchar_t **end_ptr);


/**
 * Converts a bitset value to a string.
 * If there is enough space in str_buf, the function returns str_buf.
 * Otherwise, it returns NULL.
 * The overflow flag is ignored.
 **/
char *bitset_to_string(bitset_t set, char *str_buf, size_t str_buf_size);
wchar_t *bitset_to_wstring(bitset_t set, wchar_t *str_buf, size_t str_buf_size);


/**
 * Parses an enumeration string to a bitset value.
 * A valid input may contain digits and the separators '-', ',', ';',
 * and it must match the regular expression:
 *  "[-0-9,;]*"
 *
 * The following examples assume BITSET_SIZE == 15:
 *  ""        => 000000000000000
 *  "0-2,4-5" => 000000000110111
 *  "-3,5,7-" => 111111110101111
 *  "9-,6"    => 111111001000000
 *  "8-4"     => 000000000000000
 *  "-"       => 111111111111111
 *
 * The leading and trailing space characters are ignored.
 * If the input is valid, the function returns 0.
 * Otherwise, it returns a non-zero error code.
 **/
int bitset_parse(const char *text, bitset_t *out_bitset);
#if 0
int bitset_parse(const wchar_t *text, bitset_t *out_bitset);
#endif


/**
 * Converts a bitset value to a parsable enumeration string.
 * If the input is valid, the function return text_buf.
 * Otherwise, it returns NULL.
 * The space allocated for text_buf must be large enough to hold
 * the returned string, including the terminating null character.
 **/
char *bitset_deparse(bitset_t set, char *text_buf, size_t text_buf_size);
#if 0
wchar_t *bitset_deparse(bitset_t set, wchar_t *text_buf, size_t text_buf_size);
#endif


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* CBITSET_H */
