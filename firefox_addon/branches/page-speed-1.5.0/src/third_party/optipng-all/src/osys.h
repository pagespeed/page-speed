/**
 ** osys.h
 ** System extensions.
 **
 ** Copyright (C) 2003-2009 Cosmin Truta.
 **
 ** This software is distributed under the zlib license.
 ** Please see the attached LICENSE for more information.
 **/


#ifndef OSYS_H
#define OSYS_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>


/**
 * Prints an error message to stderr and terminates the program
 * execution immediately, exiting with code 70 (EX_SOFTWARE).
 * This function does not raise SIGABRT, and it does not generate
 * other files (like core dumps, where applicable).
 **/
void osys_terminate(void);


/**
 * Creates a backup file name.
 * On success, the function returns buffer.
 * On error, it returns NULL.
 **/
char *osys_fname_mkbak(char *buffer, size_t bufsize, const char *fname);


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
    const char *old_fname, const char *new_dirname);


/**
 * Creates a file name by changing the extension of a given file name.
 * The new extension can be the empty string, indicating that the new
 * file name has no extension.  Otherwise, it must begin with the
 * extension separator (usually '.').
 * On success, the function returns buffer.
 * On error, it returns NULL.
 **/
char *osys_fname_chext(char *buffer, size_t bufsize,
    const char *old_fname, const char *new_extname);


/**
 * Compares one file name to another.
 * It returns a value (less than, equal to, or greater than 0)
 * based on the result of comparing fname1 to fname2.
 * The comparison may or may not be case sensitive, depending on
 * the operating system.
 **/
int osys_fname_cmp(const char *fname1, const char *fname2);


/**
 * Opens a file and positions it at the specified file offset.
 * On success, the function returns the pointer to the file stream.
 * On error, it returns NULL.
 **/
FILE *osys_fopen_at(const char *fname, const char *mode,
    long offset, int whence);


/**
 * Reads a block of data from the specified file offset.
 * The file-position indicator is saved and restored after reading.
 * The file buffer is flushed before and after reading.
 * On success, the function returns the number of bytes read.
 * On error, it returns 0.
 **/
size_t osys_fread_at(FILE *stream, long offset, int whence,
    void *block, size_t blocksize);


/**
 * Writes a block of data at the specified file offset.
 * The file-position indicator is saved and restored after writing.
 * The file buffer is flushed before and after writing.
 * On success, the function returns the number of bytes written.
 * On error, it returns 0.
 **/
size_t osys_fwrite_at(FILE *stream, long offset, int whence,
    const void *block, size_t blocksize);


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
int osys_ftest(const char *fname, const char *mode);


/**
 * Copies the access mode and the time stamp of the file or directory
 * named by dest_name into the file or directory named by src_name.
 * On success, the function returns 0.
 * On error, it returns -1.
 **/
int osys_fattr_copy(const char *dest_name, const char *src_name);


/**
 * Creates a new directory with the given name.
 * If the directory is successfully created, or if it already exists,
 * the function returns 0.
 * Otherwise, it returns -1.
 **/
int osys_dir_make(const char *dirname);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* OSYS_H */
