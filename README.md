# Linux Debugger

A simple Linux debugger, mainly for me to learn more about ELF / DWARF on Linux. Who knows if this will ever be a usable application.

TODO:
  * Fix attach command with a bad pid, debugger hangs
  * Add help command, list all commands, should be able to type "help [command]" to get more in depth usage
  * Add tab completion for commands
  * Add command to show debug output on console, default to off?
  * Implement DEBUG_CMD_DATA_WRITE to write data?
  * Breakout command parsing into a DebugFrontend class
  * Change Frontend command packaging, right now only one command is allowed "per frame". Have it perform more like the backend
    stream output. Allocate a chunk of memory up front, and that will allow multiple commands to be pushed to the command queue.
    The backend will then retire them one-by-one. May not ever need multiple commands per frame, but this will get rid of the
    ugly "new" and "delete" needed for strings (like the "target [exec]" command)
  * Initial GUI
    * Add console window
    * Add register window to display register values
    * Add output window (output stream from debugged program)
    * Add data window to display a block of data (current view, some data around RIP?)
    * Add initial disassembly of data into cpu mnemonics
