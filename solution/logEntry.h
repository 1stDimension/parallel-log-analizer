#pragma once

#include <stdio.h>
#include <string.h>

#define FIELD_SIZE 512

typedef struct LogEnrtry
{
    char ip[FIELD_SIZE];
    char date[FIELD_SIZE];
    char httpMethod[FIELD_SIZE];
    char url[FIELD_SIZE];
    char status[FIELD_SIZE];
} LogEnrtry;

LogEnrtry parseLogEntry(const char *logLine);

// Get field from single LogEntry. fieldOut should be at liest FIELD_SIZE
int getFieldFromLogEntry(const LogEnrtry* entry, const char *fieldName, char *fieldOut);

void printLogEntry(const LogEnrtry *LogEnrtry);
