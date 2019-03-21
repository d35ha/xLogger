#include <WinApi.h>

// compile with gcc -I..\src\ ..\src\WinApi.c TestWinApi.c

void PrintApiInfo()
{

    int i;
    printf("ApiInfo.Api = %s\n", ApiInfo.Api);
    printf("ApiInfo.ParamCount = %d\n", ApiInfo.ParamCount);
    printf("ApiInfo.SourceModule = %s\n", ApiInfo.SourceModule);
    i = 0;
    while (*ApiInfo.Headers[i])
    {
        printf("ApiInfo.Headers[%d] = %s\n", i, ApiInfo.Headers[i]);
        i++;
    }
    i = 0;
    while (*ApiInfo.Parameters[i].DataType)
    {
        printf("ApiInfo.Parameters[%d].DataType = %s\n", i, ApiInfo.Parameters[i].DataType);
        printf("ApiInfo.Parameters[%d].DataVar = %s\n", i, ApiInfo.Parameters[i].DataVar);
        i++;
    }
}

void PrintHeaderInfo()
{

    int i;
    printf("HeaderInfo.MainHeader = %s\n", HeaderInfo.MainHeader);
    printf("HeaderInfo.Base = %s\n", HeaderInfo.Base);
    printf("HeaderInfo.Type = %s\n", HeaderInfo.Type);
    printf("HeaderInfo.TypeDisplay = %s\n", HeaderInfo.TypeDisplay);
    printf("HeaderInfo.Header = %s\n", HeaderInfo.Header);
    i = 0;
    while (*HeaderInfo.Values[i].Const)
    {
        printf("HeaderInfo.Values[%d].Const = %s\n", i, HeaderInfo.Values[i].Const);
        printf("HeaderInfo.Values[%d].Value = %lli\n", i, HeaderInfo.Values[i].Value);
        i++;
    }
}

void PrintModuleInfo(int index)
{
    printf("CachedData.LibsData[%d].LibName = %s\n", index, CachedData.LibsData[index].LibName);
    printf("strlen(CachedData.LibsData[%d].LibData) = %d\n", index, strlen(CachedData.LibsData[index].LibData));
}

void TestApi(char *Module, char *Api)
{
    LoadWinApiData();
    GetApiData(Module, Api);
    PrintApiInfo();
}

void TestHeader(char *Module, char *Header)
{
    LoadWinApiData();
    GetHeaderData(Module, Header);
    PrintHeaderInfo();
}

void TestModule(char *Module)
{
    LoadWinApiData();
    int index = 0;
    while (*CachedData.LibsData[index].LibName)
    {
        if (!strcmp(CachedData.LibsData[index].LibName, Module))
        {
            PrintModuleInfo(index);
            break;
        }
        index++;
    }
}

void TestAll()
{
    LoadWinApiData();
    int i = 0, index = 0, length = 0;
    char ApiName[API_SIZE];
    char HeaderName[HEADER_SIZE];
    while (*CachedData.LibsData[index].LibName)
    {
        PrintModuleInfo(index);
        if (index < CachedData.HeadersLibsStartingIndex)
        {
            char *api = strstr(CachedData.LibsData[index].LibData, "@=");
            while (api)
            {
                api += 2;
                char *lineend = strstr(api, "\n");
                length = lineend - api;
                if (length > API_SIZE)
                {
                    length = API_SIZE;
                }
                strncpy(ApiName, api, length);
                ApiName[length] = 0;
                GetApiData(CachedData.LibsData[index].LibName, ApiName);
                PrintApiInfo();
                api = strstr(api, "@=");
            }
        }
        else
        {
            char *header = strstr(CachedData.LibsData[index].LibData, "\n[");
            while (header)
            {
                header += 2;
                char *lineend = strstr(header, "]\n");
                length = lineend - header;
                if (length > HEADER_SIZE)
                {
                    length = HEADER_SIZE;
                }
                strncpy(HeaderName, header, length);
                HeaderName[length] = 0;
                GetHeaderData(CachedData.LibsData[index].LibName, HeaderName);
                PrintHeaderInfo();
                header = strstr(header, "\n[");
            }
        }
        index++;
    }
}

int main()
{
    TestAll();
    return 0;
}