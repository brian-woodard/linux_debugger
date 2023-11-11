
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
#include <assert.h>

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

#define PTRACE(Request, Pid, Addr, Data) ({ \
   long retval = 0; \
   errno = 0; \
   retval = ptrace(Request, Pid, Addr, Data); \
   if (errno) \
   { \
      fprintf(stderr, "Error: ptrace error %s (%d) at %s:%d %s\n", strerror(errno), errno, __FILE__, __LINE__, __func__); \
   } \
   retval; \
})

const u8  SW_INTERRUPT_3 = 0xcc;
const s32 MAX_COMMAND    = 1024;

enum eDebugCommand
{
   DEBUG_CMD_UNKNOWN,
   DEBUG_CMD_CONTINUE,
   DEBUG_CMD_SET_BREAKPOINT,
   DEBUG_CMD_DELETE_BREAKPOINT,
   DEBUG_CMD_LIST_BREAKPOINTS,
   DEBUG_CMD_STEP_OVER,
   DEBUG_CMD_STEP_INTO,
   DEBUG_CMD_STEP_SINGLE,
   DEBUG_CMD_QUIT,
   DEBUG_CMD_COUNT
};

// match x86_64 registers in /usr/include/sys/user.h
enum eRegister
{
   REGISTER_R15,
   REGISTER_R14,
   REGISTER_R13,
   REGISTER_R12,
   REGISTER_RBP,
   REGISTER_RBX,
   REGISTER_R11,
   REGISTER_R10,
   REGISTER_R9,
   REGISTER_R8,
   REGISTER_RAX,
   REGSITER_RCX,
   REGISTER_RDX,
   REGISTER_RSI,
   REGISTER_RDI,
   REGISTER_ORIG_RAX,
   REGISTER_RIP,
   REGISTER_CS,
   REGISTER_EFLAGS,
   REGISTER_RSP,
   REGISTER_SS,
   REGISTER_FS_BASE,
   REGISTER_GS_BASE,
   REGISTER_DS,
   REGISTER_ES,
   REGISTER_FS,
   REGISTER_GS,
   REGISTER_COUNT
};

const char* RegisterStr[] =
{
   "r15",
   "r14",
   "r13",
   "r12",
   "rbp",
   "rbx",
   "r11",
   "r10",
   "r9",
   "r8",
   "rax",
   "rcx",
   "rdx",
   "rsi",
   "rdi",
   "orig_rax",
   "rip",
   "cs",
   "eflags",
   "rsp",
   "ss",
   "fs_base",
   "gs_base",
   "ds",
   "es",
   "fs",
   "gs",
   "Unknown Register"
};

union TRegister
{
   user_regs_struct Reg;
   u64              RegArray[REGISTER_COUNT];
};

struct TDebugState
{
   pid_t ChildPid;
   s32   BreakpointHit;
   s32   WaitStatus;
   s32   WaitOptions;
};

struct TBreakpoint
{
   u64  Address;
   u64  SavedData;
   bool Enabled;
};

TDebugState              debug_state;
std::vector<TBreakpoint> breakpoints;

void StepOverBreakpoint(int Breakpoint);

u64 GetRegister(eRegister Register)
{
   TRegister registers;
   u64       result = 0;
   long      status;

   status = PTRACE(PTRACE_GETREGS, debug_state.ChildPid, nullptr, &registers.Reg);

   if (status != -1)
   {
      result = registers.RegArray[Register];
   }

   return result;
}

bool SetRegister(eRegister Register, u64 Value)
{
   TRegister registers;
   bool      result = false;
   long      status;

   status = PTRACE(PTRACE_GETREGS, debug_state.ChildPid, nullptr, &registers.Reg);

   if (status != -1)
   {
      registers.RegArray[Register] = Value;

      status = PTRACE(PTRACE_SETREGS, debug_state.ChildPid, nullptr, &registers.Reg);

      result = (status != -1);
   }

   return result;
}

void AddBreakpoint(TDebugState State, u64 Address)
{
   TBreakpoint bp = {};

   errno = 0;
   u64 data = PTRACE(PTRACE_PEEKDATA, State.ChildPid, Address, nullptr);

   if (errno == 0)
   {
      bp.Address = Address;
      bp.Enabled = true;
      bp.SavedData = data;

      u64 data_w_int = ((data & ~0xff) | SW_INTERRUPT_3);
      PTRACE(PTRACE_POKEDATA, State.ChildPid, Address, data_w_int);

      breakpoints.push_back(bp);
   }
   else
   {
      fprintf(stderr, "Error peek data %s (%d)\n", strerror(errno), errno);
   }
}

void DeleteBreakpoint(TDebugState State, u64 Index)
{
   assert(Index < breakpoints.size());

   PTRACE(PTRACE_POKEDATA, debug_state.ChildPid, breakpoints[Index].Address, breakpoints[Index].SavedData);

   if (debug_state.BreakpointHit == Index)
   {
      debug_state.BreakpointHit = -1;
   }
   else if (Index < debug_state.BreakpointHit)
   {
      debug_state.BreakpointHit -= 1;
   }

   breakpoints.erase(breakpoints.begin() + Index);
}

void ListBreakpoints()
{
   printf("Number of breakpoints: %d\n", breakpoints.size());
   for (size_t i = 0; i < breakpoints.size(); i++)
   {
      printf("  Breakpoint % 4d: 0x%x %s\n", i+1, breakpoints[i].Address, (!breakpoints[i].Enabled) ? "(disabled)" : "");
   }
}

int CheckBreakpoints()
{
   u64 rip = GetRegister(REGISTER_RIP) - 1;

   for (size_t i = 0; i < breakpoints.size(); i++)
   {
      if (breakpoints[i].Address == rip)
      {
         debug_state.BreakpointHit = i;
         // back up one instruction
         SetRegister(REGISTER_RIP, rip);
         return i;
      }
   }

   return -1;
}

int Continue()
{
   if (debug_state.BreakpointHit != -1)
      StepOverBreakpoint(debug_state.BreakpointHit);

   PTRACE(PTRACE_CONT, debug_state.ChildPid, nullptr, nullptr);
   waitpid(debug_state.ChildPid, &debug_state.WaitStatus, debug_state.WaitOptions);

   int bp = CheckBreakpoints();
   if (bp != -1)
   {
      printf("Breakpoint %d hit at 0x%x\n", bp + 1, breakpoints[bp].Address);
   }

   return bp;
}

void StepSingle()
{
   if (debug_state.BreakpointHit != -1)
   {
      StepOverBreakpoint(debug_state.BreakpointHit);
   }
   else
   {
      PTRACE(PTRACE_SINGLESTEP, debug_state.ChildPid, nullptr, nullptr);
      waitpid(debug_state.ChildPid, &debug_state.WaitStatus, debug_state.WaitOptions);
   }
}

void StepOverBreakpoint(int Breakpoint)
{
   assert(Breakpoint >= 0 && Breakpoint < breakpoints.size());

   u64 data = PTRACE(PTRACE_PEEKDATA, debug_state.ChildPid, breakpoints[Breakpoint].Address, nullptr);
   PTRACE(PTRACE_POKEDATA, debug_state.ChildPid, breakpoints[Breakpoint].Address, breakpoints[Breakpoint].SavedData);

   StepSingle();
   debug_state.BreakpointHit = -1;

   // re-enable breakpoint
   PTRACE(PTRACE_POKEDATA, debug_state.ChildPid, breakpoints[Breakpoint].Address, data);
   Continue();
}

void RunTarget(const char* program)
{
   printf("Run target %s\n", program);

   if (PTRACE(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
   {
      return;
   }

   execl(program, program, nullptr);
}

eDebugCommand GetCommand()
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
   else if (strcmp(strings[0], "s") == 0 || strcmp(strings[0], "step") == 0)
   {
      return DEBUG_CMD_STEP_SINGLE;
   }
   else if (strcmp(strings[0], "b") == 0 || strcmp(strings[0], "break") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: break [address]\n");
         return DEBUG_CMD_UNKNOWN;
      }

      // TODO: Break this out of here, this should just handle parsing
      u64 addr = strtoll(strings[1], 0, 16);
      AddBreakpoint(debug_state, addr);
      return DEBUG_CMD_SET_BREAKPOINT;
   }
   else if (strcmp(strings[0], "delete") == 0)
   {
      if (strings.size() != 2)
      {
         printf("Invalid cmd: delete [breakpoint]\n");
         return DEBUG_CMD_UNKNOWN;
      }

      // TODO: Break this out of here, this should just handle parsing
      u64 index = strtoll(strings[1], 0, 10);

      if (index-1 >= breakpoints.size())
      {
         printf("Invalid cmd: unknown breakpoint %d\n", index);
         return DEBUG_CMD_UNKNOWN;
      }

      DeleteBreakpoint(debug_state, index-1);
      return DEBUG_CMD_DELETE_BREAKPOINT;
   }
   else if (strcmp(strings[0], "list") == 0)
   {
      if (strings.size() != 1)
      {
         printf("Invalid cmd: list\n");
         return DEBUG_CMD_UNKNOWN;
      }

      return DEBUG_CMD_LIST_BREAKPOINTS;
   }
   else if (strcmp(strings[0], "register") == 0)
   {
      if (strings.size() < 2)
      {
         printf("Invalid cmd:\n");
         printf("  register read [register_name]\n");
         printf("  register write [register_name] [value]\n");
         return DEBUG_CMD_UNKNOWN;
      }

      // TODO: Break this out of here, this should just handle parsing
      if (strcmp(strings[1], "read") == 0)
      {
         u64         value = 0;
         const char* register_name = nullptr;

         if (strings.size() != 3)
         {
            printf("Invalid cmd: register read [register_name]\n");
            return DEBUG_CMD_UNKNOWN;
         }

         for (int i = 0; i < REGISTER_COUNT; i++)
         {
            if (strcmp(strings[2], RegisterStr[i]) == 0)
            {
               value = GetRegister((eRegister)i);
               register_name = RegisterStr[i];
            }
         }

         if (!register_name)
         {
            printf("Unknown register\n");
            return DEBUG_CMD_UNKNOWN;
         }

         printf("Register %s contents: 0x%x\n", register_name, value);
      }
      else if (strcmp(strings[1], "write") == 0)
      {
         u64       value = strtoll(strings[3], 0, 16);
         eRegister register_name = REGISTER_COUNT;

         if (strings.size() != 4)
         {
            printf("Invalid cmd: register write [register_name] [value]\n");
            return DEBUG_CMD_UNKNOWN;
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
            return DEBUG_CMD_UNKNOWN;
         }

         SetRegister(register_name, value);
         printf("Wrote Register %s contents: 0x%x\n", RegisterStr[register_name], value);
      }

      return DEBUG_CMD_UNKNOWN;
   }

   printf("Unknown command\n");
   return DEBUG_CMD_UNKNOWN;
};

void RunDebugger()
{
   printf("Run debugger on pid %d\n", debug_state.ChildPid);

   // Wait for child to stop on its first instruction
   debug_state.WaitOptions = 0;
   waitpid(debug_state.ChildPid, &debug_state.WaitStatus, debug_state.WaitOptions);

   while (WIFSTOPPED(debug_state.WaitStatus))
   {
      eDebugCommand cmd = GetCommand();

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
      else if (cmd == DEBUG_CMD_STEP_SINGLE)
      {
         StepSingle();
      }
      else if (cmd == DEBUG_CMD_LIST_BREAKPOINTS)
      {
         ListBreakpoints();
      }
      else
      {
         continue;
      }
   }
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
   debug_state.BreakpointHit = -1;

   if (debug_state.ChildPid == 0)
   {
      personality(ADDR_NO_RANDOMIZE);
      RunTarget(argv[1]);
   }
   else if (debug_state.ChildPid > 0)
   {
      RunDebugger();
   }
   else
   {
      perror("fork");
      return -1;
   }

   return 0;
}
