
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dynamic_string_struct {
    char *string;
    int number;
};

void add_element(struct dynamic_string_struct **array, int *size, char *string, int number) {
    // Allocate memory for a new struct
    struct dynamic_string_struct *new_elem = malloc(sizeof(struct dynamic_string_struct));
    // Allocate memory for the string in the struct
    new_elem->string = malloc(strlen(string) + 1);
    // Copy the string into the struct
    strcpy(new_elem->string, string);
    new_elem->number = number;
    // Add the new element to the array
    (*size)++;
    *array = realloc(*array, (*size) * sizeof(struct dynamic_string_struct));
    (*array)[*size - 1] = *new_elem;
    free(new_elem);
}

void write_struct_to_file(struct dynamic_string_struct *array, int size, char *filename) {
    FILE *fp;
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    fwrite(&size, sizeof(int), 1, fp);
    for (int i = 0; i < size; i++) {
        int string_len = strlen(array[i].string) + 1; // Include null terminator
        fwrite(&string_len, sizeof(int), 1, fp);
        fwrite(array[i].string, sizeof(char), string_len, fp);
        fwrite(&array[i].number, sizeof(int), 1, fp);
    }
    fclose(fp);
}

struct dynamic_string_struct *read_struct_from_file(char *filename, int *size) {
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return NULL;
    }
    fread(size, sizeof(int), 1, fp);
    struct dynamic_string_struct *array = malloc((*size) * sizeof(struct dynamic_string_struct));
    for (int i = 0; i < *size; i++) {
        int string_len;
        fread(&string_len, sizeof(int), 1, fp);
        array[i].string = malloc(string_len * sizeof(char));
        fread(array[i].string, sizeof(char), string_len, fp);
        fread(&array[i].number, sizeof(int), 1, fp);
    }
    fclose(fp);
    return array;
}

int main() {
    // Create an array of dynamic_string_structs
    int size = 0;
    struct dynamic_string_struct *array = NULL;

    // Add elements to the array
    add_element(&array, &size, "foo", 42);
    add_element(&array, &size, "bar", 123);

    // Write the array to a file
    write_struct_to_file(array, size, "test.bin");

    // Read the array back from the file
    int new_size;
    struct dynamic_string_struct *new_array = read_struct_from_file("test.bin", &new_size);
    if (new_array == NULL) {
        return 1;
    }

    // Print the new array
    printf("New array:\n");
    for (int i = 0; i < new_size; i++) {
        printf("String: %s, Number:%d\n", new_array[i].string, new_array[i].number);
    }

}

