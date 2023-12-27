
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
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

TBuffer ReadEntireProcFile(const char* Filename)
{
   size_t  bytes_to_read = 128;
   size_t  index = 0;
   TBuffer Result = {};

   int File = open(Filename, O_RDONLY);

   if (File != -1)
   {
      Result.Data = (u8*)malloc(bytes_to_read);

      int bytes = read(File, &Result.Data[index], bytes_to_read);

      while (bytes == bytes_to_read)
      {
         Result.Size += bytes;
         Result.Data = (u8*)realloc(Result.Data, Result.Size + bytes_to_read);
         if (Result.Data == nullptr)
         {
            Result.Size = 0;
            return Result;
         }
      }

      if (bytes > 0 && bytes != bytes_to_read)
      {
         Result.Size += bytes;
      }

      // strip newline off end
      if (Result.Data[Result.Size-1] == '\n')
      {
         Result.Data[--Result.Size] = 0;
      }

      close(File);
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