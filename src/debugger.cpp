
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

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

const u8  SW_INTERRUPT_3 = 0xcc;
const s32 MAX_COMMAND = 1024;

enum eDbgCommand
{
   DEBUG_CMD_UNKNOWN,
   DEBUG_CMD_CONTINUE,
   DEBUG_CMD_SET_BREAKPOINT,
   DEBUG_CMD_STEP_OVER,
   DEBUG_CMD_STEP_INTO,
   DEBUG_CMD_STEP_SINGLE,
   DEBUG_CMD_QUIT,
   DEBUG_CMD_COUNT
};

struct TDebugState
{
   pid_t ChildPid;
   u64   InstructionsExecuted;
   s32   WaitStatus;
   s32   WaitOptions;
};

struct TBreakpoint
{
   u64  Address;
   bool Enabled;
   u8   SavedData;
};

TDebugState              debug_state;
std::vector<TBreakpoint> breakpoints;

void AddBreakpoint(TDebugState State, u64 Address)
{
   TBreakpoint bp = {};

   errno = 0;
   u64 data = ptrace(PTRACE_PEEKDATA, State.ChildPid, Address, nullptr);

   if (errno == 0)
   {
      bp.Address = Address;
      bp.Enabled = true;
      bp.SavedData = data; // save lsb

      u64 data_w_int = ((data & ~0xff) | SW_INTERRUPT_3);
      ptrace(PTRACE_POKEDATA, State.ChildPid, Address, data_w_int);

      breakpoints.push_back(bp);
   }
   else
   {
      fprintf(stderr, "Error peek data %s (%d)\n", strerror(errno), errno);
   }
}

void DeleteBreakpoint()
{

}

eDbgCommand GetCommand()
{
   char input[MAX_COMMAND] = {};
   std::vector<char*> strings;

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
      return DEBUG_CMD_CONTINUE;
   }
   else if (strcmp(strings[0], "q") == 0 || strcmp(strings[0], "quit") == 0)
   {
      return DEBUG_CMD_QUIT;
   }
   else if (strcmp(strings[0], "b") == 0 || strcmp(strings[0], "break") == 0)
   {
      u64 addr = strtoll(strings[1], 0, 16);
      AddBreakpoint(debug_state, addr);
      return DEBUG_CMD_SET_BREAKPOINT;
   }

   return DEBUG_CMD_UNKNOWN;
};

void Continue()
{
   ptrace(PTRACE_CONT, debug_state.ChildPid, nullptr, nullptr);
   waitpid(debug_state.ChildPid, &debug_state.WaitStatus, debug_state.WaitOptions);
}

void RunTarget(const char* program)
{
   printf("Run target %s\n", program);

   if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
   {
      perror("ptrace");
      return;
   }

   execl(program, program, nullptr);
}

void RunDebugger()
{
   printf("Run debugger on pid %d\n", debug_state.ChildPid);

   // Wait for child to stop on its first instruction
   debug_state.WaitOptions = 0;
   waitpid(debug_state.ChildPid, &debug_state.WaitStatus, debug_state.WaitOptions);

   // while (WIFSTOPPED(wait_status))
   // {
   //    struct user_regs_struct regs;

   //    icounter++;

   //    ptrace(PTRACE_GETREGS, pid, 0, &regs);
   //    uint32_t instr = ptrace(PTRACE_PEEKTEXT, pid, regs.rip, 0);

   //    printf("icounter = %u.  RIP = 0x%08x.  instr = 0x%08x\n",
   //           icounter, regs.rip, instr);

   //    // Make the child execute another instruction
   //    if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0)
   //    {
   //       perror("ptrace");
   //       return;
   //    }

   //    // Wait for child to stop on its next instruction
   //    wait(&wait_status);
   // }

   while (WIFSTOPPED(debug_state.WaitStatus))
   {
      eDbgCommand cmd = GetCommand();

      if (cmd == DEBUG_CMD_QUIT)
      {
         // we're done, stop the child process
         kill(debug_state.ChildPid, SIGTERM);
         break;
      }
      else if (cmd == DEBUG_CMD_CONTINUE)
      {
         Continue();
      }
      else
      {
         continue;
      }
   }

   printf("The child executed %u instructions\n", debug_state.InstructionsExecuted);
}

int main(int argc, char** argv)
{
   if (argc < 2)
   {
      fprintf(stderr, "Expected a program name as argument\n");
      return -1;
   }

   // reserve some initial max amount of breakpoints
   breakpoints.reserve(64);

   debug_state.ChildPid = fork();
   if (debug_state.ChildPid == 0)
   {
      personality(ADDR_NO_RANDOMIZE);
      RunTarget(argv[1]);
   }
   else if (debug_state.ChildPid > 0)
   {
      debug_state.InstructionsExecuted = 0;

      RunDebugger();
   }
   else
   {
      perror("fork");
      return -1;
   }

   return 0;
}
