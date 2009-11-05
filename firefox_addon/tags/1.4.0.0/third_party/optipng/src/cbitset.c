/**
 ** cbitset.c
 ** Simple C routines for bitset handling.
 **
 ** Copyright (C) 2001-2006 Cosmin Truta.
 **
 ** This software is distributed under the same licensing and warranty
 ** terms as OptiPNG.  Please see the attached LICENSE for more info.
 **/


#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include "cbitset.h"


/* private helper: ASSERT */
#ifndef ASSERT
#ifdef CDEBUG
#include <assert.h>
#define ASSERT(cond) assert(cond)
#else  /* !CDEBUG */
#define ASSERT(cond) ((void)0)
#endif /* ?CDEBUG */
#endif /* ?ASSERT */


/* private helper: SKIP_SPACES */
#if 0
#define SKIP_SPACES(_str)  \
    { while (*(_str) != 0 && isspace(*(_str))) ++(_str); }
#else
#define SKIP_SPACES(_str)  \
    { while (isspace(*(_str))) ++(_str); }
#endif


/**
 * Counts the number of elements in a bitset.
 **/
int bitset_count(bitset_t set)
{
    int result;
    unsigned int i;

    if (BITSET_GET_OVERFLOW(set))
        return -1;

    result = 0;
    for (i = 0; i < BITSET_SIZE; ++i)
        if (BITSET_GET(set, i))
            ++result;
    return result;
}


/**
 * Converts a string to a bitset value.
 **/
bitset_t string_to_bitset(const char *str, char **end_ptr)
{
    bitset_t result;
    const char *ptr;
    int overflow;

    ASSERT(str != NULL);

    ptr = str;
    SKIP_SPACES(ptr);
    if (*ptr != '0' && *ptr != '1')
    {
        if (end_ptr != NULL)
            *end_ptr = (char *)str;
        return BITSET_EMPTY;
    }

    result = BITSET_EMPTY;
    overflow = 0;
    for ( ; ; ++ptr)
    {
        if (*ptr == '0' || *ptr == '1')
        {
            result = (result << 1) | (*ptr - '0');
            if (BITSET_GET_OVERFLOW(result))
                overflow = 1;
        }
        else
        {
            if (end_ptr != NULL)
                *end_ptr = (char *)ptr;
            if (overflow)
                BITSET_SET_OVERFLOW(result);
            return result;
        }
    }
}


/**
 * Converts a bitset value to a string.
 **/
char *bitset_to_string(bitset_t set, char *str_buf, size_t str_buf_size)
{
    char *ptr;
    int i;

    ASSERT(str_buf != NULL);

    for (i = BITSET_SIZE - 1; i > 0; --i)
        if (BITSET_GET(set, i))
            break;
    if ((size_t)(i + 1) >= str_buf_size)
        return NULL;  /* insufficient buffer space */

    ptr = str_buf;
    for ( ; i >= 0; --i)
    {
        /* C ALERT ** (cond ? '1' : '0') is int instead of char */
        *ptr++ = (char)(BITSET_GET(set, i) ? '1' : '0');
    }
    *ptr = 0;
    return str_buf;
}


/**
 * Parses an enumeration string to a bitset value.
 **/
int bitset_parse(const char *text, bitset_t *out_bitset)
{
    unsigned int num1, num2, i;
    int is_range;

    ASSERT(text != NULL);
    ASSERT(out_bitset != NULL);

    *out_bitset = BITSET_EMPTY;
    for ( ; ; ++text)
    {
        SKIP_SPACES(text);
        if (*text == 0)
            return 0;  /* success */

        num1 = UINT_MAX;  /* unassigned */
        num2 = 0;
        is_range = 0;
        while ((*text >= '0' && *text <= '9') || (*text == '-'))
        {
            if (*text == '-')  /* range */
            {
                is_range = 1;
                if (num1 == UINT_MAX)
                    num1 = 0;  /* default */
                num2 = BITSET_SIZE - 1;  /* default */
                ++text;
            }
            else  /* number */
            {
                for (num2 = 0; *text >= '0' && *text <= '9'; ++text)
                {
                    num2 = 10 * num2 + (*text - '0');
                    if (num2 > BITSET_SIZE)  /* overflow protection */
                        num2 = BITSET_SIZE;
                }
                if (!is_range)
                    num1 = num2;
            }
            SKIP_SPACES(text);
        }

        if (num2 >= BITSET_SIZE)
        {
            num2 = BITSET_SIZE - 1;
            BITSET_SET_OVERFLOW(*out_bitset);
        }
        for (i = num1; i <= num2; ++i)
            BITSET_SET(*out_bitset, i);

        if (*text == 0)
            return 0;  /* success */
        if (*text != ',' && *text != ';')  /* not a separator */
            return -1;  /* failure */
    }
}


/**
 * Converts a bitset value to a parsable enumeration string.
 **/
char *bitset_deparse(bitset_t set, char *text_buf, size_t text_buf_size);
/* not implemented */
