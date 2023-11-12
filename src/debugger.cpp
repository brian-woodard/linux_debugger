
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <errno.h>
#include <vector>
#include <chrono>
#include <thread>
#include <assert.h>

#include "DebugTypes.h"

#include "PrintData.cpp"
#include "DebugBackend.cpp"

TDebugCommand GetCommand()
{
   char               input[MAX_COMMAND] = {};
   std::vector<char*> strings;
   TDebugCommand      result = {};

   printf("\ndbg> ");
   fgets(input, MAX_COMMAND-1, stdin);

   // trim newline
   int length = strlen(input);
   if (input[length-1] == '\n')
   {
      input[length-1] = 0;
      length--;
   }

   // split strings
   bool push_string = false;
   strings.push_back(input);
   for (int i = 0; i < length; i++)
   {
      if (input[i] == ' ')
      {
         push_string = true;
         input[i] = 0;
      }
      else if (push_string)
      {
         strings.push_back(&input[i]);
         push_string = false;
      }
   }

   if (strcmp(strings[0], "c") == 0 || strcmp(strings[0], "cont") == 0 || strcmp(strings[0], "continue") == 0)
   {
      result.Command = DEBUG_CMD_CONTINUE;
      return result;
   }
   else if (strcmp(strings[0], "q") == 0 || strcmp(strings[0], "quit") == 0)
   {
      result.Command = DEBUG_CMD_QUIT;
      return result;
   }
   else if (strcmp(strings[0], "s") == 0 || strcmp(strings[0], "step") == 0)
   {
      result.Command = DEBUG_CMD_STEP_SINGLE;
      return result;
   }
   else if (strcmp(strings[0], "b") == 0 || strcmp(strings[0], "break") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: break [address]\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      result.Command = DEBUG_CMD_SET_BREAKPOINT;
      result.Data.BpAddr.Address = strtoll(strings[1], 0, 16);
      return result;
   }
   else if (strcmp(strings[0], "delete") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: delete [breakpoint]\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      // TODO: Add error handling from backend
      // if (index-1 >= breakpoints.size())
      // {
      //    printf("Invalid cmd: unknown breakpoint %d\n", index);
      //    return DEBUG_CMD_UNKNOWN;
      // }

      result.Command = DEBUG_CMD_DELETE_BREAKPOINT;
      result.Data.BpIdx.Index = strtoll(strings[1], 0, 10) - 1;
      return result;
   }
   else if (strcmp(strings[0], "enable") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: enable [breakpoint]\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      result.Command = DEBUG_CMD_ENABLE_BREAKPOINT;
      result.Data.BpIdx.Index = strtoll(strings[1], 0, 10) - 1;
      return result;
   }
   else if (strcmp(strings[0], "disable") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: disable [breakpoint]\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      result.Command = DEBUG_CMD_DISABLE_BREAKPOINT;
      result.Data.BpIdx.Index = strtoll(strings[1], 0, 10) - 1;
      return result;
   }
   else if (strcmp(strings[0], "list") == 0)
   {
      if (strings.size() != 1)
      {
         printf("Invalid cmd: list\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      result.Command = DEBUG_CMD_LIST_BREAKPOINTS;
      return result;
   }
   else if (strcmp(strings[0], "register") == 0)
   {
      if (strings.size() < 2)
      {
         printf("Invalid cmd:\n");
         printf("  register read [register_name]\n");
         printf("  register write [register_name] [value]\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      if (strcmp(strings[1], "read") == 0)
      {
         eRegister register_name = REGISTER_COUNT;

         if (strings.size() != 3)
         {
            printf("Invalid cmd: register read [register_name]\n");
            result.Command = DEBUG_CMD_UNKNOWN;
            return result;
         }

         for (int i = 0; i < REGISTER_COUNT; i++)
         {
            if (strcmp(strings[2], RegisterStr[i]) == 0)
            {
               register_name = (eRegister)i;
            }
         }

         if (register_name == REGISTER_COUNT)
         {
            printf("Unknown register\n");
            result.Command = DEBUG_CMD_UNKNOWN;
            return result;
         }

         result.Command = DEBUG_CMD_REGISTER_READ;
         result.Data.Reg.Index = register_name;
         return result;
      }
      else if (strcmp(strings[1], "write") == 0)
      {
         u64       value = strtoll(strings[3], 0, 16);
         eRegister register_name = REGISTER_COUNT;

         if (strings.size() != 4)
         {
            printf("Invalid cmd: register write [register_name] [value]\n");
            result.Command = DEBUG_CMD_UNKNOWN;
            return result;
         }

         for (int i = 0; i < REGISTER_COUNT; i++)
         {
            if (strcmp(strings[2], RegisterStr[i]) == 0)
            {
               register_name = (eRegister)i;
            }
         }

         if (register_name == REGISTER_COUNT)
         {
            printf("Unknown register\n");
            result.Command = DEBUG_CMD_UNKNOWN;
            return result;
         }

         result.Command = DEBUG_CMD_REGISTER_WRITE;
         result.Data.Reg.Index = register_name;
         result.Data.Reg.Value = value;
         return result;
      }

      result.Command = DEBUG_CMD_UNKNOWN;
      return result;
   }

   printf("Unknown command\n");
   result.Command = DEBUG_CMD_UNKNOWN;
   return result;
};

void RunConsole(CDebugBackend& Debugger)
{
   while (Debugger.IsRunning())
   {
      TDebugCommand cmd = GetCommand();

      if (cmd.Command != DEBUG_CMD_UNKNOWN)
      {
         Debugger.SetCommand(cmd);

         // wait for command to be processed by backend
         while ((cmd.Command = Debugger.GetCommand()) != DEBUG_CMD_PROCESSED)
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
         }
      }
   }
}

int main(int argc, char** argv)
{
   CDebugBackend debugger;

   if (argc < 2)
   {
      fprintf(stderr, "Expected a program name as argument\n");
      return -1;
   }

   debugger.Run(argv[1]);

   RunConsole(debugger);

   return 0;
}
