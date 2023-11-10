
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
const s32 MAX_COMMAND    = 1024;

enum eDebugCommand
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
   "R15",
   "R14",
   "R13",
   "R12",
   "RBP",
   "RBX",
   "R11",
   "R10",
   "R9",
   "R8",
   "RAX",
   "RCX",
   "RDX",
   "RSI",
   "RDI",
   "ORIG RAX",
   "RIP",
   "CS",
   "EFLAGS",
   "RSP",
   "SS",
   "FS BASE",
   "GS BASE",
   "DS",
   "ES",
   "FS",
   "GS",
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

u64 GetRegister(eRegister Register)
{
   TRegister registers;
   u64       result = 0;
   long      status;

   status = ptrace(PTRACE_GETREGS, debug_state.ChildPid, nullptr, &registers.Reg);

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

   status = ptrace(PTRACE_GETREGS, debug_state.ChildPid, nullptr, &registers.Reg);

   if (status != -1)
   {
      registers.RegArray[Register] = Value;

      status = ptrace(PTRACE_SETREGS, debug_state.ChildPid, nullptr, &registers.Reg);

      result = (status != -1);
   }

   return result;
}

void AddBreakpoint(TDebugState State, u64 Address)
{
   TBreakpoint bp = {};

   errno = 0;
   u64 data = ptrace(PTRACE_PEEKDATA, State.ChildPid, Address, nullptr);

   if (errno == 0)
   {
      bp.Address = Address;
      bp.Enabled = true;
      bp.SavedData = data;

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

int CheckBreakpoints()
{
   u64 rip = GetRegister(REGISTER_RIP) - 1;

   for (int i = 0; i < breakpoints.size(); i++)
   {
      if (breakpoints[i].Address == rip)
      {
         return i;
      }
   }

   return -1;
}

int Continue()
{
   ptrace(PTRACE_CONT, debug_state.ChildPid, nullptr, nullptr);
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
   ptrace(PTRACE_SINGLESTEP, debug_state.ChildPid, nullptr, nullptr);
}

void StepOverBreakpoint(int Breakpoint)
{
   u64 data = ptrace(PTRACE_PEEKDATA, debug_state.ChildPid, breakpoints[Breakpoint].Address, nullptr);
   ptrace(PTRACE_POKEDATA, debug_state.ChildPid, breakpoints[Breakpoint].SavedData, nullptr);

   StepSingle();

   // re-enable breakbpoint
   ptrace(PTRACE_POKEDATA, debug_state.ChildPid, data, nullptr);
   Continue();
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
      // TODO: Break this out of here, this should just handle parsing
      u64 addr = strtoll(strings[1], 0, 16);
      AddBreakpoint(debug_state, addr);
      return DEBUG_CMD_SET_BREAKPOINT;
   }
   else if (strcmp(strings[0], "register") == 0)
   {
      // TODO: Break this out of here, this should just handle parsing
      if (strcmp(strings[1], "read") == 0)
      {
         u64 rip = GetRegister(REGISTER_RIP);
         printf("Register %s contents: 0x%x\n", RegisterStr[REGISTER_RIP], rip);
      }

      return DEBUG_CMD_UNKNOWN;
   }

   return DEBUG_CMD_UNKNOWN;
};

void RunDebugger()
{
   int bp = -1;

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
      eDebugCommand cmd = GetCommand();

      if (cmd == DEBUG_CMD_QUIT)
      {
         // we're done, stop the child process
         kill(debug_state.ChildPid, SIGTERM);
         break;
      }
      else if (cmd == DEBUG_CMD_CONTINUE)
      {
         if (bp != -1)
            StepOverBreakpoint(bp);
         bp = Continue();
      }
      else if (cmd == DEBUG_CMD_STEP_SINGLE)
      {
         StepSingle();
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
