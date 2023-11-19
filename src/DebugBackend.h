
#pragma once

#include <sys/types.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "DebugTypes.h"

class CDebugBackend
{
public:
   CDebugBackend();
   ~CDebugBackend();

   u64 GetData(u64 Address);
   void SetData(u64 Address, u64 Value);

   u64 GetRegister(eRegister Register);
   bool SetRegister(eRegister Register, u64 Value);

   void SetCommand(TDebugCommand Command);
   eDebugCommand GetCommand() const { return mCommand.Command; }

   bool IsRunning() const { return mRunning; }
   bool Run(const char* Filename);

   u8* PopBuffer();

private:

   void AddBreakpoint(u64 Address);
   void DeleteBreakpoint(u64 Index);
   void EnableBreakpoint(u64 Index);
   void DisableBreakpoint(u64 Index);
   void ListBreakpoints();
   int CheckBreakpoints();

   void Continue();
   void StepSingle();
   void StepOverBreakpoint();

   void HandleCommand();
   void RunTarget();
   void RunDebugger();
   void StartTarget();
   void RestartTarget();

   void GetSignalInfo();
   void Wait();

   void PushString(eDataType DataType, u8* String, u32 Size);

   std::string              mTarget;
   std::thread              mThread;
   std::mutex               mMutex;
   std::vector<TBreakpoint> mBreakpoints;
   u8*                      mOutputBuffer;
   u32                      mBufferIndex;
   u32                      mReadIndex;
   pid_t                    mChildPid;
   s32                      mBreakpointHit;
   s32                      mWaitStatus;
   s32                      mWaitOptions;
   TDebugCommand            mCommand;
   bool                     mRunning;
   bool                     mTargetRunning;
};
