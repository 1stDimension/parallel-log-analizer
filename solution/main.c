#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "mpiUtils.h"
#include "./libs/c_hashmap/hashmap.h"
#include "./libs/c-vector/cvector.h"

#define CVECTOR_LOGARITHMIC_GROWTH
#define master 0

#define LOG_FILE_NAME "fragment.log"
#define FIELD_SIZE 512

typedef struct LogEnrtry
{
    char ip[FIELD_SIZE];
    char date[FIELD_SIZE];
    char httpMethod[FIELD_SIZE];
    char url[FIELD_SIZE];
    char status[FIELD_SIZE];
} LogEnrtry;

LogEnrtry parseLogEntry(const char *logLine) {
    const char *c = logLine;
    LogEnrtry logEntry;
    int i;

    // Read ip
    for (i = 0; *c != ' '; i++)
    {
        logEntry.ip[i] = *(c++);
    }
    logEntry.ip[i] = '\0';

    // Skip to next element
    while (*c != '[') c++;
    c++;

    // Read date with minute presicion
    const int colonCountStop = 3; // For targeting precison
    int colonCount;
    for (i = 0, colonCount = 0; colonCount < colonCountStop; i++)
    {
        if (*c == ':')
            colonCount++;
        logEntry.date[i] = *(c++);
    }
    logEntry.date[i] = '\0';

    // Skip to next element
    while (*c != '"') c++;
    c++;

    // Read http method
    for (i = 0; *c != ' '; i++)
    {
        logEntry.httpMethod[i] = *(c++);
    }
    logEntry.httpMethod[i] = '\0';

    // Skip to next element
    c++;

    // Read request url
    for (i = 0; *c != ' '; i++)
    {
        logEntry.url[i] = *(c++);
    }
    logEntry.url[i] = '\0';

    // Skip to next element
    while (*c != '"') c++;
    c += 2;

    // Read response code
    for (i = 0; *c != ' '; i++)
    {
        logEntry.status[i] = *(c++);
    }
    logEntry.status[i] = '\0';

    return logEntry;
}

// Get field from single LogEntry. fieldOut should be at liest FIELD_SIZE
int getFieldFromLogEntry(const LogEnrtry* entry, const char *fieldName, char *fieldOut) {
    if (strcmp(fieldName, "addr") == 0)
        strcpy(fieldOut, entry->ip);
    else if (strcmp(fieldName, "stat") == 0)
        strcpy(fieldOut, entry->status);
    else if (strcmp(fieldName, "time") == 0)
        strcpy(fieldOut, entry->date);
    else
        return -1;

    return 0;
}

void printLogEntry(const LogEnrtry *LogEnrtry) {
    printf("ip: %s date: %s httpMethod: %s URL: %s, statusCode: %s\n",
           LogEnrtry->ip, LogEnrtry->date, LogEnrtry->httpMethod, LogEnrtry->url,
           LogEnrtry->status);
}

void mpiClenup()
{
    MPI_Finalize();
}

int printData(any_t item, any_t data) {
    int value = *(int*) data;
    printf("vlue: %d\n", value);

    return MAP_OK;
}

int main(int argc, char **argv)
{
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    atexit(mpiClenup);
    // TODO: Get selectedField from rguments
    const char* selectedField = "addr";

    // Get the number of all processes and rank of current process
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    cvector_vector_type(char *) loadedFields = NULL;

    if (world_rank == master) {
        FILE* file = fopen(LOG_FILE_NAME, "r");
        if (!file) {
            fprintf(stderr, "Unable to open read file\n");
            exit(EXIT_FAILURE);
        }

        char line[FIELD_SIZE * 10];
        while(fgets(line, sizeof(line), file)) {
            LogEnrtry entry = parseLogEntry(line);
            char *field = malloc(FIELD_SIZE * sizeof(*field));

            if (getFieldFromLogEntry(&entry, selectedField, field) != 0) {
                fprintf(stderr, "Wrong field name\n");
                for (int i = 0; i < cvector_size(loadedFields); i++)
                {
                    free(loadedFields[i]);
                }
                cvector_free(loadedFields);
                fclose(file);
                exit(EXIT_FAILURE);
            }

            cvector_push_back(loadedFields, field);
        }
        fclose(file);

        for (int i = 0; i < cvector_size(loadedFields); i++)
        {
            printf("%s\n", loadedFields[i]);
        }

    }

    char testFields[3][FIELD_SIZE];
    strcpy(testFields[0], "23.95.35.93");
    strcpy(testFields[1], "192.3.91.67");
    strcpy(testFields[2], "23.95.35.93");

    // Create data type for log field
    MPI_Datatype dt_field;
    MPI_Type_contiguous(FIELD_SIZE, MPI_CHAR, &dt_field);
    MPI_Type_commit(&dt_field);

    // Prepare fo Scatterv
    int sizes[world_size];
    int skips[world_size];

    prepareDataForAsymetricOperaton(world_size, 3, sizes, skips);
    printf("%d -> size: %d skip: %d\n", world_rank, sizes[world_rank], skips[world_rank]);

    char myFields[3][FIELD_SIZE];
    MPI_Scatterv(testFields, sizes, skips, dt_field, myFields, sizes[world_rank], dt_field, master, MPI_COMM_WORLD);

    for (int i = 0; i < sizes[world_rank]; i++)
    {
        printf("%d -> myFields[%d] = %s\n", world_rank, i, myFields[i]);
    }

    if (world_rank == master) {
        for (int i = 0; i < cvector_size(loadedFields); i++)
        {
            free(loadedFields[i]);
        }
        cvector_free(loadedFields);
    }

    MPI_Type_free(&dt_field);
    return EXIT_SUCCESS;
}