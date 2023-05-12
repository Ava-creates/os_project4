#define _XOPEN_SOURCE 1 /* Required under GLIBC for nftw() */
#define _XOPEN_SOURCE_EXTENDED 1
#include "functions.h"

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
        fread(&array[i].fileOrDirectory, sizeof(int), 1, fp);
    }

    fclose(fp);
    return array;
}

void add_metadata(struct header **array, int *size, char *file_name, struct footer *data, bool flag)
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
    if (flag == true)
    {
        data->total_file_size += file_stat.st_size;
        new_elem->fileOrDirectory = 1;
    }
    else
    {
        new_elem->fileOrDirectory = 0;
    }
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
    add_metadata(head, size, filename, data, true);

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
        fwrite(&array[i].fileOrDirectory, sizeof(int), 1, fp);
    }

    fwrite(&data->num_headers, sizeof(int), 1, fp);
    fwrite(&data->total_file_size, sizeof(int), 1, fp);
    fclose(fp);
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
    if (zip_file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    // append the file contents to the archive
    fseek(zip_file, data->total_file_size, SEEK_SET);

    char buffer[1024];
    size_t nread;

    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, nread, zip_file);
    }

    add_metadata(head, size, filename, data, true);
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
            add_metadata(head, size, entryPath, data, false);
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

int cmpfunc(const void *a, const void *b)
{
    char *s1 = *(char **)a;
    char *s2 = *(char **)b;
    int count1 = 0, count2 = 0;

    // Count the number of '/' characters in each string
    for (int i = 0; i < strlen(s1); i++)
    {
        if (s1[i] == '/')
        {
            count1++;
        }
    }

    for (int i = 0; i < strlen(s2); i++)
    {
        if (s2[i] == '/')
        {
            count2++;
        }
    }

    // Compare the counts
    return count1 - count2;
}

void heirarchy_info_2(char *filename)
{
    struct header *head = get_header(filename);
    struct footer data = get_footer_data(filename);
    for (int i = 0; i < data.num_headers; i++)
    {
        printf("%s\n", head[i].file_name);
    }
}

// void heirarchy_info(char *filename)
// {
//     struct header *head2 = get_header(filename);
//     struct footer data = get_footer_data(filename);
//     int size = data.num_headers;

//     char **string_array = malloc(size * sizeof(char *));
//     if (!string_array)
//     {
//         fprintf(stderr, "Error: Failed to allocate memory\n");
//         return;
//     }

//     // Copy the initial strings to the string array
//     for (int i = 0; i < size; i++)
//     {
//         string_array[i] = strdup(head2[i].file_name);
//         if (!string_array[i])
//         {
//             fprintf(stderr, "Error: Failed to allocate memory\n");
//             return;
//         }
//     }
//     qsort(string_array, size, sizeof(char *), cmpfunc);
//     printf("%s: Hierarchy Information\n", filename);
//     for (int i = 0; i < size; i++)
//     {
//         printf("%s\n", string_array[i]);
//     }
// }

int removeFile(const char *path, const struct stat *statBuf, int type, struct FTW *ftwBuf)
{
    if (remove(path) == -1)
    {
        perror("Error removing file");
    }
    return 0;
}

void unzip(char *filename)
{
    struct header *head2 = get_header(filename);

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    sprintf(cwd, "%s/%s", cwd, filename);

    // Check if the directory exists
    if (access("extract", F_OK) == 0)
    {
        // Recursively remove the contents of the directory
        if (nftw("extract", removeFile, 64, FTW_DEPTH | FTW_PHYS) == -1)
        {
            perror("Error removing directory contents");
            return;
        }
    }
    mkdir("extract", 0700);

    struct footer data = get_footer_data(filename);
    int size = data.num_headers;
    int start_postion_file = 0;

    for (int i = 0; i < size; i++)
    {
        int dir_back = 0;
        char *path = head2[i].file_name;

        char s[2] = "/";
        char *token;
        char *saveptr;
        if (strchr(path, '/') == NULL)
        {
            token = path;
        }
        else
        {
            token = strtok_r(path, s, &saveptr);
        }

        chdir("extract");
        while (token != NULL)
        {

            if (strchr(token, '.') != NULL)
            {

                FILE *fptr;
                FILE *fptr_read;
                fptr_read = fopen(cwd, "rb"); // zipfile
                fptr = fopen(token, "wb");    // current file

                char buffer[head2[i].file_size]; // create a buffer to hold the bytes read
                if (fptr_read == NULL)
                {
                    perror("Failed to open input file");
                }
                fseek(fptr_read, start_postion_file, SEEK_SET);
                size_t bytes_read = fread(buffer, 1, head2[i].file_size, fptr_read); // read 10 bytes from input file
                start_postion_file += head2[i].file_size;
                fwrite(buffer, 1, bytes_read, fptr); // write to the file
                fclose(fptr);
                fclose(fptr_read);
            }
            else
            {
                if (chdir(token) != 0)
                {
                    mkdir(token, head2[i].file_mode);
                    chdir(token);
                }
            }

            token = strtok_r(NULL, s, &saveptr);
            dir_back++;
        }

        for (int j = 0; j < dir_back; j++)
        {
            chdir("..");
        }
    }
}

struct AppendResult append(char *filename)
{
    struct AppendResult result;
    result.data = get_footer_data(filename);
    result.head = get_header(filename);
    return result;
}
