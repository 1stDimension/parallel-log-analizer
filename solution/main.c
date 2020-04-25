#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "./libs/c_hashmap/hashmap.h"

#define master 0

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
    MPI_Init(NULL, NULL);
    atexit(mpiClenup);

    // Get the number of all processes and rank of current process
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == master) {
        const char *logLine = "23.95.35.93 - - [12/Apr/2015:06:45:06 +0200] \"POST /xmlrpc.php HTTP/1.0\" 403 497 \"-\" \"Mozilla/5.0 (compatible; Googlebot/2.1;  http://www.google.com/bot.html)\"";
        LogEnrtry entry = parseLogEntry(logLine);
        printLogEntry(&entry);
    }

    return EXIT_SUCCESS;
}