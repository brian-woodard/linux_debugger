# Linux Debugger

A simple Linux debugger, mainly for me to learn more about ELF / DWARF on Linux. Who knows if this will ever be a usable application.

TODO:
  * Add attach command line to attach to running process
    * command line: ./debugger attach [pid]
    * command while running: attach (list of pids matching target name)
    * command while running: be able to change targets
  * Input handler
    * Fix issue where we get spurious characters when read(stdin)... not sure what's wrong
    * If enter pressed with no text entered, run last command in history
    * Handle left/right arrow key for editing console commands
      * Change mLine in CInputHandler to char[MAX_LINE] instead of C++ string
  * Verify target exists, and is executable (use ELF parsing)
  * Initial GUI
    * Add console window
    * Add register window to display register values
    * Add data window to display a block of data (current view, some data around RIP?)
    * Add initial disassembly of data into cpu mnemonics