#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define num_files 20

struct footer
{
    int total_file_size; // sum of all the file sizes
    int num_headers;     // total number of files/headers we have
};

struct header
{
    char *file_name;
    char *file_path;
    int file_size;
    mode_t file_mode;
    uid_t file_uid;
    gid_t file_gid;
    time_t file_mtime;
};

struct footer get_footer_data(char *zip_file)
{
    FILE *fp;
    fp = fopen(zip_file, "rb+");
    // printf("in1\n");
    struct footer data2;
    fseek(fp, 0, SEEK_END);
    long size2 = ftell(fp) - sizeof(struct footer);
    fseek(fp, size2, SEEK_SET);
    fread(&data2, sizeof(struct footer), 1, fp); // we have footer
    fclose(fp);
    return data2;
}

struct header *get_header(char *filename)
{
    FILE *fp;
    struct footer data = get_footer_data(filename);
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return NULL;
    }
    // fread(size, sizeof(int), 1, fp);
    int size = data.num_headers;
    printf("inside get header num_headers: %d\n", size);
    printf("inside get header size: %d\n", data.total_file_size);
    fseek(fp, data.total_file_size, SEEK_SET);
    struct header *array = malloc((size) * sizeof(struct header));
    for (int i = 0; i < data.num_headers; i++)
    {

        int string_len;

        fread(&string_len, sizeof(int), 1, fp);

        array[i].file_name = malloc(string_len * sizeof(char));
        fread(array[i].file_name, sizeof(char), string_len, fp);

        fread(&array[i].file_size, sizeof(int), 1, fp);
        printf("file_name inside: %s\n", array[i].file_name);
        fread(&array[i].file_gid, sizeof(int), 1, fp);
        fread(&array[i].file_mode, sizeof(int), 1, fp);
        fread(&array[i].file_uid, sizeof(int), 1, fp);
        fread(&array[i].file_mtime, sizeof(int), 1, fp);
    }
    fclose(fp);
    return array;
}

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

    data->num_headers += 1;
    data->total_file_size += file_stat.st_size;
}

void write_metadata(struct header *array, int size, char *filename)
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
    fclose(fp);
}

void add_files(char *filename, FILE *zip_file, struct header **head, int *size, struct footer *data)
{
    printf("adding file - a\n");
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }

    // // append the file metadata (name, size, permissions, etc.) to the archive
    // append_file_metadata_to_archive(file_path, archive_fp);
    printf(" before adding metadata \n");
    add_metadata(head, size, filename, data); // create anoither function for writing the array at one go
    zip_file = fopen("adzip.ad", "a");
    fseek(zip_file, 0, SEEK_END);
    // append the file contents to the archive
    char buffer[1024];
    size_t nread;
    printf("writing stuff \n");
    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, nread, zip_file);
    }
    printf("done with writing \n");

    fclose(file);
    fclose(zip_file);
}

// Define a structure to hold the footer and header data
struct AppendResult
{
    struct footer data2;
    struct header *head;
};

struct AppendResult append(char *filename)
{
    struct AppendResult result;
    result.data2 = get_footer_data(filename);
    // printf("%dinside append \n", result.data2.num_headers);
    // printf("%d\n", result.data2.total_file_size);
    result.head = get_header(filename);
    return result;
}

void append_files(char *filename, char *zipfile, struct header **head, int *size, struct footer *data)
{
    printf("appending file - a\n");
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }

    FILE *zip_file = fopen(zipfile, "r+");

    // // append the file metadata (name, size, permissions, etc.) to the archive
    // append_file_metadata_to_archive(file_path, archive_fp);
    printf(" before adding metadata \n");

    // append the file contents to the archive

    fseek(zip_file, data->total_file_size, SEEK_SET);
    ftruncate(fileno(zip_file), ftell(zip_file));
    char buffer[1024];
    size_t nread;
    printf("writing stuff \n");
    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, nread, zip_file);
    }
    printf("done with writing \n");
    add_metadata(head, size, filename, data); // create anoither function for writing the array at one go
    fclose(file);
    fclose(zip_file);
}
int cmpfunc(const void *a, const void *b) {
    char *s1 = *(char **)a;
    char *s2 = *(char **)b;
    int count1 = 0, count2 = 0;

    // Count the number of '/' characters in each string
    for (int i = 0; i < strlen(s1); i++) {
        if (s1[i] == '/') {
            count1++;
        }
    }

    for (int i = 0; i < strlen(s2); i++) {
        if (s2[i] == '/') {
            count2++;
        }
    }

    // Compare the counts
    return count1 - count2;
}

void heirarchy_info()
{
    struct header *head2 = get_header("adzip.ad");
    struct footer data = get_footer_data("adzip.ad");
    int size = data.num_headers;

    char **string_array = malloc(size * sizeof(char *));
    if (!string_array) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
    }

    // Copy the initial strings to the string array
    for (int i = 0; i < size; i++) {
        string_array[i] = strdup(head2[i].file_name);
        if (!string_array[i]) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
        }
    }
    qsort(string_array, size, sizeof(char *), cmpfunc);
    for (int i = 0; i < size; i++) {
    printf("%s\n", string_array[i]);
    }
}

void unzip()
{
    
    struct header *head2 = get_header("adzip.ad");
    // char path[36] ="directory/sub_dir/sub_dir3/test.txt";
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s \n", cwd);

    strcat(cwd, "/adzip.ad");

    printf("%s \n", cwd);
    mkdir("extract", 0700);
     
    struct footer data = get_footer_data("adzip.ad");
    int size = data.num_headers;
    int start_postion_file =0;

    printf("fine a\n"); 
    for( int i =0; i<size; i++)
{    
    int dir_back =0;
    char * path = head2[i].file_name;
    printf("fine a %s\n", path); 
    char s[2] = "/";
    char *token;
    if(strchr(path, '/')==NULL ){
        token = path;
    }
    else{
    token = strtok(path, s);
    }

    printf("%s token \n", token);
    chdir("extract");
    while( token != NULL ) {
        printf( " %s\n", token );

        if(strchr(token, '.')!=NULL ){
        FILE *fptr;
        FILE *fptr_read;
        fptr_read= fopen(cwd, "rb+");
        fptr = fopen(token, "wb+");
        printf("fine a inside\n"); 
        char buffer[28];  // create a buffer to hold the bytes read
        if (fptr_read == NULL) {
        perror("Failed to open input file");}
        fseek(fptr_read, start_postion_file, SEEK_SET);
        size_t bytes_read = fread(buffer, 1, 26, fptr_read );  // read 10 bytes from input file
        printf("fine a\n"); 
        fwrite(buffer, 1, bytes_read, fptr);  // write to the file 
        fclose(fptr);
        fclose(fptr_read);
    }
    else{
      if(chdir(token)!=0){
        mkdir(token, 0700);
        chdir(token);
        }}

      token = strtok(NULL, s);
      dir_back++;

   }

   start_postion_file+=head2[i].file_size;
}
}
   

// struct

// void read_file()
// {
//         // for ( int i=0; i<2; i++)
//     // {
//     //     char buffer[20];
//     //     size_t nread;
//     //     fread(buffer, 1, sizeof(buffer), zip_file);
//     //     printf("%s\n", buffer);
//     // }

//     // printf("done reading the file\n");
// }

int main()
{
    char *filename = "test.txt";
    // struct header *head = NULL;
    // int size = 0;
    // struct footer Data = {0, 0};
    // struct footer *data = &Data;

    // FILE *zip_file;
    // //
    // // int s = 0;
    // // struct footer Data = {0, 0};
    // // struct footer *data = &Data;

    // add_files(filename, zip_file, &head, &size, data);
    // write_metadata(head, size, "adzip.ad");
    // // add_files(filename2, zip_file, &head, &s, data);
    //    append_files(filename2, zip_file, &head, &size, data);
    // //

    // zip_file = fopen("adzip.ad", "a");
    // fwrite(data, sizeof(struct footer), 1, zip_file);
    // printf("data size: %d\n", data->num_headers);
    // printf("data total: %d\n", data->total_file_size);
    // // printf("out1\n");
    // fclose(zip_file);

    // struct header *head2 = get_header("adzip.ad");
    // for (int i = 0; i < 3; i++)
    // {
    //     printf("String: %s, Number:%d\n", head2[i].file_name, head2[i].file_size);
    // }

    // struct AppendResult result = append("adzip.ad");
    // struct header *head = result.head;
    // struct footer Data = result.data2;
    // struct footer *data = &Data;
    // for (int i = 0; i < Data.num_headers; i++)
    // {
    //     printf("String: %s, Number:%d\n", head[i].file_name, head[i].file_size);
    // }

    // adding third file to the zip

    // struct AppendResult result = append("adzip.ad");
    // struct header *head = result.head;

    // struct footer Data = result.data2;
    // struct footer *data = &Data;
    // int size = Data.num_headers;
    // int x = sizeof(Data);
    // for (int i = 0; i < Data.num_headers; i++)
    // {
    //     printf("String: %s, Number:%d\n", head[i].file_name, head[i].file_size);
    // }

    // append_files(filename, "adzip.ad", &head, &size, data);
    // write_metadata(head, size, "adzip.ad");
    // FILE *zip_file = fopen("adzip.ad", "a");
    // fseek(zip_file, -sizeof(x), SEEK_END);
    // fwrite(data, sizeof(struct footer), 1, zip_file);
    // printf("data size: %d\n", data->num_headers);
    // printf("data total: %d\n", data->total_file_size);
    // // // printf("out1\n");
    // fclose(zip_file);

    heirarchy_info();
    
    // unzip();

}
