
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <elf.h>

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

struct TBuffer
{
   u8* Data;
   u32 Size;
};

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
            fprintf(stderr, "ERROR: Unable to read %d\n", Filename);
         }
      }

      fclose(File);
   }
   else
   {
      fprintf(stderr, "ERROR: Unable to open %s\n", Filename);
   }

   return Result;
}

void DumpElf32(TBuffer Buffer)
{
   Elf32_Ehdr* elf_header = (Elf32_Ehdr*)Buffer.Data;

   printf("--- ELF Header size %d ---\n", sizeof(Elf32_Ehdr));
   printf("e_ident:     %s 32-bit %s\n", elf_header->e_ident,
          elf_header->e_ident[5] == 1 ? "Little Endian" : "Big Endian");
   printf("e_type:      %d\n", elf_header->e_type);
   printf("e_machine:   %d\n", elf_header->e_machine);
   printf("e_version:   %d\n", elf_header->e_version);
   printf("e_entry:     0x%x\n", elf_header->e_entry);
   printf("e_phoff:     0x%x\n", elf_header->e_phoff);
   printf("e_shoff:     0x%x\n", elf_header->e_shoff);
   printf("e_flags:     0x%x\n", elf_header->e_flags);
   printf("e_ehsize:    %d\n", elf_header->e_ehsize);
   printf("e_phentsize: %d\n", elf_header->e_phentsize);
   printf("e_phnum:     %d\n", elf_header->e_phnum);
   printf("e_shentsize: %d\n", elf_header->e_shentsize);
   printf("e_shnum:     %d\n", elf_header->e_shnum);
   printf("e_shstrndx:  %d\n", elf_header->e_shstrndx);
}

void DumpElf64(TBuffer Buffer)
{
   Elf64_Ehdr* elf_header = (Elf64_Ehdr*)Buffer.Data;

   printf("--- ELF Header size %d ---\n", sizeof(Elf64_Ehdr));
   printf("e_ident:     %s 64-bit %s\n", elf_header->e_ident,
          elf_header->e_ident[5] == 1 ? "Little Endian" : "Big Endian");
   printf("e_type:      %d\n", elf_header->e_type);
   printf("e_machine:   %d\n", elf_header->e_machine);
   printf("e_version:   %d\n", elf_header->e_version);
   printf("e_entry:     0x%x\n", elf_header->e_entry);
   printf("e_phoff:     0x%x\n", elf_header->e_phoff);
   printf("e_shoff:     0x%x\n", elf_header->e_shoff);
   printf("e_flags:     0x%x\n", elf_header->e_flags);
   printf("e_ehsize:    %d\n", elf_header->e_ehsize);
   printf("e_phentsize: %d\n", elf_header->e_phentsize);
   printf("e_phnum:     %d\n", elf_header->e_phnum);
   printf("e_shentsize: %d\n", elf_header->e_shentsize);
   printf("e_shnum:     %d\n", elf_header->e_shnum);
   printf("e_shstrndx:  %d\n", elf_header->e_shstrndx);
}

int main(int argc, char** argv)
{
   TBuffer buffer;

   if (argc != 2)
   {
      fprintf(stderr, "Expected a program name as argument\n");
      return -1;
   }

   buffer = ReadEntireFile(argv[1]);

   if (buffer.Size)
   {
      Elf32_Ehdr* elf_header = (Elf32_Ehdr*)buffer.Data;

      // ensure file is ELF (check magic number)
      if (elf_header->e_ident[EI_MAG0] == ELFMAG0 &&
          elf_header->e_ident[EI_MAG1] == ELFMAG1 &&
          elf_header->e_ident[EI_MAG2] == ELFMAG2 &&
          elf_header->e_ident[EI_MAG3] == ELFMAG3)
      {
         if (elf_header->e_ident[EI_CLASS] == ELFCLASS64)
         {
            DumpElf64(buffer);
         }
         else
         {
            DumpElf32(buffer);
         }
      }
      else
      {
         fprintf(stderr, "ERROR: %s is not an ELF file\n", argv[1]);
      }
   }

   return 0;
}
