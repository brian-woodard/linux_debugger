
#include <stdio.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <assert.h>
#include "DebugBackend.h"

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

void SigHandler(int Signal)
{
   assert(Signal >= 0 && Signal < NSIG);
   printf("Got signal %s (%d) on pid %d\n", strsignal(Signal), Signal, getpid());
}

CDebugBackend::CDebugBackend()
   : mTarget(),
     mThread(),
     mMutex(),
     mBreakpoints(),
     mChildPid(0),
     mBreakpointHit(-1),
     mWaitStatus(0),
     mWaitOptions(0),
     mCommand{},
     mRunning(false)
{
   mBreakpoints.reserve(64);
}

CDebugBackend::~CDebugBackend()
{
}

u64 CDebugBackend::GetData(u64 Address)
{
   u64 data = PTRACE(PTRACE_PEEKDATA, mChildPid, Address, nullptr);

   return data;
}

void CDebugBackend::SetData(u64 Address, u64 Value)
{
   PTRACE(PTRACE_POKEDATA, mChildPid, Address, Value);
}

u64 CDebugBackend::GetRegister(eRegister Register)
{
   TRegister registers;
   u64       result = 0;
   long      status;

   status = PTRACE(PTRACE_GETREGS, mChildPid, nullptr, &registers.Reg);

   if (status != -1)
   {
      result = registers.RegArray[Register];
   }

   return result;
}

bool CDebugBackend::SetRegister(eRegister Register, u64 Value)
{
   TRegister registers;
   bool      result = false;
   long      status;

   status = PTRACE(PTRACE_GETREGS, mChildPid, nullptr, &registers.Reg);

   if (status != -1)
   {
      registers.RegArray[Register] = Value;

      status = PTRACE(PTRACE_SETREGS, mChildPid, nullptr, &registers.Reg);

      result = (status != -1);
   }

   return result;
}

void CDebugBackend::AddBreakpoint(u64 Address)
{
   TBreakpoint bp = {};

   u64 data = GetData(Address);

   if (errno == 0)
   {
      bp.Address = Address;
      bp.Enabled = true;
      bp.SavedData = data;

      u64 data_w_int = ((data & ~0xff) | SW_INTERRUPT_3);
      SetData(Address, data_w_int);

      mBreakpoints.push_back(bp);
   }
   else
   {
      fprintf(stderr, "Error peek data %s (%d)\n", strerror(errno), errno);
   }
}

void CDebugBackend::DeleteBreakpoint(u64 Index)
{
   assert(Index < mBreakpoints.size());

   u64 data = (GetData(mBreakpoints[Index].Address) & ~0xff) | mBreakpoints[Index].SavedData;
   SetData(mBreakpoints[Index].Address, data);

   if (mBreakpointHit == Index)
   {
      mBreakpointHit = -1;
   }
   else if (mBreakpointHit != -1 && Index < mBreakpointHit)
   {
      mBreakpointHit -= 1;
   }

   mBreakpoints.erase(mBreakpoints.begin() + Index);
}

void CDebugBackend::EnableBreakpoint(u64 Index)
{
   assert(Index < mBreakpoints.size());

   if (!mBreakpoints[Index].Enabled)
   {
      u64 data_w_int = ((GetData(mBreakpoints[Index].Address) & ~0xff) | SW_INTERRUPT_3);
      SetData(mBreakpoints[Index].Address, data_w_int);
      mBreakpoints[Index].Enabled = true;
   }
}

void CDebugBackend::DisableBreakpoint(u64 Index)
{
   assert(Index < mBreakpoints.size());

   if (mBreakpoints[Index].Enabled)
   {
      u64 data = (GetData(mBreakpoints[Index].Address) & ~0xff) | mBreakpoints[Index].SavedData;
      SetData(mBreakpoints[Index].Address, data);
      mBreakpoints[Index].Enabled = false;
   }
}

void CDebugBackend::ListBreakpoints()
{
   printf("Number of breakpoints: %d\n", mBreakpoints.size());
   for (size_t i = 0; i < mBreakpoints.size(); i++)
   {
      printf("  Breakpoint % 4d: 0x%x %s\n", i+1, mBreakpoints[i].Address, (!mBreakpoints[i].Enabled) ? "(disabled)" : "");
   }
}

int CDebugBackend::CheckBreakpoints()
{
   u64 rip = GetRegister(REGISTER_RIP) - 1;

   for (size_t i = 0; i < mBreakpoints.size(); i++)
   {
      if (mBreakpoints[i].Address == rip)
      {
         mBreakpointHit = i;
         // back up one instruction
         SetRegister(REGISTER_RIP, rip);
         return i;
      }
   }

   return -1;
}

void CDebugBackend::Continue()
{
   if (mBreakpointHit != -1)
      StepOverBreakpoint();

   PTRACE(PTRACE_CONT, mChildPid, nullptr, nullptr);
   
   Wait();
}

void CDebugBackend::StepSingle()
{
   if (mBreakpointHit != -1)
   {
      StepOverBreakpoint();
   }

   PTRACE(PTRACE_SINGLESTEP, mChildPid, nullptr, nullptr);
   
   Wait();
}

void CDebugBackend::StepOverBreakpoint()
{
   assert(mBreakpointHit >= 0 && mBreakpointHit < mBreakpoints.size());

   int bp = mBreakpointHit;

   u64 data = (GetData(mBreakpoints[mBreakpointHit].Address) & ~0xff) | mBreakpoints[mBreakpointHit].SavedData;
   SetData(mBreakpoints[mBreakpointHit].Address, data);

   mBreakpointHit = -1;
   StepSingle();

   // re-enable breakpoint
   u64 data_w_int = ((GetData(mBreakpoints[bp].Address) & ~0xff) | SW_INTERRUPT_3);
   SetData(mBreakpoints[bp].Address, data_w_int);
}

void CDebugBackend::SetCommand(TDebugCommand Command)
{
   mMutex.lock();
   mCommand = Command;

   if (mCommand.Command == DEBUG_CMD_QUIT)
   {
      mRunning = false;
      if (mThread.joinable())
         mThread.join();
      mCommand.Command = DEBUG_CMD_PROCESSED;
   }
   mMutex.unlock();
}

void CDebugBackend::RunCommand()
{
   switch (mCommand.Command)
   {
      case DEBUG_CMD_CONTINUE:
         Continue();
         break;
      case DEBUG_CMD_STEP_SINGLE:
         StepSingle();
         break;
      case DEBUG_CMD_LIST_BREAKPOINTS:
         ListBreakpoints();
         break;
      case DEBUG_CMD_SET_BREAKPOINT:
         AddBreakpoint(mCommand.Data.BpAddr.Address);
         break;
      case DEBUG_CMD_DELETE_BREAKPOINT:
         DeleteBreakpoint(mCommand.Data.BpIdx.Index);
         break;
      case DEBUG_CMD_ENABLE_BREAKPOINT:
         EnableBreakpoint(mCommand.Data.BpIdx.Index);
         break;
      case DEBUG_CMD_DISABLE_BREAKPOINT:
         DisableBreakpoint(mCommand.Data.BpIdx.Index);
         break;
      case DEBUG_CMD_REGISTER_READ:
         printf("Register %s contents: 0x%x\n", RegisterStr[mCommand.Data.Reg.Index], GetRegister((eRegister)mCommand.Data.Reg.Index));
         break;
      case DEBUG_CMD_REGISTER_WRITE:
         // if writing to instruction pointer, clear breakpoint hit index
         if (mCommand.Data.Reg.Index == REGISTER_RIP)
            mBreakpointHit = -1;

         SetRegister((eRegister)mCommand.Data.Reg.Index, mCommand.Data.Reg.Value);
         printf("Wrote Register %s contents: 0x%x\n", RegisterStr[mCommand.Data.Reg.Index], mCommand.Data.Reg.Value);
         break;
      default:
         break;
   }

   mMutex.lock();
   mCommand.Command = DEBUG_CMD_PROCESSED;
   mMutex.unlock();
}

void CDebugBackend::GetSignalInfo()
{
   siginfo_t info = {};
   PTRACE(PTRACE_GETSIGINFO, mChildPid, nullptr, &info);

   assert(info.si_signo >= 0 && info.si_signo < NSIG);

   printf(">>> got signal %s (%d) from %d code %d\n", strsignal(info.si_signo), info.si_signo, mChildPid, info.si_code);
}

void CDebugBackend::Wait()
{
   // wait for debugee to stop
   mWaitOptions = 0;
   waitpid(mChildPid, &mWaitStatus, mWaitOptions);

   printf(">>> wait status 0x%x WIFEXITED %d\n", mWaitStatus, WIFEXITED(mWaitStatus));

   // get some info about the signal that caused the stop
   GetSignalInfo();

   // check breakpoints
   int bp = CheckBreakpoints();
   if (bp != -1)
   {
      printf("Breakpoint %d hit at 0x%x\n", bp + 1, mBreakpoints[bp].Address);
   }
}

void CDebugBackend::RunTarget()
{
   if (PTRACE(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
   {
      return;
   }

   // signal(SIGTERM, SigHandler);
   // signal(SIGTRAP, SigHandler);
   // signal(SIGINT, SigHandler);
   // signal(SIGSEGV, SigHandler);

   printf(">>> start target pid %d\n", getpid());

   execl(mTarget.c_str(), mTarget.c_str(), nullptr);

   printf(">>> target finished\n");
}

void CDebugBackend::RunDebugger()
{
   signal(SIGTERM, SigHandler);
   signal(SIGTRAP, SigHandler);
   signal(SIGINT, SigHandler);
   signal(SIGSEGV, SigHandler);

   mChildPid = fork();

   if (mChildPid == 0)
   {
      personality(ADDR_NO_RANDOMIZE);
      RunTarget();
   }
   else if (mChildPid < 0)
   {
      perror("fork");
      return;
   }

   // Wait for child to stop on its first instruction
   Wait();

   PTRACE(PTRACE_SETOPTIONS, mChildPid, nullptr, PTRACE_O_TRACEEXIT);

   while (mRunning)
   {
      RunCommand();

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }

   // we're done, stop the child process
   kill(mChildPid, SIGTERM);
}

bool CDebugBackend::Run(const char* Filename)
{
   mTarget = Filename;

   // TODO: Verify target exists, and is executable

   // start thread to run debugger
   if (!mThread.joinable())
   {
      mRunning = true;
      mThread = std::thread(&CDebugBackend::RunDebugger, this);
   }

   return true;
}

