
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
const u32 OUTPUT_BUFFER  = 1024 * 1024;
const u32 MAX_DATA       = 64;

#define ArrayCount(array) sizeof(array)/sizeof(array[0])

struct TBuffer
{
   u8* Data;
   u64 Size;
};

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
   DEBUG_CMD_REGISTER_READ_ALL,
   DEBUG_CMD_REGISTER_WRITE,
   DEBUG_CMD_DATA_READ,
   DEBUG_CMD_GET_TARGET,
   DEBUG_CMD_SET_TARGET,
   DEBUG_CMD_RUN,
   DEBUG_CMD_START,
   DEBUG_CMD_STOP,
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
      struct TDataRead
      {
         u64 Address;
         u64 Bytes;
      } Read;
      struct TStringData
      {
         u8* String;
         u64 Size;
      } String;

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

enum eDataType
{
   DATA_TYPE_STREAM_ERROR,
   DATA_TYPE_STREAM_WARNING,
   DATA_TYPE_STREAM_INFO,
   DATA_TYPE_STREAM_DEBUG,
   DATA_TYPE_REGISTERS,
   DATA_TYPE_DATA,
   DATA_TYPE_COUNT
};

struct TBufferHeader
{
   eDataType DataType;
   u32       Size;
};

// ELF Header types and constants
#define ELF_NIDENT 16

struct TElfHeader64
{
   u8  e_ident[ELF_NIDENT]; // Magic number and other info
   u16 e_type;              // Object file type 
   u16 e_machine;           // Architecture
   u32 e_version;           // Object file version
   u64 e_entry;             // Entry point virtual address
   u64 e_phoff;             // Program header table file offset
   u64 e_shoff;             // Section header table file offset
   u32 e_flags;             // Processor-specific flags
   u16 e_ehsize;            // ELF header size in bytes
   u16 e_phentsize;         // Program header table entry size
   u16 e_phnum;             // Program header table entry count
   u16 e_shentsize;         // Section header table entry size
   u16 e_shnum;             // Section header table entry count
   u16 e_shstrndx;          // Section header string table index
} Elf64_Ehdr;

// Fields in the e_ident array. The ELF_* macros are indices into the
// array. The macros under each ELF_* macro are the values the byte
// may have.

#define ELF_MAGIC_0     0     // File identification byte 0 index
#define ELF_MAGIC_0_VAL 0x7f  // Magic number byte 0

#define ELF_MAGIC_1     1     // File identification byte 1 index
#define ELF_MAGIC_1_VAL 'E'   // Magic number byte 1

#define ELF_MAGIC_2     2     // File identification byte 2 index
#define ELF_MAGIC_2_VAL 'L'   // Magic number byte 2

#define ELF_MAGIC_3     3     // File identification byte 3 index
#define ELF_MAGIC_3_VAL 'F'   // Magic number byte 3

#define ELF_MAGIC       "\177ELF"
#define ELF_MAGIC_SIZE  4

#define ELF_CLASS       4     // File class byte index

enum eElfClass
{
   ELF_CLASS_NONE,
   ELF_CLASS_32BIT,
   ELF_CLASS_64BIT,
   ELF_CLASS_COUNT
};