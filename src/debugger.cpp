
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

void run_target(const char* program)
{
   printf("Run target %s\n", program);

   if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0)
   {
      perror("ptrace");
      return;
   }

   execl(program, program, 0);
}

void run_debugger(pid_t pid)
{
   int wait_status;
   uint32_t icounter = 0;

   printf("Run debugger on pid %d\n", pid);

   // Wait for child to stop on its first instruction
   wait(&wait_status);

   while (WIFSTOPPED(wait_status))
   {
      struct user_regs_struct regs;

      icounter++;

      ptrace(PTRACE_GETREGS, pid, 0, &regs);
      uint32_t instr = ptrace(PTRACE_PEEKTEXT, pid, regs.rip, 0);

      printf("icounter = %u.  RIP = 0x%08x.  instr = 0x%08x\n",
             icounter, regs.rip, instr);

      // Make the child execute another instruction
      if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0)
      {
         perror("ptrace");
         return;
      }

      // Wait for child to stop on its next instruction
      wait(&wait_status);
   }

   printf("The child executed %u instructions\n", icounter);
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
      run_target(argv[1]);
   else if (child_pid > 0)
      run_debugger(child_pid);
   else
   {
      perror("fork");
      return -1;
   }

   return 0;
}
