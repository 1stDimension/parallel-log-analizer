#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "./libs/c_hashmap/hashmap.h"

#define master 0

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
    MPI_Init(NULL, NULL);
    atexit(mpiClenup);

    // Get the number of all processes and rank of current process
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == master) {
        int error;
        map_t mymap;
        mymap = hashmap_new();

        int values[] = {7, 3, 5};

        hashmap_put(mymap, "aa", &values[0]);
        hashmap_put(mymap, "ba", &values[1]);

        int *out;
        hashmap_get(mymap, "aa", (void **) &out);
        *out = (*out) + 1;

        hashmap_get(mymap, "aa", (void **) &out);

        printf("Retrived value: %d\n", *out);

        hashmap_iterate(mymap, printData, NULL);

        hashmap_free(mymap);
    }

    return EXIT_SUCCESS;
}