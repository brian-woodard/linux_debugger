
#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>

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

#define ArrayCount(array) sizeof(array)/sizeof(array[0])

enum eDebugCommand
{
   DEBUG_CMD_UNKNOWN,
   DEBUG_CMD_CONTINUE,
   DEBUG_CMD_SET_BREAKPOINT,
   DEBUG_CMD_DELETE_BREAKPOINT,
   DEBUG_CMD_ENABLE_BREAKPOINT,
   DEBUG_CMD_DISABLE_BREAKPOINT,
   DEBUG_CMD_LIST_BREAKPOINTS,
   DEBUG_CMD_STEP_OVER,
   DEBUG_CMD_STEP_INTO,
   DEBUG_CMD_STEP_SINGLE,
   DEBUG_CMD_REGISTER_READ,
   DEBUG_CMD_REGISTER_WRITE,
   DEBUG_CMD_RUN,
   DEBUG_CMD_QUIT,
   DEBUG_CMD_PROCESSED,
   DEBUG_CMD_COUNT
};

struct TDebugCommand
{
   eDebugCommand Command;

   union
   {
      struct TBreakpointAddress
      {
         u64 Address;
      } BpAddr;
      struct TBreakpointIndex
      {
         u64 Index;
      } BpIdx;
      struct TRegisterData
      {
         u64 Index;
         u64 Value;
      } Reg;
   } Data;
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

extern const char* RegisterStr[];

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
   u8   SavedData;
   bool Enabled;
};