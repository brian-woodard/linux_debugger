
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string>
#include <thread>

// GLFWwindow *window = nullptr;
// CDebugBackend debug_backend;

struct TGuiState
{
   GLFWwindow*   Window;
   std::string   Target;
   CDebugBackend Backend;
};

TGuiState gui_state;

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
static void ShowMenuFile()
{
   if (ImGui::MenuItem("Set Target", nullptr, false, !gui_state.Backend.IsRunning()))
   {
      printf(">>> get target name\n");
   }
   ImGui::Separator();
   if (ImGui::MenuItem("Quit", "Alt+F4"))
   {
      glfwSetWindowShouldClose(gui_state.Window, GLFW_TRUE);
   }
}

// Demonstrate creating a "main" fullscreen menu bar and populating it.
// Note the difference between BeginMainMenuBar() and BeginMenuBar():
// - BeginMenuBar() = menu-bar inside current window (which needs the ImGuiWindowFlags_MenuBar flag!)
// - BeginMainMenuBar() = helper to create menu-bar-sized window at the top of the main viewport + call BeginMenuBar() into it.
static void ShowAppMainMenuBar()
{
   if (ImGui::BeginMainMenuBar())
   {
      if (ImGui::BeginMenu("File"))
      {
         ShowMenuFile();
         ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
   }
}

static void glfw_error_callback(int error, const char *description)
{
   fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int gui_init(const char *Title)
{
   gui_state.Window = nullptr;
   gui_state.Target = "";

   glfwSetErrorCallback(glfw_error_callback);
   if (!glfwInit())
      return 0;

   // GL 3.0 + GLSL 130
   const char *glsl_version = "#version 130";
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

   // Create window with graphics context
   gui_state.Window = glfwCreateWindow(1280, 720, Title, NULL, NULL);
   if (gui_state.Window == NULL)
      return 0;

   glfwMakeContextCurrent(gui_state.Window);
   glfwSwapInterval(1); // Enable vsync

   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO &io = ImGui::GetIO();
   (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
   // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
   io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
   io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
   // io.ConfigViewportsNoAutoMerge = true;
   // io.ConfigViewportsNoTaskBarIcon = true;

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   // ImGui::StyleColorsLight();

   // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
   ImGuiStyle &style = ImGui::GetStyle();
   if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
   {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
   }

   // Setup Platform/Renderer backends
   ImGui_ImplGlfw_InitForOpenGL(gui_state.Window, true);
   ImGui_ImplOpenGL3_Init(glsl_version);

   return 1;
}


void RunDebugger(CDebugBackend& Debugger)
{
   TDebugCommand cmd;
   pid_t         debug_pid = 0;

   // // wait for debug backend to startup by waiting for a cmd processed
   // while ((cmd.Command = Debugger.GetCommand()) != DEBUG_CMD_PROCESSED)
   // {
   //    std::this_thread::sleep_for(std::chrono::milliseconds(10));
   // }

   // while (Debugger.IsRunning())
   // {
   //    cmd = GetCommand(Input);

   //    if (cmd.Command == DEBUG_CMD_INTERRUPT && cmd.Data.Integer.Value == -1)
   //    {
   //       //ptrace(PTRACE_INTERRUPT, debug_pid, 0, 0);
   //       kill(debug_pid, SIGINT);
   //    }
   //    else if (cmd.Command != DEBUG_CMD_UNKNOWN)
   //    {
   //       Debugger.SetCommand(cmd);

   //       // wait for command to be processed by backend
   //       std::this_thread::sleep_for(std::chrono::milliseconds(10));

   //       // process any data output from backend
   //       u8* data;
   //       while ((data = Debugger.PopData()))
   //       {
   //          TBufferHeader* header = (TBufferHeader*)data;
   //          switch (header->DataType)
   //          {
   //             case DATA_TYPE_STREAM_ERROR:
   //             {
   //                char* str = (char*)&data[sizeof(TBufferHeader)];
   //                printf("ERROR: %.*s\r\n", header->Size, str);
   //                break;
   //             }
   //             case DATA_TYPE_STREAM_WARNING:
   //             {
   //                char* str = (char*)&data[sizeof(TBufferHeader)];
   //                printf("WARNING: %.*s\r\n", header->Size, str);
   //                break;
   //             }
   //             case DATA_TYPE_STREAM_DEBUG:
   //             {
   //                char* str = (char*)&data[sizeof(TBufferHeader)];
   //                printf("DEBUG: %.*s\r\n", header->Size, str);
   //                break;
   //             }
   //             case DATA_TYPE_STREAM_INFO:
   //             {
   //                char* str = (char*)&data[sizeof(TBufferHeader)];
   //                printf("\r%.*s\r\n", header->Size, str);
   //                break;
   //             }
   //             case DATA_TYPE_STREAM_TARGET_OUTPUT:
   //             {
   //                char* str = (char*)&data[sizeof(TBufferHeader)];
   //                printf("\rOUTPUT: %.*s\r\n", header->Size, str);
   //                break;
   //             }
   //             case DATA_TYPE_REGISTERS:
   //             {
   //                TRegister* registers = (TRegister*)&data[sizeof(TBufferHeader)];
   //                printf("Register values:\r\n");
   //                for (int i = 0; i < REGISTER_COUNT; i++)
   //                   printf("  %s: %.*s 0x%08x\r\n", RegisterStr[i], 8 - strlen(RegisterStr[i]), "          ", registers->RegArray[i]);
   //                break;
   //             }
   //             case DATA_TYPE_DATA:
   //             {
   //                data += sizeof(TBufferHeader);

   //                u64 address = *(u64*)data;
   //                data += sizeof(u64);

   //                printf("Data read at address 0x%x bytes %d:\r\n", address, header->Size-sizeof(u64));
   //                printf("%s\r\n", CPrintData::GetDataAsString((char*)data, header->Size-sizeof(u64), "\r\n", address));
   //                break;
   //             }
   //             case DATA_TYPE_PID:
   //             {
   //                data += sizeof(TBufferHeader);
   //                debug_pid = *(pid_t*)data;
   //                break;
   //             }
   //             default:
   //                break;
   //          }
   //       }
   //    }
   // }
}

void gui_run()
{
   // Our state
   ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

   while (!glfwWindowShouldClose(gui_state.Window))
   {
      // Poll and handle events (inputs, window resize, etc.)
      // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
      // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
      // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
      // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
      glfwPollEvents();

      RunDebugger(gui_state.Backend);

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ShowAppMainMenuBar();

      // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
      {
         static float f = 0.0f;
         static int counter = 0;
         static bool show_demo = false;

         ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

         ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)

         ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
         ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

         if (ImGui::Button("Demo"))
            show_demo = !show_demo;

         if (show_demo)
            ImGui::ShowDemoWindow();

         if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
         ImGui::SameLine();
         ImGui::Text("counter = %d", counter);

         ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
         ImGui::End();
      }

      // Rendering
      ImGui::Render();
      int display_w, display_h;
      glfwGetFramebufferSize(gui_state.Window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      // Update and Render additional Platform Windows
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(gui_state.Window);

      glfwSwapBuffers(gui_state.Window);
   }
}

void gui_shutdown()
{
   gui_state.Backend.Quit();

   // Cleanup
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwDestroyWindow(gui_state.Window);
   glfwTerminate();
}
