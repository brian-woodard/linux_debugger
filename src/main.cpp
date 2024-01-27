
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "imgui_impl_glfw.cpp"

#undef GL_VERSION_1_1

#include "imgui_impl_opengl3.cpp"

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
