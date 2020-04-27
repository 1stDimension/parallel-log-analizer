#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "mpiUtils.h"
#include "logEntry.h"
#include "./libs/c_hashmap/hashmap.h"
#include "./libs/c-vector/cvector.h"

#define CVECTOR_LOGARITHMIC_GROWTH
#define master 0

// TODO: Read this from input
#define LOG_FILE_NAME "fragment.log"

// For atexit()
void mpiClenup()
{
    MPI_Finalize();
}

// Helper utility struct
typedef struct ArrayAndIterator
{
    void *array;
    int i;
} ArrayAndIterator;

typedef struct FieldAndCount
{
    char field[FIELD_SIZE];
    int count;
} FieldAndCount;

// Function for itterating over hash map and displaying each Field and it's coccurence count
// First argument can accept process id or NULL
int printMapOfFieldsAndCount(any_t item, any_t data);

// Function for itterating over hash map and moving each value to simple array
// Each value from hashmap is freed()
int coppyMapOfFieldsAndCountToArray(any_t item, any_t data);

// Generate map of each field and it's coressponding occurence count from array of fields
// For mapping filds by single process
void mapDataToOccuranceCount(char fields[][FIELD_SIZE], int fieldCount, map_t* mapOut);

void parseArguments(int argc, char **argv, char **selectedField, char **fileName)
{
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 3)
    {
        // TODO: List possible fields
        if (world_rank == master)
            printf("Invalid argument count.\nProgram expects 2 arguments: <field_selection> <log_file_name>\n"
                   "Example usage: mpirun -n 4 ./log-analizer addr fragment.log\n\n");
        exit(EXIT_FAILURE);
    }

    *selectedField = argv[1];
    *fileName = argv[2];
}

int main(int argc, char **argv)
{
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    atexit(mpiClenup);
    char* selectedField = NULL;
    char *fileName = NULL;


    // Get the number of all processes and rank of current process
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    parseArguments(argc, argv, &selectedField, &fileName);
    cvector_vector_type(char *) loadedFields = NULL;

    // ---------- Load selected fields from file ----------
    if (world_rank == master) {
        FILE* file = fopen(fileName, "r");
        if (!file) {
            fprintf(stderr, "Unable to open read file\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
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
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

            cvector_push_back(loadedFields, field);
        }
        fclose(file);
    }

    // ---------- Send parts of data to each process ----------
    // Prepare for Scatterv
    int *sizes;
    int *skips;
    if (world_rank == master)
    {
        sizes = malloc(world_size * sizeof(*sizes));
        skips = malloc(world_size * sizeof(*skips));
        prepareDataForAsymetricOperaton(world_size, cvector_size(loadedFields), sizes, skips);
    }

    // Create data type for log field
    MPI_Datatype dt_field;
    MPI_Type_contiguous(FIELD_SIZE, MPI_CHAR, &dt_field);
    MPI_Type_commit(&dt_field);

    int mySize;
    MPI_Scatter(sizes, 1, MPI_INT, &mySize, 1, MPI_INT, master, MPI_COMM_WORLD);
#ifdef DEBUG
    printf("%d -> size: %d\n", world_rank, mySize);
#endif
    char myPart[mySize][FIELD_SIZE];

    // Loaded data needs to be flattened before scatter
    // To avoid this vector could store field sturctures but this allows me to try out MPI_Type_contiguous()
    char **flattendedData = NULL;
    if (world_rank == master)
    {
        flattendedData = malloc(cvector_size(loadedFields) * FIELD_SIZE * sizeof(*loadedFields));
        char* p = (char*) flattendedData;
        for (int i = 0; i < cvector_size(loadedFields); i++)
        {
            memcpy(p, loadedFields[i], FIELD_SIZE * sizeof(*loadedFields));
            p += FIELD_SIZE;
            free(loadedFields[i]); // Now vector can be freed
        }
        cvector_free(loadedFields);
    }

    // Scatter data
    MPI_Scatterv(flattendedData, sizes, skips, dt_field, myPart, mySize, dt_field, master, MPI_COMM_WORLD);
    free(flattendedData);

#ifdef DEBUG
    for (int i = 0; i < mySize; i++)
    {
        // if (world_rank == master)
        printf("%d -> myPart[%d] = %s\n", world_rank, i, myPart[i]);
    }
#endif

    // ---------- Map ----------
    map_t myMap = hashmap_new();
    mapDataToOccuranceCount(myPart, mySize, &myMap);

#ifdef DEBUG
    hashmap_iterate(myMap, printMapOfFieldsAndCount, &world_rank);
#endif

    // ---------- Reduce ----------
    // Coppy mapping results to simple array
    int myMapLength = hashmap_length(myMap);

    FieldAndCount mappings[myMapLength];
    ArrayAndIterator arrayAndIterator;
    arrayAndIterator.array = mappings;
    arrayAndIterator.i = 0;

    hashmap_iterate(myMap, coppyMapOfFieldsAndCountToArray, &arrayAndIterator);
#ifdef DEBUG
    for (int i = 0; i < myMapLength; i++)
    {
        printf("%d => field: %s count %d\n", world_rank, mappings[i].field, mappings[i].count);
    }
#endif

    // --- Gatter data at master ---
    // Prepare data for Gatterv
    MPI_Gather(&myMapLength, 1, MPI_INT, sizes, 1, MPI_INT, master, MPI_COMM_WORLD);
    int allMappingLenght = 0;
    FieldAndCount* allMappings = NULL;

    if (world_rank == master)
    {
        for (int i = 0; i < world_size; i++)
            allMappingLenght += sizes[i];

        allMappings = malloc(allMappingLenght * sizeof(*allMappings));
        skips[0] = 0;
        for (int i = 1; i < world_size; i++)
        {
            skips[i] = skips[i-1] + sizes[i-1];
        }
    }

    // Create MPI Datatype
    MPI_Datatype dt_field_and_count;
    int blockLenghts[] = {FIELD_SIZE, 1};
    MPI_Aint displacements[] = {offsetof(FieldAndCount, field), offsetof(FieldAndCount, count)};
    MPI_Datatype types[] = {MPI_CHAR, MPI_INT};
    MPI_Type_create_struct(2, blockLenghts, displacements, types, &dt_field_and_count);
    MPI_Type_commit(&dt_field_and_count);

    MPI_Gatherv(mappings, myMapLength, dt_field_and_count, allMappings,
                sizes, skips, dt_field_and_count, master, MPI_COMM_WORLD);

    if (world_rank == master)
    {
#ifdef DEBUG
        for (int i = 0; i < allMappingLenght; i++)
        {
            printf("allMappings[%d] = field: %s count: %d\n", i, allMappings[i].field, allMappings[i].count);
        }
#endif

        // Actual reduction
        map_t finalMap = hashmap_new();
        for (int i = 0; i < allMappingLenght; i++)
        {
            FieldAndCount *fieldAndCount;
            int status = hashmap_get(finalMap, allMappings[i].field, (void**)(&fieldAndCount));
            if (status == MAP_MISSING)
            {
                // This is a bit distructive to allMappings[] but it's no longer needed
                hashmap_put(finalMap, allMappings[i].field, &allMappings[i]);
            }
            else
            {
                fieldAndCount->count += allMappings[i].count;
            }
        }
#ifdef DEBUG
        printf("\nFinal results:\n");
#endif
        hashmap_iterate(finalMap, printMapOfFieldsAndCount, NULL);

        hashmap_free(finalMap);
    }

    // Free resources
    hashmap_free(myMap);
    if (world_rank == master) {
        free(allMappings);
        free(sizes);
        free(skips);
    }

    MPI_Type_free(&dt_field_and_count);
    MPI_Type_free(&dt_field);
    return EXIT_SUCCESS;
}

int printMapOfFieldsAndCount(any_t item, any_t data)
{
    FieldAndCount *value = (FieldAndCount *) data;
    if (item != NULL)
    {
        int processRank = *(int *) item;
        printf("%d: %s -> %d\n", processRank, value->field, value->count);
    }
    else
    {
        printf("%s -> %d\n", value->field, value->count);
    }

    return MAP_OK;
}

int coppyMapOfFieldsAndCountToArray(any_t item, any_t data)
{
    FieldAndCount *value = (FieldAndCount *) data;
    ArrayAndIterator *arrayAndIterator = (ArrayAndIterator *) item;
    FieldAndCount *outputArray = arrayAndIterator->array;

    memcpy(outputArray + arrayAndIterator->i, value, sizeof(*value));
    arrayAndIterator->i += 1;

    free(value);
    return MAP_OK;
}

void mapDataToOccuranceCount(char fields[][FIELD_SIZE], int fieldCount, map_t* mapOut)
{
    for (int i = 0; i < fieldCount; i++)
    {
        FieldAndCount *fieldAndCount;
        int status = hashmap_get(*mapOut, fields[i], (void**)(&fieldAndCount));
        if (status == MAP_MISSING)
        {
            fieldAndCount = malloc(sizeof(*fieldAndCount));
            strcpy(fieldAndCount->field, fields[i]);
            fieldAndCount->count = 1;
            if (hashmap_put(*mapOut, fieldAndCount->field, fieldAndCount) != MAP_OK)
            {
                fprintf(stderr, "Unable to put value to map");
                exit(EXIT_FAILURE);
            }
        }
        else //Increment value
        {
            fieldAndCount->count += 1;
        }
    }
}
