
#include <stdio.h>
#include <sys/stat.h>
#include "DebugUtils.h"

TBuffer ReadEntireFile(const char* Filename)
{
   TBuffer Result = {};

   FILE* File = fopen(Filename, "rb");

   if (File)
   {
      struct stat Stat;
      stat(Filename, &Stat);

      Result.Size = Stat.st_size;
      Result.Data = new u8[Stat.st_size];

      if (Result.Data)
      {
         if (fread(Result.Data, Result.Size, 1, File) != 1)
         {
            delete [] Result.Data;
            Result.Size = 0;
            Result.Data = nullptr;
         }
      }

      fclose(File);
   }

   return Result;
}

bool IsFileElf64(TBuffer Buffer)
{
   bool          result = false;
   TElfHeader64* elf_header = (TElfHeader64*)Buffer.Data;

   // ensure file is ELF (check magic number)
   if (elf_header->e_ident[ELF_MAGIC_0] == ELF_MAGIC_0_VAL &&
       elf_header->e_ident[ELF_MAGIC_1] == ELF_MAGIC_1_VAL &&
       elf_header->e_ident[ELF_MAGIC_2] == ELF_MAGIC_2_VAL &&
       elf_header->e_ident[ELF_MAGIC_3] == ELF_MAGIC_3_VAL)
   {
      result = (elf_header->e_ident[ELF_CLASS] == ELF_CLASS_64BIT);
   }

   return result;
}