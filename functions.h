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

#define _XOPEN_SOURCE 1 /* Required under GLIBC for nftw() */
#define _XOPEN_SOURCE_EXTENDED 1

struct footer
{
    int num_headers;     // total number of files/headers we have
    int total_file_size; // sum of all the file sizes
};

struct header
{
    char *file_name;     // file name
    int file_size;       // file size
    mode_t file_mode;    // file permissions
    uid_t file_uid;      // user id
    gid_t file_gid;      // group id
    time_t file_mtime;   // last modified time
    int fileOrDirectory; // flag --> 1, file. folder --> 0
};

struct AppendResult
{
    // struct to combine metadata
    struct footer data;
    struct header *head;
};

// different functions created for zipping and unzipping

// getting the metadata (struct header + footer) that is stored at the end of given archive file
struct AppendResult append(char *filename);

// storing the metadata of a given file file_name to the given array of structs **array
void add_metadata(struct header **array, int *size, char *file_name, struct footer *data, bool flag);

// adding the contents of the given file filename to the given zip file
void add_files(char *filename, char *zip, struct header **head, int *size, struct footer *data);

// writing all the metadata stored in array and data to the given zip file
void write_metadata(struct header *array, int size, char *filename, struct footer *data);

// appending the contents of the given file to the given zip file
void append_files(char *filename, char *zipfile, struct header **head, int *size, struct footer *data);

// getting the footer data from the given zip file
struct footer get_footer_data(char *zip_file);

// getting the header data from the given zip file
struct header *get_header(char *filename);

// function to recursively traverse the given directory and append / add directory contents t the given zip file
void traverseDirectory(const char *directoryPath, char *zip, struct header **head, int *size, struct footer *data, bool flag);

// convert the filemode from binary to the format that appears on linux
void printFileMode(mode_t fileMode);

// function to print the time as it appears on linux
void printTime(time_t timestamp);

int cmpfunc(const void *a, const void *b);

// function to print the heirachy of the given folder
void heirarchy_info_2(char *filename);

// void heirarchy_info(char *filename);

// function to remove the unzip folder extract if it already exists
int removeFile(const char *path, const struct stat *statBuf, int type, struct FTW *ftwBuf);

// function to unzip the given zip file
void unzip(char *filename);
