# Elevator
Windows process launcher supporting elevate up or down, hide, wait and it's **NOT a Console app** (built as Windows GUI exe) which  avoids creating unsightly default console window - yet will create console to show usage and debug output where requested

## Usage
```
Elevator [options] prog [args]
-?    - Shows this help
-v    - debug mode
-wait - Waits until prog terminates
-hide - Launches with hidden window
-elev - specify elevation change (from host process):
          name.exe - elevate same as existing name.exe
          low      - force low (going high to low precludes -wait option).
          high     - force high
-dir  - working directory
-c    - Launches via %COMSPEC% /s /c prog "args"
        (/s removes the outer quotes and persists everything else; see 'help cmd.exe')
prog  - The program to execute
args  - Optional command line arguments to prog
```

## Motivation
Initially looking to **seamlessly** launch DevEnv.exe to edit files from command line, no matter whether DevEnv or cmd is currently elevated.  Notably, DevEnv is conveniently single instance by default. There are basically 4 possible combinations of elevation going from the current cmd.exe process to the possibly running DevEnv.exe process:

cmd | DevEnv | transition required
--- | --- | ---
low | high | this is kinda the main typical route 
high | low | this required the shifting down trick, i.e. lauching by proxy thru another low process (e.g. explorer.exe)
low | low | really nothing to be done here
high | high | nor here
low or high | not running | launch with cmd.exe's current elevation

## Attribution
Taking no credit here.
* started with [jpassing/elevate](https://github.com/jpassing/elevate)
* added in [Raymond Chen's](https://blogs.msdn.microsoft.com/oldnewthing/20131118-00/?p=2643) approach of dropping from elevated to unelevated via Explorer->ShellExecute
* cobbled together various stack-o's attributed in the main Elevator.cpp
  * notably the approach of building as a Windows GUI exe to avoid showing undesirable default console windows, yet attaching to an existing cmd.exe console if one is present.

Solution is compiling on Visual Studio 2017 v15.4.0 with Desktop C++ workload installed.

I should mention, I am not at all a C++ programmer.  This code is likely to be riddled with copy-paste bugs and inefficiencies.

## Open with DevEnv.reg
```
Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\*\shell\Open with DevEnv]
"icon"="C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\Common7\\IDE\\devenv.exe"

[HKEY_CLASSES_ROOT\*\shell\Open with DevEnv\command]
@="c:\\bin\\Elevator.exe -elev devenv.exe \"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\Common7\\IDE\\devenv.exe\" /edit \\\"%1\\\""
```

## edit.cmd (put in your path)
```
@echo off
SETLOCAL

set editor="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\devenv.exe" /edit

if exist %1 goto :skipCreate
set doNew=y
set /p doNew=Create new file? [y] 
if "%doNew%"=="y" (echo.>%1) else goto :EOF
:skipCreate

Elevator -dir "%cd%" -elev %editor% \"%1\"
```

## another handy usage
associate .bat files with **hidden** launch
(use .cmd for normal operation)
```
assoc .bat=ElevatorHiddenBatch
ftype ElevatorHiddenBatch=c:\bin\elevator.exe -hide -c \"%1\"
```
