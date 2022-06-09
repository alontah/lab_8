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
    puts("This function will be implemented in a later section");
}

void print_symbols(state *s){
    puts("This function will be implemented in a later section");
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
                    {"Quit", quit}, {NULL, NULL}};


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
