
// Elevator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

/*----------------------------------------------------------------------
* Purpose:
*		Execute a process on the command line with elevated rights on Vista
*
* Copyright:
*		Johannes Passing (johannes.passing@googlemail.com)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
#define BANNER L"Original work by (c) 2007 - Johannes Passing - http://int3.de/\nEnhancements cobbled on by Brent Anderson - https://github.com/Beej126/Elevator\n\n"

typedef struct _COMMAND_LINE_ARGS
{
  BOOL Debug;
  BOOL ShowHelp;
  BOOL Wait;
  BOOL StartComspec;
  BOOL Hide;
  PCWSTR Elev;
  PCWSTR Dir;
  PCWSTR ApplicationName;
  PCWSTR CommandLine;
} COMMAND_LINE_ARGS, *PCOMMAND_LINE_ARGS;

//from here: https://stackoverflow.com/a/865207
std::vector<DWORD> GetPidsByName(PCWSTR name)
{
  std::vector<DWORD> results;
  if (name == NULL) return results;

  // Create toolhelp snapshot.
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process;
  ZeroMemory(&process, sizeof(process));
  process.dwSize = sizeof(process);

  // Walkthrough all processes.
  if (Process32First(snapshot, &process))
  {
    do
    {
      // Compare process.szExeFile based on format of name, i.e., trim file path
      // trim .exe if necessary, etc.
      if (_wcsicmp(process.szExeFile, name) == 0)
      {
        results.push_back(process.th32ProcessID);
      }
    } while (Process32Next(snapshot, &process));
  }

  CloseHandle(snapshot);

  // Not found
  return results;
}

//from here:https://stackoverflow.com/a/8196291/813599
BOOL IsElevated(PCWSTR name = NULL) {
  BOOL fRet = FALSE;
  HANDLE hProc = NULL;

  if (name != NULL) {
    std::vector<DWORD> pids = GetPidsByName(name);
    if (pids.size() != 0)
    {
      for (DWORD& pid : pids) {
        hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        //if we find a pid we can open then let's use it
        if (hProc != NULL) break;
      }
      //otherwise if we can't open any & they do exist (pids.size != 0), then we can presume they're all elevated (and we're not)
      if (hProc == NULL) return true;
    }
  }

  HANDLE hToken = NULL;
  if (OpenProcessToken(hProc ? hProc : GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof(TOKEN_ELEVATION);
    if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
      fRet = Elevation.TokenIsElevated;
    }
  }

  if (hToken) CloseHandle(hToken);
  if (hProc) CloseHandle(hProc);

  return fRet;
}

BOOL hasConsole = false;
bool hasParentConsole = false;
LPTSTR progName;

_CRT_STDIO_INLINE int __CRTDECL fwprintf2(
  _Inout_                       FILE*          const _Stream,
  _In_z_ _Printf_format_string_ wchar_t const* const _Format,
  ...
) {

  if (!hasConsole) {
    //https://stackoverflow.com/questions/15952892/using-the-console-in-a-gui-app-in-windows-only-if-its-run-from-a-console
    //first try grabbing parent console if we're running in cmd.exe
    hasConsole = AttachConsole(ATTACH_PARENT_PROCESS);
    if (hasConsole) hasParentConsole = true;
    else {
      //otherwise create our own if we're running from a Windows GUI (e.g. Explorer Context Menu)
      hasConsole = AllocConsole();
      if (!hasConsole) _ASSERTE(hasConsole); //blow up if we don't get either of those
    }

    FILE *str_in = 0, *str_out = 0, *str_err = 0;
    //freopen_s(&str_in, "CONIN$", "r", stdin);
    freopen_s(&str_out, "CONOUT$", "w", stdout);
    freopen_s(&str_err, "CONOUT$", "w", stderr);
  }

  //https://stackoverflow.com/questions/150543/forward-an-invocation-of-a-variadic-function-in-c
  va_list args;
  va_start(args, _Format);
  vfwprintf(_Stream, _Format, args);
  va_end(args);

  fwprintf(stdout,
    BANNER
    L"Execute a process on the command line with following options\n"
    L"\n"
    L"Usage: %s [options] prog [args]\n"
    L"-?    - Shows this help\n"
    L"-v    - debug mode\n"
    L"-wait - Waits until prog terminates\n"
    L"-hide - Launches with hidden window\n"
    L"-elev - specify elevation change (from host process):\n"
    L"          name.exe - elevate same as existing name.exe\n"
    L"          low      - force low (going high to low precludes -wait option).\n"
    L"          high     - force high \n"
    L"-dir  - working directory\n"
    L"-c    - Launches via %%COMSPEC%% /s /c prog \"args\"\n"
    L"        (/s removes the outer quotes and persists everything else; see 'help cmd.exe')\n"
    L"prog  - The program to execute\n"
    L"args  - Optional command line arguments to prog\n\n", 
    progName);

  system("pause");

  return 0;
}




//following approach is from here (Raymond Chen): https://blogs.msdn.microsoft.com/oldnewthing/20131118-00/?p=2643
//goes to some effort to use explorer to "ShellExecute" so new process runs under logged in user's context
//this avoids needing to pass in login creds or otherwise prompt for them
//which is the case with using SysInternals psexec launching an un-elevated process for example
void FindDesktopFolderView(REFIID riid, void **ppv)
{
  CComPtr<IShellWindows> spShellWindows;
  spShellWindows.CoCreateInstance(CLSID_ShellWindows);

  CComVariant vtLoc(CSIDL_DESKTOP);
  CComVariant vtEmpty;
  long lhwnd;
  CComPtr<IDispatch> spdisp;
  spShellWindows->FindWindowSW(&vtLoc, &vtEmpty, SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp);

  CComPtr<IShellBrowser> spBrowser;
  CComQIPtr<IServiceProvider>(spdisp)->
    QueryService(SID_STopLevelBrowser,
      IID_PPV_ARGS(&spBrowser));

  CComPtr<IShellView> spView;
  spBrowser->QueryActiveShellView(&spView);

  spView->QueryInterface(riid, ppv);
}

void GetDesktopAutomationObject(REFIID riid, void **ppv)
{
  CComPtr<IShellView> spsv;
  FindDesktopFolderView(IID_PPV_ARGS(&spsv));
  CComPtr<IDispatch> spdispView;
  spsv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&spdispView));
  spdispView->QueryInterface(riid, ppv);
}

class CCoInitialize {
public:
  CCoInitialize() : m_hr(CoInitialize(NULL)) { }
  ~CCoInitialize() { if (SUCCEEDED(m_hr)) CoUninitialize(); }
  operator HRESULT() const { return m_hr; }
  HRESULT m_hr;
};

VOID ShellExecuteFromExplorer(
  PCWSTR pszFile,
  PCWSTR pszParameters = nullptr,
  PCWSTR pszDirectory = nullptr,
  PCWSTR pszOperation = nullptr,
  int nShowCmd = SW_SHOWNORMAL)
{
  CCoInitialize init;

  CComPtr<IShellFolderViewDual> spFolderView;
  GetDesktopAutomationObject(IID_PPV_ARGS(&spFolderView));
  CComPtr<IDispatch> spdispShell;
  spFolderView->get_Application(&spdispShell);

  CComQIPtr<IShellDispatch2>(spdispShell)
    ->ShellExecute(CComBSTR(pszFile),
      CComVariant(pszParameters ? pszParameters : L""),
      CComVariant(pszDirectory ? pszDirectory : L""),
      CComVariant(pszOperation ? pszOperation : L""),
      CComVariant(nShowCmd));
}
/*****************************************************************/


INT Launch(
  __in PCWSTR ApplicationName,
  __in PCWSTR CommandLine,
  __in BOOL Wait,
  __in BOOL Hide,
  __in BOOL isFromElevated,
  __in BOOL isToElevated,
  __in PCWSTR Dir
)
{
  SHELLEXECUTEINFO Shex;
  ZeroMemory(&Shex, sizeof(SHELLEXECUTEINFO));
  Shex.cbSize = sizeof(SHELLEXECUTEINFO);
  Shex.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
  Shex.lpVerb = !isFromElevated && isToElevated ? L"runas" : NULL;
  Shex.lpDirectory = Dir;
  Shex.lpFile = ApplicationName;
  Shex.lpParameters = CommandLine;
  Shex.nShow = Hide ? SW_HIDE : SW_SHOWNORMAL;

  //if we're currently elevated, trying to drop to unelevated, must use the explorer trick
  if (isFromElevated && !isToElevated) {
    //Explorer ShellEx doesn't return a process handle to wait on
    //nor a resultCode to inspect
    ShellExecuteFromExplorer(Shex.lpFile, Shex.lpParameters, Shex.lpDirectory, nullptr/*operation: e.g. "open", "edit", etc*/, Shex.nShow);
  }
  //otherwise, low-low, high-high & low-high can use full fidelity ShellEx
  else {
    if (!ShellExecuteEx(&Shex)) {
      DWORD Err = GetLastError();
      fwprintf2(stderr, L"%s could not be launched: %d\n", ApplicationName, Err);
      return EXIT_FAILURE;
    }
    _ASSERTE(Shex.hProcess);
    if (Wait) WaitForSingleObject(Shex.hProcess, INFINITE);
    CloseHandle(Shex.hProcess);
  }

  return EXIT_SUCCESS;
}

INT DispatchCommand(__in PCOMMAND_LINE_ARGS Args)
{
  WCHAR AppNameBuffer[MAX_PATH];
  WCHAR CmdLineBuffer[MAX_PATH * 2];

  if (Args->StartComspec)
  {
    //
    // Resolve COMSPEC
    //
    if (0 == GetEnvironmentVariable(L"COMSPEC", AppNameBuffer, _countof(AppNameBuffer)))
    {
      fwprintf2(stderr, L"%%COMSPEC%% is not defined\n");
      return EXIT_FAILURE;
    }
    Args->ApplicationName = AppNameBuffer;

    //
    // Prepend /c and quote arguments
    //
    if (FAILED(StringCchPrintf(
      CmdLineBuffer,
      _countof(CmdLineBuffer),
      L"/s /c \"%s\"",
      Args->CommandLine)))
    {
      fwprintf2(stderr, L"Creating command line failed\n");
      return EXIT_FAILURE;
    }
    Args->CommandLine = CmdLineBuffer;
  }

  BOOL isFromElevated = IsElevated();
  BOOL isToElevated = !(Args->Elev) ? false :
    (StrStrI(Args->Elev, L".exe") != NULL) ? IsElevated(Args->Elev) //if exe passed, check if it's elevated
    : (_wcsicmp(Args->Elev, L"high") == 0 ? true //otherwise if high passed
      : false); //lastly low

  if (Args->Debug) {
    return fwprintf2(stdout,
      L"=======Debug=======\n"
      L"Wait:            %s\n"
      L"Hide:            %s\n"
      L"Elev:            %s\n"
      L"FromElev:        %s\n"
      L"ToElev:          %s\n"
      L"Dir:             %s\n"
      L"StartComspec:    %s\n"
      L"ApplicationName: %s\n"
      L"CommandLine:     %s\n\n",
      Args->Wait ? L"Y" : L"",
      Args->Hide ? L"Y" : L"",
      Args->Elev,
      isFromElevated ? L"Y" : L"",
      isToElevated ? L"Y" : L"",
      Args->Dir,
      Args->StartComspec ? L"Y" : L"",
      Args->ApplicationName,
      Args->CommandLine);
  }

  return Launch(Args->ApplicationName, Args->CommandLine, Args->Wait, Args->Hide, isFromElevated, isToElevated, Args->Dir);
}

int __cdecl /*wmain*/theMain(
  __in int Argc,
  __in WCHAR* Argv[]
)
{
  OSVERSIONINFO OsVer;
  COMMAND_LINE_ARGS Args;
  INT Index;
  BOOL FlagsRead = FALSE;
  WCHAR CommandLineBuffer[260] = { 0 };

  ZeroMemory(&OsVer, sizeof(OSVERSIONINFO));
  OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  ZeroMemory(&Args, sizeof(COMMAND_LINE_ARGS));
  Args.CommandLine = CommandLineBuffer;

  //
  // Check OS version
  //
/*
  if (GetVersionEx(&OsVer) &&
    OsVer.dwMajorVersion < 6)
  {
    fwprintf2(stderr, L"This tool is for Windows Vista and above only.\n");
    return EXIT_FAILURE;
  }
*/

//
// Parse command line
//
  for (Index = 1; Index < Argc; Index++)
  {
    if (!FlagsRead &&
      (Argv[Index][0] == L'-' || Argv[Index][0] == L'/'))
    {
      PCWSTR FlagName = &Argv[Index][1];
      if (0 == _wcsicmp(FlagName, L"v"))
      {
        Args.Debug = TRUE;
      }
      else if (0 == _wcsicmp(FlagName, L"?"))
      {
        Args.ShowHelp = TRUE;
      }
      else if (0 == _wcsicmp(FlagName, L"wait"))
      {
        Args.Wait = TRUE;
      }
      else if (0 == _wcsicmp(FlagName, L"c"))
      {
        Args.StartComspec = TRUE;
      }
      else if (0 == _wcsicmp(FlagName, L"hide"))
      {
        Args.Hide = TRUE;
      }
      else if (0 == _wcsicmp(FlagName, L"elev"))
      {
        Args.Elev = Argv[++Index];
      }
      else if (0 == _wcsicmp(FlagName, L"dir"))
      {
        /* cool, quoted args are were automatically resolved
        //keeping string conversion code for future reference
        #include <string>
        std::wstring s = Argv[++Index];
        if (s.front() == '"') {
        s.erase(0, 1); // erase the first character
        s.erase(s.size() - 1); // erase the last character
        }
        Args.Dir = s.c_str();
        */
        Args.Dir = Argv[++Index];
      }
      else
      {
        fwprintf2(stderr, L"Unrecognized Flag %s\n", FlagName);
        return EXIT_FAILURE;
      }
    }
    else
    {
      FlagsRead = TRUE;
      if (Args.ApplicationName == NULL && !Args.StartComspec)
      {
        Args.ApplicationName = Argv[Index];
      }
      else
      {
        if (FAILED(StringCchCat(
          CommandLineBuffer,
          _countof(CommandLineBuffer),
          Argv[Index])) ||
          FAILED(StringCchCat(
            CommandLineBuffer,
            _countof(CommandLineBuffer),
            L" ")))
        {
          fwprintf2(stderr, L"Command Line too long\n");
          return EXIT_FAILURE;
        }
      }
    }
  }

  //
  // Validate args
  //
  if (Argc <= 1)
  {
    Args.ShowHelp = TRUE;
  }

  if (//!Args.ShowHelp &&
    ((Args.StartComspec && 0 == wcslen(Args.CommandLine)) ||
    (!Args.StartComspec && Args.ApplicationName == NULL)))
  {
    fwprintf2(stderr, L"Must specify progname or -c\n");
    return EXIT_FAILURE;
  }

  return DispatchCommand(&Args);
}


/*****************************************************************************/
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR    lpCmdLine,
  _In_ int       nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  //FILE *str_in = 0, *str_out = 0, *str_err = 0;

  BOOL isDebug = StrStrI(lpCmdLine, L"-v") != NULL;

  //https://msdn.microsoft.com/en-us/library/windows/desktop/bb776391(v=vs.85).aspx
  LPWSTR *szArglist;
  int nArgs;

  szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
  progName = StrDup(szArglist[0]);
  PathStripPath(progName);
  PathRemoveExtension(progName);
  theMain(nArgs, szArglist);
  LocalFree(szArglist);
  //if (str_out) fclose(str_out);
  //if (str_err) fclose(str_err);

  FreeConsole();

  return 1;
}