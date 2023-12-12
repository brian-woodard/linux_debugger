# Linux Debugger

A simple Linux debugger, mainly for me to learn more about ELF / DWARF on Linux. Who knows if this will ever be a usable application.

TODO:
  * Add attach command line to attach to running process
    * command line: ./debugger attach [pid]
    * command while running: attach (list of pids matching target name)
    * command while running: be able to change targets
  * Add help command, list all commands, should be able to type "help [command]" to get more in depth usage
  * Add tab completion for commands
  * Get output from debugged program into a separate debug output stream
  * Handle a debugee that loops forever, when target is changed, need to stop the current process first
    or if a re-run is commanded, make sure to stop first debugee
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
