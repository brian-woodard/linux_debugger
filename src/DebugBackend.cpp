
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "DebugBackend.h"

#define PTRACE(Request, Pid, Addr, Data) ({ \
   long retval = 0; \
   errno = 0; \
   retval = ptrace(Request, Pid, Addr, Data); \
   if (errno) \
   { \
      char msg[256]; \
      sprintf(msg, "ptrace error %s (%d) at %s:%d %s", strerror(errno), errno, __FILE__, __LINE__, __func__); \
      PushString(DATA_TYPE_STREAM_ERROR, (u8*)msg, strlen(msg)); \
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
     mOutputBuffer(nullptr),
     mBufferIndex(0),
     mChildPid(0),
     mBreakpointHit(-1),
     mWaitStatus(0),
     mWaitOptions(0),
     mCommand{},
     mRunning(false),
     mTargetRunning(false)
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
   char msg[256];

   sprintf(msg, "Number of breakpoints: %d", mBreakpoints.size());
   PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));

   for (size_t i = 0; i < mBreakpoints.size(); i++)
   {
      sprintf(msg, "  Breakpoint % 4d: 0x%x %s", i+1, mBreakpoints[i].Address, (!mBreakpoints[i].Enabled) ? "(disabled)" : "");
      PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
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

void CDebugBackend::HandleCommand()
{
   switch (mCommand.Command)
   {
      case DEBUG_CMD_CONTINUE:
         if (!mTargetRunning)
         {
            char msg[256];
            sprintf(msg, "Target is not running");
            PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
         }
         else
            Continue();
         break;
      case DEBUG_CMD_RUN:
         if (mTargetRunning)
         {
            char msg[256];
            sprintf(msg, "Target is already running, pid %d", mChildPid);
            PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
         }
         else
         {
            if (WIFEXITED(mWaitStatus))
            {
               RestartTarget();
            }
            mTargetRunning = true;
            Continue();
         }
         break;
      case DEBUG_CMD_STEP_SINGLE:
         if (!mTargetRunning)
         {
            char msg[256];
            sprintf(msg, "Target is not running");
            PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
         }
         else
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
      {
         char msg[256];
         sprintf(msg, "Register %s contents: 0x%x", RegisterStr[mCommand.Data.Reg.Index], GetRegister((eRegister)mCommand.Data.Reg.Index));
         PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
         break;
      }
      case DEBUG_CMD_REGISTER_WRITE:
      {
         char msg[256];

         // if writing to instruction pointer, clear breakpoint hit index
         if (mCommand.Data.Reg.Index == REGISTER_RIP)
            mBreakpointHit = -1;

         SetRegister((eRegister)mCommand.Data.Reg.Index, mCommand.Data.Reg.Value);
         sprintf(msg, "Wrote Register %s contents: 0x%x", RegisterStr[mCommand.Data.Reg.Index], mCommand.Data.Reg.Value);
         PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
         break;
      }
      default:
         break;
   }

   mMutex.lock();
   mCommand.Command = DEBUG_CMD_PROCESSED;
   mMutex.unlock();
}

void CDebugBackend::GetSignalInfo()
{
   char      msg[256];
   siginfo_t info = {};
   PTRACE(PTRACE_GETSIGINFO, mChildPid, nullptr, &info);

   assert(info.si_signo >= 0 && info.si_signo < NSIG);

   // not really sure what do with this info?
   //sprintf(msg, "got signal %s (%d) from %d code %d", strsignal(info.si_signo), info.si_signo, mChildPid, info.si_code);
   //PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
}

void CDebugBackend::Wait()
{
   char msg[256];

   // wait for debugee to stop
   mWaitOptions = 0;
   waitpid(mChildPid, &mWaitStatus, mWaitOptions);

   if (WIFEXITED(mWaitStatus))
   {
      mTargetRunning = false;

      sprintf(msg, "Target execution exited cleanly");
      PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));

      // start a new instance
      RestartTarget();
      return;
   }

   // get some info about the signal that caused the stop
   GetSignalInfo();

   // check breakpoints
   int bp = CheckBreakpoints();
   if (bp != -1)
   {
      sprintf(msg, "Breakpoint %d hit at 0x%x", bp + 1, mBreakpoints[bp].Address);
      PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));
   }
}

void CDebugBackend::RunTarget()
{
   char msg[256];

   if (PTRACE(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
   {
      return;
   }

   sprintf(msg, "Debugging started on %s, pid %d", mTarget.c_str(), getpid());
   PushString(DATA_TYPE_STREAM_INFO, (u8*)msg, strlen(msg));

   execl(mTarget.c_str(), mTarget.c_str(), nullptr);
}

void CDebugBackend::StartTarget()
{
   if (!mTargetRunning)
   {
      mChildPid = fork();

      if (mChildPid == 0)
      {
         personality(ADDR_NO_RANDOMIZE);
         RunTarget();
      }
      else if (mChildPid < 0)
      {
         char msg[256];
         sprintf(msg, "Error forking child process %s (%d)", strerror(errno), errno);
         PushString(DATA_TYPE_STREAM_ERROR, (u8*)msg, strlen(msg));
         mRunning = false;
         return;
      }

      // Wait for child to stop on its first instruction
      Wait();

      //PTRACE(PTRACE_SETOPTIONS, mChildPid, nullptr, PTRACE_O_TRACEEXIT);
   }
}

void CDebugBackend::RestartTarget()
{
   StartTarget();

   // set all breakpoints on new instance
   size_t num_breakpoints = mBreakpoints.size();
   for (size_t i = 0; i < num_breakpoints; i++)
   {
      AddBreakpoint(mBreakpoints[i].Address);
      if (!mBreakpoints[i].Enabled)
      {
         size_t new_bp_idx = mBreakpoints.size();
         DisableBreakpoint(new_bp_idx);
      }
   }

   // delete old breakpoints in reverse order
   for (size_t i = num_breakpoints; i > 0; i--)
   {
      mBreakpoints.erase(mBreakpoints.begin() + (i - 1));
   }
}

void CDebugBackend::PushString(eDataType DataType, u8* String, u32 Size)
{
   mMutex.lock();
   TBufferHeader* header = (TBufferHeader*)&mOutputBuffer[mBufferIndex];

   header->DataType = DataType;
   header->Size = Size;

   mBufferIndex += sizeof(TBufferHeader);

   memcpy(&mOutputBuffer[mBufferIndex], String, Size);

   mBufferIndex += Size;
   mMutex.unlock();
}

u8* CDebugBackend::PopBuffer()
{
   u8* result = nullptr;

   mMutex.lock();
   if (mReadIndex < mBufferIndex)
   {
      TBufferHeader* header = (TBufferHeader*)&mOutputBuffer[mReadIndex];

      result = &mOutputBuffer[mReadIndex];

      mReadIndex += sizeof(TBufferHeader);
      mReadIndex += header->Size;
   }
   else
   {
      mBufferIndex = 0;
      mReadIndex = 0;
   }
   mMutex.unlock();

   return result;
}

void CDebugBackend::RunDebugger()
{
   signal(SIGTERM, SigHandler);
   signal(SIGTRAP, SigHandler);
   signal(SIGINT, SigHandler);
   signal(SIGSEGV, SigHandler);

   mOutputBuffer = new u8[OUTPUT_BUFFER];

   StartTarget();

   while (mRunning)
   {
      HandleCommand();

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

