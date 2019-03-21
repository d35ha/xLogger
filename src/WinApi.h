#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define API_SIZE 0x90
#define CONST_SIZE 0x64
#define PARAMETERS_COUNT 0x14
#define VALUES_COUNT 0x800
#define BASE_SIZE 0x32
#define LIB_FILE_PATH_SIZE 0x64
#define NUMBER_OF_LIBS 0x170
#define MODULE_SIZE 0x28
#define TYPE_SIZE 0x4
#define HEADERS_PER_API 0x5
#define TYPE_DISPLAY_SIZE 0x14
#define LIB_CONTENT_SIZE 0x30000
#define VALUE_SIZE 0xB
#define PARAMETER_SIZE 0x50
#define HEADER_SIZE 0x32

struct ParameterData
{
    char DataType[PARAMETER_SIZE + 1];
    char DataVar[PARAMETER_SIZE + 1];
};

struct ValueData
{
    char Const[CONST_SIZE + 1];
    long long int Value;
};

struct ApiData
{
    int ParamCount;
    char Api[API_SIZE + 1];
    char SourceModule[MODULE_SIZE + 1];
    char Headers[HEADERS_PER_API + 1][MODULE_SIZE + 1];
    struct ParameterData Parameters[PARAMETERS_COUNT + 1];
} ApiInfo;

struct HeaderData
{
    char MainHeader[HEADER_SIZE + 1];
    char Type[TYPE_SIZE + 1];
    char Base[BASE_SIZE + 1];
    char Header[MODULE_SIZE + 1];
    char TypeDisplay[TYPE_DISPLAY_SIZE + 1];
    struct ValueData Values[VALUES_COUNT + 1];
} HeaderInfo;

struct CachedLibData
{
    char LibName[MODULE_SIZE + 1];
    char *LibData;
};

struct Cache
{
    struct CachedLibData LibsData[NUMBER_OF_LIBS + 1];
    int ApisLibsStartingIndex;
    int HeadersLibsStartingIndex;
} CachedData;

void LoadWinApiData();
void GetApiData(char *Module, char *Api);
void GetHeaderData(char *Module, char *Header);