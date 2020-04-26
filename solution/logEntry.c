#include "logEntry.h"

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
