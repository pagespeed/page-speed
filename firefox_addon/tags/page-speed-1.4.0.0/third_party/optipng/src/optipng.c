/**
 ** OptiPNG: Advanced PNG optimization program.
 ** http://optipng.sourceforge.net/
 **
 ** Copyright (C) 2001-2009 Cosmin Truta.
 **
 ** This software is distributed under the zlib license.
 ** Please see the attached LICENSE for more information.
 **
 ** PNG optimization is described in detail in the PNG-Tech article
 ** "A guide to PNG optimization"
 ** http://www.cs.toronto.edu/~cosmin/pngtech/optipng.html
 **
 ** The idea of running multiple compression trials with different
 ** PNG filters and zlib parameters is inspired from the pngcrush
 ** program by Glenn Randers-Pehrson.
 ** The idea of performing lossless image reductions is inspired from
 ** the pngrewrite program by Jason Summers.
 **
 ** Requirements:
 **    ANSI C89, ISO C90 or ISO C99 compiler and library.
 **    zlib version 1.2.1 or newer.
 **    libpng version 1.2.9 or newer, with pngxtern.
 **    cexcept version 2.0.1 or newer.
 **    POSIX or Windows API for enhanced functionality.
 **/

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proginfo.h"
#include "optipng.h"
#include "cbitset.h"
#include "osys.h"
#include "strutil.h"
#include "png.h"
#include "zlib.h"


static const char *msg_intro =
   PROGRAM_NAME " " PROGRAM_VERSION ": " PROGRAM_DESCRIPTION ".\n"
   PROGRAM_COPYRIGHT ".\n\n";

static const char *msg_license =
   "This program is open-source software. See LICENSE for more details.\n"
   "\n"
   "Portions of this software are based in part on the work of:\n"
   "  Jean-loup Gailly and Mark Adler (zlib)\n"
   "  Glenn Randers-Pehrson and the PNG Development Group (libpng)\n"
   "  Miyasaka Masaru (BMP support)\n"
   "  David Koblas (GIF support)\n"
   "\n";

static const char *msg_short_help =
   "Synopsis:\n"
   "    optipng [options] files ...\n"
   "Files:\n"
   "    Image files of type: PNG, BMP, GIF, PNM or TIFF\n"
   "Basic options:\n"
   "    -?, -h, -help\tshow the extended help\n"
   "    -o <level>\t\toptimization level (0-7)\t\tdefault 2\n"
   "    -v\t\t\tverbose mode / show copyright and version info\n"
   "Examples:\n"
   "    optipng file.png\t\t\t(default speed)\n"
   "    optipng -o5 file.png\t\t(moderately slow)\n"
   "    optipng -o7 file.png\t\t(very slow)\n"
   "Type \"optipng -h\" for extended help.\n";

static const char *msg_help =
   "Synopsis:\n"
   "    optipng [options] files ...\n"
   "Files:\n"
   "    Image files of type: PNG, BMP, GIF, PNM or TIFF\n"
   "Basic options:\n"
   "    -?, -h, -help\tshow this help\n"
   "    -o <level>\t\toptimization level (0-7)\t\tdefault 2\n"
   "    -v\t\t\tverbose mode / show copyright and version info\n"
   "General options:\n"
   "    -fix\t\tenable error recovery\n"
   "    -force\t\tenforce writing of a new output file\n"
#if 0  /* parallel processing is not implemented */
   "    -jobs <number>\tallow parallel jobs\n"
#endif
   "    -keep\t\tkeep a backup of the modified files\n"
   "    -preserve\t\tpreserve file attributes if possible\n"
   "    -quiet\t\tquiet mode\n"
   "    -simulate\t\tsimulation mode\n"
   "    -snip\t\tcut one image out of multi-image or animation files\n"
#if 0  /* multi-image splitting is not implemented */
   "    -split\t\tsplit multi-image/animation files into separate images\n"
#endif
   "    -out <file>\t\twrite output file to <file>\n"
   "    -dir <directory>\twrite output file(s) to <directory>\n"
   "    -log <file>\t\tlog messages to <file>\n"
   "    --\t\t\tstop option switch parsing\n"
   "Optimization options:\n"
#if 0  /* not implemented */
   "    -b  <depth>\t\tbit depth (1,2,4,8,16)\t\t\tdefault <min>\n"
   "    -c  <type>\t\tcolor type (0,2,3,4,6)\t\t\tdefault <input>\n"
#endif
   "    -f  <filters>\tPNG delta filters (0-5)\t\t\tdefault 0,5\n"
   "    -i  <type>\t\tPNG interlace type (0-1)\t\tdefault <input>\n"
   "    -zc <levels>\tzlib compression levels (1-9)\t\tdefault 9\n"
   "    -zm <levels>\tzlib memory levels (1-9)\t\tdefault 8\n"
   "    -zs <strategies>\tzlib compression strategies (0-3)\tdefault 0-3\n"
   "    -zw <window size>\tzlib window size (32k,16k,8k,4k,2k,1k,512,256)\n"
   "    -full\t\tproduce a full report on IDAT (might reduce speed)\n"
   "    -nb\t\t\tno bit depth reduction\n"
   "    -nc\t\t\tno color type reduction\n"
   "    -np\t\t\tno palette reduction\n"
#if 0  /* metadata optimization is not implemented */
   "    -nm\t\t\tno metadata optimization\n"
#endif
   "    -nz\t\t\tno IDAT recompression (also disable reductions)\n"
   "Optimization details:\n"
   "    The optimization level presets\n"
   "        -o0  <=>  -nz\n"
   "        -o1  <=>  [use the libpng heuristics]\t(1 trial)\n"
   "        -o2  <=>  -zc9 -zm8 -zs0-3 -f0,5\t(8 trials)\n"
   "        -o3  <=>  -zc9 -zm8-9 -zs0-3 -f0,5\t(16 trials)\n"
   "        -o4  <=>  -zc9 -zm8 -zs0-3 -f0-5\t(24 trials)\n"
   "        -o5  <=>  -zc9 -zm8-9 -zs0-3 -f0-5\t(48 trials)\n"
   "        -o6  <=>  -zc1-9 -zm8 -zs0-3 -f0-5\t(120 trials)\n"
   "        -o7  <=>  -zc1-9 -zm8-9 -zs0-3 -f0-5\t(240 trials)\n"
   "    The libpng heuristics\n"
   "        -o1  <=>  -zc9 -zm8 -zs0 -f0\t\t(if PLTE is present)\n"
   "        -o1  <=>  -zc9 -zm8 -zs1 -f5\t\t(if PLTE is not present)\n"
   "    The most exhaustive search (not generally recommended)\n"
   "      [no preset] -zc1-9 -zm1-9 -zs0-3 -f0-5\t(1080 trials)\n"
   "Examples:\n"
   "    optipng file.png\t\t\t\t(default speed)\n"
   "    optipng -o5 file.png\t\t\t(moderately slow)\n"
   "    optipng -o7 file.png\t\t\t(very slow)\n"
   "    optipng -i1 -o7 -v -full -sim experiment.png -log experiment.log\n";


static enum { OP_NONE, OP_HELP, OP_RUN } operation;
static struct opng_options options;
static FILE *con_file;
static FILE *log_file;
int start_of_line;


/** Error handling **/
static void
error(const char *fmt, ...)
{
    va_list arg_ptr;

    /* Print the error message to stderr and exit. */
    fprintf(stderr, "** Error: ");
    va_start(arg_ptr, fmt);
    vfprintf(stderr, fmt, arg_ptr);
    va_end(arg_ptr);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}


/** Panic handling **/
static void
panic(const char *msg)
{
    /* Print the panic message to stderr and terminate abnormally. */
    fprintf(stderr, "\n** INTERNAL ERROR: %s\n", msg);
    fprintf(stderr, "Please submit a defect report.\n");
    fprintf(stderr, PROGRAM_URI "\n\n");
    fflush(stderr);
    osys_terminate();
}


/** String-to-integer conversion **/
static long
str2long(const char *str, long *value)
{
    char *endptr;

    /* Extract the value from the string. */
    *value = strtol(str, &endptr, 10);
    if (endptr == NULL || endptr == str)
    {
        errno = EINVAL;  /* matching failure */
        return -1;
    }

    /* Check for the 'kilo' suffix. */
    if (*endptr == 'k' || *endptr == 'K')
    {
        ++endptr;
        if (*value > LONG_MAX / 1024)
        {
            errno = ERANGE;  /* overflow */
            *value = LONG_MAX;
        }
        else if (*value < LONG_MIN / 1024)
        {
            errno = ERANGE;  /* overflow */
            *value = LONG_MIN;
        }
        else
            *value *= 1024;
    }

    /* Check for trailing garbage. */
    while (isspace(*endptr))
        ++endptr;  /* skip whitespace */
    if (*endptr != 0)
    {
        errno = EINVAL;  /* garbage in input */
        return -1;
    }

    return 0;
}


/** Command line error handling **/
static void
err_option(const char *opt_desc, const char *opt_arg)
{
    /* Issue an error regarding the incorrect use of the option. */
    if (opt_arg != NULL && opt_arg[0] != 0)
        error("Invalid %s: %s", opt_desc, opt_arg);
    else
        error("Missing %s", opt_desc);
}


/** Command line parsing **/
static int
scan_option(char *str, char opt_buf[], size_t opt_buf_size, char **opt_arg_ptr)
{
    char *ptr;
    unsigned int opt_len;

    /* Check if arg is an "-option". */
    if (str[0] != '-' || str[1] == 0)  /* no "-option", or just "-" */
        return 0;

    /* Extract the normalized option, and possibly the option argument. */
    opt_len = 0;
    ptr = str + 1;
    while (*ptr == '-')  /* "--option", "---option", etc. */
        ++ptr;
    if (*ptr == 0)  /* "--" */
        --ptr;
    if (isalpha(*ptr))  /* "-option" */
    {
        do
        {
            if (opt_buf_size > opt_len)  /* truncate "-verylongoption" */
                opt_buf[opt_len] = (char)tolower(*ptr);
            ++opt_len;
            ++ptr;
        } while (isalpha(*ptr) || (*ptr == '-'));  /* "-option", "-option-x" */
        while (*(ptr - 1) == '-')  /* put back trailing '-' in "-option-" */
        {
            --opt_len;
            --ptr;
        }
    }
    else  /* "--", "-@", etc. */
    {
        if (opt_buf_size > 1)
            opt_buf[0] = *ptr;
        opt_len = 1;
        ++ptr;
    }

    /* Finalize the normalized option. */
    if (opt_buf_size > 0)
    {
        if (opt_len < opt_buf_size)
            opt_buf[opt_len] = '\0';
        else
            opt_buf[opt_buf_size - 1] = '\0';
    }
    if (*ptr == '=')  /* "-option=arg" */
        ++ptr;
    else while (isspace(*ptr))  /* "-option arg" */
        ++ptr;
    *opt_arg_ptr = (*ptr != 0) ? ptr : NULL;
    return 1;
}


/** Command line parsing **/
static void
parse_args(int argc, char *argv[])
{
    char opt[16];
    char *arg, *xopt;
    unsigned int file_count;
    int stop_switch, i;
    int val;
    long lval;
    bitset_t set;

    /* Initialize. */
    memset(&options, 0, sizeof(options));
    options.optim_level = -1;
    options.interlace = -1;
    file_count = 0;

    /* Iterate over args. */
    stop_switch = 0;
    for (i = 1; i < argc; ++i)
    {
        arg = argv[i];
        if (stop_switch || scan_option(arg, opt, sizeof(opt), &xopt) < 1)
        {
            ++file_count;
            continue;  /* leave file names for process_files() */
        }

        /* Prevent process_files() from seeing this arg. */
        argv[i] = NULL;

        /* Check the simple options (without option arguments). */
        if (strcmp(opt, "-") == 0)  /* "--" */
        {
            stop_switch = 1;
        }
        else if (strcmp(opt, "?") == 0 ||
                 string_prefix_min_cmp("help", opt, 1) == 0)
        {
            options.help = 1;
        }
        else if (string_prefix_min_cmp("fix", opt, 2) == 0)
        {
            options.fix = 1;
        }
        else if (string_prefix_min_cmp("force", opt, 2) == 0)
        {
            options.force = 1;
        }
        else if (string_prefix_min_cmp("full", opt, 2) == 0)
        {
            options.full = 1;
        }
        else if (string_prefix_min_cmp("keep", opt, 1) == 0)
        {
            options.keep = 1;
        }
        else if (strcmp(opt, "nb") == 0)
        {
            options.nb = 1;
        }
        else if (strcmp(opt, "nc") == 0)
        {
            options.nc = 1;
        }
#if 0
        else if (strcmp(opt, "nm") == 0)
        {
            options.nm = 1;
        }
#endif
        else if (strcmp(opt, "np") == 0)
        {
            options.np = 1;
        }
        else if (strcmp(opt, "nz") == 0)
        {
            options.nz = 1;
        }
        else if (string_prefix_min_cmp("preserve", opt, 1) == 0)
        {
            options.preserve = 1;
        }
        else if (string_prefix_min_cmp("quiet", opt, 1) == 0)
        {
            options.quiet = 1;
        }
        else if (string_prefix_min_cmp("simulate", opt, 2) == 0)
        {
            options.simulate = 1;
        }
        else if (string_prefix_min_cmp("snip", opt, 2) == 0)
        {
            options.snip = 1;
        }
        else if (strcmp(opt, "v") == 0)
        {
            options.verbose = 1;
            options.version = 1;
        }
        else if (string_prefix_min_cmp("verbose", opt, 4) == 0)
        {
            options.verbose = 1;
        }
        else if (string_prefix_min_cmp("version", opt, 4) == 0)
        {
            options.version = 1;
        }
        else  /* possibly an option with an argument */
        {
            if (xopt == NULL)
            {
                if (++i < argc)
                {
                    xopt = argv[i];
                    /* Prevent process_files() from seeing this xopt. */
                    argv[i] = NULL;
                }
                else
                    xopt = "";
            }
        }

        /* Check the options that have option arguments. */
        if (xopt == NULL)
        {
            /* Do nothing, an option without argument is already recognized. */
        }
        else if (strcmp(opt, "o") == 0)
        {
            if (str2long(xopt, &lval) != 0 || lval < 0 || lval > 99)
                err_option("optimization level", xopt);
            val = (int)lval;
            if (options.optim_level < 0)
                options.optim_level = val;
            else if (options.optim_level != val)
                error("Multiple optimization levels are not permitted");
        }
        else if (strcmp(opt, "i") == 0)
        {
            if (str2long(xopt, &lval) != 0 || lval < 0 || lval > 1)
                err_option("interlace type", xopt);
            val = (int)lval;
            if (options.interlace < 0)
                options.interlace = (int)val;
            else if (options.interlace != (int)val)
                error("Multiple interlace types are not permitted");
        }
        else if (strcmp(opt, "b") == 0)
        {
            /* options.bit_depth = ... */
            error("Selection of bit depth is not implemented");
        }
        else if (strcmp(opt, "c") == 0)
        {
            /* options.color_type = ... */
            error("Selection of color type is not implemented");
        }
        else if (strcmp(opt, "f") == 0)
        {
            if (bitset_parse(xopt, &set) != 0)
                err_option("filter(s)", xopt);
            options.filter_set |= set;
        }
        else if (strcmp(opt, "zc") == 0)
        {
            if (bitset_parse(xopt, &set) != 0)
                err_option("zlib compression level(s)", xopt);
            options.compr_level_set |= set;
        }
        else if (strcmp(opt, "zm") == 0)
        {
            if (bitset_parse(xopt, &set) != 0)
                err_option("zlib memory level(s)", xopt);
            options.mem_level_set |= set;
        }
        else if (strcmp(opt, "zs") == 0)
        {
            if (bitset_parse(xopt, &set) != 0)
                err_option("zlib compression strategy", xopt);
            options.strategy_set |= set;
        }
        else if (strcmp(opt, "zw") == 0)
        {
            if (str2long(xopt, &lval) != 0)
                lval = 0;
            for (val = 15; val >= 8; --val)
                if ((1L << val) == lval)
                    break;
            if (val < 8)
                err_option("zlib window size", xopt);
            if (options.window_bits == 0)
                options.window_bits = val;
            else if (options.window_bits != val)
                error("Multiple window sizes are not permitted");
        }
        else if (string_prefix_min_cmp("out", opt, 2) == 0)
        {
            if (options.out_name != NULL)
                error("Duplicate output file name");
            if (xopt[0] == 0)
                err_option("output file name", xopt);
            options.out_name = xopt;
        }
        else if (string_prefix_min_cmp("dir", opt, 1) == 0)
        {
            if (options.dir_name != NULL)
                error("Duplicate output dir name");
            if (xopt[0] == 0)
                err_option("output dir name", xopt);
            options.dir_name = xopt;
        }
        else if (string_prefix_min_cmp("log", opt, 1) == 0)
        {
            if (options.log_name != NULL)
                error("Duplicate log file name");
            if (xopt[0] == 0)
                err_option("log file name", xopt);
            options.log_name = xopt;
        }
        else if (string_prefix_min_cmp("jobs", opt, 1) == 0)
        {
            error("Parallel processing is not implemented");
        }
        else
        {
            error("Unrecognized option: %s", arg);
        }
    }

    /* Finalize. */
    if (options.out_name != NULL)
    {
        if (file_count > 1)
            error("-out requires one input file");
        if (options.dir_name != NULL)
            error("-out and -dir are mutually exclusive");
    }
    if (options.log_name != NULL)
    {
        if (string_suffix_case_cmp(options.log_name, ".log") != 0)
            error("To prevent accidental data corruption,"
                  " the log file name must end with \".log\"");
    }
    if (options.optim_level == 0)
        options.nz = 1;
    if (options.nz)
        options.nb = options.nc = options.np = 1;
    operation = (options.help || file_count == 0) ? OP_HELP : OP_RUN;
}


/** Initialization **/
static void
app_init(void)
{
    /* Initialize the console output. */
    con_file = (!options.quiet || options.help) ? stdout : NULL;

    /* Open the log file, line-buffered, if requested. */
    if (options.log_name != NULL)
    {
        if ((log_file = fopen(options.log_name, "a")) == NULL)
            error("Can't open log file: %s\n", options.log_name);
        setvbuf(log_file, NULL, _IOLBF, BUFSIZ);
    }

    /* Initialize the internal printing routines. */
    start_of_line = 1;
}


/** Finalization **/
static void
app_finish(void)
{
    if (log_file != NULL)
    {
        /* Close the log file. */
        fclose(log_file);
    }
}


/** Application-defined printf callback **/
static void
app_printf(const char *fmt, ...)
{
    va_list arg_ptr;

    if (fmt[0] == 0)
        return;
    start_of_line = (fmt[strlen(fmt) - 1] == '\n') ? 1 : 0;

    if (con_file != NULL)
    {
        va_start(arg_ptr, fmt);
        vfprintf(con_file, fmt, arg_ptr);
        va_end(arg_ptr);
    }
    if (log_file != NULL)
    {
        va_start(arg_ptr, fmt);
        vfprintf(log_file, fmt, arg_ptr);
        va_end(arg_ptr);
    }
}


/** Application-defined control print callback **/
static void
app_print_cntrl(int cntrl_code)
{
    const char *con_str, *log_str;
    int i;

    if (cntrl_code == '\r')
    {
        /* CR: reset line in console, new line in log file. */
        con_str = "\r";
        log_str = "\n";
        start_of_line = 1;
    }
    else if (cntrl_code == '\v')
    {
        /* VT: new line if current line is not empty, nothing otherwise. */
        if (!start_of_line)
        {
            con_str = log_str = "\n";
            start_of_line = 1;
        }
        else
            con_str = log_str = "";
    }
    else if (cntrl_code < 0 && cntrl_code > -80 && start_of_line)
    {
        /* Minus N: erase first N characters from line, in console only. */
        if (con_file != NULL)
        {
            for (i = 0; i > cntrl_code; --i)
                fputc(' ', con_file);
        }
        con_str = "\r";
        log_str = "";
    }
    else
    {
        /* Unhandled control code (due to internal error): show err marker. */
        con_str = log_str = "<?>";
    }

    if (con_file != NULL)
        fprintf(con_file, con_str);
    if (log_file != NULL)
        fprintf(log_file, log_str);
}


/** Application-defined progress update callback **/
static void
app_progress(unsigned long current_step, unsigned long total_steps)
{
    /* There will be a potentially long wait, so flush the console output. */
    if (con_file != NULL)
        fflush(con_file);
    /* An eager flush of the line-buffered log file is not very important. */

    /* A GUI application would normally update a progress bar. */
    /* Here we ignore the progress info. */
    if (current_step && total_steps)
        return;
}


/** File list processing **/
static int
process_files(int argc, char *argv[])
{
    int result;
    struct opng_ui ui;
    int i;

    /* Initialize the optimization engine. */
    ui.printf_fn      = app_printf;
    ui.print_cntrl_fn = app_print_cntrl;
    ui.progress_fn    = app_progress;
    ui.panic_fn       = panic;
    if (opng_initialize(&options, &ui) != 0)
        panic("Can't initialize optimization engine");

    /* Iterate over file names. */
    result = EXIT_SUCCESS;
    for (i = 1; i < argc; ++i)
    {
        if (argv[i] == NULL || argv[i][0] == 0)
            continue;  /* this was an "-option" */
        if (opng_optimize(argv[i]) != 0)
            result = EXIT_FAILURE;
    }

    /* Finalize the optimization engine. */
    if (opng_finalize() != 0)
        panic("Can't finalize optimization engine");

    return result;
}


/** main **/
int
main(int argc, char *argv[])
{
    int result;

    /* Parse the user options and initialize the application. */
    parse_args(argc, argv);
    app_init();

    /* Print the copyright and version info. */
    app_printf("%s", msg_intro);
    if (options.version)
    {
        /* Print the licensing and extended version info. */
        app_printf(msg_license);
        app_printf("Using libpng version %s and zlib version %s\n\n",
            png_get_libpng_ver(NULL), zlibVersion());
        /* Print the help text only if explicitly requested. */
        if (operation == OP_HELP && !options.help)
            operation = OP_NONE;
    }

    /* Print the help text or run the application. */
    switch (operation)
    {
    case OP_RUN:
        result = process_files(argc, argv);
        break;
    case OP_HELP:
        app_printf("%s", options.help ? msg_help : msg_short_help);
        /* Fall through. */
    default:
        result = EXIT_SUCCESS;
    }

    /* Finalize the application. */
    app_finish();
    return result;
}
