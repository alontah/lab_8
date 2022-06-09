#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

#define NAME_LEN 128
#define BUFF_SIZE 10000
#define OPTION_BUFF_SIZE 256

int current_fd;

typedef struct {
    int debug;
    char file_name[NAME_LEN];
    void *map_start;
    Elf32_Ehdr *elf_header;
    Elf32_Shdr *section_headers;
    char *section_strings;
} state;

typedef struct {
    char* func_name;
    void (*func)(state *s);
} func_desc;

void toggle_debug(state *s){
    if(s-> debug){
        s->debug = 0;
        puts("Debug flag now off");
    } else {
        s->debug = 1;
        puts("Debug flag now on");
    }
}

void examine_elf(state *s){
    puts("please enter ELF file name:");
    fgets(s->file_name, NAME_LEN, stdin);
    s->file_name[strlen(s->file_name)-1] = 0; //removing '\n'

    if(current_fd){
        close(current_fd);
    }

    current_fd = open(s->file_name, O_RDWR);
    if(current_fd == -1){
        perror("File Error");
        return;
    }

    s->map_start = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, current_fd, 0);
    if (s->map_start == MAP_FAILED)
    {
        perror("Error mapping");
        close(current_fd);
        current_fd = -1;
        return;
    }

    char magic[3];
    strncpy(magic, s->map_start + 1, 3);
   

    if(strncmp(magic, "ELF", 3) != 0){
        printf("file is not of ELF format\n");
        munmap(s->map_start, BUFF_SIZE);
        close(current_fd);
        current_fd = -1;
        return;
    }

    s->elf_header = s->map_start;
    s->section_headers = (Elf32_Shdr *)(s->map_start + s->elf_header->e_shoff);
    Elf32_Shdr *string_section_header = (s->section_headers + s->elf_header->e_shstrndx);
    s->section_strings = (char *)(s->map_start + string_section_header->sh_offset);
    s->debug = 0;
    
    char *data_encoding;
    if (s->elf_header->e_ident[EI_DATA])
    {
        if (s->elf_header->e_ident[EI_DATA] == ELFDATA2LSB)
        {
            data_encoding = "2's complement, little endian";
        }
        else
        {
            data_encoding = "1's complement, big endian";
        }
    }
    else 
    {
        printf("Error in data encoding\n");
        return;
    }
    

    printf("magic: %s\n", magic);
    printf("encoding: %s\n", data_encoding);
    printf("entry point: %X\n", s->elf_header->e_entry);
    printf("section header table offset: %X (bytes into file)\n", s->elf_header->e_shoff);
    printf("number of section: %u\n", s->elf_header->e_shnum);
    printf("size of each section: %u (bytes)\n", s->elf_header->e_shentsize);
    printf("program header table offset: %X (bytes into file)\n", s->elf_header->e_phoff);
    printf("number of program headers: %u\n",  s->elf_header->e_phnum);
    printf("size of each program header: %u (bytes)\n", s->elf_header->e_phentsize);
}

void print_section_names(state *s){
    if(s->debug){
        printf("Debug: e_shoff: %x\n", s->elf_header->e_shoff);
        printf("Debug: e_shnum: %u\n", s->elf_header->e_shnum);
        printf("Debug: eshstrndx: %u\n", s->elf_header->e_shstrndx);
    }

    for(int i = 0; i < s->elf_header->e_shnum; i++){
        Elf32_Shdr * curr_section = (Elf32_Shdr *)(s->section_headers + i);
        
        printf("[%d] %s\t%08x\t%08x\t%08x\t%d\n",
        i,
        s->section_strings + curr_section->sh_name,
        curr_section->sh_addr,
        curr_section->sh_offset,
        curr_section->sh_size,
        curr_section->sh_type);
    }
}

void print_symbol_table(state *s, Elf32_Shdr *sym_section, Elf32_Shdr *string_section){

    Elf32_Sym *first_sym = s->map_start + sym_section->sh_offset;
    if(s->debug){
            printf("Debug: symbol table size: %u (in bytes)\n", sym_section->sh_size);
            printf("Debug: number of symbols in table: %u\n", sym_section->sh_size/sizeof(Elf32_Sym));
    }
    
    for(int i = 0; i < sym_section->sh_size/sizeof(Elf32_Sym); i++){
        
        Elf32_Sym *curr_sym = (Elf32_Sym *)(first_sym + i);
        
        char *section_name = "UND";
        if (curr_sym->st_shndx < 0xff00)
        {
            Elf32_Shdr *section_of_symbol = (Elf32_Shdr *)(s->section_headers + curr_sym->st_shndx);
            section_name = s->section_strings + section_of_symbol->sh_name;
        }
        else
        {
            if (curr_sym->st_shndx == SHN_ABS)
            {
                section_name = "ABS";
            }
        }
        
        
        printf("[%d] %08x\t%u\t%s\n",
        i,
        curr_sym->st_value,
        curr_sym->st_shndx,
        section_name);
    }
}

void print_symbols(state *s){
    
    for(int i = 0; i < s->elf_header->e_shnum; i++){
        Elf32_Shdr * curr_section = (Elf32_Shdr *)(s->section_headers + i);
        
        if(curr_section->sh_type == SHT_SYMTAB || curr_section->sh_type == SHT_DYNSYM){
            printf("------------------------------------------\n");
            print_symbol_table(s, curr_section, (Elf32_Shdr *)(s->section_strings + curr_section->sh_link));
            printf("------------------------------------------\n");
        }
    }
}

// Elf32_Sym * get_main_sym(state *s, Elf32_Shdr *sym_section, Elf32_Shdr *string_section){
//     Elf32_Sym *first_sym = s->map_start + sym_section->sh_offset;
//     if(s->debug){
//             printf("Debug: symbol table size: %u (in bytes)\n", sym_section->sh_size);
//             printf("Debug: number of symbols in table: %u\n", sym_section->sh_size/sizeof(Elf32_Sym));
//     }
    
//     for(int i = 0; i < sym_section->sh_size/sizeof(Elf32_Sym); i++){
        
//         Elf32_Sym *curr_sym = (Elf32_Sym *)(first_sym + i);
        
//         char *section_name = "UND";
//         if (curr_sym->st_shndx < 0xff00)
//         {
//             Elf32_Shdr *section_of_symbol = (Elf32_Shdr *)(s->section_headers + curr_sym->st_shndx);
//             section_name = s->section_strings + section_of_symbol->sh_name;
//         }
//         else
//         {
//             if (curr_sym->st_shndx == SHN_ABS)
//             {
//                 section_name = "ABS";
//             }
//         }
        
        
//         printf("[%d] %08x\t%u\t%s\n",
//         i,
//         curr_sym->st_value,
//         curr_sym->st_shndx,
//         section_name);
//     }
// }

// void print_main(state *s){
//     Elf32_Sym * main_sym = NULL;

//     for(int i = 0; i < s->elf_header->e_shnum; i++){
//         Elf32_Shdr * curr_section = (Elf32_Shdr *)(s->section_headers + i);
        
//         if(curr_section->sh_type == SHT_SYMTAB || curr_section->sh_type == SHT_DYNSYM){
//             main_sym = get_main_sym(s, curr_section, (Elf32_Shdr *)(s->section_strings + curr_section->sh_link));
//             if(main_sym) break;
//         }
//     }

//     if(!main_sym){
//         puts("main not found");
//         return;
//     }

//     Elf32_Shdr * main_section = (Elf32_Shdr *)(s->section_headers + main_sym->st_shndx);
//     printf("%08x\n", main_sym->st_value);
//     char  main_code[main_sym->st_size + 1];
//     char main_code[main_sym->st_size] = 0;
//     strncpy(main_code,(char*) (s->map_start + main_sym->st_value), main_sym->st_size);
//     printf("%s\n", main_code);
// }

void print_main(state * s){
    //TODO: ask if this is what the instructions meant or do i need to find in with code and not readelf
    char main[136];
    strncpy(main, (char *)(s->map_start + 0x770), 136);
    for(int i = 0; i < 136; i++){
        printf("%X", main[i]);
        
    }
    printf("\n");
}

void quit(state * s){
    free(s);
    exit(0);
}

int main(int argc, char const *argv[])
{
    state *curr_state = malloc(sizeof(state));
    func_desc func_menu[] = {{"Toggle Debug Mode", toggle_debug}, 
                    {"Examine ELF File", examine_elf}, 
                    {"Print Section Names", print_section_names}, 
                    {"Print Symbols", print_symbols},
                    {"Print Main:", print_main },
                    {"Quit", quit},
                     {NULL, NULL}};


    int func_menu_size = sizeof(func_menu)/sizeof(func_desc);

    while(1){
        puts("Choose action:");
        for(int i = 0; func_menu[i].func != NULL; i++){
            printf("%d - %s\n", i, func_menu[i].func_name);
        }

        char option_input[OPTION_BUFF_SIZE];
        fgets(option_input, OPTION_BUFF_SIZE, stdin);
        int option = -1;
        sscanf(option_input, "%d", &option);
        
        if(option >= func_menu_size || option < 0){
            puts("please choose a number within bounds");
        } else {
            func_menu[option].func(curr_state);
        }
    }
    
    return 0;
}
