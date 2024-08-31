#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <getopt.h>

#include "sc_def.h"
#include "sc_enclib.h"

struct mem_buffer crypto_mem_buffer;

char reg_output_buff[REGISTER_INFO_LENGTH];

static const struct option options[] = {
    {"registers", 0, 0, 'r'},
    {"memory", 0, 0, 'm'},
    {"exception-level", 1, 0, 'e'},
    {"addr", 1, 0, 'a'},
    {"size", 1, 0, 's'},
    {"pa-access", 1, 0, 'p'},
    {"watchpoint", 1, 0, 'w'},
    {"breakpoint", 1, 0, 'b'},
    {"instruction", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {"decryption", 0, 0, 'd'},
    {"help", 0, 0, 'h'},
    {NULL, 0, 0, 0},
};

static const char *optstring = "e:a:s:w:b:i:p:o:drmh";

void usage(void)
{
    printf("SCRUTINIZER usage: sc_user_client [options]\n");
    printf("Options:\n");
    printf("  -r|--registers                          Dump registers\n\n");

    printf("  -m|--memory                             Memory acquisition. E.g., dump Hafinum's exception vector table(-m -e 0x2 -a 0x6000800 -s 2048)\n");
    printf("  -e|--exception-level                    Give the EL of target (0/1/2)\n");
    printf("  -a|--addr                               Give the base address of target\n");
    printf("  -s|--size                               Give the acquisition length of target\n");
    printf("  -p|--pa-access                          Direct physical memory acquisition (1:enable; 0 disable). Full physical memory dumping by loop)\n\n");

    printf("  -w|--watchpoint                         Watchpoint trapped to dump memory&registers. E.g., -w 0x630df28\n");
    printf("  -b|--breakpoint                         Breakpoint trapped to dump memory&registers. E.g., -b 0x62d462c\n\n");

    printf("  -i|--instruction                        Instruction tracing (1:enable; 2:disable&dump)\n\n");

    printf("  -o|--output                             Dump specify output type (reg/mem) from buf\n\n");

    printf("  -d|--decryption                          Decrypt the contents for test purpose\n");

    printf("  -h|--help                               Show help messages\n");
}

int buff2file(const char *output_file, const char *buff, unsigned int buff_len)
{
    int fd;

    if (!output_file)
    {
        printf("Invalid output_file\n");
        return -1;
    }
    if (!buff)
    {
        printf("Invalid buff\n");
        return -1;
    }

    fd = open(output_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1)
    {
        printf("Fail to open %s (%s)\n", output_file, strerror(errno));
        return -1;
    }

    printf("Writing %s\n", output_file);
    if (write(fd, (void *)buff, buff_len) != buff_len)
    {
        printf("Fail to write %s (%s)\n", output_file, strerror(errno));
        close(fd);
        return -1;
    }
    else
    {
        close(fd);
        return 0;
    }
}

void printf_registers_info(const sc_reg_info_t *info, char *output_buff, size_t buff_size)
{
    snprintf(output_buff, buff_size,
             "x0: 0x%llx, x1: 0x%llx, x2: 0x%llx, x3: 0x%llx\n"
             "x4: 0x%llx, x5: 0x%llx, x6: 0x%llx, x7: 0x%llx\n"
             "x8: 0x%llx, x9: 0x%llx, x10: 0x%llx, x11: 0x%llx\n"
             "x12: 0x%llx, x13: 0x%llx, x14: 0x%llx, x15: 0x%llx\n"
             "x16: 0x%llx, x17: 0x%llx, x18: 0x%llx, x19: 0x%llx\n"
             "x20: 0x%llx, x21: 0x%llx, x22: 0x%llx, x23: 0x%llx\n"
             "x24: 0x%llx, x25: 0x%llx, x26: 0x%llx, x27: 0x%llx\n"
             "x28: 0x%llx, x29: 0x%llx, x30: 0x%llx\n"
             "scr_el3: 0x%llx, elr_el3: 0x%llx, esr_el3: 0x%llx, far_el3: 0x%llx\n"
             "sctlr_el3: 0x%llx, ttbr0_el3: 0x%llx, gpccr_el3: 0x%llx, gptbr_el3: 0x%llx\n"
             "sctlr_el1: 0x%llx, sp_el1: 0x%llx, esr_el1: 0x%llx, ttbr0_el1: 0x%llx\n"
             "ttbr1_el1: 0x%llx, vbar_el1: 0x%llx, spsr_el1: 0x%llx, hcr_el2: 0x%llx\n"
             "sctlr_el2: 0x%llx, sp_el2: 0x%llx, esr_el2: 0x%llx, elr_el2: 0x%llx\n"
             "ttbr0_el2: 0x%llx, vsttbr_el2: 0x%llx, vttbr_el2: 0x%llx, vbar_el2: 0x%llx\n"
             "spsr_el2: 0x%llx\n",
             info->x0, info->x1, info->x2, info->x3,
             info->x4, info->x5, info->x6, info->x7,
             info->x8, info->x9, info->x10, info->x11,
             info->x12, info->x13, info->x14, info->x15,
             info->x16, info->x17, info->x18, info->x19,
             info->x20, info->x21, info->x22, info->x23,
             info->x24, info->x25, info->x26, info->x27,
             info->x28, info->x29, info->x30,
             info->scr_el3, info->elr_el3, info->esr_el3, info->far_el3,
             info->sctlr_el3, info->ttbr0_el3, info->gpccr_el3, info->gptbr_el3,
             info->sctlr_el1, info->sp_el1, info->esr_el1, info->ttbr0_el1,
             info->ttbr1_el1, info->vbar_el1, info->spsr_el1, info->hcr_el2,
             info->sctlr_el2, info->sp_el2, info->esr_el2, info->elr_el2,
             info->ttbr0_el2, info->vsttbr_el2, info->vttbr_el2, info->vbar_el2,
             info->spsr_el2);
}

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0)) {
        usage();
        return EXIT_SUCCESS;
    }
    
    int fd;
    void *map_base;

    int fd_sc;
    fd_sc = open(DEV_PATH, O_RDWR, 0);
    printf("fd_sc:%d\n", fd_sc);
    if (fd_sc < 0)
    {
        printf("Can't open %s\n", DEV_PATH);
        return -1;
    }

    int longindex, c, ret, pkttype = -1;
    unsigned long auth_key = AUTH_TEST_KEY;
    const char *output_file = NULL;
    const char *output_type = NULL;
    struct stat sb;

    /* disable output buffering */
    setbuf(stdout, NULL);

    unsigned int el = 0, addr = 0, size = 0, ctl_inst_tracing = 0, pa_dump = 0;
    int memory_dump = 0;
    int registers_dump = 0;
    int enable_dec = 0;
    unsigned long brk_addr = 0, wrk_addr = 0;

    for (;;)
    {
        c = getopt_long(argc, argv, optstring, options, &longindex);
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 'r':
            registers_dump = 1;
            break;

        case 'm':
            memory_dump = 1;
            break;

        case 'e':
            el = strtoul(optarg, NULL, 0);
            break;

        case 'a':
            addr = strtoul(optarg, NULL, 0);
            break;

        case 's':
            size = strtoul(optarg, NULL, 0);
            break;

        case 'p':
            pa_dump = atoi(optarg);
            break;

        case 'w':
            wrk_addr = strtoul(optarg, NULL, 0);
            break;

        case 'b':
            brk_addr = strtoul(optarg, NULL, 0);
            break;

        case 'i':
            ctl_inst_tracing = atoi(optarg);
            break;

        case 'o':
            output_type = optarg;
            break;

        case 'd':
            enable_dec = 1;
            break;

        case 'h':
            usage();
            return EXIT_SUCCESS;

        default:
            printf("Unknown argument: %c\n", c);
            usage();
            return EXIT_FAILURE;
        }
    }

    if (registers_dump)
    {
        if (ioctl(fd_sc, SC_AUTH_TEST, &auth_key) < 0)
        {
            printf("SC_AUTH_TEST: ioctl failed\n");
            return -1;
        }
        void *buf = malloc(REGISTER_INFO_LENGTH);
        if (buf == NULL)
        {
            printf("Memory allocation failed\n");
            return 1;
        }
        memset(buf, 0, REGISTER_INFO_LENGTH);
        sc_reg_info_t *registers_info = (sc_reg_info_t *)buf;
        if (ioctl(fd_sc, SC_REGISTER, registers_info) < 0)
        {
            printf("SC_REGISTER: ioctl failed\n");
            return -1;
        }

        // decryption
        if(enable_dec)
        {
            crypto_mem_buffer.virt_base = (void *)registers_info;
            crypto_mem_buffer.size = REGISTER_INFO_LENGTH;
            crypto_mem_buffer.flag = 0x7;
            crypto_mem_buffer.hash = 0;
            restore_str_buffer(&crypto_mem_buffer);
        }

        memset(reg_output_buff, 0, sizeof(reg_output_buff));

        printf_registers_info(registers_info, reg_output_buff, sizeof(reg_output_buff));

        if (buff2file("registers_info.txt", reg_output_buff, strlen(reg_output_buff)) < 0)
        {
            printf("Failed to save registers info to file\n");
        }

        free(buf);
    }

    if (memory_dump)
    {
        printf("memory dump with EL=%u, addr=0x%x, size=%u\n", el, addr, size);
        if(pa_dump == 0)
        {
            if (addr < PLAT_ARM_TRUSTED_DRAM_BASE || addr > PLAT_ARM_TRUSTED_DRAM_BASE + PLAT_ARM_TRUSTED_DRAM_SIZE || size == 0 || size > 0x1400000 || el < 0 || el > 2)
            {
                printf("Invalid arguments: EL=%u, addr=0x%x, size=%u\n", el, addr, size);
                return -1;
            }
        }
      
        if(brk_addr&&wrk_addr)
        {
            printf("Invalid arguments. -b and -w should only set one at a time\n");
            return -1;
        }
        sc_mem_info_t mem_info;
        mem_info.addr = addr;
        mem_info.size = size;
        mem_info.el = el;
        mem_info.pa_access = pa_dump;
        if(brk_addr || wrk_addr)
        {
            if (wrk_addr)
            {
                mem_info.watchpoint_addr = wrk_addr;
                if (ioctl(fd_sc, SC_AUTH_TEST, &auth_key) < 0)
                {
                    printf("SC_AUTH_TEST: ioctl failed\n");
                    return -1;
                }
                if (ioctl(fd_sc, SC_SET_WATCHPOINT, &mem_info) < 0)
                {
                    printf("SC_SET_WATCHPOINT: ioctl failed\n");
                    return -1;
                }
            }
            else if (brk_addr)
            {
                mem_info.breakpoint_addr = brk_addr;
                if (ioctl(fd_sc, SC_AUTH_TEST, &auth_key) < 0)
                {
                    printf("SC_AUTH_TEST: ioctl failed\n");
                    return -1;
                }
                if (ioctl(fd_sc, SC_SET_BREAKPOINT, &mem_info) < 0)
                {
                    printf("SC_SET_BREAKPOINT: ioctl failed\n");
                    return -1;
                }
            }
        }
        else
        {
            if (ioctl(fd_sc, SC_AUTH_TEST, &auth_key) < 0)
            {
                printf("SC_AUTH_TEST: ioctl failed\n");
                return -1;
            }
            if (ioctl(fd_sc, SC_MEMORY_DUMP, &mem_info) < 0)
            {
                printf("SC_MEMORY_DUMP: ioctl failed\n");
                return -1;
            }
        }

    }

    if (ctl_inst_tracing)
    {
        sc_ins_info_t ins_info;
        if(ctl_inst_tracing == 1)
        {
                if (ioctl(fd_sc, SC_AUTH_TEST, &auth_key) < 0)
                {
                    printf("SC_AUTH_TEST: ioctl failed\n");
                    return -1;
                }
                if (ioctl(fd_sc, SC_ETE_ON, &ins_info) < 0)
                {
                    printf("SC_ETE_ON: ioctl failed\n");
                    return -1;
                }
        }
        else if(ctl_inst_tracing == 2)
        {
                if (ioctl(fd_sc, SC_AUTH_TEST, &auth_key) < 0)
                {
                    printf("SC_AUTH_TEST: ioctl failed\n");
                    return -1;
                }

                if (ioctl(fd_sc, SC_ETE_OFF, &ins_info) < 0)
                {
                    printf("SC_ETE_OFF: ioctl failed\n");
                    return -1;
                }

                if(ins_info.ins_share_buf_phys)
                {
                    printf("SC_ETE_OFF: mapping shsare_ins_buf:0x%lx\n", ins_info.ins_share_buf_phys);
                    map_base = mmap(NULL, TRBE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_sc, ins_info.ins_share_buf_phys);
                    if (map_base == MAP_FAILED) {
                        perror("Error mapping memory");
                        close(fd_sc);
                        return EXIT_FAILURE;
                    }

                    if(enable_dec)
                    {
                        crypto_mem_buffer.virt_base = map_base;
                        crypto_mem_buffer.size = TRBE_SIZE;
                        crypto_mem_buffer.flag = 0x7;
                        crypto_mem_buffer.hash = 0;
                        restore_str_buffer(&crypto_mem_buffer);
                    }

                    if (buff2file("ins_dump", (char *)map_base, TRBE_SIZE) < 0) {
                        printf("Failed to save ins dump to file\n");
                    }

                    if (munmap(map_base, TRBE_SIZE) == -1) {
                        perror("Error unmapping memory");
                    }
                }   
        }
        else
        {
            printf("Invalid 1-enable; 2-disable \n");
            return -1;
        }
       
    }

    if (output_type)
    {
        if (strcmp(output_type, "reg") == 0)
        {

            void *buf = malloc(REGISTER_INFO_LENGTH);
            if (buf == NULL)
            {
                printf("memory allocation failed\n");
                return 1;
            }
            memset(buf, 0, REGISTER_INFO_LENGTH);
            sc_reg_info_t *registers_info = (sc_reg_info_t *)buf;
            if (ioctl(fd_sc, SC_REGISTER_SAVE, registers_info) < 0)
            {
                printf("SC_REGISTER: ioctl failed\n");
                return -1;
            }

            if(enable_dec)
            {
                crypto_mem_buffer.virt_base = (void *)registers_info;
                crypto_mem_buffer.size = REGISTER_INFO_LENGTH;
                crypto_mem_buffer.flag = 0x7;
                crypto_mem_buffer.hash = 0;
                restore_str_buffer(&crypto_mem_buffer);
            }

            memset(reg_output_buff, 0, sizeof(reg_output_buff));

            printf_registers_info(registers_info, reg_output_buff, sizeof(reg_output_buff));

            if (buff2file("registers_info.txt", reg_output_buff, strlen(reg_output_buff)) < 0)
            {
                printf("Failed to save registers info to file\n");
            }

            free(buf);
        }
        else if (strcmp(output_type, "mem") == 0)
        {
            fd = open("/dev/mem", O_RDWR | O_SYNC);
            if (fd == -1) {
                perror("Error opening /dev/mem");
                return EXIT_FAILURE;
            }
            map_base = mmap(NULL, AGENT_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AGENT_DATA_ADDRESS);
            if (map_base == MAP_FAILED) {
                perror("Error mapping memory");
                close(fd);
                return EXIT_FAILURE;
            }

            if(enable_dec)
            {
                crypto_mem_buffer.virt_base = map_base;
                crypto_mem_buffer.size = AGENT_DATA_SIZE;
                crypto_mem_buffer.flag = 0x7;
                crypto_mem_buffer.hash = 0;
                restore_str_buffer(&crypto_mem_buffer);
            }

            if (buff2file("mem_dump", (char *)map_base, AGENT_DATA_SIZE) < 0) {
                printf("Failed to save memory dump to file\n");
            }

            if (munmap(map_base, AGENT_DATA_SIZE) == -1) {
                perror("Error unmapping memory");
            }
        }
        else
        {
            printf("Invalid output type: %s\n", output_type);
            return -1;
        }
    }

    if (argc != optind)
    {
        printf("Invalid arguments\n");
        usage();
        return EXIT_FAILURE;
    }

    close(fd_sc);
    close(fd);
    return 0;
}
