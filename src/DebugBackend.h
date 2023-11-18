
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

   void GetSignalInfo();
   void Wait();

   std::string              mTarget;
   std::thread              mThread;
   std::mutex               mMutex;
   std::vector<TBreakpoint> mBreakpoints;
   pid_t                    mChildPid;
   s32                      mBreakpointHit;
   s32                      mWaitStatus;
   s32                      mWaitOptions;
   TDebugCommand            mCommand;
   bool                     mRunning;
   bool                     mTargetRunning;
};
