# Linux Debugger

A simple Linux debugger, mainly for me to learn more about ELF / DWARF on Linux. Who knows if this will ever be a usable application.

TODO:
  * Add signal handler to backend to handle signals from debugged program
  * Don't do fprintf/printf in backend, add a way to stream output data to frontend
  * Add mTargetRunning variable and "run" command to backend