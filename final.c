
#include "functions.h"

#define MAX_TOKENS 100 // number of files / directories passed in a comma separated list on cli

int main(int argc, char *argv[])
{
    // check the number of arguments
    if (argc > 4 || argc < 3)
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

    // invalid argument
    if (!validOption)
    {
        printf("Invalid command option: %s\n", option);
        return 1;
    }

    // if flag = -c or flag = -a, argc -> 4
    if (strcmp(option, "-c") == 0 || strcmp(option, "-a") == 0)
    {
        if (argc != 4)
        {
            printf("Usage: %s {-c|-a|-x|-m|-p} <archive-file> <file/directory list>\n", argv[0]);
            return 1;
        }
    }

    // if other valid flags, argc -> 3
    if (strcmp(option, "-x") == 0 || strcmp(option, "-m") == 0 ||
        strcmp(option, "-p") == 0)
    {
        if (argc != 3)
        {
            printf("Usage: %s {-c|-a|-x|-m|-p} <archive-file>\n", argv[0]);
            return 1;
        }
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
    char *saveptr;
    char *token;
    char *delimiter = ",";
    token = strtok_r(fileList, delimiter, &saveptr);
    // files & directories are given as a comma separated list, parse them here
    while (token != NULL)
    {
        // Store the token in the array
        tokens[numTokens] = token;
        numTokens++;

        // Move to the next token
        token = strtok_r(NULL, delimiter, &saveptr);
    }

    // create a new archive file
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
                // directiry
                if (S_ISDIR(fileStat.st_mode))
                {
                    // store the metadata of the directory and recursively traverse it
                    add_metadata(&head, &size, fileOrDirectory, data, false);
                    traverseDirectory(fileOrDirectory, archiveFile, &head, &size, data, true);
                }
                else // file
                {
                    // add file contents and the metadata to the zip file
                    add_files(fileOrDirectory, archiveFile, &head, &size, data);
                }
            }
        }

        // write the metadata at the end of the file
        write_metadata(head, size, archiveFile, data);
        printf("Created %s\n", archiveFile);

        // uncomment to check the contents of head after reading from the zip
        // struct header *head2 = get_header(archiveFile);

        // for (int i = 0; i < data->num_headers; i++)
        // {
        //     printf("String: %s, Number:%d\n", head2[i].file_name, head2[i].file_size);
        // }
    }
    else if (strcmp(option, "-a") == 0) // append flag
    {
        // validate the archive file
        FILE *file = fopen(archiveFile, "rw+");
        if (file == NULL)
        {
            printf("Failed to open archive file: %s\n", archiveFile);
            return 1;
        }
        fclose(file);

        // store the metadata in head2 and number of files / directories + the size of all files in data2
        struct AppendResult result = append(archiveFile);
        struct header *head2 = result.head;
        struct footer Data = result.data;
        struct footer *data2 = &Data;
        int size = Data.num_headers;

        // open the file to truncate it, i.e., remove the metadata so that we can start appending new content from there
        int fd = open(archiveFile, O_RDWR);
        if (fd == -1)
        {
            printf("Failed to open archive file: %s\n", archiveFile);
            return 1;
        }
        ftruncate(fd, Data.total_file_size);
        close(fd);
        for (int i = 0; i < numTokens; i++)
        {
            char *fileOrDirectory = tokens[i];

            // file or directory??
            struct stat fileStat;
            if (stat(fileOrDirectory, &fileStat) == 0)
            {
                if (S_ISDIR(fileStat.st_mode))
                {
                    // store directory's metadata and recursively traverse it
                    add_metadata(&head2, &size, fileOrDirectory, data2, false);
                    traverseDirectory(fileOrDirectory, archiveFile, &head2, &size, data2, false);
                }
                else
                {
                    // append file contents to the archive file and store its meta data
                    append_files(fileOrDirectory, archiveFile, &head2, &size, data2);
                }
            }
        }

        // write metadata to the archive file
        write_metadata(head2, size, archiveFile, data2);
        printf("Appended to %s\n", archiveFile);

        // uncomment to read the metadata from the archive file for validation
        // struct header *head = get_header(archiveFile);
        // for (int i = 0; i < data2->num_headers; i++)
        // {
        //     printf("String: %s, Number:%d\n", head[i].file_name, head[i].file_size);
        // }
    }
    else if (strcmp(option, "-m") == 0)
    {
        // get the metadata in the below structures
        struct AppendResult result = append(archiveFile);
        struct header *head = result.head;
        struct footer Data = result.data;
        printf("%s: Metadata Information\n", archiveFile);
        // traverse head and print the metadata
        for (int i = 0; i < Data.num_headers; i++)
        {
            printf("File Name: %s, File Size:%d, File GID:%d, File UID:%d, ", head[i].file_name, head[i].file_size, head[i].file_gid, head[i].file_uid);
            printTime(head[i].file_mtime);
            printFileMode(head[i].file_mode);
        }
    }
    else if (strcmp(option, "-p") == 0)
    {
        // validate if the zip file exists
        FILE *file = fopen(archiveFile, "r");
        if (file == NULL)
        {
            printf("Failed to open archive file: %s\n", archiveFile);
            return 1;
        }
        fclose(file);
        // print the heirarchy
        heirarchy_info_2(archiveFile);
    }
    else if (strcmp(option, "-x") == 0)
    {
        // validate if the archive file to unzip exists
        FILE *file = fopen(archiveFile, "r");
        if (file == NULL)
        {
            printf("Failed to open archive file: %s\n", archiveFile);
            return 1;
        }
        fclose(file);
        // call the unzip function
        unzip(archiveFile);
        printf("Extracted %s to folder named extracted\n", archiveFile);
    }
}
