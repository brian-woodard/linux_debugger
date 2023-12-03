# Linux Debugger

A simple Linux debugger, mainly for me to learn more about ELF / DWARF on Linux. Who knows if this will ever be a usable application.

TODO:
  * Add attach command line to attach to running process
    * command line: ./debugger attach [pid]
    * command while running: attach (list of pids matching target name)
    * command while running: be able to change targets
  * Input handler
    * DEL key should delete any characters to the right of the cursor
    * Verify input handler correctly handles MAX_LINE (test with MAX_LINE=5)
  * Verify target exists, and is executable (use ELF parsing)
  * Initial GUI
    * Add console window
    * Add register window to display register values
    * Add data window to display a block of data (current view, some data around RIP?)
    * Add initial disassembly of data into cpu mnemonics