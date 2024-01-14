
#include "DebugUtils.cpp"
#include "DebugBackend.cpp"
#include "gui.cpp"

int main(int argc, char* argv[])
{

   if (gui_init("Debugger"))
   {
      gui_run();
   }

   gui_shutdown();

   return 0;
}
