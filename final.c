#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>

#define MAX_TOKENS 100

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
};

void add_metadata(struct header **array, int *size, char *file_name, struct footer *data)
{
    // Allocate memory for a new struct
    struct header *new_elem = malloc(sizeof(struct header));
    struct stat file_stat;
    if (stat(file_name, &file_stat) == -1)
    {
        fprintf(stderr, "Error: could not retrieve file metadata for %s\n", file_name);
        return;
    }

    // Allocate memory for the string in the struct
    new_elem->file_name = malloc(strlen(file_name) + 1);
    // Copy the string into the struct
    strcpy(new_elem->file_name, file_name);

    // add rest of the attributes
    new_elem->file_size = file_stat.st_size;
    new_elem->file_gid = file_stat.st_gid;
    new_elem->file_uid = file_stat.st_uid;
    new_elem->file_mode = file_stat.st_mode;
    new_elem->file_mtime = file_stat.st_mtime;

    // Add the new element to the array
    (*size)++;
    *array = realloc(*array, (*size) * sizeof(struct header));
    (*array)[*size - 1] = *new_elem;
    free(new_elem);

    // modify the footer contents
    data->num_headers += 1;
    data->total_file_size += file_stat.st_size;
}

void add_files(char *filename, char *zip, struct header **head, int *size, struct footer *data)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }

    // append the file metadata (name, size, permissions, etc.) to the archive
    add_metadata(head, size, filename, data);

    // open the zip file and add file content to the end of the file
    FILE *zip_file = fopen(zip, "a");

    char buffer[1024];
    size_t nread;
    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, nread, zip_file);
    }

    // close both the files
    fclose(file);
    fclose(zip_file);
}

void write_metadata(struct header *array, int size, char *filename, struct footer *data)
{
    FILE *fp;
    fp = fopen(filename, "rb+");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    // fwrite(&size, sizeof(int), 1, fp);
    fseek(fp, 0, SEEK_END);
    for (int i = 0; i < size; i++)
    {
        int string_len = strlen(array[i].file_name) + 1; // Include null terminator
        fwrite(&string_len, sizeof(int), 1, fp);
        fwrite(array[i].file_name, sizeof(char), string_len, fp);
        fwrite(&array[i].file_size, sizeof(int), 1, fp);
        fwrite(&array[i].file_gid, sizeof(int), 1, fp);
        fwrite(&array[i].file_mode, sizeof(int), 1, fp);
        fwrite(&array[i].file_uid, sizeof(int), 1, fp);
        fwrite(&array[i].file_mtime, sizeof(int), 1, fp);
    }

    fwrite(&data->num_headers, sizeof(int), 1, fp);
    fwrite(&data->total_file_size, sizeof(int), 1, fp);
    fclose(fp);
}
struct footer get_footer_data(char *zip_file)
{
    FILE *fp = fopen(zip_file, "rb+"); // open the archive file
    struct footer data;
    // position where the footer is stored --> size of the file - size of the footer
    // this is because we store the footer info at the end of the file
    // seek to that position
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp) - sizeof(struct footer);
    fseek(fp, size, SEEK_SET);
    fread(&data, sizeof(struct footer), 1, fp);
    fclose(fp);
    return data;
}
struct header *get_header(char *filename)
{
    struct footer data = get_footer_data(filename);
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return NULL;
    }

    int size = data.num_headers;

    // seek to the position where the file content ends at the meta data beginds
    fseek(fp, data.total_file_size, SEEK_SET);

    // create an array of structs to store the the meta data for each file
    struct header *array = malloc((size) * sizeof(struct header));
    for (int i = 0; i < data.num_headers; i++)
    {

        int string_len;
        fread(&string_len, sizeof(int), 1, fp);
        array[i].file_name = malloc(string_len * sizeof(char));
        fread(array[i].file_name, sizeof(char), string_len, fp);
        fread(&array[i].file_size, sizeof(int), 1, fp);
        fread(&array[i].file_gid, sizeof(int), 1, fp);
        fread(&array[i].file_mode, sizeof(int), 1, fp);
        fread(&array[i].file_uid, sizeof(int), 1, fp);
        fread(&array[i].file_mtime, sizeof(int), 1, fp);
    }

    fclose(fp);
    return array;
}

// Define a structure to hold the footer and header data
struct AppendResult
{
    struct footer data;
    struct header *head;
};

struct AppendResult append(char *filename)
{
    struct AppendResult result;
    result.data = get_footer_data(filename);
    result.head = get_header(filename);
    return result;
}

void append_files(char *filename, char *zipfile, struct header **head, int *size, struct footer *data)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }

    FILE *zip_file = fopen(zipfile, "r+");

    // append the file contents to the archive
    fseek(zip_file, data->total_file_size, SEEK_SET);
    ftruncate(fileno(zip_file), ftell(zip_file));
    char buffer[1024];
    size_t nread;

    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, nread, zip_file);
    }

    add_metadata(head, size, filename, data);
    fclose(file);
    fclose(zip_file);
}

void traverseDirectory(const char *directoryPath, char *zip, struct header **head, int *size, struct footer *data, bool flag)
{
    DIR *directory = opendir(directoryPath);
    if (directory == NULL)
    {
        printf("Failed to open directory: %s\n", directoryPath);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char entryPath[512];
        sprintf(entryPath, "%s/%s", directoryPath, entry->d_name);

        struct stat fileStat;
        if (stat(entryPath, &fileStat) < 0)
        {
            printf("Failed to retrieve file information: %s\n", entryPath);
            continue;
        }

        if (S_ISDIR(fileStat.st_mode))
        {
            // It's a directory, recursively traverse
            traverseDirectory(entryPath, zip, head, size, data, flag);
        }
        else
        {
            // It's a file, do something
            if (flag == true)
            {
                add_files(entryPath, zip, head, size, data);
            }
            else
            {
                append_files(entryPath, zip, head, size, data);
            }
        }
    }

    closedir(directory);
}

void printFileMode(mode_t fileMode)
{
    // File Type
    char fileType;
    if (S_ISDIR(fileMode))
        fileType = 'd';
    else if (S_ISLNK(fileMode))
        fileType = 'l';
    else if (S_ISFIFO(fileMode))
        fileType = 'p';
    else if (S_ISSOCK(fileMode))
        fileType = 's';
    else if (S_ISCHR(fileMode))
        fileType = 'c';
    else if (S_ISBLK(fileMode))
        fileType = 'b';
    else
        fileType = '-';

    // Permissions
    char filePermissions[11];
    filePermissions[0] = fileType;
    filePermissions[1] = (fileMode & S_IRUSR) ? 'r' : '-';
    filePermissions[2] = (fileMode & S_IWUSR) ? 'w' : '-';
    filePermissions[3] = (fileMode & S_IXUSR) ? 'x' : '-';
    filePermissions[4] = (fileMode & S_IRGRP) ? 'r' : '-';
    filePermissions[5] = (fileMode & S_IWGRP) ? 'w' : '-';
    filePermissions[6] = (fileMode & S_IXGRP) ? 'x' : '-';
    filePermissions[7] = (fileMode & S_IROTH) ? 'r' : '-';
    filePermissions[8] = (fileMode & S_IWOTH) ? 'w' : '-';
    filePermissions[9] = (fileMode & S_IXOTH) ? 'x' : '-';
    filePermissions[10] = '\0';

    printf("File Mode: %s\n", filePermissions);
}

void printTime(time_t timestamp)
{
    struct tm *timeinfo;
    timeinfo = localtime(&timestamp);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("File Mime: %s ", buffer);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s {-c|-a|-x|-m|-p} <archive-file> <file/directory list>\n", argv[0]);
        return 1;
    }

    // Validate the command option
    const char *option = argv[1];
    bool validOption = false;
    if (strcmp(option, "-c") == 0 || strcmp(option, "-a") == 0 ||
        strcmp(option, "-x") == 0 || strcmp(option, "-m") == 0 ||
        strcmp(option, "-p") == 0)
    {
        validOption = true;
    }
    if (!validOption)
    {
        printf("Invalid command option: %s\n", option);
        return 1;
    }

    // Validate the archive file
    char *archiveFile = argv[2];
    // Check if the archiveFile ends with ".ad"
    char *postfix = ".ad";
    size_t fileLength = strlen(archiveFile);
    size_t postfixLength = strlen(postfix);
    if (fileLength < postfixLength || strcmp(archiveFile + fileLength - postfixLength, postfix) != 0)
    {
        printf("Invalid archive file: %s\n", archiveFile);
        return 1;
    }

    // Process the file/directory list
    char *fileList = argv[3];

    char *tokens[MAX_TOKENS];
    int numTokens = 0;
    char *token;
    char *delimiter = ",";
    token = strtok(fileList, delimiter);
    while (token != NULL)
    {
        // Store the token in the array
        tokens[numTokens] = token;
        numTokens++;

        // Move to the next token
        token = strtok(NULL, delimiter);
    }

    if (strcmp(option, "-c") == 0)
    {
        FILE *file = fopen(archiveFile, "w");
        if (file == NULL)
        {
            printf("Failed to open archive file: %s\n", archiveFile);
            return 1;
        }
        fclose(file);

        // initialise variables needed for zipping the file
        struct header *head = NULL;
        int size = 0;
        struct footer Data = {0, 0};
        struct footer *data = &Data;

        // Iterate over the array of tokens
        for (int i = 0; i < numTokens; i++)
        {
            char *fileOrDirectory = tokens[i];

            // file or directory??
            struct stat fileStat;
            if (stat(fileOrDirectory, &fileStat) == 0)
            {
                if (S_ISDIR(fileStat.st_mode))
                {
                    traverseDirectory(fileOrDirectory, archiveFile, &head, &size, data, true);
                }
                else
                {
                    add_files(fileOrDirectory, archiveFile, &head, &size, data);
                }
            }
        }

        write_metadata(head, size, archiveFile, data);

        // checking the contents of head after reading from the zip
        // struct header *head2 = get_header(archiveFile);
        // for (int i = 0; i < data->num_headers; i++)
        // {
        //     printf("String: %s, Number:%d\n", head2[i].file_name, head2[i].file_size);
        // }
    }
    else if (strcmp(option, "-a") == 0)
    {
        FILE *file = fopen(archiveFile, "r");
        if (file == NULL)
        {
            printf("Failed to open archive file: %s\n", archiveFile);
            return 1;
        }
        fclose(file);

        struct AppendResult result = append(archiveFile);
        struct header *head2 = result.head;
        struct footer Data = result.data;
        struct footer *data2 = &Data;
        int size = Data.num_headers;

        for (int i = 0; i < numTokens; i++)
        {
            char *fileOrDirectory = tokens[i];

            // file or directory??
            struct stat fileStat;
            if (stat(fileOrDirectory, &fileStat) == 0)
            {
                if (S_ISDIR(fileStat.st_mode))
                {
                    traverseDirectory(fileOrDirectory, archiveFile, &head2, &size, data2, false);
                }
                else
                {
                    append_files(fileOrDirectory, archiveFile, &head2, &size, data2);
                }
            }
        }

        write_metadata(head2, size, archiveFile, data2);

        // struct header *head = get_header(archiveFile);
        // for (int i = 0; i < data2->num_headers; i++)
        // {
        //     printf("String: %s, Number:%d\n", head[i].file_name, head[i].file_size);
        // }
    }
    else if (strcmp(option, "-m") == 0)
    {
        struct AppendResult result = append(archiveFile);
        struct header *head = result.head;
        struct footer Data = result.data;

        for (int i = 0; i < Data.num_headers; i++)
        {
            printf("File Name: %s, File Size:%d, File GID:%d, File UID:%d, ", head[i].file_name, head[i].file_size, head[i].file_gid, head[i].file_uid);
            printTime(head[i].file_mtime);
            printFileMode(head[i].file_mode);
        }
    }
}