#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

bool is_elf_file(uint8_t *elf_file) {
  return elf_file[0] == 0x7f && strcmp(&elf_file[1], "ELF");
}

void cleanup(uint8_t *elf_file, struct stat *file_stat, int fd) {
  munmap(elf_file, file_stat->st_size);
  close(fd);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "[-] Usage: %s INPUT_FILE\n", argv[0]);
    exit(1);
  }

  printf("[+] Parsing %s\n", argv[1]);
  int fd = 0;
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    perror("open");
    exit(1);
  }

  struct stat file_stat;
  if (fstat(fd, &file_stat) < 0) {
    perror("fstat");
    exit(1);
  }
  printf("[+] File is %lu bytes\n", file_stat.st_size);

  uint8_t *elf_file =
      (uint8_t *)mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (elf_file == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  printf("[+] File mapped into memory at %p\n", elf_file);

  if (!is_elf_file(elf_file)) {
    fprintf(stderr, "[-] Not a valid ELF file, unable to continue\n");
    cleanup(elf_file, &file_stat, fd);
    exit(1);
  }

  // Display entrypoint
  Elf64_Ehdr *elf_header = (Elf64_Ehdr *)elf_file;
  printf("[+] Entry point is %p\n", (void *)elf_header->e_entry);

  /*
  if (elf_header->e_type != ET_EXEC) {
    fprintf(stderr, "[-] Only parsing EXEC files\n");
    cleanup(elf_file, &file_stat, fd);
    exit(1);
  }
  */

  // Get section and program headers
  Elf64_Phdr *phdr = (Elf64_Phdr *)(elf_file + elf_header->e_phoff);
  Elf64_Shdr *shdr = (Elf64_Shdr *)(elf_file + elf_header->e_shoff);
  printf("[+] Program header table at %p, section header table at %p\n", phdr,
         shdr);

  char *string_table = elf_file + shdr[elf_header->e_shstrndx].sh_offset;
  printf("[+] String table is located at %p\n", string_table);

  for (int i = 1; i < elf_header->e_shnum; ++i) {
    printf("[+] Section header at index %d is %s\n", i,
           &string_table[shdr[i].sh_name]);
  }

  printf("[+] Cleaning up\n");
  cleanup(elf_file, &file_stat, fd);
  return 0;
}
