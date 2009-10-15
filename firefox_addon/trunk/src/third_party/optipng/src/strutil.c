/**
 ** strutil.c
 ** General-purpose string manipulation utilities.
 **
 ** Copyright (C) 2001-2006 Cosmin Truta.
 **
 ** This software is distributed under the same licensing and warranty
 ** terms as OptiPNG.  Please see the attached LICENSE for more info.
 **
 ** TO DO: Implement the corresponding wide-char functions.
 **/


#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include "strutil.h"


#if !defined(HAVE_STRCASECMP)
#if !defined(HAVE_STRICMP) && !defined(HAVE__STRICMP)
#if !defined(HAVE_STRCMPI)
/**
 * Compares two strings without case sensitivity.
 **/
int string_case_cmp(const char *str1, const char *str2)
{
    int ch1, ch2;

    for ( ; ; )
    {
        /* Use toupper, not tolower,
         * in case that two lowercase letters correspond to the same uppercase.
         */
        ch1 = toupper(*str1++);
        ch2 = toupper(*str2++);
        if (ch1 != ch2)
            return ch1 - ch2;
        if (ch1 == 0)  /* no need to check if ch2 == 0 */
            return 0;
    }
}
#endif  /* !HAVE_STRCMPI */
#endif  /* !HAVE_STRICMP && !HAVE__STRICMP */
#endif  /* !HAVE_STRCASECMP */


#if !defined(HAVE_STRNCASECMP)
#if !defined(HAVE_STRNICMP) && !defined(HAVE__STRNICMP)
#if !defined(HAVE_STRNCMPI)
/**
 * Compares portions of two strings without case sensitivity.
 **/
int string_num_case_cmp(const char *str1, const char *str2, size_t num)
{
    int ch1, ch2;

    for ( ; num > 0; num--)
    {
        /* Use toupper, not tolower,
         * in case that two lowercase letters correspond to the same uppercase.
         */
        ch1 = toupper(*str1++);
        ch2 = toupper(*str2++);
        if (ch1 != ch2)
            return ch1 - ch2;
        if (ch1 == 0)  /* no need to check if ch2 == 0 */
            return 0;
    }
    return 0;
}
#endif  /* !HAVE_STRNCMPI */
#endif  /* !HAVE_STRNICMP && !HAVE__STRNICMP */
#endif  /* !HAVE_STRNCASECMP */


#if !defined(HAVE_STRLWR) && !defined(HAVE__STRLWR)
/**
 * Converts the letters in a string to lowercase.
 **/
char *string_lower(char *str)
{
    char *sptr;

    for (sptr = str; *sptr != 0; sptr++)
        *sptr = (char)tolower(*sptr);
    return str;
}
#endif  /* !HAVE_STRLWR && !HAVE__STRLWR */


#if !defined(HAVE_STRUPR) && !defined(HAVE__STRUPR)
/**
 * Converts the letters in a string to uppercase.
 **/
char *string_upper(char *str)
{
    char *sptr;

    for (sptr = str; *sptr != 0; sptr++)
        *sptr = (char)toupper(*sptr);
    return str;
}
#endif  /* !HAVE_STRUPR && !HAVE__STRUPR */


/**
 * Checks if the given string has the given prefix, with case sensitivity.
 **/
int string_prefix_cmp(const char *str, const char *prefix)
{
    int cs, cp;

    while ((cp = *prefix++) != 0)
    {
        if ((cs = *str++) == 0)
            return -1;  /* str is shorter than prefix */
        if (cs < cp)
            return -1;
        if (cs > cp)
            return 1;
    }
    return 0;
}


/**
 * Checks if the given string has the given prefix, without case sensitivity.
 **/
int string_prefix_case_cmp(const char *str, const char *prefix)
{
    int cs, cp;

    while ((cp = toupper(*prefix++)) != 0)
    {
        if ((cs = toupper(*str++)) == 0)
            return -1;  /* str is shorter than prefix */
        if (cs < cp)
            return -1;
        if (cs > cp)
            return 1;
    }
    return 0;
}


/**
 * Checks if the given string has the given prefix, with case sensitivity.
 **/
int string_prefix_min_cmp(const char *str, const char *prefix, size_t minlen)
{
    int cs, cp;
    size_t matchlen;

    matchlen = 0;
    while ((cp = *prefix++) != 0)
    {
        if ((cs = *str++) == 0)
            return -1;  /* str is shorter than prefix */
        if (cs == cp)
            matchlen++;
        else
            return (cs < cp) ? -1 : 1;  /* different characters */
    }
    return (matchlen >= minlen) ? 0 : 1;
}


/**
 * Checks if the given string has the given prefix, without case sensitivity.
 **/
int
string_prefix_min_case_cmp(const char *str, const char *prefix, size_t minlen)
{
    int cs, cp;
    size_t matchlen;

    matchlen = 0;
    while ((cp = toupper(*prefix++)) != 0)
    {
        if ((cs = toupper(*str++)) == 0)
            return -1;  /* str is shorter than prefix */
        if (cs == cp)
            matchlen++;
        else
            return (cs < cp) ? -1 : 1;  /* different characters */
    }
    return (matchlen >= minlen) ? 0 : 1;
}


/**
 * Checks if the given string has the given suffix, with case sensitivity.
 **/
int string_suffix_cmp(const char *str, const char *suffix)
{
    size_t str_len, suffix_len;

    str_len = strlen(str);
    suffix_len = strlen(suffix);
    if (str_len < suffix_len)
        return -1;  /* str is shorter than suffix */
    return strcmp(str + str_len - suffix_len, suffix);
}


/**
 * Checks if the given string has the given suffix, without case sensitivity.
 **/
int string_suffix_case_cmp(const char *str, const char *suffix)
{
    size_t str_len, suffix_len;

    str_len = strlen(str);
    suffix_len = strlen(suffix);
    if (str_len < suffix_len)
        return -1;  /* str is shorter than suffix */
    return string_case_cmp(str + str_len - suffix_len, suffix);
}
