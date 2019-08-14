#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "xPE/xPE.h"
#include "INIReader.h"
#include <dbghelp.h>
#include <string>
#include <vector>
#include <map>

#define PSAPI_VERSION 1
#include <psapi.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "dbghelp.lib")

typedef struct _MY_FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} MY_FILE_NAME_INFORMATION, * PMY_FILE_NAME_INFORMATION;

static INT PageSize = 0;
static INT DllsPageAlignment = 0;
static BOOL IsVerbous = FALSE;
static BOOL DetachOnExit = FALSE;
static BOOL DetachOnSystem = FALSE;
static BOOL ExternalConsole = FALSE;
static BOOL HideDebugger = FALSE;
static HANDLE hProcess = NULL;
static HANDLE hThread = NULL;

static DWORD dwPid = 0;
static DWORD dwModNum = 0;

static std::string ApisDir;
static std::string HeadersDir;

std::vector<std::string> ExternalModules;
std::map<std::string,
    std::vector<std::string>> ExcludedApis;
std::vector<std::string> ExcludedModules;
std::vector<std::string> AllExcludedExceptModules;
std::map<std::string,
    std::vector<std::string>> AllExcludedExceptApis;

#ifdef INI_ALLOW_INLINE_COMMENTS
#undef INI_ALLOW_INLINE_COMMENTS
#endif

#define TOP_NODE 218, 196, 196, 196, 196
#define MIDDLE_NODE 195, 196, 196, 196, 196
#define BASE_NODE 192, 196, 196, 196, 196

struct _ShellCodeInfo
{
    WORD wSizeOfExtModLst;
    HANDLE hLogFile;
    LPVOID lpWinApi;
    LPVOID lpLogApi;
    LPVOID lpImageTop;
    LPVOID lpImageBase;
    LPVOID lpNtWriteFile;
    LPVOID lpNtQueryPerfCounter;
    LPVOID lpRtlQueryPerfCounter;
    std::vector<BYTE> WinApiData;
    std::vector<BYTE> ShellCode;
} ShellCodeInfo;

struct _ApiInfo
{
    DWORD dwApiInfoOffset;
    DWORD dwHookOffset;
    LPVOID lpAddress;
};

struct _DllInfo
{
    DWORD dwDllSize;
    DWORD dwNumberOfApis;
    DWORD dwHookChainSize;
    LPVOID lpHookChain;
    std::string szDllName;
    std::map<std::string, _ApiInfo> ApiArray;
};

struct _SupportedApiInfo
{
    DWORD dwApiInfoOffset;
    std::string SourceModule;
};

typedef struct _SupportedHeaderInfo
{
    DWORD dwHeaderInfoOffset;
    DWORD dwHeaderSize;

} SupportedHeaderInfo, *LPSupportedHeaderInfo;

struct _WinApi
{
    std::map<LPVOID, _DllInfo> DllArray;
    std::map<std::string,
        std::map<std::string, _SupportedApiInfo>> KnownDlls;
    std::map<std::string,
        std::map<std::string, SupportedHeaderInfo>> HeadersInfo;
    std::vector<std::map<std::string,
        std::map<std::string,
        _SupportedApiInfo>>::iterator> CachedSourceModules;
    std::vector<std::string> CrtDlls;
    std::map<std::string, DWORD> WinTypesSizes;
    std::vector<std::string> WinTypesStr;
} WinApi;

#if defined(_M_X64) || defined(__amd64__)
BYTE bLoggerShellCode[] =
{
    0x65, 0x48, 0x8b, 0x04, 0x25, 0x60, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x40, 0x18, 0x48, 0x8b, 0x40,
    0x10, 0x49, 0x89, 0xc2, 0x4c, 0x8b, 0x58, 0x30, 0x4c, 0x39, 0x5c, 0x24, 0x10, 0x7c, 0x11, 0x44,
    0x8b, 0x58, 0x40, 0x4c, 0x03, 0x58, 0x30, 0x4c, 0x39, 0x5c, 0x24, 0x10, 0x7d, 0x02, 0xeb, 0x08,
    0x48, 0x8b, 0x00, 0x4c, 0x39, 0xd0, 0x75, 0xdc, 0x4c, 0x39, 0xd0, 0x0f, 0x84, 0xaf, 0x00, 0x00,
    0x00, 0x4c, 0x8d, 0x15, 0xbd, 0x00, 0x00, 0x00, 0x4d, 0x0f, 0xb7, 0x1a, 0x4d, 0x85, 0xdb, 0x0f,
    0x84, 0x98, 0x00, 0x00, 0x00, 0x48, 0x83, 0xec, 0x28, 0x48, 0x89, 0x0c, 0x24, 0x48, 0x89, 0x54,
    0x24, 0x08, 0x4c, 0x89, 0x4c, 0x24, 0x10, 0x4c, 0x89, 0x44, 0x24, 0x18, 0x48, 0x89, 0x44, 0x24,
    0x20, 0x48, 0x8b, 0x40, 0x60, 0x48, 0x31, 0xc9, 0x49, 0x83, 0xc2, 0x02, 0x41, 0x8a, 0x0a, 0x49,
    0xff, 0xc2, 0x49, 0xff, 0xcb, 0x48, 0x31, 0xd2, 0x44, 0x8a, 0x0c, 0x50, 0x41, 0x80, 0xf9, 0x5a,
    0x7f, 0x0a, 0x41, 0x80, 0xf9, 0x41, 0x7c, 0x04, 0x41, 0x80, 0xc1, 0x20, 0x45, 0x8a, 0x04, 0x12,
    0x45, 0x38, 0xc8, 0x75, 0x26, 0x48, 0xff, 0xc2, 0x48, 0x39, 0xca, 0x75, 0xdb, 0x48, 0x8b, 0x0c,
    0x24, 0x48, 0x8b, 0x54, 0x24, 0x08, 0x4c, 0x8b, 0x4c, 0x24, 0x10, 0x4c, 0x8b, 0x44, 0x24, 0x18,
    0x48, 0x8b, 0x44, 0x24, 0x20, 0x48, 0x83, 0xc4, 0x28, 0xeb, 0x25, 0x49, 0x29, 0xcb, 0x49, 0x01,
    0xca, 0x4d, 0x85, 0xdb, 0x75, 0xa6, 0x48, 0x8b, 0x0c, 0x24, 0x48, 0x8b, 0x54, 0x24, 0x08, 0x4c,
    0x8b, 0x4c, 0x24, 0x10, 0x4c, 0x8b, 0x44, 0x24, 0x18, 0x48, 0x83, 0xc4, 0x28, 0xc2, 0x08, 0x00,
    0x49, 0x89, 0xc3, 0x48, 0x8d, 0x05, 0x0b, 0x00, 0x00, 0x00, 0x4c, 0x0f, 0xb7, 0x10, 0x4a, 0x8d,
    0x44, 0x10, 0x02, 0xff, 0xe0,
#define EXTERNAL_MODS_LIST_OFFSET 0x105
    0x00, 0x00, 0x48, 0x8d, 0x05, 0x65, 0x05, 0x00, 0x00, 0x48, 0x3b,
    0x44, 0x24, 0x10, 0x75, 0x03, 0xc2, 0x08, 0x00, 0x56, 0x57, 0x53, 0x55, 0x41, 0x57, 0x41, 0x51,
    0x41, 0x50, 0x52, 0x51, 0x48, 0xbb,
#define LOG_FILE_HANDLE_OFFSET 0x126
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x48, 0xbd,
#define API_INFO_ADDRESS_OFFSET 0x130
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x87, 0x6c, 0x24, 0x50, 0x48, 0x03, 0x6c,
    0x24, 0x50, 0x48, 0x31, 0xff, 0x48, 0x31, 0xd2, 0x57, 0x57, 0x57, 0x57, 0x48, 0x8d, 0x35, 0x06,
    0x07, 0x00, 0x00, 0xf3, 0x90, 0xb0, 0x01, 0x86, 0x06, 0x84, 0xc0, 0x75, 0xf6, 0x48, 0x83, 0xc6,
    0x09, 0x49, 0x89, 0xf2, 0x49, 0x83, 0xc3, 0x58, 0x49, 0x8b, 0x43, 0x08, 0x66, 0x41, 0x8b, 0x13,
    0x66, 0xd1, 0xea, 0x4c, 0x89, 0xd7, 0xc6, 0x07, 0x5b, 0xc6, 0x44, 0x17, 0x01, 0x5d, 0xc6, 0x44,
    0x17, 0x02, 0x20, 0x4d, 0x31, 0xdb, 0x49, 0x39, 0xd3, 0x74, 0x0e, 0x42, 0x8a, 0x0c, 0x58, 0x42,
    0x88, 0x4c, 0x1f, 0x01, 0x49, 0xff, 0xc3, 0xeb, 0xed, 0x48, 0x83, 0xc2, 0x03, 0x48, 0x01, 0xd7,
    0x49, 0x89, 0x52, 0xf8, 0x65, 0x48, 0x8b, 0x04, 0x25, 0x48, 0x00, 0x00, 0x00, 0x89, 0xc0, 0xe8,
    0xfc, 0x05, 0x00, 0x00, 0xc6, 0x07, 0x5b, 0xc6, 0x44, 0x17, 0x01, 0x5d, 0xc6, 0x44, 0x17, 0x02,
    0x20, 0x48, 0xff, 0xc7, 0x48, 0x89, 0xce, 0x48, 0x89, 0xd1, 0xf3, 0xa4, 0x48, 0x83, 0xc2, 0x03,
    0x49, 0x01, 0x52, 0xf8, 0x8a, 0x55, 0x00, 0x48, 0xff, 0xc5, 0x48, 0x89, 0xd1, 0x4c, 0x89, 0xd7,
    0x49, 0x03, 0x7a, 0xf8, 0x48, 0x89, 0xee, 0xf3, 0xa4, 0x4c, 0x89, 0xd6, 0x48, 0x01, 0x56, 0xf8,
    0x48, 0x01, 0xd5, 0x48, 0x8d, 0x05, 0x4f, 0x06, 0x00, 0x00, 0x50, 0x48, 0x83, 0xec, 0x20, 0x48,
    0x0f, 0xb6, 0x7d, 0x00, 0x48, 0xff, 0xc5, 0x48, 0x85, 0xff, 0x0f, 0x84, 0x71, 0x03, 0x00, 0x00,
    0x4d, 0x31, 0xff, 0x48, 0x0f, 0xb6, 0x55, 0x00, 0x48, 0xff, 0xc5, 0x48, 0x89, 0xd1, 0x49, 0x89,
    0xf0, 0x49, 0x89, 0xf9, 0x48, 0x89, 0xf7, 0x48, 0x03, 0x7e, 0xf8, 0x48, 0x89, 0xee, 0xf3, 0xa4,
    0x4c, 0x89, 0xc6, 0x48, 0x01, 0x56, 0xf8, 0x4c, 0x89, 0xcf, 0x48, 0x01, 0xd5, 0x49, 0x83, 0xff,
    0x04, 0x7c, 0x0a, 0x4a, 0x8b, 0x84, 0xfc, 0xa8, 0x00, 0x00, 0x00, 0xeb, 0x05, 0x4a, 0x8b, 0x44,
    0xfc, 0x48, 0x48, 0x0f, 0xb6, 0x55, 0x00, 0x48, 0xff, 0xc5, 0x48, 0x83, 0xfa, 0x08, 0x74, 0x18,
    0x89, 0xc0, 0x48, 0x83, 0xfa, 0x04, 0x74, 0x10, 0x25, 0xff, 0xff, 0x00, 0x00, 0x48, 0x83, 0xfa,
    0x02, 0x74, 0x05, 0x25, 0xff, 0x00, 0x00, 0x00, 0x48, 0x0f, 0xb6, 0x55, 0x00, 0x48, 0xff, 0xc5,
    0x48, 0x85, 0xd2, 0x74, 0x12, 0x48, 0x83, 0xfa, 0x01, 0x74, 0x7a, 0x48, 0x83, 0xfa, 0x02, 0x0f,
    0x84, 0x71, 0x01, 0x00, 0x00, 0xeb, 0x41, 0xe8, 0x14, 0x05, 0x00, 0x00, 0x49, 0x89, 0xf0, 0x49,
    0x89, 0xf9, 0x48, 0x89, 0xf7, 0x48, 0x03, 0x7e, 0xf8, 0x48, 0x89, 0xce, 0x48, 0x89, 0xd1, 0xf3,
    0xa4, 0x4c, 0x89, 0xc6, 0x48, 0x01, 0x56, 0xf8, 0x4c, 0x89, 0xcf, 0x49, 0xff, 0xc7, 0x49, 0x39,
    0xff, 0x0f, 0x84, 0xd5, 0x02, 0x00, 0x00, 0x48, 0x8b, 0x56, 0xf8, 0xc6, 0x04, 0x16, 0x2c, 0x48,
    0xff, 0x46, 0xf8, 0xe9, 0x3b, 0xff, 0xff, 0xff, 0x49, 0x89, 0xf0, 0x4c, 0x03, 0x46, 0xf8, 0x48,
    0x85, 0xc0, 0x75, 0x13, 0x41, 0xc7, 0x00, 0x46, 0x41, 0x4c, 0x53, 0x41, 0xc6, 0x40, 0x04, 0x45,
    0x48, 0x83, 0x46, 0xf8, 0x05, 0xeb, 0xc4, 0x41, 0xc7, 0x00, 0x54, 0x52, 0x55, 0x45, 0x48, 0x83,
    0x46, 0xf8, 0x04, 0xeb, 0xb6, 0x49, 0x89, 0xea, 0x48, 0x31, 0xc9, 0x49, 0x89, 0xf9, 0x8b, 0x6d,
    0x00, 0x48, 0x03, 0xac, 0x24, 0x98, 0x00, 0x00, 0x00, 0x80, 0x7d, 0x00, 0x00, 0x75, 0x4d, 0x48,
    0xff, 0xc5, 0x48, 0x0f, 0xb7, 0x55, 0x00, 0x48, 0x83, 0xc5, 0x02, 0x48, 0x85, 0xd2, 0x0f, 0x84,
    0xb4, 0x00, 0x00, 0x00, 0x48, 0xff, 0xca, 0x48, 0x01, 0xcd, 0x8b, 0x7d, 0x00, 0x48, 0x83, 0xc5,
    0x04, 0x48, 0x0f, 0xb6, 0x4d, 0x00, 0x48, 0xff, 0xc5, 0x48, 0x39, 0xf8, 0x75, 0xdd, 0x48, 0x89,
    0xf7, 0x48, 0x03, 0x7e, 0xf8, 0x48, 0x89, 0xee, 0xf3, 0xa4, 0x4c, 0x89, 0xc6, 0x48, 0x89, 0xf9,
    0x48, 0x29, 0xf1, 0x48, 0x89, 0x4e, 0xf8, 0xe9, 0x8b, 0x00, 0x00, 0x00, 0x48, 0xff, 0xc5, 0x48,
    0x0f, 0xb7, 0x55, 0x00, 0x48, 0x83, 0xc5, 0x02, 0x49, 0x89, 0xc3, 0x48, 0x31, 0xc0, 0x48, 0x85,
    0xd2, 0x74, 0x50, 0x48, 0xff, 0xca, 0x48, 0x01, 0xcd, 0x8b, 0x7d, 0x00, 0x48, 0x83, 0xc5, 0x04,
    0x48, 0x0f, 0xb6, 0x4d, 0x00, 0x48, 0xff, 0xc5, 0x48, 0x89, 0xfe, 0x4c, 0x21, 0xdf, 0x48, 0x39,
    0xfe, 0x75, 0xdb, 0x4c, 0x89, 0xc7, 0x48, 0x03, 0x7f, 0xf8, 0x48, 0x89, 0xee, 0xf3, 0xa4, 0x4c,
    0x89, 0xc6, 0x66, 0xc7, 0x07, 0x20, 0x7c, 0xc6, 0x47, 0x02, 0x20, 0x48, 0x83, 0xc7, 0x03, 0x48,
    0x89, 0xf9, 0x48, 0x29, 0xf1, 0x48, 0x89, 0x4e, 0xf8, 0x48, 0xff, 0xc0, 0x48, 0x0f, 0xb6, 0x4d,
    0xff, 0xeb, 0xab, 0x48, 0x89, 0xc2, 0x4c, 0x89, 0xd8, 0x4c, 0x89, 0xc6, 0x48, 0x85, 0xd2, 0x74,
    0x07, 0x48, 0x83, 0x6e, 0xf8, 0x03, 0xeb, 0x0f, 0x4c, 0x89, 0xcf, 0x4c, 0x89, 0xd5, 0x48, 0x83,
    0xc5, 0x04, 0xe9, 0xa0, 0xfe, 0xff, 0xff, 0x4c, 0x89, 0xcf, 0x4c, 0x89, 0xd5, 0x48, 0x83, 0xc5,
    0x04, 0xe9, 0xb5, 0xfe, 0xff, 0xff, 0x48, 0x31, 0xd2, 0x49, 0x89, 0xf0, 0x4c, 0x03, 0x46, 0xf8,
    0x48, 0x3d, 0xff, 0xff, 0x00, 0x00, 0x7f, 0x1a, 0x48, 0x85, 0xc0, 0x0f, 0x85, 0x76, 0xfe, 0xff,
    0xff, 0x41, 0xc7, 0x00, 0x4e, 0x55, 0x4c, 0x4c, 0x48, 0x83, 0x46, 0xf8, 0x04, 0xe9, 0x89, 0xfe,
    0xff, 0xff, 0x80, 0x78, 0x01, 0x00, 0x0f, 0x84, 0x8d, 0x00, 0x00, 0x00, 0x41, 0xc6, 0x00, 0x22,
    0x8a, 0x0c, 0x10, 0x84, 0xc9, 0x0f, 0x84, 0x0f, 0x01, 0x00, 0x00, 0x80, 0xf9, 0x5c, 0x75, 0x0b,
    0x41, 0xc6, 0x44, 0x10, 0x01, 0x5c, 0x49, 0xff, 0xc0, 0xeb, 0x4f, 0x80, 0xf9, 0x22, 0x75, 0x0b,
    0x41, 0xc6, 0x44, 0x10, 0x01, 0x5c, 0x49, 0xff, 0xc0, 0xeb, 0x3f, 0x80, 0xf9, 0x0a, 0x75, 0x10,
    0x66, 0x41, 0xc7, 0x44, 0x10, 0x01, 0x5c, 0x6e, 0x48, 0xff, 0xc2, 0x49, 0xff, 0xc0, 0xeb, 0xc0,
    0x80, 0xf9, 0x09, 0x75, 0x10, 0x66, 0x41, 0xc7, 0x44, 0x10, 0x01, 0x5c, 0x74, 0x48, 0xff, 0xc2,
    0x49, 0xff, 0xc0, 0xeb, 0xab, 0x80, 0xf9, 0x0d, 0x75, 0x10, 0x66, 0x41, 0xc7, 0x44, 0x10, 0x01,
    0x5c, 0x72, 0x48, 0xff, 0xc2, 0x49, 0xff, 0xc0, 0xeb, 0x96, 0x80, 0xf9, 0x20, 0x0f, 0x8c, 0xa7,
    0x00, 0x00, 0x00, 0x80, 0xf9, 0x7e, 0x0f, 0x8f, 0x9e, 0x00, 0x00, 0x00, 0x41, 0x88, 0x4c, 0x10,
    0x01, 0x48, 0xff, 0xc2, 0xe9, 0x77, 0xff, 0xff, 0xff, 0x41, 0xc6, 0x00, 0x4c, 0x41, 0xc6, 0x40,
    0x01, 0x22, 0x8a, 0x0c, 0x50, 0x84, 0xc9, 0x74, 0x7e, 0x80, 0xf9, 0x5c, 0x75, 0x0b, 0x41, 0xc6,
    0x44, 0x10, 0x02, 0x5c, 0x49, 0xff, 0xc0, 0xeb, 0x4f, 0x80, 0xf9, 0x22, 0x75, 0x0b, 0x41, 0xc6,
    0x44, 0x10, 0x02, 0x5c, 0x49, 0xff, 0xc0, 0xeb, 0x3f, 0x80, 0xf9, 0x0a, 0x75, 0x10, 0x66, 0x41,
    0xc7, 0x44, 0x10, 0x02, 0x5c, 0x6e, 0x48, 0xff, 0xc2, 0x49, 0xff, 0xc0, 0xeb, 0xc4, 0x80, 0xf9,
    0x09, 0x75, 0x10, 0x66, 0x41, 0xc7, 0x44, 0x10, 0x02, 0x5c, 0x74, 0x48, 0xff, 0xc2, 0x49, 0xff,
    0xc0, 0xeb, 0xaf, 0x80, 0xf9, 0x0d, 0x75, 0x10, 0x66, 0x41, 0xc7, 0x44, 0x10, 0x02, 0x5c, 0x74,
    0x48, 0xff, 0xc2, 0x49, 0xff, 0xc0, 0xeb, 0x9a, 0x80, 0xf9, 0x20, 0x7c, 0x1a, 0x80, 0xf9, 0x7e,
    0x7f, 0x15, 0x41, 0x88, 0x4c, 0x10, 0x02, 0x8a, 0x4c, 0x50, 0x01, 0x84, 0xc9, 0x75, 0x08, 0x48,
    0xff, 0xc2, 0xe9, 0x7b, 0xff, 0xff, 0xff, 0x48, 0xff, 0xc2, 0x48, 0x83, 0xfa, 0x03, 0x0f, 0x8c,
    0x33, 0xfd, 0xff, 0xff, 0x41, 0xc6, 0x44, 0x10, 0x01, 0x22, 0x48, 0x83, 0xc2, 0x02, 0x49, 0x29,
    0xf0, 0x4c, 0x2b, 0x46, 0xf8, 0x4c, 0x01, 0xc2, 0x48, 0x01, 0x56, 0xf8, 0xe9, 0x3a, 0xfd, 0xff,
    0xff, 0x49, 0x89, 0xf0, 0x49, 0x89, 0xf9, 0x48, 0x89, 0xf7, 0x48, 0x03, 0x7e, 0xf8, 0xba, 0x02,
    0x00, 0x00, 0x00, 0x48, 0x8d, 0x35, 0x99, 0x02, 0x00, 0x00, 0xeb, 0x19, 0x49, 0x89, 0xf0, 0x49,
    0x89, 0xf9, 0x48, 0x89, 0xf7, 0x48, 0x03, 0x7e, 0xf8, 0xba, 0x03, 0x00, 0x00, 0x00, 0x48, 0x8d,
    0x35, 0x7d, 0x02, 0x00, 0x00, 0x48, 0x89, 0xd1, 0xf3, 0xa4, 0x4c, 0x89, 0xc6, 0x48, 0x01, 0x56,
    0xf8, 0x4c, 0x89, 0xcf, 0x80, 0x7d, 0x00, 0x00, 0x0f, 0x85, 0x84, 0x01, 0x00, 0x00, 0x48, 0x83,
    0xec, 0x08, 0x48, 0x89, 0xe1, 0x48, 0x83, 0xec, 0x20, 0x48, 0xb8,
#define NT_QUERY_PERF_COUNTER_CALL_OFFSET 0x5db
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xd0, 0x48, 0x83, 0xc4, 0x20, 0x4c, 0x8b, 0x56, 0xf8, 0x4c, 0x29, 0xd4,
    0x41, 0x52, 0x49, 0x89, 0xf0, 0x49, 0x89, 0xf9, 0x48, 0x89, 0xe7, 0x48, 0x83, 0xc7, 0x08, 0x4c,
    0x89, 0xd1, 0xf3, 0xa4, 0x4c, 0x89, 0xc6, 0x4c, 0x89, 0xcf, 0x30, 0xc0, 0x86, 0x46, 0xf7, 0x4a,
    0x8b, 0x4c, 0x14, 0x58, 0x4a, 0x8b, 0x54, 0x14, 0x60, 0x4e, 0x8b, 0x44, 0x14, 0x68, 0x4e, 0x8b,
    0x4c, 0x14, 0x70, 0x49, 0x89, 0xe7, 0x4d, 0x01, 0xd7, 0x48, 0x83, 0xff, 0x04, 0x7e, 0x31, 0x48,
    0x83, 0xef, 0x04, 0x48, 0x83, 0xe4, 0xf0, 0x48, 0x89, 0xfd, 0x48, 0xd1, 0xed, 0x48, 0xd1, 0xe5,
    0x48, 0x39, 0xfd, 0x74, 0x07, 0x48, 0xff, 0xc5, 0x48, 0x83, 0xec, 0x08, 0x48, 0x85, 0xed, 0x74,
    0x0d, 0x41, 0xff, 0xb4, 0xef, 0xd0, 0x00, 0x00, 0x00, 0x48, 0xff, 0xcd, 0xeb, 0xee, 0xeb, 0x04,
    0x48, 0x83, 0xe4, 0xf0, 0x48, 0x83, 0xec, 0x20, 0x4d, 0x29, 0xd7, 0x43, 0xff, 0x94, 0x17, 0xa0,
    0x00, 0x00, 0x00, 0x4c, 0x89, 0xfc, 0xf3, 0x90, 0xb1, 0x01, 0x86, 0x4e, 0xf7, 0x84, 0xc9, 0x75,
    0xf5, 0x5a, 0x49, 0x89, 0xf0, 0x48, 0x89, 0xf7, 0x48, 0x89, 0xe6, 0x48, 0x89, 0xd1, 0xf3, 0xa4,
    0x4c, 0x89, 0xc6, 0x48, 0x89, 0x56, 0xf8, 0x48, 0x01, 0xd4, 0xe8, 0x11, 0x01, 0x00, 0x00, 0x49,
    0x89, 0xf0, 0x48, 0x89, 0xf7, 0x48, 0x03, 0x7e, 0xf8, 0xc6, 0x07, 0x2d, 0xc6, 0x47, 0x01, 0x3e,
    0xc6, 0x47, 0x02, 0x20, 0xc6, 0x44, 0x17, 0x03, 0x20, 0x48, 0x83, 0xc7, 0x03, 0x48, 0x89, 0xce,
    0x48, 0x89, 0xd1, 0xf3, 0xa4, 0x4c, 0x89, 0xc6, 0x48, 0x83, 0xc2, 0x04, 0x48, 0x01, 0x56, 0xf8,
    0x48, 0x83, 0xec, 0x10, 0x48, 0x89, 0xe1, 0x48, 0x8d, 0x54, 0x24, 0x08, 0x48, 0x89, 0xc5, 0x48,
    0x83, 0xec, 0x20, 0x48, 0xb8,
#define NT_QUERY_PERF_COUNTER_RET_OFFSET 0x6e5
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xd0, 0x48,
    0x83, 0xc4, 0x20, 0x48, 0x8b, 0x04, 0x24, 0x48, 0x2b, 0x44, 0x24, 0x10, 0x48, 0x69, 0xc0, 0x40,
    0x42, 0x0f, 0x00, 0x48, 0x31, 0xd2, 0x48, 0xf7, 0x74, 0x24, 0x08, 0x48, 0x83, 0xc4, 0x18, 0xe8,
    0x9c, 0x00, 0x00, 0x00, 0x48, 0x89, 0xe8, 0x49, 0x89, 0xf0, 0x48, 0x89, 0xf7, 0x48, 0x03, 0x7e,
    0xf8, 0xc6, 0x07, 0x5b, 0xc6, 0x44, 0x17, 0x01, 0x20, 0xc7, 0x44, 0x17, 0x02, 0xc2, 0xb5, 0x73,
    0x5d, 0x48, 0x83, 0xc7, 0x01, 0x48, 0x89, 0xce, 0x48, 0x89, 0xd1, 0xf3, 0xa4, 0x4c, 0x89, 0xc6,
    0x48, 0x83, 0xc2, 0x06, 0x48, 0x01, 0x56, 0xf8, 0xe8, 0x0d, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc4,
    0x10, 0xc3, 0xe8, 0x03, 0x00, 0x00, 0x00, 0xc2, 0x08, 0x00, 0x41, 0x5f, 0x48, 0x8b, 0x56, 0xf8,
    0xc7, 0x04, 0x16, 0x0a, 0x0a, 0x00, 0x00, 0x48, 0x83, 0xc2, 0x02, 0x48, 0x89, 0x54, 0x24, 0x30,
    0x48, 0x89, 0x74, 0x24, 0x28, 0x4d, 0x31, 0xc9, 0x4d, 0x31, 0xc0, 0x48, 0x31, 0xd2, 0x48, 0x89,
    0xd9, 0x48, 0x89, 0xc5, 0x48, 0xb8,
#define NT_WRITE_FILE_ADDR_OFFSET 0x786
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xd0,
    0x48, 0x89, 0xe8, 0x45, 0x30, 0xd2, 0x44, 0x86, 0x56, 0xf7, 0x48, 0x83, 0xc4, 0x48, 0x4d, 0x89,
    0xfa, 0x59, 0x5a, 0x41, 0x58, 0x41, 0x59, 0x41, 0x5f, 0x5d, 0x5b, 0x5f, 0x5e, 0x41, 0x52, 0xc3,
    0x48, 0x8d, 0x0d, 0x7e, 0x00, 0x00, 0x00, 0xba, 0x02, 0x00, 0x00, 0x00, 0x50, 0x48, 0x85, 0xc0,
    0x75, 0x09, 0xc6, 0x04, 0x11, 0x30, 0x48, 0xff, 0xc2, 0xeb, 0x65, 0x48, 0x89, 0xe0, 0x48, 0x83,
    0xc0, 0x07, 0x44, 0x8a, 0x00, 0x45, 0x84, 0xc0, 0x75, 0x05, 0x48, 0xff, 0xc8, 0xeb, 0xf3, 0x44,
    0x8a, 0x00, 0x45, 0x88, 0xc1, 0x41, 0xc0, 0xe8, 0x04, 0x41, 0x80, 0xf8, 0x0a, 0x7c, 0x06, 0x41,
    0x80, 0xc0, 0x57, 0xeb, 0x0f, 0x45, 0x84, 0xc0, 0x75, 0x06, 0x48, 0x83, 0xfa, 0x02, 0x74, 0x0b,
    0x41, 0x80, 0xc0, 0x30, 0x44, 0x88, 0x04, 0x11, 0x48, 0xff, 0xc2, 0x41, 0x80, 0xe1, 0x0f, 0x41,
    0x80, 0xf9, 0x0a, 0x7c, 0x06, 0x41, 0x80, 0xc1, 0x57, 0xeb, 0x04, 0x41, 0x80, 0xc1, 0x30, 0x44,
    0x88, 0x0c, 0x11, 0x48, 0xff, 0xc2, 0x48, 0x39, 0xe0, 0x74, 0x05, 0x48, 0xff, 0xc8, 0xeb, 0xaf,
    0x58, 0xc3, 0x0a, 0x29, 0x20, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
};
std::vector<BYTE> LoggerShellCode(bLoggerShellCode,
    bLoggerShellCode + sizeof(bLoggerShellCode));

#else
BYTE bLoggerShellCode[] =
{
    0x50, 0x64, 0xa1, 0x30, 0x00, 0x00, 0x00, 0x8b, 0x40, 0x0c, 0x8b, 0x40, 0x0c, 0x89, 0xc1, 0x8b,
    0x50, 0x18, 0x39, 0x54, 0x24, 0x0c, 0x7c, 0x0e, 0x8b, 0x50, 0x20, 0x03, 0x50, 0x18, 0x39, 0x54,
    0x24, 0x0c, 0x7d, 0x02, 0xeb, 0x06, 0x8b, 0x00, 0x39, 0xc8, 0x75, 0xe3, 0x39, 0xc8, 0x0f, 0x84,
    0x8a, 0x00, 0x00, 0x00, 0xb9, 0xd1, 0x00, 0x00, 0x00, 0x03, 0x0c, 0x24, 0x0f, 0xb7, 0x11, 0x85,
    0xd2, 0x74, 0x75, 0x83, 0xec, 0x14, 0x89, 0x34, 0x24, 0x89, 0x7c, 0x24, 0x04, 0x89, 0x5c, 0x24,
    0x08, 0x89, 0x6c, 0x24, 0x0c, 0x89, 0x44, 0x24, 0x10, 0x8b, 0x40, 0x30, 0x31, 0xf6, 0x83, 0xc1,
    0x02, 0x0f, 0xb6, 0x31, 0x41, 0x4a, 0x31, 0xff, 0x0f, 0xb6, 0x1c, 0x78, 0x80, 0xfb, 0x5a, 0x7f,
    0x08, 0x80, 0xfb, 0x41, 0x7c, 0x03, 0x80, 0xc3, 0x20, 0x0f, 0xb6, 0x2c, 0x39, 0x39, 0xdd, 0x75,
    0x1d, 0x47, 0x39, 0xf7, 0x75, 0xe2, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0x8b, 0x5c, 0x24,
    0x08, 0x8b, 0x6c, 0x24, 0x0c, 0x8b, 0x44, 0x24, 0x10, 0x83, 0xc4, 0x14, 0xeb, 0x20, 0x29, 0xf2,
    0x01, 0xf1, 0x85, 0xd2, 0x75, 0xbb, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0x8b, 0x5c, 0x24,
    0x08, 0x8b, 0x6c, 0x24, 0x0c, 0x83, 0xc4, 0x14, 0x83, 0xc4, 0x04, 0xc2, 0x04, 0x00, 0x89, 0xc2,
    0xb8, 0xd1, 0x00, 0x00, 0x00, 0x03, 0x04, 0x24, 0x0f, 0xb7, 0x08, 0x8d, 0x44, 0x08, 0x02, 0xff,
    0xe0,
#define EXTERNAL_MODS_LIST_OFFSET 209
    0x00, 0x00, 0xb8, 0x1d, 0x05, 0x00, 0x00, 0x03, 0x04, 0x24, 0x01, 0xc8, 0x3b, 0x44, 0x24,
    0x0c, 0x75, 0x06, 0x83, 0xc4, 0x04, 0xc2, 0x04, 0x00, 0x56, 0x57, 0x53, 0x55, 0xbd,
#define API_INFO_ADDRESS_OFFSET 238
    0x00, 0x00, 0x00, 0x00,
    0x87, 0x6c, 0x24, 0x18, 0x03, 0x6c, 0x24, 0x18, 0xbe, 0x01, 0x07, 0x00, 0x00, 0x03,
    0x74, 0x24, 0x10, 0x01, 0xce, 0xf3, 0x90, 0xb0, 0x01, 0x86, 0x06, 0x84, 0xc0, 0x75, 0xf6, 0x83,
    0xc6, 0x05, 0x56, 0x83, 0xc2, 0x2c, 0x8b, 0x42, 0x04, 0x0f, 0xb7, 0x12, 0x66, 0xd1, 0xea, 0x8b,
    0x3c, 0x24, 0xc6, 0x07, 0x5b, 0xc6, 0x44, 0x17, 0x01, 0x5d, 0xc6, 0x44, 0x17, 0x02, 0x20, 0x31,
    0xc9, 0x39, 0xca, 0x74, 0x0a, 0x8a, 0x1c, 0x48, 0x88, 0x5c, 0x0f, 0x01, 0x41, 0xeb, 0xf2, 0x83,
    0xc2, 0x03, 0x01, 0xd7, 0x89, 0x56, 0xfc, 0x64, 0xa1, 0x24, 0x00, 0x00, 0x00, 0xe8, 0x26, 0x05,
    0x00, 0x00, 0xc6, 0x07, 0x5b, 0xc6, 0x44, 0x17, 0x01, 0x5d, 0xc6, 0x44, 0x17, 0x02, 0x20, 0x47,
    0x89, 0xce, 0x89, 0xd1, 0xf3, 0xa4, 0x83, 0xc2, 0x03, 0x8b, 0x34, 0x24, 0x01, 0x56, 0xfc, 0x8a,
    0x55, 0x00, 0x45, 0x89, 0xd1, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0x89, 0xee, 0xf3, 0xa4, 0x5e, 0x01,
    0x56, 0xfc, 0x01, 0xd5, 0x0f, 0xb6, 0x7d, 0x00, 0x45, 0x83, 0xec, 0x08, 0x85, 0xff, 0x0f, 0x84,
    0xf7, 0x02, 0x00, 0x00, 0x31, 0xdb, 0x0f, 0xb6, 0x55, 0x00, 0x45, 0x89, 0xd1, 0x89, 0x34, 0x24,
    0x89, 0x7c, 0x24, 0x04, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0x89, 0xee, 0xf3, 0xa4, 0x8b, 0x34, 0x24,
    0x01, 0x56, 0xfc, 0x8b, 0x7c, 0x24, 0x04, 0x01, 0xd5, 0x8b, 0x44, 0x9c, 0x28, 0x0f, 0xb6, 0x55,
    0x00, 0x45, 0x83, 0xfa, 0x08, 0x74, 0x31, 0x83, 0xfa, 0x04, 0x74, 0x0f, 0x25, 0xff, 0xff, 0x00,
    0x00, 0x83, 0xfa, 0x02, 0x74, 0x05, 0x25, 0xff, 0x00, 0x00, 0x00, 0x0f, 0xb6, 0x55, 0x00, 0x45,
    0x85, 0xd2, 0x74, 0x42, 0x83, 0xfa, 0x01, 0x0f, 0x84, 0x97, 0x00, 0x00, 0x00, 0x83, 0xfa, 0x02,
    0x0f, 0x84, 0x54, 0x01, 0x00, 0x00, 0xeb, 0x67, 0xe8, 0x7b, 0x04, 0x00, 0x00, 0x89, 0x34, 0x24,
    0x89, 0x7c, 0x24, 0x04, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0xc6, 0x04, 0x17, 0x3a, 0x89, 0xce, 0x89,
    0xd1, 0xf3, 0xa4, 0x42, 0x8b, 0x34, 0x24, 0x01, 0x56, 0xfc, 0x8b, 0x7c, 0x24, 0x04, 0x43, 0x47,
    0x8b, 0x44, 0x9c, 0x28, 0xeb, 0xb5, 0xe8, 0x4d, 0x04, 0x00, 0x00, 0x89, 0x34, 0x24, 0x89, 0x7c,
    0x24, 0x04, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0x89, 0xce, 0x89, 0xd1, 0xf3, 0xa4, 0x8b, 0x34, 0x24,
    0x01, 0x56, 0xfc, 0x8b, 0x7c, 0x24, 0x04, 0x43, 0x39, 0xfb, 0x0f, 0x84, 0x56, 0x02, 0x00, 0x00,
    0x8b, 0x56, 0xfc, 0xc6, 0x04, 0x16, 0x2c, 0xff, 0x46, 0xfc, 0xe9, 0x37, 0xff, 0xff, 0xff, 0x89,
    0xf1, 0x03, 0x4e, 0xfc, 0x85, 0xc0, 0x75, 0x10, 0xc7, 0x01, 0x46, 0x41, 0x4c, 0x53, 0xc6, 0x41,
    0x04, 0x45, 0x83, 0x46, 0xfc, 0x05, 0xeb, 0xcb, 0xc7, 0x01, 0x54, 0x52, 0x55, 0x45, 0x83, 0x46,
    0xfc, 0x04, 0xeb, 0xbf, 0x55, 0x31, 0xc9, 0x89, 0x7c, 0x24, 0x08, 0x8b, 0x6d, 0x00, 0x03, 0x6c,
    0x24, 0x24, 0x80, 0x7d, 0x00, 0x00, 0x75, 0x38, 0x45, 0x0f, 0xb7, 0x55, 0x00, 0x83, 0xc5, 0x02,
    0x85, 0xd2, 0x0f, 0x84, 0x88, 0x00, 0x00, 0x00, 0x4a, 0x01, 0xcd, 0x8b, 0x7d, 0x00, 0x83, 0xc5,
    0x04, 0x0f, 0xb6, 0x4d, 0x00, 0x45, 0x39, 0xf8, 0x75, 0xe6, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0x89,
    0xee, 0xf3, 0xa4, 0x8b, 0x74, 0x24, 0x04, 0x89, 0xf9, 0x29, 0xf1, 0x89, 0x4e, 0xfc, 0xeb, 0x6d,
    0x45, 0x0f, 0xb7, 0x55, 0x00, 0x83, 0xc5, 0x02, 0x50, 0x31, 0xc0, 0x85, 0xd2, 0x74, 0x40, 0x4a,
    0x01, 0xcd, 0x8b, 0x7d, 0x00, 0x83, 0xc5, 0x04, 0x0f, 0xb6, 0x4d, 0x00, 0x45, 0x89, 0xfe, 0x23,
    0x3c, 0x24, 0x39, 0xfe, 0x75, 0xe5, 0x8b, 0x7c, 0x24, 0x08, 0x03, 0x7f, 0xfc, 0x89, 0xee, 0xf3,
    0xa4, 0x8b, 0x74, 0x24, 0x08, 0x66, 0xc7, 0x07, 0x20, 0x7c, 0xc6, 0x47, 0x02, 0x20, 0x83, 0xc7,
    0x03, 0x89, 0xf9, 0x29, 0xf1, 0x89, 0x4e, 0xfc, 0x40, 0x0f, 0xb6, 0x4d, 0xff, 0xeb, 0xbc, 0x89,
    0xc2, 0x58, 0x8b, 0x74, 0x24, 0x04, 0x85, 0xd2, 0x74, 0x06, 0x83, 0x6e, 0xfc, 0x03, 0xeb, 0x0d,
    0x8b, 0x7c, 0x24, 0x08, 0x5d, 0x83, 0xc5, 0x04, 0xe9, 0xe9, 0xfe, 0xff, 0xff, 0x8b, 0x7c, 0x24,
    0x08, 0x5d, 0x83, 0xc5, 0x04, 0xe9, 0xf9, 0xfe, 0xff, 0xff, 0x31, 0xd2, 0x89, 0x7c, 0x24, 0x04,
    0x89, 0xf7, 0x03, 0x7e, 0xfc, 0x3d, 0xff, 0xff, 0x00, 0x00, 0x7f, 0x2b, 0x85, 0xc0, 0xe8, 0x15,
    0x03, 0x00, 0x00, 0x89, 0x34, 0x24, 0x89, 0xce, 0x89, 0xd1, 0xf3, 0xa4, 0x8b, 0x34, 0x24, 0x01,
    0x56, 0xfc, 0x0f, 0x85, 0xcb, 0xfe, 0xff, 0xff, 0xc7, 0x07, 0x4e, 0x55, 0x4c, 0x4c, 0x83, 0x46,
    0xfc, 0x04, 0xe9, 0xbc, 0xfe, 0xff, 0xff, 0x80, 0x78, 0x01, 0x00, 0x74, 0x69, 0xc6, 0x07, 0x22,
    0x8a, 0x0c, 0x10, 0x84, 0xc9, 0x0f, 0x84, 0xcd, 0x00, 0x00, 0x00, 0x80, 0xf9, 0x5c, 0x75, 0x08,
    0xc6, 0x44, 0x17, 0x01, 0x5c, 0x47, 0xeb, 0x3d, 0x80, 0xf9, 0x22, 0x75, 0x08, 0xc6, 0x44, 0x17,
    0x01, 0x5c, 0x47, 0xeb, 0x30, 0x80, 0xf9, 0x0a, 0x75, 0x0b, 0x66, 0xc7, 0x44, 0x17, 0x01, 0x5c,
    0x6e, 0x42, 0x47, 0xeb, 0xcb, 0x80, 0xf9, 0x09, 0x75, 0x0b, 0x66, 0xc7, 0x44, 0x17, 0x01, 0x5c,
    0x74, 0x42, 0x47, 0xeb, 0xbb, 0x80, 0xf9, 0x0d, 0x75, 0x0b, 0x66, 0xc7, 0x44, 0x17, 0x01, 0x5c,
    0x72, 0x42, 0x47, 0xeb, 0xab, 0x80, 0xf9, 0x20, 0x7c, 0x7e, 0x80, 0xf9, 0x7e, 0x7f, 0x79, 0x88,
    0x4c, 0x17, 0x01, 0x42, 0xeb, 0x9a, 0xc6, 0x07, 0x4c, 0xc6, 0x47, 0x01, 0x22, 0x8a, 0x0c, 0x50,
    0x84, 0xc9, 0x74, 0x63, 0x80, 0xf9, 0x5c, 0x75, 0x08, 0xc6, 0x44, 0x17, 0x02, 0x5c, 0x47, 0xeb,
    0x3d, 0x80, 0xf9, 0x22, 0x75, 0x08, 0xc6, 0x44, 0x17, 0x02, 0x5c, 0x47, 0xeb, 0x30, 0x80, 0xf9,
    0x0a, 0x75, 0x0b, 0x66, 0xc7, 0x44, 0x17, 0x02, 0x5c, 0x6e, 0x42, 0x47, 0xeb, 0xcf, 0x80, 0xf9,
    0x09, 0x75, 0x0b, 0x66, 0xc7, 0x44, 0x17, 0x02, 0x5c, 0x74, 0x42, 0x47, 0xeb, 0xbf, 0x80, 0xf9,
    0x0d, 0x75, 0x0b, 0x66, 0xc7, 0x44, 0x17, 0x02, 0x5c, 0x74, 0x42, 0x47, 0xeb, 0xaf, 0x80, 0xf9,
    0x20, 0x7c, 0x14, 0x80, 0xf9, 0x7e, 0x7f, 0x0f, 0x88, 0x4c, 0x17, 0x02, 0x8a, 0x4c, 0x50, 0x01,
    0x84, 0xc9, 0x75, 0x03, 0x42, 0xeb, 0x96, 0x42, 0xc6, 0x44, 0x17, 0x01, 0x22, 0x83, 0xc2, 0x02,
    0x29, 0xf7, 0x2b, 0x7e, 0xfc, 0x01, 0xfa, 0x01, 0x56, 0xfc, 0xe9, 0xc4, 0xfd, 0xff, 0xff, 0x8b,
    0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0xe9, 0x9b, 0xfd, 0xff, 0xff, 0x89, 0x34, 0x24, 0x89, 0x7c,
    0x24, 0x04, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0xba, 0x02, 0x00, 0x00, 0x00, 0xe8, 0x02, 0x00, 0x00,
    0x00, 0x29, 0x20, 0x5e, 0xeb, 0x1a, 0x89, 0x34, 0x24, 0x89, 0x7c, 0x24, 0x04, 0x89, 0xf7, 0x03,
    0x7e, 0xfc, 0xba, 0x03, 0x00, 0x00, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x0a, 0x29, 0x20, 0x5e,
    0x89, 0xd1, 0xf3, 0xa4, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0x01, 0x56, 0xfc, 0x83, 0xc4,
    0x08, 0x80, 0x7d, 0x00, 0x00, 0x0f, 0x85, 0x4f, 0x01, 0x00, 0x00, 0x45, 0x0f, 0xb6, 0x6d, 0x00,
    0x55, 0x83, 0xec, 0x08, 0x54, 0xb8,
#define NT_QUERY_PERF_COUNTER_CALL_OFFSET 1254
    0x00, 0x00, 0x00, 0x00,
    0xff, 0xd0, 0x2b, 0x66, 0xfc, 0xff,
    0x76, 0xfc, 0x83, 0xec, 0x08, 0x89, 0x34, 0x24, 0x89, 0x7c, 0x24, 0x04, 0x8d, 0x7c, 0x24, 0x0c,
    0x8b, 0x4e, 0xfc, 0xf3, 0xa4, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0x83, 0xc4, 0x08, 0x8b,
    0x56, 0xfc, 0x30, 0xc0, 0x86, 0x46, 0xfb, 0x89, 0xe3, 0x01, 0xd3, 0x83, 0xe4, 0xf0, 0x89, 0xfd,
    0x85, 0xed, 0x74, 0x07, 0xff, 0x74, 0xab, 0x2c, 0x4d, 0xeb, 0xf5, 0x29, 0xd3, 0xff, 0x54, 0x13,
    0x24, 0x89, 0xdc, 0xf3, 0x90, 0xb1, 0x01, 0x86, 0x4e, 0xfb, 0x84, 0xc9, 0x75, 0xf5, 0x5a, 0x83,
    0xec, 0x08, 0x89, 0x34, 0x24, 0x89, 0x7c, 0x24, 0x04, 0x89, 0xf7, 0x8d, 0x74, 0x24, 0x08, 0x89,
    0xd1, 0xf3, 0xa4, 0x8b, 0x34, 0x24, 0x89, 0x56, 0xfc, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04,
    0x83, 0xc4, 0x08, 0x01, 0xd4, 0xe8, 0x0e, 0x01, 0x00, 0x00, 0x83, 0xec, 0x08, 0x89, 0x34, 0x24,
    0x89, 0x7c, 0x24, 0x04, 0x89, 0xf7, 0x03, 0x7e, 0xfc, 0xc6, 0x07, 0x2d, 0xc6, 0x47, 0x01, 0x3e,
    0xc6, 0x47, 0x02, 0x20, 0xc6, 0x44, 0x17, 0x03, 0x20, 0x83, 0xc7, 0x03, 0x89, 0xce, 0x89, 0xd1,
    0xf3, 0xa4, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0x83, 0xc4, 0x08, 0x83, 0xc2, 0x04, 0x01,
    0x56, 0xfc, 0x83, 0xec, 0x10, 0x89, 0xe1, 0x8d, 0x54, 0x24, 0x08, 0x52, 0x51, 0x89, 0xc5, 0xb8,
#define NT_QUERY_PERF_COUNTER_RET_OFFSET 1456
    0x00, 0x00, 0x00, 0x00,
    0xff, 0xd0, 0x8b, 0x04, 0x24, 0x2b, 0x44, 0x24, 0x10, 0x69, 0xc0, 0x40,
    0x42, 0x0f, 0x00, 0x31, 0xd2, 0xf7, 0x74, 0x24, 0x08, 0x83, 0xc4, 0x18, 0xe8, 0xa7, 0x00, 0x00,
    0x00, 0x89, 0xe8, 0x83, 0xec, 0x08, 0x89, 0x34, 0x24, 0x89, 0x7c, 0x24, 0x04, 0x89, 0xf7, 0x03,
    0x7e, 0xfc, 0xc6, 0x07, 0x5b, 0xc6, 0x44, 0x17, 0x01, 0x20, 0xc7, 0x44, 0x17, 0x02, 0xc2, 0xb5,
    0x73, 0x5d, 0x83, 0xc7, 0x01, 0x89, 0xce, 0x89, 0xd1, 0xf3, 0xa4, 0x8b, 0x34, 0x24, 0x8b, 0x7c,
    0x24, 0x04, 0x83, 0xc4, 0x08, 0x83, 0xc2, 0x06, 0x01, 0x56, 0xfc, 0x5d, 0x85, 0xed, 0x74, 0x11,
    0xe8, 0x20, 0x00, 0x00, 0x00, 0x83, 0xc4, 0x0c, 0x8b, 0x0c, 0x24, 0x8d, 0x64, 0x94, 0x04, 0x51,
    0xc3, 0xe8, 0x0f, 0x00, 0x00, 0x00, 0x83, 0xc4, 0x0c, 0xc3, 0xe8, 0x06, 0x00, 0x00, 0x00, 0x83,
    0xc4, 0x04, 0xc2, 0x04, 0x00, 0x5b, 0x8b, 0x56, 0xfc, 0xc7, 0x04, 0x16, 0x0a, 0x0a, 0x00, 0x00,
    0x83, 0xc2, 0x02, 0x31, 0xc9, 0x51, 0x51, 0x52, 0x56, 0xe8, 0x08, 0x00, 0x00, 0x00, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x51, 0x51, 0x51, 0x68,
#define LOG_FILE_HANDLE_OFFSET 1626
    0x00, 0x00, 0x00, 0x00,
    0x89, 0xc5,
    0xb8,
#define NT_WRITE_FILE_ADDR_OFFSET 1633
    0x00, 0x00, 0x00, 0x00,
    0xff, 0xd0, 0x89, 0xe8, 0x30, 0xc9, 0x86, 0x4e, 0xfb, 0x89, 0xd9,
    0x89, 0xfa, 0x5d, 0x5b, 0x5f, 0x5e, 0x51, 0xc3, 0x83, 0xec, 0x08, 0x89, 0x34, 0x24, 0x89, 0x7c,
    0x24, 0x04, 0xe8, 0x0c, 0x00, 0x00, 0x00, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x90, 0x90, 0x59, 0xba, 0x02, 0x00, 0x00, 0x00, 0x50, 0x85, 0xc0, 0x75, 0x07, 0xc6, 0x04,
    0x11, 0x30, 0x42, 0xeb, 0x50, 0x89, 0xe0, 0x83, 0xc0, 0x03, 0x0f, 0xb6, 0x30, 0x85, 0xf6, 0x75,
    0x03, 0x48, 0xeb, 0xf6, 0x0f, 0xb6, 0x30, 0x89, 0xf7, 0xc1, 0xee, 0x04, 0x83, 0xfe, 0x0a, 0x7c,
    0x05, 0x83, 0xc6, 0x57, 0xeb, 0x0c, 0x85, 0xf6, 0x75, 0x05, 0x83, 0xfa, 0x02, 0x74, 0x09, 0x83,
    0xc6, 0x30, 0x96, 0x88, 0x04, 0x11, 0x96, 0x42, 0x83, 0xe7, 0x0f, 0x83, 0xff, 0x0a, 0x7c, 0x05,
    0x83, 0xc7, 0x57, 0xeb, 0x03, 0x83, 0xc7, 0x30, 0x97, 0x88, 0x04, 0x11, 0x97, 0x42, 0x39, 0xe0,
    0x74, 0x03, 0x48, 0xeb, 0xbf, 0x58, 0x8b, 0x34, 0x24, 0x8b, 0x7c, 0x24, 0x04, 0x83, 0xc4, 0x08,
    0xc3
};
std::vector<BYTE> LoggerShellCode(bLoggerShellCode, 
    bLoggerShellCode + sizeof(bLoggerShellCode));

#endif

#if defined(_M_X64) || defined(__amd64__)
BYTE bTrampolineCode[] =
{
    0x68,
#define API_INFO_OFF_OFFSET 0x1
    0x00, 0x00, 0x00, 0x00,
    0x48, 0xB8,
#define API_LOGGER_ADDR_OFFSET 0x7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xD0, 0x48, 0xB8,
#define API_ADDR_OFFSET 0x13
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xE0
};
std::vector<BYTE> TrampolineCode(bTrampolineCode,
    bTrampolineCode + sizeof(bTrampolineCode));

#else
BYTE bTrampolineCode[] =
{
    0x68,
#define API_INFO_OFF_OFFSET 0x1
    0x00, 0x00, 0x00, 0x00,
    0xB8,
#define API_LOGGER_ADDR_OFFSET 0x6
    0x00, 0x00, 0x00, 0x00,
    0xFF, 0xD0, 0xB8,
#define API_ADDR_OFFSET 0xD
    0x00, 0x00, 0x00, 0x00,
    0xFF, 0xE0
};
std::vector<BYTE> TrampolineCode(bTrampolineCode,
    bTrampolineCode + sizeof(bTrampolineCode));

#endif

BOOL ParseLoadedDll(HANDLE hProcess, std::map<LPVOID, _DllInfo>::iterator DllInfo);
BOOL InitializeWinApi();
BOOL HookDll(HANDLE hProcess, _DllInfo DllInfo);

INT main(INT argc, CHAR** argv)
{
    if (!Utils::InitUtils())
    {
        Utils::Printf::Fail("Cannot initialize properly");
        return FALSE;
    };

    if (argc < 3)
    {
        Utils::Printf::Info(
            "xLogger v0.0 (https://github.com/d35ha/xLogger)\n"
            "A super simple CLI windows api logger\n"
            "Usage: xLogger <Options> [-c [CommandLine]|-attach [Pid]] -l [LogFile]\n"
            "Options:\n"
            "    --external-mods <list of modules>: Log calls from these modules with the main module\n"
            "    --exclude-apis <list of dll:api>: Never log calls made to these apis\n"
            "    --exclude-mods <list of modules>: Never log calls made to these modules\n"
            "    --exclude-all-apis-except <list of dll:api>: Only log calls made to these apis\n"
            "    --exclude-all-mods-except <list of modules>: Only log calls made to these modules\n"
            "    --detach-on-exit: Keep the process when the logger exits (the process will continue logging)\n"
            "    --detach-on-system: Detach on reaching system/attach breakpoint (the process will continue logging)\n"
            "    --hide-debugger: Hide the debugger from being detected by IsDebuggerPresent,PEB BeingDebugged,..\n"
            "    --external-console: Run the process at external console (only with -c switch)\n"
            "    --apis-dir <directory path>: directory of api definition files (default: WinApi)\n"
            "    --headers-dir <directory path>: directory of headers definition files (default: WinApi\\headers)\n"
            "    -v: verbous mode\n"
            "CommandLine: The command line of the target executable\n"
            "Pid: The pid of the target process\n"
            "LogFile: Path of the file to save the logged calls to\n"
            "Examples:\n"
            "    xLogger -c evil.exe -l log.txt\n"
            "    xLogger -attach 1337 -l log.txt\n"
            "    xLogger -c \"evil.exe param\" --exclude-mods ntdll.dll -l \"log.txt\"\n"
            "    xLogger -c \"\\\"c:\\malware folder\\evil.exe\\\" \\\"param one\\\"\" -l log.txt\n"
            "    xLogger -c \"c:\\python\\python.exe c:\\bad.py\" --external-mods python27.dll -l log.txt\n"
            "    xLogger -c evil.exe --exclude-apis kernel32.dll:GetLastError,ntdll.dll:NtReadFile -v -l log.txt\n"
            "    xLogger -c evil.exe -l log.txt --exclude-all-apis-except kernel32.dll:CreateProcessInternalW\n"
            "    xLogger -c evil.exe -l log.txt --exclude-all-mods-except ntdll.dll,msvcrt.dll\n"
        );
        return TRUE;
    };

    std::string szExeCmd;
    std::string szLogFile;

    auto ParseArgList = [](const std::string& szSuppliedList) -> std::vector<std::string>
    {
        std::vector<std::string> ParsedList;
        std::string szList = szSuppliedList;
        if (szList.compare(""))
        {
            DWORD dwPos = 0;
            std::string szElement;
            if (szList.back() != ',') szList.append(",");
            while ((dwPos = (DWORD)szList.find(",")) != (DWORD)std::string::npos) {
                if (dwPos)
                {
                    szElement = szList.substr(0, dwPos);
                    while (szElement.back() == ' ') szElement.pop_back();
                    while (szElement.front() == ' ') szElement.erase(0, 1);
                    if (szElement.compare(""))
                        ParsedList.push_back(szElement);
                };
                szList.erase(0, (SIZE_T)dwPos + 1);
            };
        };
        std::sort(ParsedList.begin(), ParsedList.end());
        ParsedList.erase(
            std::unique(ParsedList.begin(), ParsedList.end()), ParsedList.end()
        );
        return ParsedList;
    };

    auto SplitAndStoreListElement = [](const std::vector<std::string>& ListOfElements,
                                       std::map<std::string, std::vector<std::string>>* MapToSaveTo) -> bool
    {
        std::vector<std::string> SplittedListOfElements = ListOfElements;
        std::vector<std::string>::iterator ListIter =
        SplittedListOfElements.begin();
        while (ListIter != SplittedListOfElements.end())
        {
            DWORD dwPos = (DWORD)ListIter->find_first_of(":");
            if (dwPos == (DWORD)std::string::npos)
            {
                Utils::Printf::Fail("The apis array should be at this format dll0:api0,dll1:api1,dll2:api2,...");
                return FALSE;
            };
            std::string DllName = ListIter->substr(0, dwPos);
            std::string ApiName = ListIter->substr((SIZE_T)dwPos + 1);
            while (DllName.back() == ' ') DllName.pop_back();
            while (ApiName.front() == ' ') ApiName.erase(0, 1);
            std::for_each(DllName.begin(), DllName.end(), [](CHAR& ch) {ch = tolower(ch); });
            if (ApiName.find_first_of(' ') != std::string::npos
                    || ApiName.find_first_of(':') != std::string::npos)
            {
                Utils::Printf::Fail("Bad api name '%s'", ApiName.c_str());
                return FALSE;
            };
            std::map<std::string,std::vector<std::string>>::iterator
                    SplittedListIter;
            if ((SplittedListIter = (*MapToSaveTo).find(DllName)) ==
                    (*MapToSaveTo).end())
            {
                std::vector<std::string> EmptyList;
                SplittedListIter = (*MapToSaveTo).insert((*MapToSaveTo).end(),
                                   std::make_pair(DllName, EmptyList));
            };
            SplittedListIter->second.push_back(ApiName);
            ListIter++;
        };
        return TRUE;
    };

    for (DWORD dwParamIndex = 1; dwParamIndex < (DWORD)argc; dwParamIndex++)
    {
        if (!strcmp(argv[dwParamIndex], "-c"))
        {
            if (!szExeCmd.empty())
            {
                Utils::Printf::Fail("CommandLine switch already used with '%s'", szExeCmd.c_str());
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch -c needs a valid command line", szExeCmd.c_str());
                return FALSE;
            };
            szExeCmd = argv[dwParamIndex];
        }
        else if (!strcmp(argv[dwParamIndex], "-attach"))
        {
            if (dwPid)
            {
                Utils::Printf::Fail("Attach switch already used with %d", dwPid);
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch -attach needs a pid");
                return FALSE;
            };
            dwPid = atoi(argv[dwParamIndex]);
        }
        else if (!strcmp(argv[dwParamIndex], "-l"))
        {
            if (!szLogFile.empty())
            {
                Utils::Printf::Fail("LogFile switch already used with '%s'", szLogFile.c_str());
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch -l needs a valid log file name");
                return FALSE;
            };
            szLogFile = argv[dwParamIndex];
        }
        else if (!strcmp(argv[dwParamIndex], "--apis-dir"))
        {
            if (!ApisDir.empty())
            {
                Utils::Printf::Fail("Apis definition directory switch already used with '%s'", ApisDir.c_str());
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --apis-dir needs a valid directory path");
                return FALSE;
            };
            ApisDir = argv[dwParamIndex];
        }
        else if (!strcmp(argv[dwParamIndex], "--headers-dir"))
        {
            if (!HeadersDir.empty())
            {
                Utils::Printf::Fail("Headers definition directory switch already used with '%s'", HeadersDir.c_str());
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --headers-dir needs a valid directory path");
                return FALSE;
            };
            HeadersDir = argv[dwParamIndex];
        }
        else if (!strcmp(argv[dwParamIndex], "-v"))
        {
            if (IsVerbous)
            {
                Utils::Printf::Fail("Verbous switch already used");
                return FALSE;
            };
            IsVerbous = TRUE;
        }
        else if (!strcmp(argv[dwParamIndex], "--detach-on-exit"))
        {
            if (DetachOnExit)
            {
                Utils::Printf::Fail("Detach on exit switch already used");
                return FALSE;
            };
            DetachOnExit = TRUE;
        }
        else if (!strcmp(argv[dwParamIndex], "--detach-on-system"))
        {
            if (DetachOnSystem)
            {
                Utils::Printf::Fail("Detach on system/attach breakpoint switch already used");
                return FALSE;
            };
            DetachOnSystem = TRUE;
        }
        else if (!strcmp(argv[dwParamIndex], "--hide-debugger"))
        {
            if (HideDebugger)
            {
                Utils::Printf::Fail("Hide debugger switch already used");
                return FALSE;
            };
            HideDebugger = TRUE;
        }
        else if (!strcmp(argv[dwParamIndex], "--external-console"))
        {
            if (ExternalConsole)
            {
                Utils::Printf::Fail("External console switch already used");
                return FALSE;
            };
            ExternalConsole = TRUE;
        }
        else if (!strcmp(argv[dwParamIndex], "--external-mods"))
        {
            if (!ExternalModules.empty())
            {
                Utils::Printf::Fail("External modules switch already used");
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --external-mods needs a valid comma separated module list");
                return FALSE;
            };
            PCHAR lpParam = (PCHAR)argv[dwParamIndex];
            while (*lpParam) {
                *lpParam = tolower(*lpParam);
                lpParam++;
            };
            ExternalModules = ParseArgList(argv[dwParamIndex]);
        }
        else if (!strcmp(argv[dwParamIndex], "--exclude-mods"))
        {
            if (!ExcludedModules.empty())
            {
                Utils::Printf::Fail("Exclude modules switch already used");
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --exclude-mods needs a valid comma separated module list");
                return FALSE;
            };
            PCHAR lpParam = (PCHAR)argv[dwParamIndex];
            while (*lpParam) {
                *lpParam = tolower(*lpParam);
                lpParam++;
            };
            ExcludedModules = ParseArgList(argv[dwParamIndex]);
        }
        else if (!strcmp(argv[dwParamIndex], "--exclude-apis"))
        {
            if (!ExcludedApis.empty())
            {
                Utils::Printf::Fail("Exclude apis switch already used");
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --exclude-apis needs a valid comma separated dll!api list");
                return FALSE;
            };
            std::vector<std::string> ExcludedModApiList =
                ParseArgList(argv[dwParamIndex]);
            if (!SplitAndStoreListElement(ExcludedModApiList,
                                          &ExcludedApis)) return FALSE;
        }
        else if (!strcmp(argv[dwParamIndex], "--exclude-all-mods-except"))
        {
            if (!AllExcludedExceptModules.empty())
            {
                Utils::Printf::Fail("Exclude all modules except module list switch already used");
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --exclude-all-mods-except needs a valid comma separated module list");
                return FALSE;
            };
            PCHAR lpParam = (PCHAR)argv[dwParamIndex];
            while (*lpParam) {
                *lpParam = tolower(*lpParam);
                lpParam++;
            };
            AllExcludedExceptModules = ParseArgList(argv[dwParamIndex]);
        }
        else if (!strcmp(argv[dwParamIndex], "--exclude-all-apis-except"))
        {
            if (!AllExcludedExceptApis.empty())
            {
                Utils::Printf::Fail("Exclude all apis except api list switch already used");
                return FALSE;
            };
            if (++dwParamIndex == argc)
            {
                Utils::Printf::Fail("Switch --exclude-all-apis-except needs a valid comma separated dll:api list");
                return FALSE;
            };
            std::vector<std::string> AllExcludedModApiExceptApiList =
                ParseArgList(argv[dwParamIndex]);
            if (!SplitAndStoreListElement(AllExcludedModApiExceptApiList,
                                          &AllExcludedExceptApis)) return FALSE;
        }
        else
        {
            Utils::Printf::Fail("Unknown switch %s", argv[dwParamIndex]);
            return FALSE;
        };
    };

    if (!szExeCmd.empty() && dwPid)
    {
        Utils::Printf::Fail("Both Pid and CommandLine cannot be supplied together");
        return FALSE;
    }
    else if (szExeCmd.empty() && !dwPid)
    {
        Utils::Printf::Fail("At least one of CommandLine or Pid should be supplied");
        return FALSE;
    };

    if (dwPid && ExternalConsole)
    {
        Utils::Printf::Fail("External console switch should be used with -c switch");
        return FALSE;
    };

    if (szLogFile.empty())
    {
        Utils::Printf::Fail("The LogFile should be supplied");
        return FALSE;
    };

    if (!AllExcludedExceptModules.empty() &&
            (!AllExcludedExceptApis.empty() || !ExcludedModules.empty() || !ExcludedApis.empty()))
    {
        Utils::Printf::Fail("Switch --exclude-all-mods-except cannot be used with any of --exclude-all-apis-except, --exclude-apis, --exclude-mods");
        return FALSE;
    };

    if (!AllExcludedExceptApis.empty() &&
            (!AllExcludedExceptModules.empty() || !ExcludedModules.empty() || !ExcludedApis.empty()))
    {
        Utils::Printf::Fail("Switch --exclude-all-apis-except cannot be used with any of --exclude-all-mods-except, --exclude-apis, --exclude-mods");
        return FALSE;
    };

    if (ApisDir.empty()) ApisDir = "WinApi";
    if (HeadersDir.empty()) HeadersDir = "WinApi\\headers";

    if (!InitializeWinApi())
    {
        Utils::Printf::Fail("Cannot load win api information");
        return FALSE;
    };

    if (!DeleteFileA(szLogFile.c_str()))
    {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            Utils::Reportf::ApiError("DeleteFileA", "Cannot delete the log file");
            return FALSE;
        }
    }

    HANDLE hFile = NULL;
    if (!(hFile = CreateFileA(
                      szLogFile.c_str(),
                      GENERIC_WRITE,
                      FILE_SHARE_READ,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL
                  )) || INVALID_HANDLE_VALUE == hFile)
    {
        Utils::Reportf::ApiError("CreateFileA", "Cannot create the log file");
        return FALSE;
    };

    if (!szExeCmd.empty())
    {
        STARTUPINFO StartUpInfo;
        PROCESS_INFORMATION ProcessInformation;
        ZeroMemory(
            &StartUpInfo,
            sizeof(StartUpInfo)
        );

        StartUpInfo.cb = sizeof(StartUpInfo);
        ZeroMemory(
            &ProcessInformation,
            sizeof(ProcessInformation)
        );

        std::string FilePath;
        while (szExeCmd.front() == ' ') szExeCmd.erase(0, 1);
        if (szExeCmd.front() == '"')
        {
            SIZE_T DoubleQuotePos = szExeCmd.find_first_of('"', 1);
            if (DoubleQuotePos == std::string::npos
                    || (DoubleQuotePos != szExeCmd.length() - 1
                        && szExeCmd.at(DoubleQuotePos + 1) != ' '))
            {
                Utils::Printf::Fail("Cannot extract the working directory from the supplied command line");
                return FALSE;
            };
            FilePath = szExeCmd.substr(1, --DoubleQuotePos);
            szExeCmd.erase(1, FilePath.length());
        }
        else
        {
            SIZE_T SpacePos = szExeCmd.find_first_of(' ');
            SpacePos == std::string::npos ? FilePath = szExeCmd :
                    FilePath = szExeCmd.substr(0, SpacePos);
            szExeCmd.erase(0, FilePath.length());
        };

        SIZE_T SlashPos = FilePath.find_last_of("\\/");
        if (SlashPos == std::string::npos)
        {
            FilePath.insert(0, ".\\");
            SlashPos = 1;
        };

        if (!CreateProcessA(
                    (LPCSTR)FilePath.c_str(),
                    (LPSTR)szExeCmd.c_str(),
                    NULL,
                    NULL,
                    FALSE,
                    ExternalConsole ?
                    (DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE)
                    : DEBUG_ONLY_THIS_PROCESS,
                    NULL,
                    FilePath.substr(0, SlashPos).c_str(),
                    &StartUpInfo,
                    &ProcessInformation
                ))
        {
            Utils::Reportf::ApiError("CreateProcessA", "Cannot create a new process from '%s'", FilePath.c_str());
            return FALSE;
        };
        Utils::Printf::Success("Process created successfully with pid %d", ProcessInformation.dwProcessId);
        hProcess = ProcessInformation.hProcess;
        hThread = ProcessInformation.hThread;
    }
    else
    {
        if (!(hProcess = OpenProcess(
                             PROCESS_ALL_ACCESS,
                             FALSE,
                             dwPid
                         )))
        {
            Utils::Reportf::ApiError("OpenProcess", "Cannot open the target process with pid '%d'", dwPid);
            return FALSE;
        };

        if (!DebugActiveProcess(
                    dwPid
                ))
        {
            Utils::Reportf::ApiError("DebugActiveProcess", "Cannot debug the target process with pid '%d'", dwPid);
            return FALSE;
        };

        DWORD dwNeededSize = 0;
        if (!EnumProcessModules(
                    hProcess,
                    NULL,
                    0,
                    &dwNeededSize))
        {
            if (IsVerbous) Utils::Printf::Info("Cannot get the number of modules for process '%d'", dwPid);
            dwNeededSize = sizeof(HMODULE);
        };

        dwModNum = dwNeededSize / sizeof(HMODULE) - 1;
        Utils::Printf::Success("Attached successfully with the process with pid %d", dwPid);
    };

    if (!DuplicateHandle(
                GetCurrentProcess(),
                hFile,
                hProcess,
                &ShellCodeInfo.hLogFile,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS

            ))
    {
        Utils::Reportf::ApiError("DuplicateHandle",
                                 "Cannot dublicate the handle of the logging file for the target process");
        return FALSE;
    };

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    PageSize = SystemInfo.dwPageSize;
    DllsPageAlignment = PageSize << 4;

    DWORD dwArch = x32;
    if (!Load::Process::GetArch(
                hProcess,
                &dwArch
            ))
    {
        Utils::Printf::Fail("Unable to get the architecture of this process");
        return FALSE;
    };

#if defined(_M_X64) || defined(__amd64__)
    if (dwArch == x32)
    {
        Utils::Printf::Fail("Use the x32 binary to handle this process");
        return FALSE;
    };
#else
    if (dwArch == x64)
    {
        Utils::Printf::Fail("Use the x64 binary to handle this process");
        return FALSE;
    };
#endif

    BYTE bPeb[sizeof(PEB)] = { 0 };
    PPEB lpPeb = (PPEB)bPeb;
    LPVOID lpPebAddress = NULL;
    if (!Load::Process::GetPEB(
                hProcess,
                lpPeb,
                &lpPebAddress
            ))
    {
        Utils::Printf::Fail("Unable to get process PEB");
        return FALSE;
    };
    ShellCodeInfo.lpImageBase = lpPeb->Reserved3[1];

    BYTE bNtHeader[sizeof(IMAGE_NT_HEADERS)] = { 0 };
    PIMAGE_NT_HEADERS lpNtHeader = (PIMAGE_NT_HEADERS)bNtHeader;
    if (!Load::Process::GetModNtHeader(
                hProcess,
                ShellCodeInfo.lpImageBase,
                lpNtHeader
            ))
    {
        Utils::Printf::Fail("Unable to get the process NT header");
        return FALSE;
    };
    ShellCodeInfo.lpImageTop = (LPVOID)((uintptr_t)ShellCodeInfo.lpImageBase + lpNtHeader->OptionalHeader.SizeOfImage);

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
    {
        Utils::Reportf::ApiError("GetModuleHandleA", "Unable to get ntdll handle");
        return FALSE;
    };
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32)
    {
        Utils::Reportf::ApiError("GetModuleHandleA", "Unable to get kernel32 handle");
        return FALSE;
    };
    ShellCodeInfo.lpNtWriteFile = (LPVOID)GetProcAddress(
                                      hNtdll,
                                      "NtWriteFile"
                                  );
    if (!ShellCodeInfo.lpNtWriteFile)
    {
        Utils::Printf::Fail("Unable to get NtWriteFile address");
        return FALSE;
    };
    ShellCodeInfo.lpNtQueryPerfCounter = (LPVOID)GetProcAddress(
            hNtdll,
            "NtQueryPerformanceCounter"
                                         );
    if (!ShellCodeInfo.lpNtQueryPerfCounter)
    {
        Utils::Printf::Fail("Unable to get NtQueryPerformanceCounter address");
        return FALSE;
    };
    ShellCodeInfo.lpRtlQueryPerfCounter = (LPVOID)GetProcAddress(
            hNtdll,
            "RtlQueryPerformanceCounter"
                                          );
    if (!ShellCodeInfo.lpRtlQueryPerfCounter)
    {
        ShellCodeInfo.lpRtlQueryPerfCounter = (LPVOID)GetProcAddress(
            hKernel32,
            "QueryPerformanceCounter"
        );
        if (!ShellCodeInfo.lpRtlQueryPerfCounter)
        {
            Utils::Printf::Fail("Unable to get QueryPerformanceCounter address");
            return FALSE;
        };
    };

    BOOL ReachedSystemBP = FALSE;
    SIZE_T stWrittenBytes = 0;
    DWORD dwContinueStatus = DBG_CONTINUE;

    PCHAR pEnd;
    IO_STATUS_BLOCK IoStatusBlock;

    LPVOID fnNtQueryInformationFile = NULL;
    if (!(fnNtQueryInformationFile = (LPVOID)GetProcAddress(hNtdll, "NtQueryInformationFile")))
    {
        Utils::Printf::Fail("Cannot get NtQueryInformationFile address");
        return FALSE;
    };

    if (DetachOnExit)
    {
        if (!DebugSetProcessKillOnExit(
            FALSE
        ))
        {
            Utils::Reportf::ApiError("DebugSetProcessKillOnExit",
                "Cannot set the option of detaching at exiting the logger");
            return FALSE;
        };
    };

    for (;;)
    {
        CHAR szDllName[MAX_PATH] = { 0 };
        WCHAR FileNameInformation[MAX_PATH + sizeof(ULONG) / 2] = { 0 };
        DEBUG_EVENT DebugEvent = { 0 };

        PMY_FILE_NAME_INFORMATION lpFileNameInformation =
            (PMY_FILE_NAME_INFORMATION)FileNameInformation;

        if (!WaitForDebugEvent(
                    &DebugEvent,
                    INFINITE))
        {
            Utils::Reportf::ApiError("WaitForDebugEvent", "Cannot debug the current process");
            return FALSE;
        };

        _DllInfo DllInfo;
        DllInfo.dwDllSize = 0;
        DllInfo.dwHookChainSize = 0;
        DllInfo.dwNumberOfApis = 0;
        DllInfo.lpHookChain = NULL;

        std::map<LPVOID, _DllInfo>::iterator DllInfoIter;

        switch (DebugEvent.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:

            dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
            if (!(!ReachedSystemBP &&
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode ==
                EXCEPTION_BREAKPOINT))
            {
#if defined(_M_X64) || defined(__amd64__)
                if (IsVerbous) Utils::Printf::Info("%s chance exception 0x%lx at 0x%llx",
                    DebugEvent.u.Exception.dwFirstChance ? "First" : "Last",
                    DebugEvent.u.Exception.ExceptionRecord.ExceptionCode,
                    DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress
                );
#else
                if (IsVerbous) Utils::Printf::Info("%s chance exception 0x%lx at 0x%lx",
                    DebugEvent.u.Exception.dwFirstChance ? "First" : "Last",
                    DebugEvent.u.Exception.ExceptionRecord.ExceptionCode,
                    DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress
                );
#endif
                break;
            };

#if defined(_M_X64) || defined(__amd64__)
            Utils::Printf::Success("Process reached system/attach breakpoint at 0x%llx",
                DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress);
#else
            Utils::Printf::Success("Process reached system/attach breakpoint at 0x%lx",
                DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress);
#endif
            ReachedSystemBP = TRUE;

            if (HideDebugger)
            {
                BYTE BeingDebugged = 0;
                SIZE_T stWrittenBytes = 0;
                if (!WriteProcessMemory(
                    hProcess,
                    (LPVOID)((uintptr_t)lpPebAddress + offsetof(PEB, BeingDebugged)),
                    &BeingDebugged,
                    sizeof(BeingDebugged),
                    &stWrittenBytes
                ) || stWrittenBytes != sizeof(BeingDebugged))
                {
                    Utils::Reportf::ApiError("WriteProcessMemory", "Unable to hide the debugger");
                    return FALSE;
                };

                ULONG NtGlobalFlags = 0;
                if (!WriteProcessMemory(
                    hProcess,
#if defined(_M_X64) || defined(__amd64__)
                    (LPVOID)((uintptr_t)lpPebAddress + 0xBC),
#else
                    (LPVOID)((uintptr_t)lpPebAddress + 0x68),
#endif
                    & NtGlobalFlags,
                    sizeof(NtGlobalFlags),
                    &stWrittenBytes
                ) || stWrittenBytes != sizeof(NtGlobalFlags))
                {
                    Utils::Reportf::ApiError("WriteProcessMemory", "Unable to hide the debugger");
                    return FALSE;
                };
            };

            if (DetachOnSystem)
            {
                DebugActiveProcessStop(DebugEvent.dwProcessId);
                return TRUE;
            };
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            Utils::Printf::Success("Process exited with code 0x%lx", DebugEvent.u.ExitProcess.dwExitCode);
            return TRUE;
        case CREATE_PROCESS_DEBUG_EVENT:

            if (ShellCodeInfo.WinApiData.empty())
            {
                Utils::Printf::Fail("No apis loaded at all");
                return FALSE;
            };
            if (!(ShellCodeInfo.lpWinApi = VirtualAllocEx(
                hProcess,
                NULL,
                ShellCodeInfo.WinApiData.size(),
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
            )))
            {
                Utils::Reportf::ApiError("VirtualAllocEx", "Cannot allocate for the api data");
                return FALSE;
            };

            if (!WriteProcessMemory(
                hProcess,
                ShellCodeInfo.lpWinApi,
                ShellCodeInfo.WinApiData.data(),
                ShellCodeInfo.WinApiData.size(),
                &stWrittenBytes
            ) || stWrittenBytes != ShellCodeInfo.WinApiData.size())
            {
                Utils::Reportf::ApiError("WriteProcessMemory", "Cannot write the api data");
                return FALSE;
            };

            for (DWORD dwIndex = 0;
                dwIndex < sizeof(LPVOID); dwIndex++)
            {
                LoggerShellCode.at(LOG_FILE_HANDLE_OFFSET + (SIZE_T)dwIndex) =
                    ((PBYTE)& ShellCodeInfo.hLogFile)[dwIndex];
                LoggerShellCode.at(API_INFO_ADDRESS_OFFSET + (SIZE_T)dwIndex) =
                    ((PBYTE)& ShellCodeInfo.lpWinApi)[dwIndex];
                LoggerShellCode.at(NT_WRITE_FILE_ADDR_OFFSET + (SIZE_T)dwIndex) =
                    ((PBYTE)& ShellCodeInfo.lpNtWriteFile)[dwIndex];
                LoggerShellCode.at(NT_QUERY_PERF_COUNTER_RET_OFFSET + (SIZE_T)dwIndex) =
                    ((PBYTE)& ShellCodeInfo.lpNtQueryPerfCounter)[dwIndex];
                LoggerShellCode.at(NT_QUERY_PERF_COUNTER_CALL_OFFSET + (SIZE_T)dwIndex) =
                    ((PBYTE)& ShellCodeInfo.lpRtlQueryPerfCounter)[dwIndex];
            };

            ShellCodeInfo.wSizeOfExtModLst = 0;
            std::for_each(ExternalModules.begin(), ExternalModules.end(),
                [&](const std::string& szExternalModule) -> void
                {
                    LoggerShellCode.insert(
                        LoggerShellCode.begin() + EXTERNAL_MODS_LIST_OFFSET + ShellCodeInfo.wSizeOfExtModLst + 2,
                        (BYTE)szExternalModule.length()
                    );
                    ShellCodeInfo.wSizeOfExtModLst++;
                    LoggerShellCode.insert(
                        LoggerShellCode.begin() + EXTERNAL_MODS_LIST_OFFSET + ShellCodeInfo.wSizeOfExtModLst + 2,
                        szExternalModule.begin(),
                        szExternalModule.end()
                    );
                    ShellCodeInfo.wSizeOfExtModLst += (WORD)szExternalModule.length();
                });

            LoggerShellCode.at(EXTERNAL_MODS_LIST_OFFSET) = ((PBYTE)& ShellCodeInfo.wSizeOfExtModLst)[0];
            LoggerShellCode.at(EXTERNAL_MODS_LIST_OFFSET + 1) = ((PBYTE)& ShellCodeInfo.wSizeOfExtModLst)[1];


            if (!(ShellCodeInfo.lpLogApi = VirtualAllocEx(
                hProcess,
                NULL,
                LoggerShellCode.size() + PageSize,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_EXECUTE_READWRITE
            )))
            {
                Utils::Reportf::ApiError("VirtualAllocEx", "Cannot allocate for the logger");
                return FALSE;
            };

            if (!WriteProcessMemory(
                hProcess,
                ShellCodeInfo.lpLogApi,
                LoggerShellCode.data(),
                LoggerShellCode.size(),
                &stWrittenBytes
            ) || stWrittenBytes != LoggerShellCode.size())
            {
                Utils::Reportf::ApiError("WriteProcessMemory", "Cannot write the logger");
                return FALSE;
            };

            for (DWORD dwIndex = 0;
                dwIndex < sizeof(LPVOID); dwIndex++)
            {
                TrampolineCode.at(API_LOGGER_ADDR_OFFSET + (SIZE_T)dwIndex) =
                    ((PBYTE)& ShellCodeInfo.lpLogApi)[dwIndex];
            };

            break;
        case LOAD_DLL_DEBUG_EVENT:

            if ((*(NTSTATUS(WINAPI*)(
                HANDLE,
                PIO_STATUS_BLOCK,
                PVOID,
                ULONG,
                INT
                )) fnNtQueryInformationFile)(
                    DebugEvent.u.LoadDll.hFile,
                    &IoStatusBlock,
                    lpFileNameInformation,
                    sizeof(FileNameInformation),
                    0x9
                    ))
            {
                Utils::Printf::Fail("Cannot get a loaded library name at 0x%lx",
                    DebugEvent.u.LoadDll.lpBaseOfDll);
                return FALSE;
            };

            wcstombs(
                szDllName,
                lpFileNameInformation->FileName,
                lpFileNameInformation->FileNameLength / sizeof(WCHAR)
            );
            szDllName[lpFileNameInformation->FileNameLength / sizeof(WCHAR)] = 0;

            pEnd = &szDllName[strlen(szDllName)];
            while (*pEnd != (CHAR)'\\')
            {
                *pEnd = tolower(*pEnd);
                pEnd--;
            };
            strncpy(szDllName, ++pEnd, sizeof(szDllName) - 1);
            szDllName[sizeof(szDllName) - 1] = 0;

#if defined(_M_X64) || defined(__amd64__)
            Utils::Printf::Success("%c%c%c%c%c Dll %s loaded at 0x%llx", TOP_NODE,
                szDllName, DebugEvent.u.LoadDll.lpBaseOfDll);
#else
            Utils::Printf::Success("%c%c%c%c%c Dll %s loaded at 0x%lx", TOP_NODE,
                                   szDllName, DebugEvent.u.LoadDll.lpBaseOfDll);
#endif
            if ((DllInfoIter = WinApi.DllArray.find(DebugEvent.u.LoadDll.lpBaseOfDll))
                    != WinApi.DllArray.end())
            {
                Utils::Printf::Fail("Dll %s is loaded at the same base", DllInfoIter->second.szDllName.c_str());
                return FALSE;
            };

            DllInfo.szDllName = szDllName;
            DllInfoIter = WinApi.DllArray.insert(WinApi.DllArray.end(),
                                                 std::make_pair(DebugEvent.u.LoadDll.lpBaseOfDll, DllInfo)
                                                );

            if (!ParseLoadedDll(
                        hProcess,
                        DllInfoIter
                    ))
            {
                Utils::Printf::Fail("Cannot parse the %s", szDllName);
                return FALSE;
            };

            if (!HookDll(hProcess,
                         WinApi.DllArray[DebugEvent.u.LoadDll.lpBaseOfDll]))
            {
                Utils::Printf::Fail("Cannot hook the %s", szDllName);
                return FALSE;
            };

            if (dwPid && (dwModNum == WinApi.DllArray.size()))
            {
                dwModNum = 0;
                _DllInfo MainModule;
                MainModule.dwDllSize = 0;
                MainModule.dwHookChainSize = 0;
                MainModule.dwNumberOfApis = 0;
                MainModule.lpHookChain = NULL;

                MainModule.szDllName = "Main";
                MainModule.dwDllSize = (DWORD)((uintptr_t)ShellCodeInfo.lpImageTop -
                                               (uintptr_t)ShellCodeInfo.lpImageBase);
                std::map<LPVOID, _DllInfo> MainModuleDllArray;
                MainModuleDllArray[ShellCodeInfo.lpImageBase] = MainModule;

                std::map<LPVOID, _DllInfo>::iterator DllArrayIter
                    = WinApi.DllArray.begin();
                for (;;)
                {
                    if (DllArrayIter == WinApi.DllArray.end()) DllArrayIter = MainModuleDllArray.begin();
                    else if (DllArrayIter == MainModuleDllArray.end()) break;

                    Utils::Printf::Success("Hooking the IAT of the %s module",
                        DllArrayIter->second.szDllName.c_str());

                    BYTE bNtHeader[sizeof(IMAGE_NT_HEADERS)] = { 0 };
                    PIMAGE_NT_HEADERS lpNtHeader = (PIMAGE_NT_HEADERS)bNtHeader;
                    if (!Load::Process::GetModNtHeader(
                                hProcess,
                                DllArrayIter->first,
                                lpNtHeader
                            ))
                    {
                        Utils::Printf::Fail("Unable to get the module %s NT header",
                                            DllArrayIter->second.szDllName.c_str());
                        return FALSE;
                    };

                    DWORD dwImportBaseOffset = GET_DIRECTORY_ENTRY(lpNtHeader, IMAGE_DIRECTORY_ENTRY_IMPORT);
                    DWORD dwImportSize = GET_DIRECTORY_SIZE(lpNtHeader, IMAGE_DIRECTORY_ENTRY_IMPORT);

                    if (!dwImportBaseOffset)
                    {
                        if (IsVerbous) Utils::Printf::Info("Module %s has no import table",
                                                               DllArrayIter->second.szDllName.c_str());
                        DllArrayIter++;
                        continue;
                    };

                    SIZE_T stReadBytes = 0;
                    PBYTE pbImportDir = new BYTE[dwImportSize];
                    if (!ReadProcessMemory(
                                hProcess,
                                (LPVOID)((uintptr_t)DllArrayIter->first + dwImportBaseOffset),
                                pbImportDir,
                                dwImportSize,
                                &stReadBytes
                            ) || stReadBytes != dwImportSize)
                    {
                        Utils::Reportf::ApiError("ReadProcessMemory", "Cannot read the import table for module %s",
                                                 DllArrayIter->second.szDllName.c_str());
                        return FALSE;
                    };
                    PIMAGE_IMPORT_DESCRIPTOR lpImportData = (PIMAGE_IMPORT_DESCRIPTOR)pbImportDir;

                    while (lpImportData->Name)
                    {
                        DWORD dwFirstThunk = lpImportData->FirstThunk;
                        if (!dwFirstThunk)
                        {
                            Utils::Printf::Fail("Invalid import table found at module %s",
                                                DllArrayIter->second.szDllName.c_str());
                            return FALSE;
                        };

                        MEMORY_BASIC_INFORMATION mbi = { 0 };
                        if (!VirtualQueryEx(
                                    hProcess,
                                    (LPVOID)((uintptr_t)DllArrayIter->first + dwFirstThunk),
                                    &mbi,
                                    sizeof(mbi)
                                ))
                        {
                            Utils::Reportf::ApiError("VirtualQueryEx",
                                                     "Cannot query the information of the IAT memory");
                            return FALSE;
                        };

                        DWORD dwOldProtection = 0;
                        if (!VirtualProtectEx(
                                    hProcess,
                                    mbi.BaseAddress,
                                    mbi.RegionSize,
                                    PAGE_READWRITE,
                                    &dwOldProtection
                                ))
                        {
                            Utils::Reportf::ApiError("VirtualProtectEx",
                                                     "Cannot change the protection of the IAT memory");
                            return FALSE;
                        };

                        for (;;)
                        {
                            BYTE bImageThunk[sizeof(IMAGE_THUNK_DATA)] = { 0 };
                            if (!ReadProcessMemory(
                                        hProcess,
                                        (LPVOID)((uintptr_t)DllArrayIter->first + dwFirstThunk),
                                        bImageThunk,
                                        sizeof(bImageThunk),
                                        &stReadBytes
                                    ) || stReadBytes != sizeof(bImageThunk))
                            {
                                Utils::Reportf::ApiError("ReadProcessMemory",
                                                         "Cannot read the import thunk at rva 0x%lx for module %s",
                                                         dwFirstThunk, DllArrayIter->second.szDllName.c_str());
                                return FALSE;
                            };
                            PIMAGE_THUNK_DATA dwFThunk = (PIMAGE_THUNK_DATA)bImageThunk;
                            if (!dwFThunk->u1.AddressOfData) break;

                            std::map<LPVOID, _DllInfo>::iterator ImportedDllArrayIter
                                = WinApi.DllArray.begin();
                            while (ImportedDllArrayIter != WinApi.DllArray.end())
                            {
                                if ((LPVOID)dwFThunk->u1.AddressOfData >= ImportedDllArrayIter->first
                                        && (LPVOID)dwFThunk->u1.AddressOfData < (LPVOID)((uintptr_t)ImportedDllArrayIter->first +
                                                ImportedDllArrayIter->second.dwDllSize)) break;
                                ImportedDllArrayIter++;
                            };
                            if (ImportedDllArrayIter == WinApi.DllArray.end())
                            {
#if defined(_M_X64) || defined(__amd64__)
                                Utils::Printf::Fail("The imported address 0x%llx in module %s relies in unlisted dll",
                                                    dwFThunk->u1.AddressOfData, DllArrayIter->second.szDllName.c_str());
#else
                                Utils::Printf::Fail("The imported address 0x%lx in module %s relies in unlisted dll",
                                                    dwFThunk->u1.AddressOfData, DllArrayIter->second.szDllName.c_str());
#endif
                                return FALSE;
                            };

                            std::map<std::string, _ApiInfo>::iterator ApiInfoIter
                                = ImportedDllArrayIter->second.ApiArray.begin();
                            while (ApiInfoIter != ImportedDllArrayIter->second.ApiArray.end())
                            {
                                if (ApiInfoIter->second.lpAddress == (LPVOID)dwFThunk->u1.AddressOfData)
                                {
                                    LPVOID lpNewAddress = (LPVOID)((uintptr_t)ImportedDllArrayIter->second.lpHookChain +
                                                                   ApiInfoIter->second.dwHookOffset);

                                    if (!WriteProcessMemory(
                                                hProcess,
                                                (LPVOID)((uintptr_t)DllArrayIter->first + dwFirstThunk),
                                                &lpNewAddress,
                                                sizeof(lpNewAddress),
                                                &stWrittenBytes
                                            ) || stWrittenBytes != sizeof(lpNewAddress))
                                    {
#if defined(_M_X64) || defined(__amd64__)
                                        Utils::Reportf::ApiError("WriteProcessMemory",
                                                                 "Cannot write the new api address at 0x%llx for %s's IAT",
                                                                 (LPVOID)((uintptr_t)DllArrayIter->first + dwFirstThunk),
                                                                 DllArrayIter->second.szDllName.c_str());
#else
                                        Utils::Reportf::ApiError("WriteProcessMemory",
                                                                 "Cannot write the new api address at 0x%lx for %s's IAT",
                                                                 (LPVOID)((uintptr_t)DllArrayIter->first + dwFirstThunk),
                                                                 DllArrayIter->second.szDllName.c_str());
#endif

                                        return FALSE;
                                    };
                                    break;
                                };
                                ApiInfoIter++;
                            };
                            dwFirstThunk += sizeof(dwFThunk);
                        };
                        if (!VirtualProtectEx(
                                    hProcess,
                                    mbi.BaseAddress,
                                    mbi.RegionSize,
                                    dwOldProtection,
                                    &dwOldProtection
                                ))
                        {
                            Utils::Reportf::ApiError("VirtualProtectEx",
                                                     "Cannot restore the protection of the IAT memory");
                            return FALSE;
                        };
                        lpImportData++;
                    };
                    DllArrayIter++;
                    delete[] pbImportDir;
                };

                Utils::Printf::Success("Successfully hooked all of the IATs of the loaded modules");
            };
            break;
        case UNLOAD_DLL_DEBUG_EVENT:

#if defined(_M_X64) || defined(__amd64__)
            if ((DllInfoIter = WinApi.DllArray.find(DebugEvent.u.UnloadDll.lpBaseOfDll))
                    == WinApi.DllArray.end())
            {
                Utils::Printf::Info("Unknown dll unloaded from 0x%llx",
                                    DebugEvent.u.UnloadDll.lpBaseOfDll);
            }
            else
            {
                Utils::Printf::Success("Dll %s unloaded from 0x%llx",
                                       DllInfoIter->second.szDllName.c_str(), DebugEvent.u.UnloadDll.lpBaseOfDll);
            };
#else
            if ((DllInfoIter = WinApi.DllArray.find(DebugEvent.u.UnloadDll.lpBaseOfDll))
                    == WinApi.DllArray.end())
            {
                Utils::Printf::Info("Unknown dll unloaded from 0x%lx",
                                    DebugEvent.u.UnloadDll.lpBaseOfDll);
            }
            else
            {
                Utils::Printf::Success("Dll %s unloaded from 0x%lx",
                                       DllInfoIter->second.szDllName.c_str(), DebugEvent.u.UnloadDll.lpBaseOfDll);
            };
#endif
            if (DllInfoIter != WinApi.DllArray.end())
            {
                if (DllInfoIter->second.lpHookChain) {
                    if (!VirtualFreeEx(
                                hProcess,
                                DllInfoIter->second.lpHookChain,
                                0,
                                MEM_RELEASE
                            ))
                    {
                        Utils::Reportf::ApiError("VirtualFreeEx",
                                                 "Cannot free the HookChain of the unloaded dll");
                        return FALSE;
                    };
                };

                DllInfoIter->second.ApiArray.clear();
                DllInfoIter->second.szDllName.clear();
                WinApi.DllArray.erase(DllInfoIter);
            };

            break;
        };

        if (!ContinueDebugEvent(
                    DebugEvent.dwProcessId,
                    DebugEvent.dwThreadId,
                    dwContinueStatus
                ))
        {
            Utils::Reportf::ApiError("ContinueDebugEvent", "Cannot continue the suspended process");
            return FALSE;
        };
        dwContinueStatus = DBG_CONTINUE;
    };

    CloseHandle(hProcess);
    if (hThread) CloseHandle(hThread);
    CloseHandle(hFile);
    return TRUE;
};

BOOL ParseLoadedDll(HANDLE hProcess, std::map<LPVOID, _DllInfo>::iterator DllInfo)
{

    std::map<std::string, std::vector<std::string>>::iterator
        BlockedApisIter = ExcludedApis.find(DllInfo->second.szDllName);

    BYTE bNtHeader[sizeof(IMAGE_NT_HEADERS)] = { 0 };
    PIMAGE_NT_HEADERS lpNtHeader = (PIMAGE_NT_HEADERS)bNtHeader;
    if (!Load::Process::GetModNtHeader(
        hProcess,
        DllInfo->first,
        lpNtHeader
    ))
    {
        Utils::Printf::Fail("Unable to get the dll NT header");
        return FALSE;
    };
    DllInfo->second.dwDllSize = lpNtHeader->OptionalHeader.SizeOfImage;

    if (!AllExcludedExceptModules.empty())
    {
        std::vector<std::string>::iterator
        ExceptedFromExclusionModsIter = AllExcludedExceptModules.begin();
        while (ExceptedFromExclusionModsIter != AllExcludedExceptModules.end())
        {
            if (!ExceptedFromExclusionModsIter->compare(DllInfo->second.szDllName)) break;
            ExceptedFromExclusionModsIter++;
        };
        if (ExceptedFromExclusionModsIter == AllExcludedExceptModules.end())
        {
            if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll is excluded", MIDDLE_NODE);
            return TRUE;
        };
    };

    std::map<std::string, std::vector<std::string>>::iterator
            ExceptedFromExclusionApisIter = AllExcludedExceptApis.find(DllInfo->second.szDllName);

    if (!AllExcludedExceptApis.empty()
            && ExceptedFromExclusionApisIter == AllExcludedExceptApis.end())
    {
        if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll's apis are excluded", MIDDLE_NODE);
        return TRUE;
    };

    std::vector<std::string>::iterator
    BlockedDllsIter = ExcludedModules.begin();
    while (BlockedDllsIter != ExcludedModules.end())
    {
        if (!BlockedDllsIter->compare(DllInfo->second.szDllName)) break;
        BlockedDllsIter++;
    };
    if (BlockedDllsIter != ExcludedModules.end())
    {
        if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll is blocked", MIDDLE_NODE);
        return TRUE;
    };

    std::map<std::string,
        std::map<std::string, _SupportedApiInfo>>::iterator SupportedDllInfo;

    if ((SupportedDllInfo = WinApi.KnownDlls.find(
                                DllInfo->second.szDllName))
            == WinApi.KnownDlls.end())
    {
        std::vector<std::string>::iterator CrtDllInfo =
            WinApi.CrtDlls.begin();
        while (CrtDllInfo != WinApi.CrtDlls.end())
        {
            if (!(*CrtDllInfo).compare(DllInfo->second.szDllName))
            {
                if ((SupportedDllInfo = WinApi.KnownDlls.find(
                                            "msvcrxx.dll"))
                        == WinApi.KnownDlls.end())
                {
                    if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll needs msvcrxx.api but it's not loaded", MIDDLE_NODE);
                    return TRUE;
                };
                break;
            };
            CrtDllInfo++;
        };
        if (CrtDllInfo == WinApi.CrtDlls.end())
        {
            if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll is not supported", MIDDLE_NODE);
            return TRUE;
        };
    };

    DWORD dwExportBaseOffset = GET_DIRECTORY_ENTRY(lpNtHeader, IMAGE_DIRECTORY_ENTRY_EXPORT);
    DWORD dwExportSize = GET_DIRECTORY_SIZE(lpNtHeader, IMAGE_DIRECTORY_ENTRY_EXPORT);

    if (!dwExportBaseOffset || !dwExportSize)
    {
        if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll has no export table found", MIDDLE_NODE);
        return TRUE;
    };

    SIZE_T stReadBytes = 0;
    PBYTE pbExportDir = new BYTE[dwExportSize];
    if (!ReadProcessMemory(
                hProcess,
                (LPVOID)((uintptr_t)DllInfo->first + dwExportBaseOffset),
                pbExportDir,
                dwExportSize,
                &stReadBytes
            ) || stReadBytes != dwExportSize)
    {
        Utils::Reportf::ApiError("ReadProcessMemory", "Cannot read the export table for this dll");
        return FALSE;
    };
    PIMAGE_EXPORT_DIRECTORY lpExportDir = (PIMAGE_EXPORT_DIRECTORY)pbExportDir;

    if (!lpExportDir->NumberOfNames)
    {
        delete[] pbExportDir;
        if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll has no exported symbols", MIDDLE_NODE);
        return TRUE;
    };

    DWORD dwOffsetOfNames = lpExportDir->AddressOfNames - dwExportBaseOffset;
    PDWORD pdwNameRvaArray = (PDWORD)((uintptr_t)lpExportDir + dwOffsetOfNames);

    DWORD dwOffsetOfOrdinals = lpExportDir->AddressOfNameOrdinals - dwExportBaseOffset;
    PWORD pwOrdinalRvaArray = (PWORD)((uintptr_t)lpExportDir + dwOffsetOfOrdinals);

    DWORD dwOffsetOfFunctions = lpExportDir->AddressOfFunctions - dwExportBaseOffset;
    PDWORD pdwFunctionRvaArray = (PDWORD)((uintptr_t)lpExportDir + dwOffsetOfFunctions);

    if (dwOffsetOfNames < 0 || dwOffsetOfNames >= dwExportSize
            || dwOffsetOfOrdinals < 0 || dwOffsetOfOrdinals >= dwExportSize
            || dwOffsetOfFunctions < 0 || dwOffsetOfFunctions >= dwExportSize)
    {
        Utils::Printf::Fail("Invalid export table");
        return FALSE;
    };

    DWORD dwOldProtection = 0;
    if (!VirtualProtectEx(
                hProcess,
                (LPVOID)((uintptr_t)DllInfo->first + dwExportBaseOffset),
                dwExportSize,
                PAGE_READWRITE,
                &dwOldProtection
            ))
    {
        Utils::Reportf::ApiError("VirtualProtectEx", "Cannot change the protection for the export table memory");
        return FALSE;
    };

    DllInfo->second.dwNumberOfApis = 0;
    DllInfo->second.dwHookChainSize = 0;

    for (DWORD dwFunIndex = 0; dwFunIndex < lpExportDir->NumberOfNames; dwFunIndex++)
    {
        DWORD dwOffsetOfName = *pdwNameRvaArray - dwExportBaseOffset;
        LPCSTR szFunName = (LPCSTR)((uintptr_t)lpExportDir + dwOffsetOfName);
        PDWORD pdwFunRva = (PDWORD)((uintptr_t)pdwFunctionRvaArray + sizeof(DWORD) * *pwOrdinalRvaArray);

        if (*pdwFunRva >= dwExportBaseOffset && *pdwFunRva < dwExportBaseOffset + dwExportSize)
        {
            if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping %s because it's forwarded to %s", MIDDLE_NODE, szFunName,
                                                   (LPCSTR)((uintptr_t)lpExportDir + *pdwFunRva - dwExportBaseOffset));
            pdwNameRvaArray++;
            pwOrdinalRvaArray++;
            continue;
        };

        if (*pdwFunRva < lpNtHeader->OptionalHeader.BaseOfCode ||
                *pdwFunRva >= lpNtHeader->OptionalHeader.BaseOfCode + lpNtHeader->OptionalHeader.SizeOfCode)
        {
            MEMORY_BASIC_INFORMATION mbi = { 0 };
            if (!VirtualQueryEx(
                        hProcess,
                        (LPVOID)((uintptr_t)DllInfo->first + *pdwFunRva),
                        &mbi,
                        sizeof(mbi)
                    ))
            {
                Utils::Reportf::ApiError("VirtualQueryEx", "Cannot query the protection for the symbol address");
                return FALSE;
            };
            if (mbi.Protect != PAGE_EXECUTE &&
                    mbi.Protect != PAGE_EXECUTE_READ &&
                    mbi.Protect != PAGE_EXECUTE_READWRITE &&
                    mbi.Protect != PAGE_EXECUTE_WRITECOPY
               )
            {
                if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping %s because it's not a function", MIDDLE_NODE, szFunName);
                pdwNameRvaArray++;
                pwOrdinalRvaArray++;
                continue;
            };
        };

        if (!AllExcludedExceptApis.empty())
        {
            std::vector<std::string>::iterator
            ExceptedFromExclusionApiIter = ExceptedFromExclusionApisIter->second.begin();
            while (ExceptedFromExclusionApiIter != ExceptedFromExclusionApisIter->second.end())
            {
                if (!ExceptedFromExclusionApiIter->compare(szFunName)) break;
                ExceptedFromExclusionApiIter++;
            };
            if (ExceptedFromExclusionApiIter == ExceptedFromExclusionApisIter->second.end())
            {
                if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping %s because it's excluded", MIDDLE_NODE, szFunName);
                pdwNameRvaArray++;
                pwOrdinalRvaArray++;
                continue;
            };
        };

        if (BlockedApisIter != ExcludedApis.end())
        {
            std::vector<std::string>::iterator
            BlockedApiIter = BlockedApisIter->second.begin();
            while (BlockedApiIter != BlockedApisIter->second.end())
            {
                if (!BlockedApiIter->compare(szFunName)) break;
                BlockedApiIter++;
            };
            if (BlockedApiIter != BlockedApisIter->second.end())
            {
                if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping %s because it's blocked", MIDDLE_NODE, szFunName);
                pdwNameRvaArray++;
                pwOrdinalRvaArray++;
                continue;
            };
        };

        std::string szFunSupportedName = szFunName;
        while (szFunSupportedName.front() == '_') szFunSupportedName.erase(0, 1);

        if (szFunSupportedName.at(0) == 'Z' &&
                szFunSupportedName.at(1) == 'w')
        {
            szFunSupportedName.at(0) = 'N';
            szFunSupportedName.at(1) = 't';
        };

        std::map<std::string, _SupportedApiInfo >::iterator SupportedApiInfo =
            SupportedDllInfo->second.begin();
        while (SupportedApiInfo != SupportedDllInfo->second.end())
        {
            if (!SupportedApiInfo->first.compare(szFunSupportedName)) break;
            SupportedApiInfo++;
        };
        if (SupportedApiInfo == SupportedDllInfo->second.end())
        {
            if (szFunSupportedName.back() == 'A' ||
                    szFunSupportedName.back() == 'W')
            {
                szFunSupportedName.pop_back();
                SupportedApiInfo = SupportedDllInfo->second.begin();
                while (SupportedApiInfo != SupportedDllInfo->second.end())
                {
                    if (!SupportedApiInfo->first.compare(szFunSupportedName)) break;
                    SupportedApiInfo++;
                };
            };
        };

        if (SupportedApiInfo == SupportedDllInfo->second.end())
        {
            if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping %s because it's not supported", MIDDLE_NODE, szFunName);
            pdwNameRvaArray++;
            pwOrdinalRvaArray++;
            continue;
        };

        while (!SupportedApiInfo->second.SourceModule.empty())
        {
            std::map<std::string,
                std::map<std::string, _SupportedApiInfo>>::iterator SourceModuleInfo;

            std::vector<std::map<std::string,
                std::map<std::string,
                _SupportedApiInfo>>::iterator>::iterator CachedSourceModulesIter =
                    WinApi.CachedSourceModules.begin();

            while (CachedSourceModulesIter != WinApi.CachedSourceModules.end())
            {
                if (!(*CachedSourceModulesIter)->first.compare(SupportedApiInfo->second.SourceModule))
                {
                    SourceModuleInfo = *CachedSourceModulesIter;
                    break;
                };
                CachedSourceModulesIter++;
            };
            if (CachedSourceModulesIter == WinApi.CachedSourceModules.end())
            {
                if ((SourceModuleInfo = WinApi.KnownDlls.find(
                                            SupportedApiInfo->second.SourceModule))
                        == WinApi.KnownDlls.end())
                {
                    if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping api %s because its source module %s is unsupported",
                                                           MIDDLE_NODE, SupportedApiInfo->first.c_str(), SupportedApiInfo->second.SourceModule.c_str());
                    pdwNameRvaArray++;
                    pwOrdinalRvaArray++;
                    break;
                };
                WinApi.CachedSourceModules.push_back(SourceModuleInfo);
            };

            std::map<std::string, _SupportedApiInfo >::iterator SourceApiDefInfo;
            if ((SourceApiDefInfo = SourceModuleInfo->second.find(SupportedApiInfo->first))
                    == SourceModuleInfo->second.end())
            {
                if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Skipping api %s because its source definition doesn't exist",
                                                       MIDDLE_NODE, SupportedApiInfo->first.c_str());
                pdwNameRvaArray++;
                pwOrdinalRvaArray++;
                break;
            };
            SupportedApiInfo = SourceApiDefInfo;
        };
        if (!SupportedApiInfo->second.SourceModule.empty()) continue;

        _ApiInfo ApiInfo = { 0 };
        ApiInfo.lpAddress = (LPVOID)(*pdwFunRva + (uintptr_t)DllInfo->first);
        ApiInfo.dwHookOffset = DllInfo->second.dwHookChainSize;
        ApiInfo.dwApiInfoOffset = SupportedApiInfo->second.dwApiInfoOffset;

        DllInfo->second.dwHookChainSize += (DWORD)TrampolineCode.size();
        DllInfo->second.ApiArray[szFunName] = ApiInfo;
        DllInfo->second.dwNumberOfApis++;

        pdwNameRvaArray++;
        pwOrdinalRvaArray++;
    };

    DllInfo->second.lpHookChain = (LPVOID)((uintptr_t)DllInfo->first + Utils::AlignUp(
            lpNtHeader->OptionalHeader.SizeOfImage,
            DllsPageAlignment
                                           ));

    DWORD dwAlignedSizeDelta = 0;
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    do
    {
        DllInfo->second.lpHookChain = (LPVOID)((uintptr_t)DllInfo->second.lpHookChain + mbi.RegionSize);
        if (!VirtualQueryEx(
                    hProcess,
                    DllInfo->second.lpHookChain,
                    &mbi,
                    sizeof(mbi)
                ))
        {
            Utils::Reportf::ApiError("VirtualQueryEx", "Cannot search for a free region for the HookChain");
            return FALSE;
        };

        DWORD dwLowPart = (DWORD)(uintptr_t)DllInfo->second.lpHookChain;
        dwAlignedSizeDelta = Utils::AlignUp(
                                 dwLowPart,
                                 DllsPageAlignment
                             ) - dwLowPart;

    } while ((mbi.State != MEM_FREE) || ((uintptr_t)DllInfo->second.dwHookChainSize +
                                         (uintptr_t)dwAlignedSizeDelta > mbi.RegionSize));
    DllInfo->second.lpHookChain = (LPVOID)((uintptr_t)DllInfo->second.lpHookChain + dwAlignedSizeDelta);

    if (!DllInfo->second.dwHookChainSize)
    {
        delete[] pbExportDir;
        DllInfo->second.lpHookChain = NULL;
        if (IsVerbous) Utils::Printf::Info("%c%c%c%c%c Dll has no hooked symbols", MIDDLE_NODE);
        return TRUE;
    };

    if (!VirtualAllocEx(
                hProcess,
                DllInfo->second.lpHookChain,
                DllInfo->second.dwHookChainSize,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
            ))
    {

#if defined(_M_X64) || defined(__amd64__)
        Utils::Reportf::ApiError("VirtualAllocEx",
                                 "Cannot allocate for the HookChain of the %s at 0x%llx", DllInfo->second.szDllName, DllInfo->second.lpHookChain);

#else
        Utils::Reportf::ApiError("VirtualAllocEx",
                                 "Cannot allocate for the HookChain of the %s at 0x%lx", DllInfo->second.szDllName, DllInfo->second.lpHookChain);

#endif
        return FALSE;
    };

    pdwNameRvaArray = (PDWORD)((uintptr_t)lpExportDir + dwOffsetOfNames);
    pwOrdinalRvaArray = (PWORD)((uintptr_t)lpExportDir + dwOffsetOfOrdinals);

    std::map<std::string, _ApiInfo>::iterator ApiInfo =
        DllInfo->second.ApiArray.begin();

    for (DWORD dwFunIndex = 0; dwFunIndex < lpExportDir->NumberOfNames; dwFunIndex++)
    {
        DWORD dwOffsetOfName = *pdwNameRvaArray - dwExportBaseOffset;
        LPCSTR szFunName = (LPCSTR)((uintptr_t)lpExportDir + dwOffsetOfName);
        PDWORD pdwFunRva = (PDWORD)((uintptr_t)pdwFunctionRvaArray +
                                    sizeof(DWORD) * *pwOrdinalRvaArray);

        if (ApiInfo == DllInfo->second.ApiArray.end()) break;
        if (!ApiInfo->first.compare(szFunName))
        {
            *pdwFunRva = ApiInfo->second.dwHookOffset +
                         (DWORD)((uintptr_t)DllInfo->second.lpHookChain - (uintptr_t)DllInfo->first);
            ApiInfo++;
        };

        pdwNameRvaArray++;
        pwOrdinalRvaArray++;
    };

    SIZE_T stWrittenBytes = 0;
    if (!WriteProcessMemory(
                hProcess,
                (LPVOID)((uintptr_t)DllInfo->first + dwExportBaseOffset),
                pbExportDir,
                dwExportSize,
                &stWrittenBytes
            ) || stWrittenBytes != dwExportSize)
    {
        Utils::Reportf::ApiError("WriteProcessMemory", "Cannot write the new export table to this dll");
        return FALSE;
    };

    if (!VirtualProtectEx(
                hProcess,
                (LPVOID)((uintptr_t)DllInfo->first + dwExportBaseOffset),
                dwExportSize,
                dwOldProtection,
                &dwOldProtection
            ))
    {
        Utils::Reportf::ApiError("VirtualProtectEx", "Cannot restore the protection for the export table memory");
        return FALSE;
    };

    delete[] pbExportDir;
    return TRUE;
};

BOOL HookDll(HANDLE hProcess, _DllInfo DllInfo)
{
    if (DllInfo.lpHookChain && DllInfo.dwHookChainSize)
    {
        LPVOID lpHookChain = NULL;
        if (!(lpHookChain = VirtualAlloc(
                                NULL,
                                DllInfo.dwHookChainSize,
                                (MEM_COMMIT | MEM_RESERVE),
                                PAGE_READWRITE
                            )))
        {
            Utils::Reportf::ApiError("VirtualAlloc", "Cannot allocate memory for the HookChain");
            return FALSE;
        };

        std::for_each(DllInfo.ApiArray.begin(), DllInfo.ApiArray.end(), 
            [&](std::pair<std::string, _ApiInfo> ApiInfo) -> void {
                for (DWORD dwIndex = 0;
                    dwIndex < sizeof(DWORD); dwIndex++)
                {
                    TrampolineCode.at(API_INFO_OFF_OFFSET + (SIZE_T)dwIndex) =
                        ((PBYTE)& ApiInfo.second.dwApiInfoOffset)[dwIndex];
                };

                for (DWORD dwIndex = 0;
                    dwIndex < sizeof(LPVOID); dwIndex++)
                {
                    TrampolineCode.at(API_ADDR_OFFSET + (SIZE_T)dwIndex) =
                        ((PBYTE)& ApiInfo.second.lpAddress)[dwIndex];
                };

                CopyMemory(
                    (LPVOID)((uintptr_t)lpHookChain + ApiInfo.second.dwHookOffset),
                    TrampolineCode.data(),
                    TrampolineCode.size()
                );
            });

        SIZE_T stWrittenBytes = 0;
        if (!WriteProcessMemory(
                    hProcess,
                    DllInfo.lpHookChain,
                    lpHookChain,
                    DllInfo.dwHookChainSize,
                    &stWrittenBytes
                ) || stWrittenBytes != DllInfo.dwHookChainSize)
        {
            Utils::Reportf::ApiError("WriteProcessMemory", "Cannot write the hook chain");
            return FALSE;
        };

        if (!VirtualFree(
                    lpHookChain,
                    0,
                    MEM_RELEASE
                ))
        {
            Utils::Reportf::ApiError("VirtualFree", "Cannot free the buffer of the HookChain");
            return FALSE;
        };
    };

    Utils::Printf::Success("%c%c%c%c%c Hooked %d api successfully", BASE_NODE,
                           DllInfo.dwNumberOfApis);
    return TRUE;
};

BOOL InitializeWinApi()
{
    WinApi.CrtDlls.push_back("msvcrt.dll");
    WinApi.CrtDlls.push_back("msvcr70.dll");
    WinApi.CrtDlls.push_back("msvcr70d.dll");
    WinApi.CrtDlls.push_back("msvcr71.dll");
    WinApi.CrtDlls.push_back("msvcr71d.dll");
    WinApi.CrtDlls.push_back("msvcr80.dll");
    WinApi.CrtDlls.push_back("msvcr80d.dll");
    WinApi.CrtDlls.push_back("msvcr90.dll");
    WinApi.CrtDlls.push_back("msvcr90d.dll");
    WinApi.CrtDlls.push_back("msvcr100.dll");
    WinApi.CrtDlls.push_back("msvcr100d.dll");
    WinApi.CrtDlls.push_back("msvcr100_clr0400.dll");

    WinApi.WinTypesSizes["BYTE"] = 1;
    WinApi.WinTypesSizes["CHAR"] = 1;
    WinApi.WinTypesSizes["INT8"] = 1;
    WinApi.WinTypesSizes["WORD"] = 2;
    WinApi.WinTypesSizes["ATOM"] = 2;
    WinApi.WinTypesSizes["TBYTE"] = 1;
    WinApi.WinTypesSizes["TCHAR"] = 1;
    WinApi.WinTypesSizes["UCHAR"] = 1;
    WinApi.WinTypesSizes["CCHAR"] = 1;
    WinApi.WinTypesSizes["UINT8"] = 1;
    WinApi.WinTypesSizes["SHORT"] = 2;
    WinApi.WinTypesSizes["INT16"] = 2;
    WinApi.WinTypesSizes["WCHAR"] = 2;
    WinApi.WinTypesSizes["USHORT"] = 2;
    WinApi.WinTypesSizes["LANGID"] = 2;
    WinApi.WinTypesSizes["UINT16"] = 2;
    WinApi.WinTypesSizes["HALF_PTR"] = sizeof(LPVOID) / 2;
    WinApi.WinTypesSizes["UHALF_PTR"] = sizeof(LPVOID) / 2;

    /*
    WinApi.WinTypesSizes["unsigned char"] = 1;
    WinApi.WinTypesSizes["wint_t"] = 2;
    WinApi.WinTypesSizes["unsigned short"] = 2;
    WinApi.WinTypesSizes["wchar_t"] = 1;
    WinApi.WinTypesSizes["RTL_ATOM"] = 2;
    WinApi.WinTypesSizes["u_short"] = 2;
    WinApi.WinTypesSizes["IP_ADDR_FAMILY"] =  2;
    WinApi.WinTypesSizes["PRUint16"] = 2;
    WinApi.WinTypesSizes["PRint16"] = 2;
    WinApi.WinTypesSizes["_ctype_t"] = 2;
    WinApi.WinTypesSizes["CLIPFORMAT"] = 2;
    WinApi.WinTypesSizes["DBCOMMANDOP"] = 2;
    */


#if defined(_M_X64) || defined(__amd64__)
    WinApi.WinTypesSizes["INT"] = 4;
    WinApi.WinTypesSizes["LCID"] = 4;
    WinApi.WinTypesSizes["LONG"] = 4;
    WinApi.WinTypesSizes["UINT"] = 4;
    WinApi.WinTypesSizes["HFILE"] = 4;
    WinApi.WinTypesSizes["FLOAT"] = 4;
    WinApi.WinTypesSizes["DWORD"] = 4;
    WinApi.WinTypesSizes["ULONG"] = 4;
    WinApi.WinTypesSizes["INT32"] = 4;
    WinApi.WinTypesSizes["PROPID"] = 4;
    WinApi.WinTypesSizes["U_LONG"] = 4;
    WinApi.WinTypesSizes["LCTYPE"] = 4;
    WinApi.WinTypesSizes["LGRPID"] = 4;
    WinApi.WinTypesSizes["LONG32"] = 4;
    WinApi.WinTypesSizes["UINT32"] = 4;
    WinApi.WinTypesSizes["DWORD32"] = 4;
    WinApi.WinTypesSizes["HRESULT"] = 4;
    WinApi.WinTypesSizes["ULONG32"] = 4;
    WinApi.WinTypesSizes["COLORREF"] = 4;

    /*
    WinApi.WinTypesSizes["DHCP_OPTION_ID"] = 4;
    WinApi.WinTypesSizes["DHCP_IP_ADDRESS"] = 4;
    WinApi.WinTypesSizes["REAL"] = 4;
    WinApi.WinTypesSizes["unsigned int"] = 4;
    WinApi.WinTypesSizes["MCIDEVICEID"] = 4;
    WinApi.WinTypesSizes["unsigned long"] = 4;
    WinApi.WinTypesSizes["__time32_t"] = 4;
    WinApi.WinTypesSizes["DEVINST"] = 4;
    WinApi.WinTypesSizes["socklen_t"] = 4;
    WinApi.WinTypesSizes["PRUint32"] = 4;
    WinApi.WinTypesSizes["PRInt32"] = 4;
    WinApi.WinTypesSizes["PRSpecialFD"] = 4;
    WinApi.WinTypesSizes["HTTPAPI_VERSION"] = 4;
    WinApi.WinTypesSizes["DATE"] = 4;
    WinApi.WinTypesSizes["ARGB"] = 4;
    WinApi.WinTypesSizes["GpUnit"] = 4;
    WinApi.WinTypesSizes["DEVNODE"] = 4;
    WinApi.WinTypesSizes["CLFS_CONTAINER_ID"] = 4;
    WinApi.WinTypesSizes["signed int"] = 4;
    WinApi.WinTypesSizes["FOURCC"] = 4;
    WinApi.WinTypesSizes["OLE_COLOR"] = 4;
    WinApi.WinTypesSizes["SYSKIND"] = 4;
    WinApi.WinTypesSizes["MSIHANDLE"] = 4;
    WinApi.WinTypesSizes["long int"] = 4;
    WinApi.WinTypesSizes["JET_GRBIT"] = 4;
    WinApi.WinTypesSizes["GraphicsContainer"] = 4;
    WinApi.WinTypesSizes["GraphicsState"] = 4;
    WinApi.WinTypesSizes["IPAddr"] = 4;
    WinApi.WinTypesSizes["NET_IFINDEX"] = 4;
    WinApi.WinTypesSizes["IP_STATUS"] = 4;
    WinApi.WinTypesSizes["MFSequencerElementId"] = 4;
    WinApi.WinTypesSizes["MF_TOPOLOGY_TYPE"] = 4;
    WinApi.WinTypesSizes["MFASYNC_WORKQUEUE_TYPE"] = 4;
    WinApi.WinTypesSizes["MF_FILE_ACCESSMODE"] = 4;
    WinApi.WinTypesSizes["MFStandardVideoFormat"] = 4;
    WinApi.WinTypesSizes["MFP_CREATION_OPTIONS"] = 4;
    WinApi.WinTypesSizes["METHODS"] = 4;
    WinApi.WinTypesSizes["SCODE"] = 4;
    WinApi.WinTypesSizes["RESTRICTIONS"] = 4;
    WinApi.WinTypesSizes["SSLKEAType"] = 4;
    */

#else
    WinApi.WinTypesSizes["INT64"] = 8;
    WinApi.WinTypesSizes["POINT"] = 8;
    WinApi.WinTypesSizes["QWORD"] = 8;
    WinApi.WinTypesSizes["LONG64"] = 8;
    WinApi.WinTypesSizes["DOUBLE"] = 8;
    WinApi.WinTypesSizes["UINT64"] = 8;
    WinApi.WinTypesSizes["DWORD64"] = 8;
    WinApi.WinTypesSizes["ULONG64"] = 8;
    WinApi.WinTypesSizes["LONGLONG"] = 8;
    WinApi.WinTypesSizes["ULONGLONG"] = 8;
    WinApi.WinTypesSizes["DWORDLONG"] = 8;
    WinApi.WinTypesSizes["LARGE_INTEGER"] = 8;
    WinApi.WinTypesSizes["ULARGE_INTEGER"] = 8;
    WinApi.WinTypesSizes["FILETIME"] = 8;
    WinApi.WinTypesSizes["unsigned __int64"] = 8;
    WinApi.WinTypesSizes["__int64"] = 8;
    WinApi.WinTypesSizes["__time64_t"] = 8;
    WinApi.WinTypesSizes["HTTP_URL_GROUP_ID"] = 8;
    WinApi.WinTypesSizes["GROUPID"] = 8;
    WinApi.WinTypesSizes["HTTP_SERVER_SESSION_ID"] = 8;
    WinApi.WinTypesSizes["SP_LOG_TOKEN"] = 8;
    WinApi.WinTypesSizes["time_t"] = 8;
    WinApi.WinTypesSizes["MFWORKITEM_KEY = 8"] = 8;

#endif

    WinApi.WinTypesStr.push_back("PSTR");
    WinApi.WinTypesStr.push_back("PWSTR");
    WinApi.WinTypesStr.push_back("PTSTR");
    WinApi.WinTypesStr.push_back("PCHAR");
    WinApi.WinTypesStr.push_back("PCSTR");
    WinApi.WinTypesStr.push_back("LPSTR");
    WinApi.WinTypesStr.push_back("LPTSTR");
    WinApi.WinTypesStr.push_back("PCTSTR");
    WinApi.WinTypesStr.push_back("PCWSTR");
    WinApi.WinTypesStr.push_back("LPWSTR");
    WinApi.WinTypesStr.push_back("PTCHAR");
    WinApi.WinTypesStr.push_back("PUCHAR");
    WinApi.WinTypesStr.push_back("PWCHAR");
    WinApi.WinTypesStr.push_back("LPCSTR");
    WinApi.WinTypesStr.push_back("LPCTSTR");
    WinApi.WinTypesStr.push_back("LPCWSTR");
    WinApi.WinTypesStr.push_back("LPOLESTR");
    WinApi.WinTypesStr.push_back("const char*");
    WinApi.WinTypesStr.push_back("const wchar*");
    WinApi.WinTypesStr.push_back("const char_t*");
    WinApi.WinTypesStr.push_back("const wchar_t*");

    /*
    WinApi.WinTypesStr.push_back("LMSTR");
    WinApi.WinTypesStr.push_back("const PCHAR");
    WinApi.WinTypesStr.push_back("JET_PCSTR");
    WinApi.WinTypesStr.push_back("P3PCURL");
    WinApi.WinTypesStr.push_back("P3PURL");
    WinApi.WinTypesStr.push_back("PWCH");
    WinApi.WinTypesStr.push_back("PCH");
    WinApi.WinTypesStr.push_back("CHAR*");
    WinApi.WinTypesStr.push_back("WCHAR*");
    WinApi.WinTypesStr.push_back("PCCH");
    WinApi.WinTypesStr.push_back("const PWSTR");
    WinApi.WinTypesStr.push_back("BSTR");
    WinApi.WinTypesStr.push_back("RPC_TSTR");
    WinApi.WinTypesStr.push_back("VSS_PWSZ");
    */

    auto IsStrType = [](const std::string& DataType) -> BOOL {
        return std::find(WinApi.WinTypesStr.begin(),
                         WinApi.WinTypesStr.end(), DataType) != WinApi.WinTypesStr.end();
    };

    auto IsBool = [](const std::string& DataType) -> BOOL {
        return !DataType.compare("BOOL") || !DataType.compare("BOOLEAN");
    };

    auto GetTypeSize = [](const std::string& DataType) -> DWORD {
        DWORD dwSize = sizeof(LPVOID);
        if (DataType.back() == '*') return sizeof(LPVOID);
        if (DataType.at(0) == 'L'
                && DataType.at(1) == 'P') return sizeof(LPVOID);
        std::find_if(WinApi.WinTypesSizes.begin(), WinApi.WinTypesSizes.end(),
        [&](const std::pair<std::string, DWORD>& WinType) -> bool {
            if (!stricmp(DataType.c_str(), WinType.first.c_str()))
            {
                dwSize = WinType.second;
                return TRUE;
            };
            return FALSE;
        });
        return dwSize;
    };

    HANDLE hFind;
    WIN32_FIND_DATA WinFindData;
    DWORD dwCurrentOffset = 0;

    while (HeadersDir.back() == '\\') HeadersDir.pop_back();
    while (ApisDir.back() == '\\') ApisDir.pop_back();

    HeadersDir.append("\\");
    ApisDir.append("\\");

    std::string HeadersDefDir = HeadersDir;
    HeadersDefDir.append("*.h.api");
    std::string ApisDefDir = ApisDir;
    ApisDefDir.append("*.api");

    Utils::Printf::Success("Loading WinApi ...");

    if (!(hFind = FindFirstFileA(
                      HeadersDefDir.c_str(),
                      &WinFindData
                  )) || hFind == INVALID_HANDLE_VALUE)
    {
        Utils::Reportf::ApiError("FindFirstFileA", "Cannot load the headers information from %s", HeadersDir.c_str());
        return FALSE;
    };

    do
    {
        std::string HeaderFileName = HeadersDir;
        HeaderFileName.append(WinFindData.cFileName);

        INIReader HeaderReader(HeaderFileName.c_str());
        if (HeaderReader.ParseError() < 0) {
            if (IsVerbous) Utils::Printf::Info("Error happend at parsing %s", HeaderFileName.c_str());
            continue;
        };

        std::string HeaderFile = WinFindData.cFileName;
        std::for_each(HeaderFile.begin(), HeaderFile.end(), [](CHAR& ch) {ch = tolower(ch); });

        std::map<std::string, SupportedHeaderInfo> HeaderFileInfo;
        std::set<std::string> HeaderSections = HeaderReader.Sections();

        std::for_each(HeaderSections.begin(), HeaderSections.end(),
            [&](std::string const& HeaderSection) -> void {

                std::vector<BYTE> WinHeaderInfo;
                SupportedHeaderInfo HeaderInfo = { 0 };

                HeaderInfo.dwHeaderInfoOffset = dwCurrentOffset;

                std::string szBase = HeaderReader.Get(HeaderSection.c_str(), "Base", "");
                std::string szType = HeaderReader.Get(HeaderSection.c_str(), "Type", "");
                std::string szTypeDisplay = HeaderReader.Get(HeaderSection.c_str(), "TypeDisplay", "");
                std::string szHeader = HeaderReader.Get(HeaderSection.c_str(), "Header", "");
                if (!szHeader.empty()) { while (szHeader.back() == ';') szHeader.pop_back(); }
                std::for_each(szHeader.begin(), szHeader.end(), [](CHAR& ch) {ch = tolower(ch); });

                std::map<std::string, ULONG> AdditionalHeaderData;
                if (szBase.front() == '[' && szBase.back() == ']')
                {
                    szBase.pop_back();
                    szBase.erase(0, 1);

                    if (szHeader.empty())
                    {
                        if (IsVerbous) Utils::Printf::Info("Header %s has base %s with no header",
                            HeaderSection.c_str(), szBase.c_str());
                        return;
                    };

                    std::string szHeaderPath = HeadersDir + szHeader;
                    INIReader AdditionalHeaderReader(szHeaderPath.c_str());

                    std::set<std::string>::iterator AdditionalSection =
                        AdditionalHeaderReader.Sections().begin();

                    while (AdditionalSection != AdditionalHeaderReader.Sections().end()) {
                        if (!AdditionalSection->compare(szBase))
                        {
                            std::string szNewBase = AdditionalHeaderReader.Get(szBase.c_str(), "Base", "");
                            std::string szNewType = AdditionalHeaderReader.Get(szBase.c_str(), "Type", "");
                            std::string szNewTypeDisplay = AdditionalHeaderReader.Get(szBase.c_str(), "TypeDisplay", "");

                            if (!szNewBase.empty()) szBase = szNewBase;
                            if (!szNewType.empty()) szType = szNewType;
                            if (!szNewTypeDisplay.empty()) szTypeDisplay = szNewTypeDisplay;

                            DWORD dwIndex = 0;
                            for (;;)
                            {
                                CHAR bIndex[10] = { 0 };
                                itoa(++dwIndex, bIndex, 10);
                                std::string szIndex = bIndex;
                                std::string szConstKey = "Const" + szIndex;
                                std::string szValueKey = "Value" + szIndex;

                                std::string szConst = AdditionalHeaderReader.Get(AdditionalSection->c_str(),
                                    szConstKey.c_str(), "");
                                if (szConst.empty()) break;

                                std::string szValue = AdditionalHeaderReader.Get(AdditionalSection->c_str(),
                                    szValueKey.c_str(), "");
                                if (szValue.empty())
                                {
                                    if (IsVerbous) Utils::Printf::Info("Header %s has const %s with no value supplied",
                                        AdditionalSection->c_str(), szConstKey.c_str());
                                    return;
                                };

                                DWORD dwValue = 0;
                                INT ret = 0;
                                if (!(ret = sscanf(szValue.c_str(), 
                                    (szValue.length() > 2 && (szValue[0] == '0' && tolower(szValue[1]) == 'x')) ? "%lx" : "%lu",
                                    &dwValue))
                                    || ret == EOF)
                                {
                                    if (IsVerbous) Utils::Printf::Info("Header %s has const %s with bad value supplied %s",
                                        AdditionalSection->c_str(), szConstKey.c_str(), szValue.c_str());
                                    return;
                                };

                                AdditionalHeaderData.insert(AdditionalHeaderData.end(),
                                    std::make_pair(szConst, dwValue));
                            };
                            break;
                        };
                        AdditionalSection++;
                    };
                };

                if (szTypeDisplay.empty() && szBase.empty())
                {
                    if (IsVerbous) Utils::Printf::Info("Header %s has no size",
                        HeaderSection.c_str());
                    return;
                };

                szTypeDisplay.empty() ? HeaderInfo.dwHeaderSize = GetTypeSize(szBase) :
                    HeaderInfo.dwHeaderSize = GetTypeSize(szTypeDisplay);

                DWORD dwIndex = 0;
                if (!szType.compare("Enum")) WinHeaderInfo.push_back((BYTE)0);
                else if (!szType.compare("Flag")) WinHeaderInfo.push_back((BYTE)1);
                else
                {
                    if (IsVerbous) Utils::Printf::Info("Header %s has unsupported type %s",
                        HeaderSection.c_str(), szType.c_str());
                    return;
                };

                for (;;)
                {
                    CHAR bIndex[10] = { 0 };
                    itoa(++dwIndex, bIndex, 10);
                    std::string szIndex = bIndex;
                    std::string szConstKey = "Const" + szIndex;
                    std::string szValueKey = "Value" + szIndex;

                    std::string szConst = HeaderReader.Get(HeaderSection.c_str(),
                        szConstKey.c_str(), "");
                    if (szConst.empty()) break;

                    std::string szValue = HeaderReader.Get(HeaderSection.c_str(),
                        szValueKey.c_str(), "");
                    if (szValue.empty())
                    {
                        if (IsVerbous) Utils::Printf::Info("Header %s has const %s with no value supplied",
                            HeaderSection.c_str(), szConstKey.c_str());
                        return;
                    };
                    DWORD dwValue = 0;
                    INT ret = 0;
                    if (!(ret = sscanf(szValue.c_str(),
                        (szValue.length() > 2 && (szValue[0] == '0' && tolower(szValue[1]) == 'x')) ? "%lx" : "%lu",
                        &dwValue))
                        || ret == EOF)
                    {
                        if (IsVerbous) Utils::Printf::Info("Header %s has const %s with bad value supplied %s",
                            HeaderSection.c_str(), szConstKey.c_str(), szValue.c_str());
                        return;
                    };

                    WinHeaderInfo.push_back(((PBYTE)& dwValue)[0]);
                    WinHeaderInfo.push_back(((PBYTE)& dwValue)[1]);
                    WinHeaderInfo.push_back(((PBYTE)& dwValue)[2]);
                    WinHeaderInfo.push_back(((PBYTE)& dwValue)[3]);
                    WinHeaderInfo.push_back((BYTE)szConst.size());
                    WinHeaderInfo.insert(WinHeaderInfo.end(),
                        szConst.begin(), szConst.end());
                };

                std::map<std::string, DWORD>::iterator HeaderDataIter =
                    AdditionalHeaderData.begin();
                while (HeaderDataIter != AdditionalHeaderData.end())
                {
                    WinHeaderInfo.push_back(((PBYTE)& HeaderDataIter->second)[0]);
                    WinHeaderInfo.push_back(((PBYTE)& HeaderDataIter->second)[1]);
                    WinHeaderInfo.push_back(((PBYTE)& HeaderDataIter->second)[2]);
                    WinHeaderInfo.push_back(((PBYTE)& HeaderDataIter->second)[3]);
                    WinHeaderInfo.push_back((BYTE)HeaderDataIter->first.size());
                    WinHeaderInfo.insert(WinHeaderInfo.end(),
                        HeaderDataIter->first.begin(), HeaderDataIter->first.end());
                    HeaderDataIter++;
                    dwIndex++;
                };
                dwIndex--;

                WinHeaderInfo.insert(WinHeaderInfo.begin() + 1, 1, ((PBYTE)& dwIndex)[0]);
                WinHeaderInfo.insert(WinHeaderInfo.begin() + 2, 1, ((PBYTE)& dwIndex)[1]);

                ShellCodeInfo.WinApiData.insert(ShellCodeInfo.WinApiData.end(),
                    WinHeaderInfo.begin(), WinHeaderInfo.end());
                dwCurrentOffset += (DWORD)WinHeaderInfo.size();
                HeaderFileInfo.insert(std::make_pair(HeaderSection, HeaderInfo));
            });
        WinApi.HeadersInfo[HeaderFile] = HeaderFileInfo;
    } while (FindNextFileA(
                 hFind,
                 &WinFindData
             ));

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        Utils::Reportf::ApiError("FindNextFileA", "Unable to continue loading headers information from %s", HeadersDir.c_str());
        return FALSE;
    };
    FindClose(hFind);

    Utils::Printf::Success("Loaded %d header files successfully", WinApi.HeadersInfo.size());

    if (!(hFind = FindFirstFileA(
                      ApisDefDir.c_str(),
                      &WinFindData
                  )) || hFind == INVALID_HANDLE_VALUE)
    {
        Utils::Reportf::ApiError("FindFirstFileA",
                                 "Cannot load the apis information from %s", ApisDir.c_str());
        return FALSE;
    };

    do
    {
        std::string ApiFile = ApisDir;
        ApiFile.append(WinFindData.cFileName);

        INIReader ApiReader(ApiFile.c_str());
        if (ApiReader.ParseError() < 0) {
            if (IsVerbous) Utils::Printf::Info("Error happend at parsing %s", ApiFile.c_str());
            continue;
        };

        std::string DllName = WinFindData.cFileName;
        std::for_each(DllName.begin(), DllName.end(), [](CHAR& ch) {ch = tolower(ch); });

        DllName.erase(DllName.find(".api"), sizeof(".api"));
        DllName.append(".dll");

        std::map<std::string, _SupportedApiInfo> DllInfo;
        std::set<std::string> ApiSections = ApiReader.Sections();
        std::set<std::string>::iterator CallingConventionIter
            = ApiSections.find("CallingConvention");
        if (CallingConventionIter == ApiSections.end())
        {
            if (IsVerbous) Utils::Printf::Info("Module %s has no calling convention",
                                                   DllName.c_str());
            continue;
        };

        std::string CallingConvention = ApiReader.Get("CallingConvention", "@", "");
        if (CallingConvention.compare("STDCALL") && CallingConvention.compare("CDECL"))
        {
            if (IsVerbous) Utils::Printf::Info("Module %s has wrong calling convention",
                                                   DllName.c_str());
            continue;
        };
        BOOL bClearStack = FALSE;
#if ! (defined(_M_X64) || defined(__amd64__))
        if (!CallingConvention.compare("STDCALL"))
            bClearStack = TRUE;
#endif

        std::map<std::string, SupportedHeaderInfo> ModuleCachedHeaders;
        auto GetHeaderInfo = [&](const std::string& szHeader,
                                 const std::string& szHeaderFile,
        LPSupportedHeaderInfo lpHeaderInfo) -> bool {
            std::map<std::string, SupportedHeaderInfo>::iterator ModuleCachedHeadersIter =
            ModuleCachedHeaders.find(szHeader);

            if (ModuleCachedHeadersIter == ModuleCachedHeaders.end())
            {
                std::map<std::string,
                std::map<std::string, SupportedHeaderInfo>>::iterator HeaderFilesInfoIter =
                        WinApi.HeadersInfo.find(szHeaderFile);
                if (HeaderFilesInfoIter == WinApi.HeadersInfo.end()) return FALSE;

                std::map<std::string, SupportedHeaderInfo>::iterator HeaderInfoIter =
                HeaderFilesInfoIter->second.find(szHeader);
                if (HeaderInfoIter == HeaderFilesInfoIter->second.end()) return FALSE;

                ModuleCachedHeadersIter = ModuleCachedHeaders.insert(ModuleCachedHeaders.end(),
                        std::make_pair(szHeader, HeaderInfoIter->second));
            };

            lpHeaderInfo->dwHeaderInfoOffset = ModuleCachedHeadersIter->second.dwHeaderInfoOffset;
            lpHeaderInfo->dwHeaderSize = ModuleCachedHeadersIter->second.dwHeaderSize;

            return TRUE;
        };

        std::for_each(ApiSections.begin(), ApiSections.end(),
            [&](std::string const& ApiSection) -> void {

                _SupportedApiInfo ApiInfo;
                ApiInfo.dwApiInfoOffset = 0;
                std::string szSourceModule = ApiReader.Get(ApiSection.c_str(), "SourceModule", "");
                std::for_each(szSourceModule.begin(), szSourceModule.end(), [](CHAR& ch) {ch = tolower(ch); });

                if (szSourceModule.compare("")) ApiInfo.SourceModule = szSourceModule;

                if (!ApiInfo.SourceModule.empty())
                {
                    DWORD dwPos = (DWORD)ApiInfo.SourceModule.find(".api");
                    if (dwPos == (DWORD)std::string::npos)
                    {
                        if (IsVerbous) Utils::Printf::Info("Bad api %s's source module %s",
                            ApiSection.c_str(), ApiInfo.SourceModule.c_str());
                        return;
                    };
                    ApiInfo.SourceModule.erase(dwPos, sizeof(".api"));
                    ApiInfo.SourceModule.append(".dll");
                    DllInfo.insert(std::make_pair(ApiSection, ApiInfo));
                    return;
                };

                std::vector<std::string> Headers;
                std::string szHeader = ApiReader.Get(ApiSection.c_str(), "Header", "");
                if (szHeader.compare(""))
                {
                    DWORD dwPos = 0;
                    std::string szHeaderFile;
                    if (szHeader.back() != ';') szHeader.append(";");
                    while ((dwPos = (DWORD)szHeader.find(";")) != (DWORD)std::string::npos) {
                        if (dwPos)
                        {
                            szHeaderFile = szHeader.substr(0, dwPos);
                            while (szHeaderFile.back() == ' ') szHeaderFile.pop_back();
                            while (szHeaderFile.front() == ' ') szHeaderFile.erase(0, 1);
                            std::for_each(szHeaderFile.begin(), szHeaderFile.end(), [](CHAR& ch) {ch = tolower(ch); });
                            if (szHeaderFile.find(".api") == std::string::npos)
                            {
                                if (IsVerbous) Utils::Printf::Info("Bad api %s's header file %s",
                                    ApiSection.c_str(), szHeaderFile.c_str());
                                return;
                            };
                            Headers.push_back(szHeaderFile);
                        };
                        szHeader.erase(0, (SIZE_T)dwPos + 1);
                    };
                };

                std::vector<BYTE> WinApiInfo;
                ApiInfo.dwApiInfoOffset = dwCurrentOffset;

                std::string UnDecoratedSymbol = ApiSection;
                if (UnDecoratedSymbol.front() == '?')
                {
                    CHAR Symbol[MAX_PATH];
                    if (!UnDecorateSymbolName(
                        UnDecoratedSymbol.c_str(),
                        Symbol,
                        sizeof(Symbol),
                        UNDNAME_COMPLETE
                    ))
                    {
                        if (IsVerbous) Utils::Printf::Info("Cannot undecorate the api %s",
                            UnDecoratedSymbol.c_str());
                        return;
                    };
                    UnDecoratedSymbol = Symbol;
                };

                WinApiInfo.push_back((BYTE)(UnDecoratedSymbol.length() + 2));
                WinApiInfo.insert(WinApiInfo.end(),
                    UnDecoratedSymbol.begin(), UnDecoratedSymbol.end());

                WinApiInfo.push_back((BYTE)' ');
                WinApiInfo.push_back((BYTE)'(');

                BOOL bUseOldStack = FALSE;
                INT dwParamCount = 0;
                dwParamCount = ApiReader.GetInteger(ApiSection.c_str(), "ParamCount", 0);
                if (dwParamCount < 0)
                {
                    bUseOldStack = TRUE;
                    dwParamCount = 0 - dwParamCount;
                };

                WinApiInfo.push_back((BYTE)dwParamCount);
                for (DWORD dwParamIndex = 1; dwParamIndex <= (DWORD)dwParamCount; dwParamIndex++)
                {
                    CHAR bIndex[10] = { 0 };
                    itoa(dwParamIndex, bIndex, 10);
                    std::string szParamIndex = bIndex;

                    std::string szParameter = ApiReader.Get(ApiSection.c_str(), szParamIndex.c_str(), "");
                    if (!szParameter.compare(""))
                    {
                        if (IsVerbous) Utils::Printf::Info("Bad api %s with empty parameter %s",
                            ApiSection.c_str(), szParamIndex.c_str());
                        return;
                    };

                    DWORD dwParamNameOffset = (DWORD)szParameter.find_last_of(" ");
                    if (!dwParamNameOffset)
                    {
                        if (IsVerbous) Utils::Printf::Info("Bad api %s with parameter %s without datatype",
                            ApiSection.c_str(), szParameter.c_str());
                        return;
                    };
                    dwParamNameOffset++;

                    std::string ParamName = szParameter.substr((SIZE_T)dwParamNameOffset);
                    std::string DataType = szParameter.substr(0, dwParamNameOffset - 1);
                    while (DataType.back() == ' ') DataType.pop_back();

                    BOOL IsHeader = FALSE;
                    if (DataType.front() == '[' && DataType.back() == ']')
                    {
                        DataType.pop_back();
                        DataType.erase(0, 1);
                        if (DataType.back() != '*' &&
                            !(DataType.front() == 'L' &&
                                DataType.at(1) == 'P')) IsHeader = TRUE;
                    };

                    DWORD dwTotaParamlSize = (DWORD)ParamName.length() + (DWORD)DataType.length() + 13;
                    WinApiInfo.push_back((BYTE)dwTotaParamlSize);

                    std::string szNewParam = "\n        ";
                    WinApiInfo.insert(WinApiInfo.end(),
                        szNewParam.begin(), szNewParam.end());

                    WinApiInfo.insert(WinApiInfo.end(),
                        DataType.begin(), DataType.end());
                    WinApiInfo.push_back((BYTE)' ');
                    WinApiInfo.insert(WinApiInfo.end(),
                        ParamName.begin(), ParamName.end());

                    WinApiInfo.push_back((BYTE)' ');
                    WinApiInfo.push_back((BYTE)'=');
                    WinApiInfo.push_back((BYTE)' ');

                    DWORD dwSize = 0;
                    if (IsHeader)
                    {
                        if (Headers.empty())
                        {
                            bUseOldStack = TRUE;
                            dwSize = GetTypeSize(DataType);
                            WinApiInfo.push_back((BYTE)dwSize);
                            WinApiInfo.push_back((BYTE)0);
                        }
                        else
                        {
                            SupportedHeaderInfo HeaderData = { 0 };
                            std::vector<std::string>::iterator HeadersIter =
                                Headers.begin();

                            while (HeadersIter != Headers.end())
                            {
                                if (GetHeaderInfo(DataType, *HeadersIter, &HeaderData))
                                {
                                    dwSize = HeaderData.dwHeaderSize;
                                    WinApiInfo.push_back((BYTE)dwSize);
                                    WinApiInfo.push_back((BYTE)1);
                                    WinApiInfo.push_back(((PBYTE)& HeaderData.dwHeaderInfoOffset)[0]);
                                    WinApiInfo.push_back(((PBYTE)& HeaderData.dwHeaderInfoOffset)[1]);
                                    WinApiInfo.push_back(((PBYTE)& HeaderData.dwHeaderInfoOffset)[2]);
                                    WinApiInfo.push_back(((PBYTE)& HeaderData.dwHeaderInfoOffset)[3]);
                                    break;
                                };
                                HeadersIter++;
                            };

                            if (HeadersIter == Headers.end())
                            {
                                bUseOldStack = TRUE;
                                dwSize = GetTypeSize(DataType);
                                WinApiInfo.push_back((BYTE)dwSize);
                                WinApiInfo.push_back((BYTE)0);
                            };
                        };
                    }
                    else
                    {
                        if (IsStrType(DataType))
                        {
                            dwSize = sizeof(LPVOID);
                            WinApiInfo.push_back((BYTE)dwSize);
                            WinApiInfo.push_back((BYTE)2);
                        }
                        else if (IsBool(DataType))
                        {
                            dwSize = 1;
                            WinApiInfo.push_back((BYTE)1);
                            WinApiInfo.push_back((BYTE)3);
                        }
                        else
                        {
                            dwSize = GetTypeSize(DataType);
                            WinApiInfo.push_back((BYTE)dwSize);
                            WinApiInfo.push_back((BYTE)0);
                        };
                    };
#if ! (defined(_M_X64) || defined(__amd64__))
                    if (dwSize == 8) bUseOldStack = TRUE;
#endif
                };

                WinApiInfo.push_back((BYTE)bUseOldStack);
                WinApiInfo.push_back((BYTE)bClearStack);
                ShellCodeInfo.WinApiData.insert(ShellCodeInfo.WinApiData.end(),
                    WinApiInfo.begin(), WinApiInfo.end());
                dwCurrentOffset += (DWORD)WinApiInfo.size();
                DllInfo.insert(std::make_pair(ApiSection, ApiInfo));
            });
        WinApi.KnownDlls[DllName] = DllInfo;

    } while (FindNextFileA(
                 hFind,
                 &WinFindData
             ));

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        Utils::Reportf::ApiError("FindNextFileA", "Unable to continue loading apis information from %s", ApisDir.c_str());
        return FALSE;
    };
    FindClose(hFind);

    if (!(hFind = FindFirstFileA(
                      HeadersDefDir.c_str(),
                      &WinFindData
                  )) || hFind == INVALID_HANDLE_VALUE)
    {
        Utils::Reportf::ApiError("FindFirstFileA", "Cannot load the headers information from %s", HeadersDir.c_str());
        return FALSE;
    };

    do
    {
        std::string HeaderFile = HeadersDir;
        HeaderFile.append("\\");
        HeaderFile.append(WinFindData.cFileName);

    } while (FindNextFileA(
                 hFind,
                 &WinFindData
             ));

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        Utils::Reportf::ApiError("FindNextFileA", "Unable to continue loading headers information from %s", HeadersDir.c_str());
        return FALSE;
    };
    FindClose(hFind);

    Utils::Printf::Success("Loaded %d api files successfully", WinApi.KnownDlls.size());
    Utils::Printf::Success("Total WinApi information buffer size is %d bytes", ShellCodeInfo.WinApiData.size());
    return TRUE;
};
