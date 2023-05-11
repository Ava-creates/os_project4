#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>
#include <ftw.h>
#include <fcntl.h>

#define _XOPEN_SOURCE 1			/* Required under GLIBC for nftw() */
#define _XOPEN_SOURCE_EXTENDED 1

struct footer
{
    int num_headers;     // total number of files/headers we have
    int total_file_size; // sum of all the file sizes
};

struct header
{
    char *file_name;
    int file_size;
    mode_t file_mode;
    uid_t file_uid;
    gid_t file_gid;
    time_t file_mtime;
    int fileOrDirectory; // flag --> 1, file. folder --> 0
};

struct AppendResult
{
    struct footer data;
    struct header *head;
};


struct AppendResult append(char *filename);

void add_metadata(struct header **array, int *size, char *file_name, struct footer *data, bool flag);

void add_files(char *filename, char *zip, struct header **head, int *size, struct footer *data);

void write_metadata(struct header *array, int size, char *filename, struct footer *data);

void append_files(char *filename, char *zipfile, struct header **head, int *size, struct footer *data);

struct footer get_footer_data(char *zip_file);

struct header *get_header(char *filename);

void traverseDirectory(const char *directoryPath, char *zip, struct header **head, int *size, struct footer *data, bool flag);

void printFileMode(mode_t fileMode);

void printTime(time_t timestamp);

int cmpfunc(const void *a, const void *b);

void heirarchy_info_2(char *filename);

// void heirarchy_info(char *filename);

int removeFile(const char *path, const struct stat *statBuf, int type, struct FTW *ftwBuf);

void unzip(char *filename);
