#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

void traverseDirectory(const char *directoryPath)
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
            traverseDirectory(entryPath);
        }
        else
        {
            // It's a file, do something
            printf("File: %s\n", entryPath);
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
int main()
{
    const char *directoryPath = "hello";

    // Recursively traverse the directory
    struct stat file_stat;
    if (stat(directoryPath, &file_stat) == -1)
    {
        fprintf(stderr, "Error: could not retrieve file metadata for %s\n", directoryPath);
    }

    if (S_ISDIR(file_stat.st_mode))
    {
        printf("folder %d\n", file_stat.st_size);
    }
    else
    {
        printf("file\n");
    }

    return 0;
}
