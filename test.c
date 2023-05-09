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

int main()
{
    const char *directoryPath = "hello";

    // Recursively traverse the directory
    traverseDirectory(directoryPath);

    return 0;
}
