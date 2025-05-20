#include <trace.h>
#include <isa.h>
#include <unistd.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

extern word_t get_jal_address(Decode *s);
extern word_t get_jalr_address(Decode *s);
extern word_t get_ret_address();

iringbuf_t iringbuf[IRINFBUF_SIZE] = {0};
int iring = 0;

void *elf = NULL;
void *strtab = NULL;
void *symtab = NULL;
int elf_fd = -1;

static inline void format_inst(char *inst)
{
    if(!inst) return;

    for (int i = 0; i < strlen(inst); i++)
        if(inst[i] == '\t')
            inst[i] = ' ';


    char *c = " ";
    char *token = strtok(inst, c);
    char new_inst[128] = {0};
    int offset = 0;
    int flag = 1;

    while (token != NULL)
    {
        char temp[64] = {0};

        if(flag)
            sprintf(temp, "%-7s", token);
        else
            sprintf(temp, "%-5s", token);
        
        strcpy(new_inst+offset, temp);
        offset += strlen(temp);

        token = strtok(NULL, c);

        if(flag)
            flag = 0;
    }

    strcpy(inst, new_inst);
}

void itrace(char *inst, word_t opcode, word_t addr)
{
    iringbuf_t trace;
    memset(&trace, 0, sizeof(iringbuf_t));

    trace.addr = addr;
    trace.opcode = opcode;
    strcpy(trace.inst, inst);

    format_inst(trace.inst);

    iringbuf[iring] = trace;
    iring = (iring + 1) % IRINFBUF_SIZE ;
}

void iringbuf_show(word_t pc)
{
    // extern CPU_state cpu;

    for (int i = 0; i < IRINFBUF_SIZE; i++)
    {
        int index = (iring + i) % IRINFBUF_SIZE;
        word_t opcode = iringbuf[index].opcode;

        if(iringbuf[index].addr == 0)
            continue;

        if(pc != iringbuf[index].addr)
            printf("%-3s 0x%x: %-5s %-27s %02x %02x %02x %02x\n", "", iringbuf[index].addr, "", iringbuf[index].inst,
                opcode >> 24 & 0xFF,
                opcode >> 16 & 0xFF,
                opcode >> 8 & 0xFF,
                opcode & 0xFF);
        else
            printf("--> 0x%x: %-5s %-27s %02x %02x %02x %02x\n", iringbuf[index].addr, "", iringbuf[index].inst,
                opcode >> 24 & 0xFF,
                opcode >> 16 & 0xFF,
                opcode >> 8 & 0xFF,
                opcode & 0xFF);
            
    }
}

void *attach_elf(char *file)
{
    void *h;
    struct stat sb;

    elf_fd = open(file, O_RDONLY);
    if(elf_fd < 0)
    {
        panic("readelf elf_fd < 0");
    }

    // Get ELF file size
    if(fstat(elf_fd, &sb) < 0)
    {
        panic("fstat %s fail", file);
    }

    // Map ELF header to memory
    h = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, elf_fd, 0);
    if(h == MAP_FAILED)
    {
        panic("mmap %s faild", file);
    }

    return h;
}

void detach_elf(int fd, void* h)
{
    struct stat sb;

    if(fd < 0 || !h)
    {
        panic("%s", __FUNCTION__);
    }

    if(fstat(fd, &sb) < 0)
    {
        panic("fstat fail");
    }

    if(munmap(h, sb.st_size) < 0)
    {
        panic("mumap faild");
    }

    close(fd);
}

static void read_elf32(void *elf)
{
    Elf32_Ehdr *ehdr32 = elf;

    // setction table header
    Elf32_Shdr *shdr32 = elf + ehdr32->e_shoff;

    // shstrtab entry in setction table
    Elf32_Shdr *shstrtab = shdr32 + ehdr32->e_shstrndx;

    // shstr(section header str)
    void *shstr = elf + shstrtab->sh_offset;
    int shstr_size =  shstrtab->sh_size;

    for(int i = 0; i < ehdr32->e_shnum; i++)
    {
        if(shdr32[i].sh_name <= shstr_size)
        {
            char *s = shstr + shdr32[i].sh_name;
            if(strcmp(s, ".symtab") == 0)
                symtab = &shdr32[i];
            if(strcmp(s, ".strtab") == 0)
                strtab = &shdr32[i];
        }
    }
}

static void read_elf64(void *elf)
{
    Elf64_Ehdr *ehdr64 = elf;

    // setction table header
    Elf64_Shdr *shdr64 = elf + ehdr64->e_shoff;

    // shstrtab entry in setction table
    Elf64_Shdr *shstrtab = shdr64 + ehdr64->e_shstrndx;

    // shstr(section header str)
    void *shstr = elf + shstrtab->sh_offset;
    int shstr_size =  shstrtab->sh_size;

    for(int i = 0; i < ehdr64->e_shnum; i++)
    {
        if(shdr64[i].sh_name <= shstr_size)
        {
            char *s = shstr + shdr64[i].sh_name;
            if(strcmp(s, ".symtab") == 0)
                symtab = &shdr64[i];
            if(strcmp(s, ".strtab") == 0)
                strtab = &shdr64[i];
        }
    }
}

void readelf(void *elf)
{
    if(!elf)
        panic("%s", __FUNCTION__);

    if (((Elf32_Ehdr *)elf)->e_ident[EI_CLASS] == ELFCLASS32) 
    {
      read_elf32(elf);
    } 
    else 
    {
      read_elf64(elf);
    }
}

static char *get_funcname_elf32(word_t addr)
{
    Elf32_Shdr *strtable = strtab;
    Elf32_Shdr *symtable = symtab;
    Elf32_Sym  *symhdr = elf + symtable->sh_offset;

    char *symstr = elf + strtable->sh_offset;
    int sym_entry = symtable->sh_size / sizeof(Elf32_Sym);

    for (int i = 0; i < sym_entry; i++)
    {
        Elf32_Sym *sym = &symhdr[i];
        if(ELF32_ST_TYPE(sym->st_info) == STT_FUNC && 
            addr >= sym->st_value && addr < sym->st_value + sym->st_size)
        {
            return &symstr[sym->st_name];
        }
    }
    return "???";
}

static char* get_funcname_elf64(word_t addr)
{
    Elf64_Shdr *strtable = strtab;
    Elf64_Shdr *symtable = symtab;
    Elf64_Sym  *symhdr = elf + symtable->sh_offset;

    char *symstr = elf + strtable->sh_offset;
    int sym_entry = symtable->sh_size / sizeof(Elf64_Sym);

    for (int i = 0; i < sym_entry; i++)
    {
        Elf64_Sym *sym = &symhdr[i];
        if(ELF64_ST_TYPE(sym->st_info) == STT_FUNC && 
            addr >= sym->st_value && addr < sym->st_value + sym->st_size)
        {
            return &symstr[sym->st_name];
        }
    }
    return NULL;
}

char* get_funcname(word_t addr)
{   
    if(!elf)
        panic("%s: elf == NULL", __FUNCTION__);

    if (((Elf32_Ehdr *)elf)->e_ident[EI_CLASS] == ELFCLASS32)
    {
        return get_funcname_elf32(addr);
    }
    else 
    {
        return get_funcname_elf64(addr);
    }

    panic("%s: unknown elf type", __FUNCTION__);
    return NULL;
}

void ftrace(Decode *s, char *inst)
{
    word_t addr, naddr;
    char *func = NULL, *nfunc = NULL;

    addr = MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc);
    func = get_funcname(addr);

    if (strncmp(inst, "ret", 3) == 0)
        naddr = get_ret_address();
    else if(strncmp(inst, "jalr", 4) == 0)
        naddr = get_jalr_address(s);
    else if (strncmp(inst, "jal", 3) == 0)
        naddr = get_jal_address(s);

    nfunc = get_funcname(naddr);

    if (strncmp(inst, "ret", 3) == 0)
        printf("0x%x %-10s: ret  [%s]\n", addr, func ? func : "???", nfunc ? nfunc : "???");
    else if(strncmp(inst, "jalr", 4) == 0 || strncmp(inst, "jal", 3) == 0)
        printf("0x%x %-10s: call [%s@0x%x]\n", addr, func ? func : "???", nfunc ? nfunc : "???", naddr);
}