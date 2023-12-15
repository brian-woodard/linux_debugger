
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
#include "InputHandler.cpp"
#include "DebugUtils.cpp"

TDebugCommand GetCommand(CInputHandler& Input)
{
   char               input[CInputHandler::MAX_LINE] = {};
   std::vector<char*> strings;
   TDebugCommand      result = {};

   Input.GetInput(input);

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
   else if (strcmp(strings[0], "r") == 0 || strcmp(strings[0], "run") == 0)
   {
      result.Command = DEBUG_CMD_RUN;
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
         printf("Invalid cmd: break [address]\r\n");
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
         printf("Invalid cmd: delete [breakpoint]\r\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      result.Command = DEBUG_CMD_DELETE_BREAKPOINT;
      result.Data.BpIdx.Index = strtoll(strings[1], 0, 10) - 1;
      return result;
   }
   else if (strcmp(strings[0], "enable") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: enable [breakpoint]\r\n");
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
         printf("Invalid cmd: disable [breakpoint]\r\n");
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
         printf("Invalid cmd: list\r\n");
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
         printf("Invalid cmd:\r\n");
         printf("  register read [register_name]\r\n");
         printf("  register write [register_name] [value]\r\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      if (strcmp(strings[1], "read") == 0)
      {
         eRegister register_name = REGISTER_COUNT;

         if (strings.size() != 3)
         {
            result.Command = DEBUG_CMD_REGISTER_READ_ALL;
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
            printf("Unknown register\r\n");
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
            printf("Invalid cmd: register write [register_name] [value]\r\n");
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
            printf("Unknown register\r\n");
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
   else if (strcmp(strings[0], "data") == 0)
   {
      if (strings.size() < 4)
      {
         printf("Invalid cmd:\r\n");
         printf("  data read [address] [bytes]\r\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      if (strcmp(strings[1], "read") == 0)
      {
         result.Command = DEBUG_CMD_DATA_READ;
         result.Data.Read.Address = strtoll(strings[2], 0, 16);
         result.Data.Read.Bytes = strtoll(strings[3], 0, 10);

         if (result.Data.Read.Bytes > 64)
         {
            printf("Max bytes that can be read is 64\r\n");
            result.Command = DEBUG_CMD_UNKNOWN;
            return result;
         }

         return result;
      }
   }
   else if (strcmp(strings[0], "target") == 0)
   {
      if (strings.size() > 2)
      {
         printf("Invalid cmd:\r\n");
         printf("  target [executable]\r\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }

      if (strings.size() == 1)
      {
         result.Command = DEBUG_CMD_GET_TARGET;
      }
      else
      {
         result.Command = DEBUG_CMD_SET_TARGET;
         result.Data.String.Size = strlen(strings[1]) + 1;
         result.Data.String.String = new u8[result.Data.String.Size];
         memcpy(result.Data.String.String, strings[1], result.Data.String.Size);
      }

      return result;
   }
   else if (strcmp(strings[0], "start") == 0)
   {
      if (strings.size() > 1)
      {
         printf("Invalid cmd:\r\n");
         printf("  start\r\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }
      
      result.Command = DEBUG_CMD_START;
      return result;
   }
   else if (strcmp(strings[0], "stop") == 0)
   {
      if (strings.size() > 1)
      {
         printf("Invalid cmd:\r\n");
         printf("  stop\r\n");
         result.Command = DEBUG_CMD_UNKNOWN;
         return result;
      }
      
      result.Command = DEBUG_CMD_STOP;
      return result;
   }

   printf("Unknown command\r\n");
   result.Command = DEBUG_CMD_UNKNOWN;
   return result;
};

void RunConsole(CDebugBackend& Debugger, CInputHandler& Input)
{
   TDebugCommand cmd;

   // wait for debug backend to startup by waiting for a cmd processed
   while ((cmd.Command = Debugger.GetCommand()) != DEBUG_CMD_PROCESSED)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }

   while (Debugger.IsRunning())
   {
      cmd = GetCommand(Input);

      if (cmd.Command != DEBUG_CMD_UNKNOWN)
      {
         Debugger.SetCommand(cmd);

         // wait for command to be processed by backend
         while ((cmd.Command = Debugger.GetCommand()) != DEBUG_CMD_PROCESSED)
         {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
         }

         // process any data output from backend
         u8* data;
         while ((data = Debugger.PopData()))
         {
            TBufferHeader* header = (TBufferHeader*)data;
            switch (header->DataType)
            {
               case DATA_TYPE_STREAM_ERROR:
               {
                  char* str = (char*)&data[sizeof(TBufferHeader)];
                  printf("ERROR: %.*s\r\n", header->Size, str);
                  break;
               }
               case DATA_TYPE_STREAM_WARNING:
               {
                  char* str = (char*)&data[sizeof(TBufferHeader)];
                  printf("WARNING: %.*s\r\n", header->Size, str);
                  break;
               }
               case DATA_TYPE_STREAM_DEBUG:
               {
                  char* str = (char*)&data[sizeof(TBufferHeader)];
                  printf("DEBUG: %.*s\r\n", header->Size, str);
                  break;
               }
               case DATA_TYPE_STREAM_INFO:
               {
                  char* str = (char*)&data[sizeof(TBufferHeader)];
                  printf("\r%.*s\r\n", header->Size, str);
                  break;
               }
               case DATA_TYPE_STREAM_TARGET_OUTPUT:
               {
                  char* str = (char*)&data[sizeof(TBufferHeader)];
                  printf("\rOUTPUT: %.*s\r\n", header->Size, str);
                  break;
               }
               case DATA_TYPE_REGISTERS:
               {
                  TRegister* registers = (TRegister*)&data[sizeof(TBufferHeader)];
                  printf("Register values:\r\n");
                  for (int i = 0; i < REGISTER_COUNT; i++)
                     printf("  %s: %.*s 0x%08x\r\n", RegisterStr[i], 8 - strlen(RegisterStr[i]), "          ", registers->RegArray[i]);
               }
               case DATA_TYPE_DATA:
               {
                  data += sizeof(TBufferHeader);

                  u64 address = *(u64*)data;
                  data += sizeof(u64);

                  printf("Data read at address 0x%x bytes %d:\r\n", address, header->Size-sizeof(u64));
                  printf("%s\r\n", CPrintData::GetDataAsString((char*)data, header->Size-sizeof(u64), "\r\n", address));
               }
               default:
                  break;
            }
         }
      }
   }
}

int main(int argc, char** argv)
{
   CDebugBackend debugger;
   CInputHandler input("dbg> ", stdout);

   if (argc < 2)
   {
      fprintf(stderr, "Expected a program name as argument\r\n");
      return -1;
   }

   debugger.Run(argv[1]);

   RunConsole(debugger, input);

   return 0;
}
