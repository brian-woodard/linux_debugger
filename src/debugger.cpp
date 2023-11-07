
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

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

TDebugState debug_state;

eDbgCommand GetCommand()
{
   char input[1024] = {};

   printf("\ndbg> ");
   scanf("%s", input);

   printf("got input [%s]\n", input);

   if (strcmp(input, "c") == 0 || strcmp(input, "cont") == 0 || strcmp(input, "continue") == 0)
   {
      return DEBUG_CMD_CONTINUE;
   }
   else if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0)
   {
      return DEBUG_CMD_QUIT;
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
   pid_t child_pid;

   if (argc < 2)
   {
      fprintf(stderr, "Expected a program name as argument\n");
      return -1;
   }

   child_pid = fork();
   if (child_pid == 0)
   {
      RunTarget(argv[1]);
   }
   else if (child_pid > 0)
   {
      debug_state.ChildPid = child_pid;
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
