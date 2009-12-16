/**
 ** osys.c
 ** System extensions.
 **
 ** Copyright (C) 2003-2009 Cosmin Truta.
 **
 ** This software is distributed under the zlib license.
 ** Please see the attached LICENSE for more information.
 **/


#if defined UNIX || defined __unix || defined __unix__
# define OSYS_UNIX
#endif

#if defined OSYS_UNIX || defined __GNUC__
# include <unistd.h>
#endif

#if defined _POSIX_VERSION
# define OSYS_POSIX
# ifndef OSYS_UNIX
#  define OSYS_UNIX
# endif
#endif

#if defined MSDOS || defined _MSDOS || defined __MSDOS__
# define OSYS_DOS
#endif

#if defined OS2 || defined OS_2 || defined __OS2__
# define OSYS_OS2
#endif

#if defined WIN32 || defined _WIN32 || defined __WIN32__ || defined _WIN32_WCE
# define OSYS_WIN32
#endif

#if defined WIN64 || defined _WIN64 || defined __WIN64__
# define OSYS_WIN64
#endif

#if /* defined _WINDOWS || */ defined OSYS_WIN32 || defined OSYS_WIN64
# define OSYS_WINDOWS
#endif


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined OSYS_UNIX || defined OSYS_DOS || defined OSYS_OS2
# include <sys/types.h>
# include <sys/stat.h>
# include <dirent.h>
# include <utime.h>
#endif
#if defined OSYS_DOS || defined OSYS_OS2
# include <process.h>
#endif
#if defined OSYS_WINDOWS
# include <windows.h>
#endif

#include "osys.h"


#if defined OSYS_DOS || defined OSYS_OS2 || defined OSYS_WINDOWS
# define OSYS_FNAME_CHR_SLASH '\\'
# define OSYS_FNAME_STR_SLASH "\\"
# define OSYS_FNAME_STRLIST_SLASH "/\\"
#else
# define OSYS_FNAME_CHR_SLASH '/'
# define OSYS_FNAME_STR_SLASH "/"
# if defined __CYGWIN__
#  define OSYS_FNAME_STRLIST_SLASH "/\\"
# else
#  define OSYS_FNAME_STRLIST_SLASH "/"
# endif
#endif
#define OSYS_FNAME_CHR_DOT '.'
#define OSYS_FNAME_STR_DOT "."
#define OSYS_FNAME_CHR_QUESTION '?'
#define OSYS_FNAME_STR_QUESTION "?"
#define OSYS_FNAME_CHR_STAR '*'
#define OSYS_FNAME_STR_STAR "*"

#if defined OSYS_DOS || defined OSYS_OS2 || \
    defined OSYS_WINDOWS || defined __CYGWIN__
# define OSYS_FNAME_DOS
# define OSYS_FNAME_ICASE 1
#else  /* OSYS_UNIX and others */
# define OSYS_FNAME_ICASE 0
#endif


#ifdef R_OK
# define OSYS_FTEST_READ R_OK
#else
# define OSYS_FTEST_READ 4
#endif
#ifdef W_OK
# define OSYS_FTEST_WRITE W_OK
#else
# define OSYS_FTEST_WRITE 2
#endif
#ifdef X_OK
# define OSYS_FTEST_EXEC X_OK
#else
# define OSYS_FTEST_EXEC 1
#endif
#ifdef F_OK
# define OSYS_FTEST_FILE F_OK
#else
# define OSYS_FTEST_FILE 0
#endif


/**
 * Prints an error message to stderr and terminates the program
 * execution immediately, exiting with code 70 (EX_SOFTWARE).
 * This function does not raise SIGABRT, and it does not generate
 * other files (like core dumps, where applicable).
 **/
void osys_terminate(void)
{
    fprintf(stderr,
        "The execution of this program has been terminated abnormally.\n");
    fflush(stderr);
    exit(70);  /* EX_SOFTWARE */
}


/**
 * Creates a backup file name.
 * On success, the function returns buffer.
 * On error, it returns NULL.
 **/
char *osys_fname_mkbak(char *buffer, size_t bufsize, const char *fname)
{
    if (strlen(fname) + sizeof(OSYS_FNAME_STR_DOT "bak") > bufsize)
        return NULL;

#if defined OSYS_DOS

    return osys_fname_chext(buffer, bufsize, fname, OSYS_FNAME_STR_DOT "bak");

#else  /* OSYS_UNIX and others */

    strcpy(buffer, fname);
    strcat(buffer, OSYS_FNAME_STR_DOT "bak");
    return buffer;

#endif
}


/**
 * Creates a file name by changing the directory of a given file name.
 * The new directory name can be the empty string, indicating that
 * the new file name has no directory (or is in the default directory).
 * The directory name may or may not contain the trailing directory
 * separator (usually '/').
 * On success, the function returns buffer.
 * On error, it returns NULL.
 **/
char *osys_fname_chdir(char *buffer, size_t bufsize,
    const char *old_fname, const char *new_dirname)
{
    const char *fname, *ptr;
    size_t dirlen;

    /* Extract file name from old_fname. */
    fname = old_fname;
#ifdef OSYS_FNAME_DOS
    if (isalpha(fname[0]) && fname[1] == ':')
        fname += 2;  /* skip drive name */
#endif
    for ( ; ; )
    {
        ptr = strpbrk(fname, OSYS_FNAME_STRLIST_SLASH);
        if (ptr == NULL)
            break;
        fname = ptr + 1;
    }

    /* Make sure the buffer is large enough. */
    dirlen = strlen(new_dirname);
    if (dirlen + strlen(fname) + 2 >= bufsize)
        return NULL;  /* overflow */

    /* Copy the new directory name. Also append a slash if necessary. */
    if (dirlen > 0)
    {
        strcpy(buffer, new_dirname);
#ifdef OSYS_FNAME_DOS
        if (dirlen == 2 && buffer[1] == ':' && isalpha(buffer[0]))
            (void)0;  /* do nothing */
        else
#endif
        if (strchr(OSYS_FNAME_STRLIST_SLASH, buffer[dirlen - 1]) == NULL)
            buffer[dirlen++] = OSYS_FNAME_CHR_SLASH;  /* append slash to dir */
    }

    /* Append the file name. */
    strcpy(buffer + dirlen, fname);
    return buffer;  /* success */
}


/**
 * Creates a file name by changing the extension of a given file name.
 * The new extension can be the empty string, indicating that the new
 * file name has no extension.  Otherwise, it must begin with the
 * extension separator (usually '.').
 * On success, the function returns buffer.
 * On error, it returns NULL.
 **/
char *osys_fname_chext(char *buffer, size_t bufsize,
    const char *old_fname, const char *new_extname)
{
    size_t i, pos;

    if (new_extname[0] != OSYS_FNAME_CHR_DOT)
        return NULL;  /* invalid argument */
    for (i = 0, pos = (size_t)(-1); old_fname[i] != 0; ++i)
    {
        if (i >= bufsize)
            return NULL;  /* overflow */
        if ((buffer[i] = old_fname[i]) == OSYS_FNAME_CHR_DOT)
            pos = i;
    }
    if (i > pos)
        i = pos;  /* go back only if old_fname has an extension */
    for ( ; ; ++i, ++new_extname)
    {
        if (i >= bufsize)
            return NULL;  /* overflow */
        if ((buffer[i] = *new_extname) == 0)
            return buffer;  /* success */
    }
}


/**
 * Compares one file name to another.
 * It returns a value (less than, equal to, or greater than 0)
 * based on the result of comparing fname1 to fname2.
 * The comparison may or may not be case sensitive, depending on
 * the operating system.
 **/
int osys_fname_cmp(const char *fname1, const char *fname2)
{
#if OSYS_FNAME_ICASE
# ifdef OSYS_WINDOWS
    return lstrcmpiA(fname1, fname2);
# else
    return stricmp(fname1, fname2);
# endif
#else
    return strcmp(fname1, fname2);
#endif
}


/**
 * Opens a file and positions it at the specified file offset.
 * On success, the function returns the pointer to the file stream.
 * On error, it returns NULL.
 **/
FILE *osys_fopen_at(const char *fname, const char *mode,
    long offset, int whence)
{
    FILE *stream;

    if ((stream = fopen(fname, mode)) == NULL)
        return NULL;
    if (offset == 0 && (whence == SEEK_SET || whence == SEEK_CUR))
        return stream;
    if (fseek(stream, offset, whence) != 0)
    {
        fclose(stream);
        return NULL;
    }
    return stream;
}


/**
 * Reads a block of data from the specified file offset.
 * The file-position indicator is saved and restored after reading.
 * The file buffer is flushed before and after reading.
 * On success, the function returns the number of bytes read.
 * On error, it returns 0.
 **/
size_t osys_fread_at(FILE *stream, long offset, int whence,
    void *block, size_t blocksize)
{
    fpos_t pos;
    size_t result;

    if (fgetpos(stream, &pos) != 0 || fflush(stream) != 0)
        return 0;
    if (fseek(stream, offset, whence) == 0)
        result = fread(block, 1, blocksize, stream);
    else
        result = 0;
    if (fflush(stream) != 0)
        result = 0;
    if (fsetpos(stream, &pos) != 0)
        result = 0;
    return result;
}


/**
 * Writes a block of data at the specified file offset.
 * The file-position indicator is saved and restored after writing.
 * The file buffer is flushed before and after writing.
 * On success, the function returns the number of bytes written.
 * On error, it returns 0.
 **/
size_t osys_fwrite_at(FILE *stream, long offset, int whence,
    const void *block, size_t blocksize)
{
    fpos_t pos;
    size_t result;

    if (fgetpos(stream, &pos) != 0 || fflush(stream) != 0)
        return 0;
    if (fseek(stream, offset, whence) == 0)
        result = fwrite(block, 1, blocksize, stream);
    else
        result = 0;
    if (fflush(stream) != 0)
        result = 0;
    if (fsetpos(stream, &pos) != 0)
        result = 0;
    return result;
}


/**
 * Determines if the accessibility of the specified file satisfies
 * the specified access mode.  The access mode consists of one or more
 * characters that indicate the checks to be performed, as follows:
 *  'e': the file exists; it needs not be a regular file.
 *  'f': the file exists and is a regular file.
 *  'r': the file exists and read permission is granted.
 *  'w': the file exists and write permission is granted.
 *  'x': the file exists and execute permission is granted.
 * For example, to determine if a file can be opened for reading using
 * fopen(), use "fr" in the access mode.
 * If all checks succeed, the function returns 0.
 * Otherwise, it returns -1.
 **/
int osys_ftest(const char *fname, const char *mode)
{
    int faccess, freg;

    faccess = freg = 0;
    if (strchr(mode, 'f') != NULL)
        freg = 1;
    if (strchr(mode, 'r') != NULL)
        faccess |= OSYS_FTEST_READ;
    if (strchr(mode, 'w') != NULL)
        faccess |= OSYS_FTEST_WRITE;
    if (strchr(mode, 'x') != NULL)
        faccess |= OSYS_FTEST_EXEC;
    if (faccess == 0 && freg == 0)
        if (strchr(mode, 'e') == NULL)
            return 0;

#if defined OSYS_UNIX || defined OSYS_DOS || defined OSYS_OS2

    {
        struct stat sbuf;

        if (stat(fname, &sbuf) != 0)
            return -1;
        if (freg && !(sbuf.st_mode & S_IFREG))
            return -1;
        if (faccess == 0)
            return 0;
        return access(fname, faccess);
    }

#elif defined OSYS_WINDOWS

    {
        DWORD attr;

        attr = GetFileAttributesA(fname);
        if (attr == 0xffffffffU)
            return -1;
        if (freg && (attr & FILE_ATTRIBUTE_DIRECTORY))
            return -1;
        if ((faccess & OSYS_FTEST_WRITE) && (attr & FILE_ATTRIBUTE_READONLY))
            return -1;
        return 0;
    }

#else  /* generic */

    {
        FILE *stream;

        if (faccess & OSYS_FTEST_WRITE)
            stream = fopen(fname, "r+b");
        else
            stream = fopen(fname, "rb");
        if (stream == NULL)
            return -1;
        fclose(stream);
        return 0;
    }

#endif
}


/**
 * Copies the access mode and the time stamp of the file or directory
 * named by dest_name into the file or directory named by src_name.
 * On success, the function returns 0.
 * On error, it returns -1.
 **/
int osys_fattr_copy(const char *dest_name, const char *src_name)
{
#if defined OSYS_UNIX || defined OSYS_DOS || defined OSYS_OS2

    struct stat sbuf;
    int /* mode_t */ mode;
    struct utimbuf utbuf;

    if (stat(src_name, &sbuf) != 0)
        return -1;

    mode = (int)sbuf.st_mode;
    utbuf.actime = sbuf.st_atime;
    utbuf.modtime = sbuf.st_mtime;

    if (utime(dest_name, &utbuf) == 0 && chmod(dest_name, mode) == 0)
        return 0;
    else
        return -1;

#elif defined OSYS_WINDOWS

    static int isWinNT = -1;
    HANDLE hFile;
    FILETIME ftLastWrite;
    BOOL result;

    if (isWinNT < 0)
        isWinNT = (GetVersion() < 0x80000000U) ? 1 : 0;

    hFile = CreateFileA(src_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;
    result = GetFileTime(hFile, NULL, NULL, &ftLastWrite);
    CloseHandle(hFile);
    if (!result)
        return -1;

    hFile = CreateFileA(dest_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        (isWinNT ? FILE_FLAG_BACKUP_SEMANTICS : 0), 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;
    result = SetFileTime(hFile, NULL, NULL, &ftLastWrite);
    CloseHandle(hFile);
    if (!result)
        return -1;

    /* TODO: Copy the access mode. */

    return 0;

#else  /* generic */

    /* Do nothing. */
    return 0;

#endif
}


/**
 * Creates a new directory with the given name.
 * If the directory is successfully created, or if it already exists,
 * the function returns 0.
 * Otherwise, it returns -1.
 **/
int osys_dir_make(const char *dirname)
{
    size_t len;

    len = strlen(dirname);
    if (len == 0)  /* current directory */
        return 0;

#ifdef OSYS_FNAME_DOS
    if (len == 2 && dirname[1] == ':' && isalpha(dirname[0]))  /* [DRIVE]: */
        return 0;
#endif

#if defined OSYS_UNIX || defined OSYS_DOS || defined OSYS_OS2

    {
        struct stat sbuf;

        if (stat(dirname, &sbuf) == 0)
            return (sbuf.st_mode & S_IFDIR) ? 0 : -1;

        /* There is no directory, so create one now. */
# if defined OSYS_DOS || defined OSYS_OS2
        return mkdir(dirname);
# else
        return mkdir(dirname, 0777);
# endif
    }

#elif defined OSYS_WINDOWS

    {
        char *wildname;
        HANDLE hFind;
        WIN32_FIND_DATA wfd;

        /* See if dirname exists: find files in (dirname + "\\*"). */
        if (len + 3 < len)  /* overflow */
            return -1;
        wildname = (char *)malloc(len + 3);
        if (wildname == NULL)  /* out of memory */
            return -1;
        strcpy(wildname, dirname);
        if (strchr(OSYS_FNAME_STRLIST_SLASH, wildname[len - 1]) == NULL)
            wildname[len++] = OSYS_FNAME_CHR_SLASH;
        wildname[len++] = OSYS_FNAME_CHR_STAR;
        wildname[len] = '\0';
        hFind = FindFirstFileA(wildname, &wfd);
        free(wildname);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
            return 0;
        }

        /* There is no directory, so create one now. */
        return CreateDirectoryA(dirname, NULL) ? 0 : -1;
    }

#else  /* generic */

    /* Do nothing. */
    return 0;

#endif
}
