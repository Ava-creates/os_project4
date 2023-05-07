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
    char *file_name;
    int file_size;
    mode_t file_mode;
    uid_t file_uid;
    gid_t file_gid;
    time_t file_mtime;
    char *file_path; 
};



struct footer get_footer_data(FILE* zip_file)
{
    zip_file = fopen("adzip.ad", "rb+");
    // printf("in1\n");
    struct footer data2 ;
    fseek(zip_file,0 , SEEK_END );
    long size2 = ftell(zip_file)- sizeof(struct footer);
    fseek(zip_file, size2 , SEEK_SET);
    fread(&data2, sizeof(struct footer), 1, zip_file) ;   //we have footer 
    fclose(zip_file);
    return data2;
}

struct header * get_header(FILE* zip_file )
{
    struct footer data = get_footer_data(zip_file);
    printf("%din side header\n", data.num_headers);
    printf("%d\n", data.total_file_size);
    zip_file = fopen("adzip.ad", "rb+"); //reading the metadat a
    fseek(zip_file, data.total_file_size , SEEK_SET);

    struct header *head2 = malloc(data.num_headers* sizeof(struct header)) ;

    if(fread(head2, sizeof(struct header), data.num_headers, zip_file) !=1 )
    {
        perror("error in reading \n");
    } 

    
    for (int i = 0; i < data.num_headers; i++) {   

    printf("int he loop \n");
    // head2[i].file_name=malloc(20 * sizeof(char));
    // head2[i].file_name[strlen(head2[i].file_name)-1]='\0';
    // printf("%s\n", head2[i].file_name);
    printf("%d\n", head2[i].file_gid);
    printf("%d\n", head2[i].file_size);
    printf("%d\n", head2[i].file_uid);
    printf("%hu\n", head2[i].file_mode);
    // printf("%s\n", head2[i].file_path);
    }
    printf("out\n");

    fclose(zip_file);
    return head2;
}

void add_metadata(char* filename,  struct footer* data, struct header **head, int* size)  //
{
    struct header head2; 
    struct stat file_stat;
    
    // printf("a\n");
    char * file_path= realpath(filename, NULL); //to get the path of the file 
    if (stat(file_path, &file_stat) == -1) {
        fprintf(stderr, "Error: could not retrieve file metadata for %s\n", file_path);
        return;
    }
    char* file_name = strrchr(file_path, '/') + 1;
    head2.file_path = file_path;
    head2.file_gid= file_stat.st_gid;
    head2.file_uid= file_stat.st_uid;
    head2.file_name= file_name;
    head2.file_mtime= file_stat.st_mtime;
    head2.file_mode= file_stat.st_mode;
    head2.file_size= file_stat.st_size;
    // printf("b\n");
    data->total_file_size+=file_stat.st_size;
    data->num_headers+=1;
    // printf("c\n");

    (*size)++; 
    *head = realloc(*head, (*size)*sizeof(struct header)); 

    (*head)[(*size)-1]= head2; 

    // printf("d\n");

}

void write_metadata( FILE* zip_file, struct  header *head, int size ){
    //     for (int i= 0; i<size; i++ )
    // {
    //    printf("printing the file name: %s  \n", head[i].file_name);
    // }
    zip_file = fopen("adzip.ad", "a");
    fseek(zip_file, 0, SEEK_END);    // at end of file 
    fwrite(head, sizeof(struct header), size, zip_file);
    fclose(zip_file);
}

void add_files(char* filename, FILE* zip_file, struct  header **head , int*size, struct footer* data )
{
    printf("adding file - a\n");
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }

    zip_file=fopen("adzip.ad", "a");

    // // append the file metadata (name, size, permissions, etc.) to the archive
    // append_file_metadata_to_archive(file_path, archive_fp);
     printf(" before adding metadata \n");
    add_metadata(filename, data,  head,  size);   // create anoither function for writing the array at one go 

    // append the file contents to the archive
    char buffer[1024];
    size_t nread;
    printf("writing stuff \n");
    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        fwrite(buffer, 1, nread, zip_file);
    }
    printf("done with writing \n");
    fclose(file);
    fclose(zip_file);
}


void append_files(char* filename, FILE* zip_file, struct  header **head , int*size, struct footer* data)
{
    *data =  get_footer_data (zip_file);
    printf("%dinside append \n", data->num_headers);
    printf("%d\n", data->total_file_size);
    *head = get_header(zip_file);

    zip_file = fopen("adzip.ad", "rb+");

    fseek(zip_file, data->total_file_size , SEEK_SET);
    add_files(filename, zip_file, head, size, data);



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
//    char* filename="test.txt";
//    char* filename2= "test2.txt";
   FILE* zip_file;
//    int size =0 ; 
//    struct header * head = NULL;
//    struct footer Data= {0, 0};
//    struct footer* data= &Data;
  
//    append_files(filename, zip_file, &head, &size, data);
//    append_files(filename2, zip_file, &head, &size, data);
//    write_metadata(zip_file,head, size);

    // zip_file = fopen("adzip.ad", "a");
    // fwrite(data, sizeof(struct footer), 1, zip_file);
    // printf("out1\n");
    // fclose(zip_file);

    get_header(zip_file);


    
}
