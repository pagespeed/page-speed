/**
 ** optipng.h
 ** OptiPNG programming interface.
 **
 ** Copyright (C) 2001-2009 Cosmin Truta.
 **
 ** This software is distributed under the zlib license.
 ** Please see the attached LICENSE for more information.
 **/


#ifndef OPTIPNG_H
#define OPTIPNG_H

#include "cbitset.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Program options (e.g. extracted from the command line)
 */
struct opng_options
{
    int help;
    int fix;
    int force;
    int full;
    int interlace;
    int keep;
    int nb, nc, np, nz;
    int preserve;
    int quiet;
    int simulate;
    int snip;
    int verbose;
    int version;
    int optim_level;
    bitset_t compr_level_set;
    bitset_t mem_level_set;
    bitset_t strategy_set;
    bitset_t filter_set;
    int window_bits;
    char *out_name;
    char *dir_name;
    char *log_name;
};


/*
 * Program UI callbacks
 */
struct opng_ui
{
    void (*printf_fn)(const char *fmt, ...);
    void (*print_cntrl_fn)(int cntrl_code);
    void (*progress_fn)(unsigned long current_step, unsigned long total_steps);
    void (*panic_fn)(const char *msg);
};


/*
 * Engine initialization
 */
int opng_initialize(const struct opng_options *options,
                    const struct opng_ui *ui);


/*
 * Engine execution
 */
int opng_optimize(const char *infile_name);


/*
 * Engine finalization
 */
int opng_finalize(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* OPTIPNG_H */
