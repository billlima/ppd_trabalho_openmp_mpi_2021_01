#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>
#include <json-c/json.h>

#define MAX_LINE_LENGTH 50
#define MAX_LINES 10

typedef struct 
{
    char lines[MAX_LINES][MAX_LINE_LENGTH];
} CHAR_ARRAY;

void generateSettingsFile(int size) {
    FILE *file;
    file = fopen("data/file.json", "w");
    fputs("{\"lines\": [\"Linha1\"]}", file);
    fclose(file);

    file = fopen("data/settings.txt", "w");

    char sizeC[2];
    sprintf(sizeC, "%d", size-1);

    char users[10] = "users|";
    strcat(users, sizeC);

    fputs(users, file);
    fclose(file);
}

void generateUserFile(int userId) {
    char filename[18] = "data/user";
    char intC[2];
    sprintf(intC, "%d", userId);
    strcat(filename, intC);
    strcat(filename, ".txt");

    FILE *file;
    file = fopen(filename, "w");
    fclose(file);
}

int get_file_contents(const char *filename, char **outbuffer) {
    FILE *file = NULL;
    long filesize;
    const int blocksize = 1;
    size_t readsize;
    char *filebuffer;

    // Open the file
    file = fopen(filename, "r");
    if (NULL == file)
    {
        printf("'%s' not opened\n", filename);
        exit(EXIT_FAILURE);
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);

    // Allocate memory for the file contents
    filebuffer = (char *)malloc(sizeof(char) * filesize);
    *outbuffer = filebuffer;
    if (filebuffer == NULL)
    {
        fputs("malloc out-of-memory", stderr);
        exit(EXIT_FAILURE);
    }

    // Read in the file
    readsize = fread(filebuffer, blocksize, filesize, file);
    if (readsize != filesize)
    {
        fputs("didn't read file completely", stderr);
        exit(EXIT_FAILURE);
    }

    // Clean exit
    fclose(file);
    return EXIT_SUCCESS;
}

void getFileContent() {

    char *filename = "data/file.json";
    char *buffer = NULL;

    get_file_contents(filename, &buffer);

    struct json_object *parsed_json;
    struct json_object *dados;
    
    size_t n_dados;
    size_t i;

    parsed_json = json_tokener_parse(buffer);

    json_object_object_get_ex(parsed_json, "lines", &dados);
    int n_lines = json_object_array_length(dados);

    for (i = 0; i < n_lines; i++) {
        const char *dado = json_object_to_json_string(json_object_array_get_idx(dados, i));
        printf("%ld %s\n",i+1,  dado);
    }
}

int main(int argc, char **argv)
{
    int rank, size; 
    int root = 0; //quem vai distribuir os dados

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int settingsOk = 0;
    
    for (;;) {
        
        if (settingsOk == 0) {
            if (rank == root) {
                generateSettingsFile(size);
                settingsOk = 1;

                printf("Generate settings files\n");

                getFileContent();
            } else {
                generateUserFile(rank);
                settingsOk = 1;

                printf("Generate userFile %d\n", rank);
            }
        } 
        
        sleep(0.5);
    }


    MPI_Finalize();
}

