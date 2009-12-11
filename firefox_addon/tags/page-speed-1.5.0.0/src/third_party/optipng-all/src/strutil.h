/**
 ** strutil.h
 ** General-purpose string manipulation utilities.
 **
 ** Copyright (C) 2001-2006 Cosmin Truta.
 **
 ** This software is distributed under the same licensing and warranty
 ** terms as OptiPNG.  Please see the attached LICENSE for more info.
 **
 ** TO DO: Implement the corresponding wide-char functions.
 **/


#ifndef STRUTIL_H
#define STRUTIL_H

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************/
/* Auto-configuration                                                        */
/*****************************************************************************/


#if defined(UNIX) || defined(__unix) || defined(__unix__)
#  ifndef HAVE_STRINGS_H
#    define HAVE_STRINGS_H
#  endif
#endif

#if defined(HAVE_STRINGS_H)
#  ifndef HAVE_STRCASECMP
#    define HAVE_STRCASECMP
#  endif
#  ifndef HAVE_STRNCASECMP
#    define HAVE_STRNCASECMP
#  endif
#endif

#if defined(_MSDOS) || defined(_WIN32)
#  ifndef HAVE__STRICMP
#    define HAVE__STRICMP
#  endif
#  ifndef HAVE__STRNICMP
#    define HAVE__STRNICMP
#  endif
#  ifndef HAVE__STRLWR
#    define HAVE__STRLWR
#  endif
#  ifndef HAVE__STRUPR
#    define HAVE__STRUPR
#  endif
#endif

#if defined(__MSDOS__) || defined(__OS2__) || defined(__WIN32__)
#  ifndef HAVE_STRICMP
#    define HAVE_STRICMP
#  endif
#  ifndef HAVE_STRNICMP
#    define HAVE_STRNICMP
#  endif
#  ifndef HAVE_STRLWR
#    define HAVE_STRLWR
#  endif
#  ifndef HAVE_STRUPR
#    define HAVE_STRUPR
#  endif
#endif

#include <stddef.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif


/*****************************************************************************/
/* Definitions and prototypes                                                */
/*                                                                           */
/* It's unfortunate that ISO C has tolower and toupper,                      */
/* but it does not have stricmp, strnicmp, strlower or strupper.             */
/*****************************************************************************/


/**
 * Compares two strings without case sensitivity.
 **/
#if defined(HAVE_STRCASECMP)
#  define string_case_cmp(str1, str2) strcasecmp(str1, str2)
#elif defined(HAVE_STRICMP)
#  define string_case_cmp(str1, str2) stricmp(str1, str2)
#elif defined(HAVE__STRICMP)
#  define string_case_cmp(str1, str2) _stricmp(str1, str2)
#elif defined(HAVE_STRCMPI)
#  define string_case_cmp(str1, str2) strcmpi(str1, str2)
#else
int string_case_cmp(const char *str1, const char *str2);
#endif


/**
 * Compares portions of two strings without case sensitivity.
 **/
#if defined(HAVE_STRNCASECMP)
#  define string_num_case_cmp(str1, str2, num) strncasecmp(str1, str2, num)
#elif defined(HAVE_STRNICMP)
#  define string_num_case_cmp(str1, str2, num) strnicmp(str1, str2, num)
#elif defined(HAVE__STRNICMP)
#  define string_num_case_cmp(str1, str2, num) _strnicmp(str1, str2, num)
#elif defined(HAVE_STRNCMPI)
#  define string_num_case_cmp(str1, str2, num) strncmpi(str1, str2, num)
#else
int string_num_case_cmp(const char *str1, const char *str2, size_t num);
#endif


/**
 * Converts the letters in a string to lowercase.
 **/
#if defined(HAVE_STRLWR)
#  define string_lower(str) strlwr(str)
#elif defined(HAVE__STRLWR)
#  define string_lower(str) _strlwr(str)
#else
char *string_lower(char *str);
#endif


/**
 * Converts the letters in a string to uppercase.
 **/
#if defined(HAVE_STRUPR)
#  define string_upper(str) strupr(str)
#elif defined(HAVE__STRUPR)
#  define string_upper(str) _strupr(str)
#else
char *string_upper(char *str);
#endif


/**
 * Checks if the given string has the given prefix,
 * with/without case sensitivity.
 * Returns  0  if "prefix" is a prefix of "str"; otherwise:
 *         -1  if "str" is lexicographically smaller than "prefix";
 *          1  if "str" is lexicographically bigger than "prefix".
 **/
int string_prefix_cmp(const char *str, const char *prefix);
int string_prefix_case_cmp(const char *str, const char *prefix);


/**
 * Checks if the given string has the given prefix,
 * with/without case sensitivity.
 * Returns  0  if "prefix" is a prefix of "str" and has
 *             at least "minlen" characters; otherwise:
 *         -1  if "str" is lexicographically smaller than "prefix";
 *          1  if "str" is lexicographically bigger than "prefix".
 **/
int string_prefix_min_cmp(const char *str, const char *prefix, size_t minlen);
int string_prefix_min_case_cmp(const char *str, const char *prefix,
                               size_t minlen);


/**
 * Checks if the given string has the given suffix,
 * with/without case sensitivity.
 * Returns  0  if "suffix" is a suffix of "str"; otherwise:
 *         -1  if "str" is shorter or lexicographically smaller than "suffix";
 *          1  if "str" is lexicographically bigger than "suffix".
 **/
int string_suffix_cmp(const char *str, const char *suffix);
int string_suffix_case_cmp(const char *str, const char *suffix);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* STRUTIL_H */
