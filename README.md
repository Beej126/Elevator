# Elevator
Windows process launcher, supporting elevate up or down from current process elevation, show/hide and wait.

## Motivation
Initially looking to **seamlessly** launch DevEnv.exe to edit files from command line, no matter whether DevEnv or cmd is currently elevated.  Notably, DevEnv is conveniently single instance by default. There are 4 possible combinations of "from" & "to":
* low-low & high-high: when they're the same it's trivial to launch
* low to high: requires either login credentials or a UAC prompt. In my edit.cmd script I've chosen to pass credentials (from environment variables)
* high to low: see [Raymond Chen's](https://blogs.msdn.microsoft.com/oldnewthing/20131118-00/?p=2643) post for the problem-solution explanation.

## Attribution
Taking no credit here.
* started with [jpassing/elevate](https://github.com/jpassing/elevate)
* added in [Raymond Chen's](https://blogs.msdn.microsoft.com/oldnewthing/20131118-00/?p=2643) approach of dropping from elevated to unelevated via Explorer->ShellExecute
* cobbled together various stack-o's attributed in the main ElevateWin.cpp
  * notably the approach of building as a Windows GUI exe to avoid showing undesirable default console windows, yet attaching to an existing cmd.exe console if one is present.

Solution is compiling with Visual Studio 2017 v15.4.0 with Desktop C++ workload installed.

I should mention, I am not at all a C++ programmer, this code is likely to be riddled with copy-paste bugs.
