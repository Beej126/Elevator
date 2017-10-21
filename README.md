# Elevator
Windows process launcher, supporting elevate up or down from current, show/hide and wait

Taking no credit here.
* started with [jpassing/elevate](https://github.com/jpassing/elevate)
* added in [Raymond Chen's](https://blogs.msdn.microsoft.com/oldnewthing/20131118-00/?p=2643) approach of dropping from elevated to unelevated via Explorer->ShellExecute
* cobbled together various stack-o's attributed in the main ElevateWin.cpp
  * notably the approach of building as a Windows GUI exe to avoid showing undesirable default console windows, yet attaching to an existing cmd.exe console if one is present.

Solution is compiling with Visual Studio 2017 v15.4.0 with C++ workload installed.
