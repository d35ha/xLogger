#include <WinApi.h>

#define APIS_DIRECTORY "..\\apis_def"
#define HEADERS_DIRECTORY "..\\headers"

int ReadLibFile(char *file, char *content, int size)
{
    *content = 0;
    int length = 0;
    FILE *handler = fopen(file, "r");
    if (!handler)
    {
        return 0;
    }
    size -= 2;
    *content = '\n';
    length++;
    *content++;
    while (1)
    {
        *content = getc(handler);
        if (!size-- | *content == EOF | !*content)
        {
            *content = '\n';
            length++;
            *content++;
            *content = 0;
            break;
        }
        length++;
        *content++;
    }
    fclose(handler);
    return length;
}

void GetApiData(char *Module, char *Api)
{
    int index = CachedData.ApisLibsStartingIndex;
    *ApiInfo.Api = 0;
    while (*CachedData.LibsData[index].LibName)
    {
        if (index == CachedData.HeadersLibsStartingIndex)
        {
            break;
        }
        if (!strcmp(CachedData.LibsData[index].LibName, Module))
        {
            int length = strlen(Api);
            if (length > API_SIZE)
            {
                length = API_SIZE;
            }
            strncpy(ApiInfo.Api, Api, length);
            ApiInfo.Api[length] = 0;
            break;
        }
        index++;
    }
    if (!*ApiInfo.Api)
    {
        goto EXIT;
    }
    char *api = ApiInfo.Api;
    char api_in_brackets[API_SIZE + 3];
    sprintf(api_in_brackets, "[%s]", api);
    char *apipointer = strstr(CachedData.LibsData[index].LibData, api_in_brackets);

    if (!apipointer)
    {
        if (*api == '_')
        {
            while (*api == '_')
            {
                api++;
            }
            sprintf(api_in_brackets, "[%s]", api);
            apipointer = strstr(CachedData.LibsData[index].LibData, api_in_brackets);
        }
    }

    if (!apipointer)
    {
        int length = strlen(api);
        if (api[length - 1] == 'A' | api[length - 1] == 'W')
        {
            char *apiname = malloc(length);
            if (!apiname)
            {
                goto EXIT;
            }
            strncpy(apiname, api, length - 1);
            apiname[length - 1] = 0;
            sprintf(api_in_brackets, "[%s]", apiname);
            free(apiname);
            apipointer = strstr(CachedData.LibsData[index].LibData, api_in_brackets);
        }
    }

    if (!apipointer)
    {
        goto EXIT;
    }

    apipointer += strlen(api_in_brackets);
    api = apipointer;
    apipointer = strstr(api, "@=");

    if (!apipointer)
    {
        goto EXIT;
    }

    int apidatasize = apipointer - api;
    char *apidata = malloc(apidatasize + 2);
    if (!apidata)
    {
        goto EXIT;
    }
    strncpy(apidata, api, apidatasize);
    apidata[apidatasize] = '\n';
    apidata[apidatasize + 1] = 0;

    apipointer = strstr(apidata, "ParamCount=");
    if (!apipointer)
    {
        ApiInfo.ParamCount = 0;
    }
    else
    {
        ApiInfo.ParamCount = strtol(apipointer + 11, 0, 10);
    }

    apipointer = strstr(apidata, "SourceModule=");
    if (!apipointer)
    {
        *ApiInfo.SourceModule = 0;
    }
    else
    {
        char *lineend = strstr(apipointer, "\n");
        apipointer += 13;
        int length = lineend - apipointer;
        if (length > MODULE_SIZE)
        {
            length = MODULE_SIZE;
        }
        strncpy(ApiInfo.SourceModule, apipointer, length);
        ApiInfo.SourceModule[length] = 0;
    }

    apipointer = strstr(apidata, "Header=");
    if (!apipointer)
    {
        **ApiInfo.Headers = 0;
    }
    else
    {
        char *lineend = strstr(apipointer, "\n");
        apipointer += 7;
        int i = 0;
        while (1)
        {
            char *nextend = strstr(apipointer, ";");
            if (!nextend | lineend <= nextend | i == HEADERS_PER_API)
            {
                *ApiInfo.Headers[i] = 0;
                break;
            }
            int length = nextend - apipointer;
            if (length > MODULE_SIZE)
            {
                length = MODULE_SIZE;
            }
            strncpy(ApiInfo.Headers[i], apipointer, length);
            ApiInfo.Headers[i][length] = 0;
            apipointer = nextend + 1;
            i++;
        }
    }
    if (!ApiInfo.ParamCount)
    {
        *(*ApiInfo.Parameters).DataVar = 0;
        *(*ApiInfo.Parameters).DataType = 0;
    }
    else
    {
        char index[5];
        char *parameterdata = malloc(PARAMETER_SIZE + 1);
        if (!parameterdata)
        {
            free(apidata);
            goto EXIT;
        }
        int i, length, offset;
        if (ApiInfo.ParamCount > PARAMETERS_COUNT)
        {
            ApiInfo.ParamCount = PARAMETERS_COUNT;
        }
        for (i = 0; i < ApiInfo.ParamCount; i++)
        {
            sprintf(index, "%d=", i + 1);
            apipointer = strstr(apidata, index);
            if (!apipointer)
            {
                free(apidata);
                free(parameterdata);
                goto EXIT;
            }
            char *lineend = strstr(apipointer, "\n");
            apipointer += 2;
            length = lineend - apipointer;
            if (length > PARAMETER_SIZE)
            {
                length = PARAMETER_SIZE;
            }
            strncpy(parameterdata, apipointer, length);
            parameterdata[length] = 0;
            char *separator = strrchr(parameterdata, ' ');
            if (!separator)
            {
                free(apidata);
                free(parameterdata);
                goto EXIT;
            }
            offset = separator - parameterdata;
            separator = (char *)apipointer + offset;
            length = separator - apipointer;
            if (length > PARAMETER_SIZE)
            {
                length = PARAMETER_SIZE;
            }
            strncpy(ApiInfo.Parameters[i].DataType, apipointer, length);
            ApiInfo.Parameters[i].DataType[length] = 0;
            separator++;
            length = lineend - separator;
            if (length > PARAMETER_SIZE)
            {
                length = PARAMETER_SIZE;
            }
            strncpy(ApiInfo.Parameters[i].DataVar, separator, length);
            ApiInfo.Parameters[i].DataVar[length] = 0;
        }
        *ApiInfo.Parameters[i].DataType = 0;
        *ApiInfo.Parameters[i].DataVar = 0;
        free(parameterdata);
    }

    free(apidata);

    if (*ApiInfo.SourceModule)
    {
        return GetApiData(ApiInfo.SourceModule, Api);
    }

    return;

EXIT:
    ApiInfo.ParamCount = 0;
    *ApiInfo.Api = 0;
    *ApiInfo.SourceModule = 0;
    **ApiInfo.Headers = 0;
    *(*ApiInfo.Parameters).DataVar = 0;
    *(*ApiInfo.Parameters).DataType = 0;
    return;
}

void GetHeaderData(char *Module, char *Header)
{

    int index = CachedData.HeadersLibsStartingIndex;
    *HeaderInfo.MainHeader = 0;
    while (*CachedData.LibsData[index].LibName)
    {
        if (!strcmp(CachedData.LibsData[index].LibName, Module))
        {
            int length = strlen(Header);
            if (length > HEADER_SIZE)
            {
                length = HEADER_SIZE;
            }
            strncpy(HeaderInfo.MainHeader, Header, length);
            HeaderInfo.MainHeader[length] = 0;
            break;
        }
        index++;
    }
    if (!*HeaderInfo.MainHeader)
    {
        goto EXIT;
    }

    char *mainheader = HeaderInfo.MainHeader;
    char header_in_brackets[HEADER_SIZE + 5];
    sprintf(header_in_brackets, "\n[%s]\n", mainheader);
    char *headerpointer = strstr(CachedData.LibsData[index].LibData, header_in_brackets);

    if (!headerpointer)
    {
        goto EXIT;
    }

    headerpointer += strlen(header_in_brackets);
    mainheader = headerpointer;
    headerpointer = strstr(mainheader, "\n[");

    if (!headerpointer)
    {
        headerpointer = (char *)mainheader + strlen(mainheader);
    }

    int headerdatasize = headerpointer - mainheader;
    char *headerdata = malloc(headerdatasize + 2);

    if (!headerdata)
    {
        goto EXIT;
    }
    strncpy(headerdata, mainheader, headerdatasize);
    headerdata[headerdatasize] = '\n';
    headerdata[headerdatasize + 1] = 0;

    headerpointer = strstr(headerdata, "TypeDisplay=");
    if (!headerpointer)
    {
        *HeaderInfo.TypeDisplay = 0;
    }
    else
    {
        char *lineend = strstr(headerpointer, "\n");
        headerpointer += 12;
        int length = lineend - headerpointer;
        if (length > TYPE_DISPLAY_SIZE)
        {
            length = TYPE_DISPLAY_SIZE;
        }
        strncpy(HeaderInfo.TypeDisplay, headerpointer, length);
        HeaderInfo.TypeDisplay[length] = 0;
    }

    headerpointer = strstr(headerdata, "Base=");
    if (!headerpointer)
    {
        *HeaderInfo.Base = 0;
    }
    else
    {
        char *lineend = strstr(headerpointer, "\n");
        headerpointer += 5;
        int length = lineend - headerpointer;
        if (length > BASE_SIZE)
        {
            length = BASE_SIZE;
        }
        strncpy(HeaderInfo.Base, headerpointer, length);
        HeaderInfo.Base[length] = 0;
    }

    headerpointer = strstr(headerdata, "Type=");
    if (!headerpointer)
    {
        *HeaderInfo.Type = 0;
    }
    else
    {
        char *lineend = strstr(headerpointer, "\n");
        headerpointer += 5;
        int length = lineend - headerpointer;
        if (length > TYPE_SIZE)
        {
            length = TYPE_SIZE;
        }
        strncpy(HeaderInfo.Type, headerpointer, length);
        HeaderInfo.Type[length] = 0;
    }

    headerpointer = strstr(headerdata, "Header=");
    if (!headerpointer)
    {
        *HeaderInfo.Header = 0;
    }
    else
    {
        char *lineend = strstr(headerpointer, ";");
        headerpointer += 7;
        int length = lineend - headerpointer;
        if (length > MODULE_SIZE)
        {
            length = MODULE_SIZE;
        }
        strncpy(HeaderInfo.Header, headerpointer, length);
        HeaderInfo.Header[length] = 0;
    }

    int i, constlength, valuelength = 0;
    char constvalueindex[11];
    char value[VALUE_SIZE + 1];
    i = 0;
    while (1)
    {
        sprintf(constvalueindex, "Const%d=", i + 1);
        headerpointer = strstr(headerdata, constvalueindex);
        if (!headerpointer | i == VALUES_COUNT)
        {
            *HeaderInfo.Values[i].Const = 0;
            HeaderInfo.Values[i].Value = 0;
            break;
        }
        char *lineend = strstr(headerpointer, "\n");
        char *constpointer = headerpointer;
        constpointer += strlen(constvalueindex);
        constlength = lineend - constpointer;
        if (constlength > CONST_SIZE)
        {
            constlength = CONST_SIZE;
        }
        strncpy(HeaderInfo.Values[i].Const, constpointer, constlength);
        HeaderInfo.Values[i].Const[constlength] = 0;

        sprintf(constvalueindex, "Value%d=", i + 1);
        headerpointer = strstr(headerdata, constvalueindex);
        if (!headerpointer)
        {
            free(headerdata);
            goto EXIT;
        }
        lineend = strstr(headerpointer, "\n");
        char *valuepointer = headerpointer;
        valuepointer += strlen(constvalueindex);
        valuelength = lineend - valuepointer;
        if (valuelength > VALUE_SIZE)
        {
            valuelength = VALUE_SIZE;
        }
        strncpy(value, valuepointer, valuelength);
        value[valuelength] = 0;
        sscanf(value, "%lli", &HeaderInfo.Values[i].Value);
        i++;
    }
    free(headerdata);

    if (*HeaderInfo.Base == '[')
    {
        int length = strlen(HeaderInfo.Base);
        if (HeaderInfo.Base[length - 1] == ']')
        {
            if (!*HeaderInfo.Header)
            {
                sprintf(HeaderInfo.Header, "%s.h.api", CachedData.LibsData[index].LibName);
            }
        }
    }

    return;

EXIT:
    *HeaderInfo.MainHeader = 0;
    *HeaderInfo.TypeDisplay = 0;
    *HeaderInfo.Type = 0;
    *HeaderInfo.Base = 0;
    *HeaderInfo.Header = 0;
    *(*HeaderInfo.Values).Const = 0;
    (*HeaderInfo.Values).Value = 0;
    return;
}

void LoadWinApiData()
{

    HANDLE Find;
    WIN32_FIND_DATA Data;

    int i, length, libdatalength;
    char LibApisPath[LIB_FILE_PATH_SIZE + 1];
    char LibHeadersPath[LIB_FILE_PATH_SIZE + 1];
    char Library[MODULE_SIZE + 1];
    char LibraryContent[LIB_CONTENT_SIZE + 1];
    length = strlen(APIS_DIRECTORY);
    if (length > LIB_FILE_PATH_SIZE - 6)
    {
        length = LIB_FILE_PATH_SIZE - 6;
    }
    strncpy(LibApisPath, APIS_DIRECTORY, length);
    LibApisPath[length] = 0;
    sprintf(LibApisPath, "%s\\*.api", LibApisPath);

    length = strlen(HEADERS_DIRECTORY);
    if (length > LIB_FILE_PATH_SIZE - 8)
    {
        length = LIB_FILE_PATH_SIZE - 8;
    }
    strncpy(LibHeadersPath, HEADERS_DIRECTORY, length);
    LibHeadersPath[length] = 0;
    sprintf(LibHeadersPath, "%s\\*.h.api", LibHeadersPath);

    i = 0;
    CachedData.ApisLibsStartingIndex = i;
    Find = FindFirstFile(LibApisPath, &Data);
    do
    {
        length = strlen(Data.cFileName);
        if (length > MODULE_SIZE)
        {
            length = MODULE_SIZE;
        }
        sprintf(Library, "%s", Data.cFileName);
        Library[length] = 0;

        length = strlen(APIS_DIRECTORY);
        if (length > LIB_FILE_PATH_SIZE - strlen(Library) - 1)
        {
            length = LIB_FILE_PATH_SIZE - strlen(Library) - 1;
        }
        strncpy(LibApisPath, APIS_DIRECTORY, length);
        LibApisPath[length] = 0;
        sprintf(LibApisPath, "%s\\%s", LibApisPath, Library);

        libdatalength = ReadLibFile(LibApisPath, LibraryContent, LIB_CONTENT_SIZE);
        if (libdatalength)
        {
            if (i == NUMBER_OF_LIBS)
            {
                break;
            }
            char *libdataaddress = malloc(libdatalength + 1);
            strcpy(libdataaddress, LibraryContent);
            strcpy(CachedData.LibsData[i].LibName, Library);
            CachedData.LibsData[i].LibData = libdataaddress;
            i++;
        }

    } while (FindNextFile(Find, &Data));
    FindClose(Find);

    CachedData.HeadersLibsStartingIndex = i;
    Find = FindFirstFile(LibHeadersPath, &Data);
    do
    {
        length = strlen(Data.cFileName);
        if (length > MODULE_SIZE)
        {
            length = MODULE_SIZE;
        }
        sprintf(Library, "%s", Data.cFileName);
        Library[length] = 0;

        length = strlen(HEADERS_DIRECTORY);
        if (length > LIB_FILE_PATH_SIZE - strlen(Library) - 1)
        {
            length = LIB_FILE_PATH_SIZE - strlen(Library) - 1;
        }
        strncpy(LibHeadersPath, HEADERS_DIRECTORY, length);
        LibHeadersPath[length] = 0;
        sprintf(LibHeadersPath, "%s\\%s", LibHeadersPath, Library);

        libdatalength = ReadLibFile(LibHeadersPath, LibraryContent, LIB_CONTENT_SIZE);
        if (libdatalength)
        {
            if (i == NUMBER_OF_LIBS)
            {
                break;
            }
            char *libdataaddress = malloc(libdatalength + 1);
            strcpy(libdataaddress, LibraryContent);
            strcpy(CachedData.LibsData[i].LibName, Library);
            CachedData.LibsData[i].LibData = libdataaddress;
            i++;
        }

    } while (FindNextFile(Find, &Data));
    FindClose(Find);
    *CachedData.LibsData[i].LibName = 0;
    CachedData.LibsData[i].LibData = 0;
}