# xLogger
Log windows API calls with parameters, calling module, thread id, return code, time, last status and last error based on a simple hooking engine

# Introduction 
*Malware nature is so wild and chaotic*, you cannot know for sure how a malicious software will behave, so a method like API logging is a very useful pre-analysis information source as you can know what is going at different analysis stages.

# Traditional ApiLogging
The real difference between different tools is the hooking engine used.</br>
Commonly there's two types of API hooking:
* IAT hooking
  * Used at the great API logger ApiMonitor
  * Depends on changing the API addresses, assigned by the loader, at the IAT with the addresses of the trampoline functions
  * It will fail at so many cases:
    * Manual API resolving</br>
      When the malicious sample manually resolves all/some of its APIs, these API addresses won't be stored at the IAT so it cannot be hooked
    * Dynamic API resolving with GetProcAddress</br>
    * Manipulated IAT</br>
      I've already proved that API calls can be obfuscated by misusing the IAT at [CallObfuscator](https://github.com/d35ha/CallObfuscator)
  * Hard to be detected by the malware</br>
  
* Inline hooking (detouring)
  * The most powerful hooking technique
  * Depends on replacing the first bytes from the API prologue with an unconditional jump to the trampoline function
  * Can be detected easily by checking the first instruction
  * Perfect when used against specific API(s), but using it with a wide range of APIs (logging) will be very exhaustive

# Export address table hooking 
Whatever the way used at the dynamic/manual API resolving, the resolver has to loop on the export address table (EAT) to get the API address, so placing our hooks on the EAT will deceive the resolver by getting the address of the trampoline instead of the address of the API, this can make a really good ApiLogger that can handle the reallife advanced malware samples</br>
Of course EAT hooking comes with problems, there are some of them and how I dealt with it:
* Modules' IATs are already built
  * If the IAT is already built by the loader or the used APIs are already resolved, placing the hooks on the export table will be useless
  * To overcome this, I had to place the hooks at a very early stage, at the process initialization before the loader builds any of the loaded modules' IATs
  
* EAT has RVAs instead of absolute addresses
  * On the other side of IAT, EAT has the RVAs of the APIs taken from the dll base, so the space between the trampoline and the dll shouldn't be more than 4GB of memory which makes a real problem at x64
  * To overcome this, I had to place the trampoline chain for every dll at the nearest available place above the dll, the tool will find the convenient place to do so
  
* Requires DLL injection
  * Actually any API hooking technique will require dll injection which is less stealther
  * xLogger core was fully written as a shellcode
  
# How to use

```asm
Usage: xLogger <Options> [-c [CommandLine]|-attach [Pid]] -l [LogFile]
Options:
        --external-mods <list of modules>: Log calls from these modules with the main module
        --exclude-apis <list of dll:api>: Never log calls made to these apis
        --exclude-mods <list of modules>: Never log calls made to these modules
        --exclude-all-apis-except <list of dll:api>: Only log calls made to these apis
        --exclude-all-mods-except <list of modules>: Only log calls made to these modules
        --detach-on-exit: Keep the process when the logger exits (the process will continue logging)
        --detach-on-system: Detach on reaching system/attach breakpoint (the process will continue logging)
        --hide-debugger: Hide the debugger from being detected by IsDebuggerPresent,PEB BeingDebugged,..
        --external-console: Run the process at external console (only with -c switch)
        --apis-dir <directory path>: directory of api definition files (default: WinApi)
        --headers-dir <directory path>: directory of headers definition files (default: WinApi\headers)
        -v: verbous mode
```
* `-c`
  * Used with a command line for the target sample
  * The sample should have a full path (not from PATH environment variable)
  * Example `-c "evil.exe param"`
  * Example `-c "\"c:\malware folder\evil.exe\" \"param one\""`

* `-attach`
  * Used with the pid of any process to attach
  * The tool will use both IAT hooking and EAT hooking when using this option
  * Example `-attach 1337`
  
* `-l`
  * Followed by the path to the file to save the output to
  * If you wanted a real-time logging at the console directly, use powershell utility `Get-Content -wait`
  * Example `-l log.txt`
  
* `--external-mods`
  * By default the tool will log any calls made from the main module or any other place at the address space excluding the loaded modules
  * Use this option to include any of the loaded modules to the logging scope
  * This is very good when the sample is loading any external dll (ex: python27.dll)
  * Notice that this also can be used to include the system dlls (ex: kerenl32.dll) to the scope
  * Example `--external-mods kernelbase.dll,msvcrt.dll,evil.dll`
 
* `--exclude-apis`
  * By default the tool will log the calls made to all of the pre-defined apis
  * Using this with one or more apis will stop the tool from putting any hooks at them
  * Very good when the sample is making too many calls to some api you are not interested in
  * Notice that the apis are case sensitive
  * Example `--exclude-apis kernel32.dll:GetLastError,kernelbase.dll:GetLastError`
  
* `--exclude-mods`
  * Same as `--exclude-apis`, but instead of apis, the used modules will not be hooked
  * Notice that the modules are case insensitive
  * Example `--exclude-mods msvcrt.dll`
  
* `--exclude-all-apis-except`
  * Only place hooks on these APIs
  * Very good if you are interested in only one or some of APIs
  * Example `--exclude-all-apis-except kernel32.dll:CreateProcessInternalW,ntdll.dll:NtCreateProcesss`
  
* `--exclude-all-mods-except`
  * Only place hooks on these modules
  * Very good if you are interested in only one or some of modules
  * Example `--exclude-all-mods-except kernel32.dll`
  
* `--detach-on-exit`
  * If the logger is terminated, do not terminate the process
  * Because of using a shellcode, what is actually logging the calls is the process itself, so terminating xLogger will not stop the process from logging any subsequent calls with the same constrains defined by the used options
  
* `--detach-on-system`
  * This will make xLogger terminate itelf once it reaches the system breakpoint (with `-c`) or the attach breakpoint (with `-attach`)
  * Good for stealther logging
  * The logging will never stop, even after xLogger is terminated
  * The only drawback of terminating xLogger is that no subsequent loaded dlls will be hooked, only calls made to the already-loaded dlls will be logged
  
* `--hide-debugger`
  * xLogger itself is a small debugger, so basically it can be detected as any other debugger
  * Using this option, xLogger will use some of the anti-detection techniques to hide itself
  * Currently, the implemented techniques are PEB `NtGlobalFlags` and `BeingDebugged`, a lot to be added ...

* `--external-console`
  * By default xLogger console handle will be passed to the process, so the process stdout/stderr/stdin will be the same as the tool
  * In case of using this, the process will have a different console
  
* `--apis-dir`
  * Followed by the path of the folder holding the definitions of the windows API
  * The default is `WinApi`
  * The defined apis format is very simple 
```ini
[CreateFile]
1=LPCTSTR lpFileName
2=[FILE_ACCESS_MASK] dwDesiredAccess
3=[FILE_SHARE_MODE] dwShareMode
4=LPSECURITY_ATTRIBUTES lpSecurityAttributes
5=[CreationDisposition] dwCreationDisposition
6=[FlagsAndAttributes] dwFlagsAndAttributes
7=HANDLE hTemplateFile
ParamCount=7
Header=kernel32.h.api;native.h.api;security.h.api;windows.h.api; 
```
* Also
  * Brackets indicates that the datatype is a flag or enum and is defined at one of the header files below
  * If the API is forwarded to another dll, no hook will be placed at it but on the API to which it's forwarded.
  * If the API is forwarded to another dll with different name, the API definition has to be added to the new dll APIs file with the new name
  * Forwarded APIs can be handled from within the predefined APIs files when the forwarded API has the same name as the new one (ex: ApiSetSchema forwarders), for example `kernel32!AddDllDirectory` is forwarded to `kernelbase!AddDllDirectory` so the definition of `AddDllDirectory` will be put at `kernel32.api` and `kernelbase.api` will have
```ini
[AddDllDirectory]
SourceModule=kernel32.api
```
  
* `--headers-dir`
  * Followed by the path of the folder holding the definitions of the headers, flags/enums/(structs soon).
  * The default is `WinApi\\Headers`
  
* `-v`
  * Verbous mode
  
* Future options
  * `--auto-attach` to automatically attach the logger to any process created by the current process, I've the theory, just need time
  * `--log-all` to log calls to all of the exported functions even if it's not defined at its dll definition file

# RealLife Samples

<details>
  <summary>
  Sample 1</br>
  </summary></br>

* [mal_v1.exe](https://github.com/d35ha/xLogger/raw/master/samples/mal_v1.exe), CTF, Obfuscated, Manual API resolving, debugger check</br> 

```c++
[mal_v1.exe] [0x1484] LoadLibrary (
        LPCTSTR lpFileName = "NTDLL"
) -> 0x777d0000 [0x25 µs]
[mal_v1.exe] [0x1484] LoadLibrary (
        LPCTSTR lpFileName = "shlwapi"
) -> 0x756e0000 [0x47 µs]
[mal_v1.exe] [0x1484] LoadLibrary (
        LPCTSTR lpFileName = "shell32"
) -> 0x75ea0000 [0x2f µs]
[mal_v1.exe] [0x1484] GetEnvironmentVariable (
        LPCTSTR lpName = "localappdata",
        LPTSTR lpBuffer = L"",
        DWORD nSize = 0x104
) -> 0x24 [0xa µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "C:\\Users\\Administrator\\AppData\\Local",
        LPSTR lpString2 = "\\Microsoft\\config.vbe"
) -> 0x40b110 [0x4 µs]
[mal_v1.exe] [0x1484] GetEnvironmentVariable (
        LPCTSTR lpName = "localappdata",
        LPTSTR lpBuffer = L"",
        DWORD nSize = 0x104
) -> 0x24 [0x3 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "C:\\Users\\Administrator\\AppData\\Local",
        LPSTR lpString2 = "\\Microsoft\\tmscv.exe"
) -> 0x40b000 [0x0 µs]
[mal_v1.exe] [0x1484] GetModuleFileName (
        HMODULE hModule = 0x0,
        LPTSTR lpFilename = L"",
        DWORD nSize = 0x104
) -> 0x29 [0x3 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "C:\\Users\\Administrator\\Desktop\\mal_v1.exe"
) -> 0x0 [0x15 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "mal_v1.exe"
) -> 0xa [0x0 µs]
[mal_v1.exe] [0x1484] CreateToolhelp32Snapshot (
        CreateToolhelp32SnapshotFlags dwFlags = TH32CS_SNAPPROCESS,
        DWORD th32ProcessID = 0x0
) -> 0x78 [0x131 µs]
[mal_v1.exe] [0x1484] Process32First (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3f µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "[System Process]"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "[System Process]"
) -> 0x10 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "[system process]"
) -> 0x1 [0x5 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "System"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "System"
) -> 0x6 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "system"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3f µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "smss.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "smss.exe"
) -> 0x8 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "smss.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "csrss.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "csrss.exe"
) -> 0x9 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "csrss.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "csrss.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "csrss.exe"
) -> 0x9 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "csrss.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x50 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "wininit.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "wininit.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "wininit.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "winlogon.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "winlogon.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "winlogon.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "services.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "services.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "services.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "lsass.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "lsass.exe"
) -> 0x9 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "lsass.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x45 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "lsm.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "lsm.exe"
) -> 0x7 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "lsm.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x77 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3a µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "vmacthlp.exe"
) -> 0x149f488 [0x2 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "vmacthlp.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "vmacthlp.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x49 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x3 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x2 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x2 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2f µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x4a µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x47 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0xe µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3e µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x39 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "spoolsv.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "spoolsv.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "spoolsv.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x46 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x46 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2d µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "IpOverUsbSvc.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "IpOverUsbSvc.exe"
) -> 0x10 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "ipoverusbsvc.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "cygrunsrv.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "cygrunsrv.exe"
) -> 0xd [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "cygrunsrv.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x31 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "sqlwriter.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "sqlwriter.exe"
) -> 0xd [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "sqlwriter.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "conhost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "conhost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "conhost.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2a µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "VGAuthService.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "VGAuthService.exe"
) -> 0x11 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "vgauthservice.exe"
) -> 0xffffffff [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x45 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "sshd.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "sshd.exe"
) -> 0x8 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "sshd.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "vmtoolsd.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "vmtoolsd.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "vmtoolsd.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "ManagementAgentHost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "ManagementAgentHost.exe"
) -> 0x17 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "managementagenthost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "BuildService.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "BuildService.exe"
) -> 0x10 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "buildservice.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3d µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "CoordService.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "CoordService.exe"
) -> 0x10 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "coordservice.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x53 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "sppsvc.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "sppsvc.exe"
) -> 0xa [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "sppsvc.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x36 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x2 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2d µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "WmiPrvSE.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "WmiPrvSE.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "wmiprvse.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "msdtc.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "msdtc.exe"
) -> 0x9 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "msdtc.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x39 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x48 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "GoogleCrashHandler.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "GoogleCrashHandler.exe"
) -> 0x16 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "googlecrashhandler.exe"
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2d µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "GoogleCrashHandler64.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "GoogleCrashHandler64.exe"
) -> 0x18 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "googlecrashhandler64.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "taskhost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "taskhost.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "taskhost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "dwm.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "dwm.exe"
) -> 0x7 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "dwm.exe"
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x39 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "explorer.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "explorer.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "explorer.exe"
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2d µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "vmtoolsd.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "vmtoolsd.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "vmtoolsd.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "Everything.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "Everything.exe"
) -> 0xe [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "everything.exe"
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x47 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "jusched.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "jusched.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "jusched.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x36 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "xgTrayIcon.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "xgTrayIcon.exe"
) -> 0xe [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "xgtrayicon.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2e µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "jucheck.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "jucheck.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "jucheck.exe"
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "TrustedInstaller.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "TrustedInstaller.exe"
) -> 0x14 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "trustedinstaller.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x32 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "SearchIndexer.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "SearchIndexer.exe"
) -> 0x11 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "searchindexer.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2d µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "taskhost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "taskhost.exe"
) -> 0xc [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "taskhost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "CompatTelRunner.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "CompatTelRunner.exe"
) -> 0x13 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "compattelrunner.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "conhost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "conhost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "conhost.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "OSPPSVC.EXE"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "OSPPSVC.EXE"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "osppsvc.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "CompatTelRunner.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "CompatTelRunner.exe"
) -> 0x13 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "compattelrunner.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x46 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "cmd.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "cmd.exe"
) -> 0x7 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "cmd.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2b µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "conhost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "conhost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "conhost.exe"
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x32 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "VSSVC.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "VSSVC.exe"
) -> 0x9 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "vssvc.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x48 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "svchost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "svchost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "svchost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2f µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "SearchProtocolHost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "SearchProtocolHost.exe"
) -> 0x16 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "searchprotocolhost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x30 µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "dllhost.exe"
) -> 0x149f488 [0x2 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "dllhost.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "dllhost.exe"
) -> 0x1 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2e µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "SearchFilterHost.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "SearchFilterHost.exe"
) -> 0x14 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "searchfilterhost.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x3a µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "xLogger.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "xLogger.exe"
) -> 0xb [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "xlogger.exe"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x1 [0x2c µs]
[mal_v1.exe] [0x1484] PathStripPath (
        LPTSTR pszPath = "mal_v1.exe"
) -> 0x149f488 [0x1 µs]
[mal_v1.exe] [0x1484] lstrlen (
        LPCSTR lpString = "mal_v1.exe"
) -> 0xa [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "mal_v1.exe",
        const char* string2 = "mal_v1.exe"
) -> 0x0 [0x0 µs]
[mal_v1.exe] [0x1484] Process32Next (
        HANDLE hSnapshot = 0x78,
        LPPROCESSENTRY32 lppe = 0x149f464
) -> 0x0 [0x3f µs]
[mal_v1.exe] [0x1484] CloseHandle (
        HANDLE hObject = 0x78
) -> 0x1 [0x13 µs]
[mal_v1.exe] [0x1484] GetModuleFileName (
        HMODULE hModule = 0x0,
        LPTSTR lpFilename = "",
        DWORD nSize = 0x104
) -> 0x29 [0x6 µs]
[mal_v1.exe] [0x1484] GetModuleFileName (
        HMODULE hModule = 0x0,
        LPTSTR lpFilename = L"",
        DWORD nSize = 0x104
) -> 0x29 [0x2 µs]
[mal_v1.exe] [0x1484] GetFullPathName (
        LPCTSTR lpFileName = "C:\\Users\\Administrator\\Desktop\\mal_v1.exe",
        DWORD nBufferLength = 0x104,
        LPTSTR lpBuffer = "C:\\Users\\Administrator\\Desktop\\mal_v1.exe",
        LPTSTR* lpFilePart = 0x149f5a8
) -> 0x29 [0x1a µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = L"",
        LPSTR lpString2 = "\"RI\" "
) -> 0x40d230 [0x1 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "\"RI\" ",
        LPSTR lpString2 = "\"-path\" \"\\\""
) -> 0x40d230 [0x0 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "\"RI\" \"-path\" \"\\\"",
        LPSTR lpString2 = "C:\\Users\\Administrator\\Desktop\\mal_v1.exe"
) -> 0x40d230 [0x1 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "\"RI\" \"-path\" \"\\\"C:\\Users\\Administrator\\Desktop\\mal_v1.exe",
        LPSTR lpString2 = "\\\"\" \"-force\""
) -> 0x40d230 [0x1 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = L"",
        LPSTR lpString2 = "TYPELIB"
) -> 0x40f240 [0x1 µs]
[mal_v1.exe] [0x1484] GetModuleHandle (
        LPCTSTR lpModuleName = NULL
) -> 0x400000 [0x1 µs]
[mal_v1.exe] [0x1484] WideCharToMultiByte (
        CodePageEnum CodePage = CP_ACP,
        WideCharFlags dwFlags = 0x0,
        LPCWSTR lpWideCharStr = L"TYPELIB",
        int cchWideChar = 0x7,
        LPSTR lpMultiByteStr = L"",
        int cbMultiByte = 0x8,
        LPCSTR lpDefaultChar = NULL,
        LPBOOL lpUsedDefaultChar = 0x0
) -> 0x7 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "TYPELIB",
        const char* string2 = "TYPELIB"
) -> 0x0 [0x0 µs]
[mal_v1.exe] [0x1484] WideCharToMultiByte (
        CodePageEnum CodePage = CP_ACP,
        WideCharFlags dwFlags = 0x0,
        LPCWSTR lpWideCharStr = L"A",
        int cchWideChar = 0x1,
        LPSTR lpMultiByteStr = L"",
        int cbMultiByte = 0x2,
        LPCSTR lpDefaultChar = NULL,
        LPBOOL lpUsedDefaultChar = 0x0
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = L"A",
        const char* string2 = L"A"
) -> 0x0 [0x0 µs]
[mal_v1.exe] [0x1484] CreateFile (
        LPCTSTR lpFileName = "C:\\Users\\Administrator\\AppData\\Local\\Microsoft\\tmscv.exe",
        FILE_ACCESS_MASK dwDesiredAccess = GENERIC_WRITE,
        FILE_SHARE_MODE dwShareMode = 0x0,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = 0x0,
        CreationDisposition dwCreationDisposition = CREATE_ALWAYS,
        FlagsAndAttributes dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
        HANDLE hTemplateFile = 0x0
) -> 0x78 [0x96 µs]
[mal_v1.exe] [0x1484] WriteFile (
        HANDLE hFile = 0x78,
        LPCVOID lpBuffer = 0x414254,
        DWORD nNumberOfBytesToWrite = 0x29400,
        LPDWORD lpNumberOfBytesWritten = 0x149f57c,
        LPOVERLAPPED lpOverlapped = 0x0
) -> 0x1 [0x10e µs]
[mal_v1.exe] [0x1484] CloseHandle (
        HANDLE hObject = 0x78
) -> 0x1 [0x50 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "",
        LPSTR lpString2 = "TYPESCR"
) -> 0x40f240 [0x0 µs]
[mal_v1.exe] [0x1484] GetModuleHandle (
        LPCTSTR lpModuleName = NULL
) -> 0x400000 [0x1 µs]
[mal_v1.exe] [0x1484] WideCharToMultiByte (
        CodePageEnum CodePage = CP_ACP,
        WideCharFlags dwFlags = 0x0,
        LPCWSTR lpWideCharStr = L"TYPELIB",
        int cchWideChar = 0x7,
        LPSTR lpMultiByteStr = "TYPELIB",
        int cbMultiByte = 0x8,
        LPCSTR lpDefaultChar = NULL,
        LPBOOL lpUsedDefaultChar = 0x0
) -> 0x7 [0x0 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "TYPELIB",
        const char* string2 = "TYPESCR"
) -> 0xffffffff [0x1 µs]
[mal_v1.exe] [0x1484] WideCharToMultiByte (
        CodePageEnum CodePage = CP_ACP,
        WideCharFlags dwFlags = 0x0,
        LPCWSTR lpWideCharStr = L"TYPESCR",
        int cchWideChar = 0x7,
        LPSTR lpMultiByteStr = "TYPELIB",
        int cbMultiByte = 0x8,
        LPCSTR lpDefaultChar = NULL,
        LPBOOL lpUsedDefaultChar = 0x0
) -> 0x7 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = "TYPESCR",
        const char* string2 = "TYPESCR"
) -> 0x0 [0x0 µs]
[mal_v1.exe] [0x1484] WideCharToMultiByte (
        CodePageEnum CodePage = CP_ACP,
        WideCharFlags dwFlags = 0x0,
        LPCWSTR lpWideCharStr = L"B",
        int cchWideChar = 0x1,
        LPSTR lpMultiByteStr = L"A",
        int cbMultiByte = 0x2,
        LPCSTR lpDefaultChar = NULL,
        LPBOOL lpUsedDefaultChar = 0x0
) -> 0x1 [0x1 µs]
[mal_v1.exe] [0x1484] strcmp (
        const char* string1 = L"B",
        const char* string2 = L"B"
) -> 0x0 [0x0 µs]
[mal_v1.exe] [0x1484] CreateFile (
        LPCTSTR lpFileName = "C:\\Users\\Administrator\\AppData\\Local\\Microsoft\\config.vbe",
        FILE_ACCESS_MASK dwDesiredAccess = GENERIC_WRITE,
        FILE_SHARE_MODE dwShareMode = 0x0,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = 0x0,
        CreationDisposition dwCreationDisposition = CREATE_ALWAYS,
        FlagsAndAttributes dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
        HANDLE hTemplateFile = 0x0
) -> 0x78 [0x4a µs]
[mal_v1.exe] [0x1484] WriteFile (
        HANDLE hFile = 0x78,
        LPCVOID lpBuffer = 0x43d654,
        DWORD nNumberOfBytesToWrite = 0xf8,
        LPDWORD lpNumberOfBytesWritten = 0x149f57c,
        LPOVERLAPPED lpOverlapped = 0x0
) -> 0x1 [0x14 µs]
[mal_v1.exe] [0x1484] CloseHandle (
        HANDLE hObject = 0x78
) -> 0x1 [0x49 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = L"",
        LPSTR lpString2 = "\"RI\" \"-path\" \"\\\"C:\\Users\\Administrator\\Desktop\\mal_v1.exe\\\"\" \"-force\""
) -> 0x410300 [0x1 µs]
[mal_v1.exe] [0x1484] lstrcat (
        LPSTR lpString1 = "\"RI\" \"-path\" \"\\\"C:\\Users\\Administrator\\Desktop\\mal_v1.exe\\\"\" \"-force\"",
        LPSTR lpString2 = L""
) -> 0x410300 [0x0 µs]
[mal_v1.exe] [0x1484] ShellExecute (
        HWND hwnd = 0x0,
        LPCTSTR lpOperation = "OPEN",
        LPCTSTR lpFile = "Powershell",
        LPCTSTR lpParameters = "\"RI\" \"-path\" \"\\\"C:\\Users\\Administrator\\Desktop\\mal_v1.exe\\\"\" \"-force\"",
        LPCTSTR lpDirectory = L"",
        ShowWindowCmd nShowCmd = SW_HIDE
) -> 0x2a [0x66 µs]
[mal_v1.exe] [0x1484] ExitProcess (
        UINT uExitCode = 0x104
) 
```
</details>

# Additional
* Last error/status logging is not currently supported, but it will be supported ASAP
* The tool is not undetectable, there is nothing undetectable in this field
* The tool is critically stable as there's a lot of reasons it may crash or not work properly
* A lot of other samples will be added soon
* Download source with `git clone --recurive https://github.com/d35ha/xLogger`
* Download binaries from the [release section](https://github.com/d35ha/xLogger/releases)
