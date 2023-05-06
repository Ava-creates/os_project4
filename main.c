#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define num_files 20

struct footer{
    int total_file_size; // sum of all the file sizes
    int num_headers; //total number of files/headers we have
};

struct header
{
    char* file_name;
    int file_size;
    mode_t file_mode;
    uid_t file_uid;
    gid_t file_gid;
    time_t file_mtime;
};

void add_metadata(char* filename, FILE* zip_file, int n, struct footer* data)
{
    struct header head; 
    struct stat file_stat;
    zip_file=fopen("adzip.ad", "r+");
    // printf("a\n");
    char * file_path= realpath(filename, NULL); //to get the path of the file 
    if (stat(file_path, &file_stat) == -1) {
        fprintf(stderr, "Error: could not retrieve file metadata for %s\n", file_path);
        return;
    }
    char* file_name = strrchr(file_path, '/') + 1;
    head.file_gid= file_stat.st_gid;
    head.file_uid= file_stat.st_uid;
    head.file_name= file_name;
    head.file_mtime= file_stat.st_mtime;
    head.file_mode= file_stat.st_mode;
    head.file_size= file_stat.st_size;
    // printf("b\n");
    data->total_file_size+=file_stat.st_size;
    data->num_headers+=1;
    // printf("c\n");
    
    fseek(zip_file, n*sizeof(head), SEEK_SET); 
    fwrite(&head, sizeof(head), 1, zip_file);
    fclose(zip_file);
    // printf("d\n");

}
// void add_files(char* filename, FILE* zip_file)
// {

// }



int main()
{
   char* filename="test.txt";
   char* filename2= "test2.txt";
   FILE* zip_file;
   struct header head ;
   struct footer Data= {0, 0};
   struct footer* data= &Data;

    add_metadata(filename, zip_file, 0, data);
    printf("%d\n", data->total_file_size);
    printf("%d\n", data->num_headers);

    printf("after addign metadata 1\n");
    add_metadata(filename2, zip_file, 1, data);
    printf("%d\n", data->total_file_size);
    printf("%d\n", data->num_headers);
    printf("after addign metadata 2\n");

    zip_file = fopen("adzip.ad", "rb+");
    printf("out1\n");
    fseek(zip_file,0 , SEEK_END );
    fwrite(data, sizeof(struct footer), 1, zip_file);
    printf("out1\n");
    fclose(zip_file);



   zip_file = fopen("adzip.ad", "rb+");
    printf("in1\n");
   for (int i = 0; i <2  && fread(&head, sizeof(head), 1, zip_file) == 1; i++) {   
    printf("%s\n", head.file_name);
    printf("%d\n", head.file_gid);
    printf("%d\n", head.file_size);
    }
    printf("out\n");
    struct footer data2 ;
    
    fseek(zip_file,0 , SEEK_END );
    long size =  ftell(zip_file) - sizeof(struct footer);
    fseek(zip_file,size, SEEK_SET);
  
    fread(&data2, sizeof(struct footer), 1, zip_file);
    printf("%d\n", data2.total_file_size);
    printf("%d\n", data2.num_headers);
    fclose(zip_file);

}
