#define _XOPEN_SOURCE 1 /* Required under GLIBC for nftw() */
#define _XOPEN_SOURCE_EXTENDED 1
#include "functions.h"

// getting the footer data from the given zip file
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

// getting the header data from the given zip file
struct header *get_header(char *filename)
{
    // check if the file exists
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return NULL;
    }
    fclose(fp);
    // get the footer so we know the total number of headers
    struct footer data = get_footer_data(filename);
    int size = data.num_headers; // total number of headers in the file

    fp = fopen(filename, "rb");
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

// storing the metadata of a given file file_name to the given array of structs **array
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

    // file or folder?
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

// adding the contents of the given file filename to the given zip file
void add_files(char *filename, char *zip, struct header **head, int *size, struct footer *data)
{
    // check if the file exists
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }

    // store the file metadata (name, size, permissions, etc.) in head
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

// writing all the metadata stored in array and data to the given zip file
void write_metadata(struct header *array, int size, char *filename, struct footer *data)
{
    // validate the file
    FILE *fp;
    fp = fopen(filename, "rb+");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    // fwrite(&size, sizeof(int), 1, fp);
    // fo to the end of the file because that is where we are writing the metadata
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

// appending the contents of the given file to the given zip file
void append_files(char *filename, char *zipfile, struct header **head, int *size, struct footer *data)
{
    // check if the file exists
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    // check if the zip file exists
    FILE *zip_file = fopen(zipfile, "r+");
    if (zip_file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    // append the file contents to the end of the archive
    fseek(zip_file, data->total_file_size, SEEK_SET);

    char buffer[1024];
    size_t nread;

    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, nread, zip_file);
    }

    // store the metadata in the array head
    add_metadata(head, size, filename, data, true);
    fclose(file);
    fclose(zip_file);
}

// function to recursively traverse the given directory and append / add directory contents t the given zip file
void traverseDirectory(const char *directoryPath, char *zip, struct header **head, int *size, struct footer *data, bool flag)
{
    // check if the directory exists
    DIR *directory = opendir(directoryPath);
    if (directory == NULL)
    {
        printf("Failed to open directory: %s\n", directoryPath);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        // if entry = link to parent directory or current directory, skip
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char entryPath[512];
        sprintf(entryPath, "%s/%s", directoryPath, entry->d_name);

        struct stat fileStat;
        // get the current entry;s info
        if (stat(entryPath, &fileStat) < 0)
        {
            printf("Failed to retrieve file information: %s\n", entryPath);
            continue;
        }

        // if current entry is a directory
        if (S_ISDIR(fileStat.st_mode))
        {
            // It's a directory, store its metadata and recursively traverse
            add_metadata(head, size, entryPath, data, false);
            traverseDirectory(entryPath, zip, head, size, data, flag);
        }
        else
        {
            // It's a file, either add or append
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
    // close the directory
    closedir(directory);
}

// convert the filemode from binary to the format that appears on linux
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

// function to print the time as it appears on linux
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

// function to print the heirachy of the given folder
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

// function to remove the unzip folder extract if it already exists
int removeFile(const char *path, const struct stat *statBuf, int type, struct FTW *ftwBuf)
{
    if (remove(path) == -1)
    {
        perror("Error removing file");
    }
    return 0;
}

// function to unzip the given zip file
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

    // create a new diretory with the name extract
    mkdir("extract", 0700);

    // get the metadata from the given zip file
    struct footer data = get_footer_data(filename);
    int size = data.num_headers;
    int start_postion_file = 0;

    // traverse the array of metadata
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

        // change current working directory to extract
        chdir("extract");
        while (token != NULL)
        {
            // if current token is a file
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
                size_t bytes_read = fread(buffer, 1, head2[i].file_size, fptr_read); // read current file size bytes from input file
                start_postion_file += head2[i].file_size;
                fwrite(buffer, 1, bytes_read, fptr); // write to the file
                fclose(fptr);
                fclose(fptr_read);
            }
            else // current token is a foler
            {
                // if the foler exists, change directory. Otherwise, create the folder and change directory
                if (chdir(token) != 0)
                {
                    mkdir(token, head2[i].file_mode);
                    chdir(token);
                }
            }

            token = strtok_r(NULL, s, &saveptr);
            dir_back++;
        }

        // move back to the root directory which is extract in our case
        for (int j = 0; j < dir_back; j++)
        {
            chdir("..");
        }
    }
}

// getting the metadata (struct header + footer) that is stored at the end of given archive file
struct AppendResult append(char *filename)
{
    struct AppendResult result;
    result.data = get_footer_data(filename);
    result.head = get_header(filename);
    return result;
}
