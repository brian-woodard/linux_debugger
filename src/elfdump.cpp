
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <elf.h>
#include "PrintData.cpp"

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

const char* SectionTypeStr[] =
{
   "Null",
   "Program data",
   "Symbol table",
   "String table",
   "Relocation entries with addends",
   "Symbol hash table",
   "Dynamic linking information",
   "Notes",
   "Program space with no data (bss)",
   "Relocation entries, no addends",
   "Reserved",
   "Dynamic linker symbol table",
   "Unknown",
   "Unknown",
   "Array of constructors",
   "Array of destructors",
   "Array of pre-constructors",
   "Section group",
   "Extended section indices"
};

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

void DumpSectionHeader64(Elf64_Shdr* SectionHeaders, int NumHeaders)
{
   printf("--- ELF Section Header, %d headers size %d ---\n", NumHeaders, NumHeaders*sizeof(Elf64_Shdr));
   for (int i = 0; i < NumHeaders; i++)
   {
      printf("--- Section Header %d ---\n", i+1);
      printf("sh_name:      %d\n", SectionHeaders[i].sh_name);
      printf("sh_type:      %d (%s)\n", SectionHeaders[i].sh_type, SectionTypeStr[SectionHeaders[i].sh_type]);
      printf("sh_flags:     %d\n", SectionHeaders[i].sh_flags);
      printf("sh_addr:      0x%x\n", SectionHeaders[i].sh_addr);
      printf("sh_offset:    0x%x\n", SectionHeaders[i].sh_offset);
      printf("sh_size:      %d\n", SectionHeaders[i].sh_size);
      printf("sh_link:      %d\n", SectionHeaders[i].sh_link);
      printf("sh_info:      %d\n", SectionHeaders[i].sh_info);
      printf("sh_addralign: %d\n", SectionHeaders[i].sh_addralign);
      printf("sh_entsize:   %d\n", SectionHeaders[i].sh_entsize);
   }
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

   DumpSectionHeader64((Elf64_Shdr*)&Buffer.Data[elf_header->e_shoff], elf_header->e_shnum);
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

   if (0)
   {
      // dump binary of file
      printf("%s\n", CPrintData::GetDataAsString((char*)buffer.Data, buffer.Size));
   }
   else if (buffer.Size)
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
