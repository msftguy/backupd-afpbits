//
//  main.c
//  backupd_anyfs
//
//  Created by msftguy on 7/10/11.
//  Copyright 2011 msftguy. All rights reserved.
//
#include <stdio.h>
#include <sys/errno.h>
#include "../impHook/impHookApi.h"
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#include <sys/unistd.h>

typedef int	 (*pfn_fsctl_t)(const char *,unsigned long,void*,unsigned int);

static pfn_fsctl_t s_origFsctl;

typedef struct {
    unsigned int cmd1;
    unsigned char unk0x04[0x4];
    unsigned char flags1;
    unsigned char unk0x9[7];
    unsigned char unk0x10[0x8];
    unsigned int cmd2;
    unsigned int cmd3;
    unsigned char unk0x20[0x10];
} afp_fsctl_t;

// compile time assert
#define ct_assert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
ct_assert(sizeof(afp_fsctl_t) == 0x30);

int	 my_fsctl(const char * a1, unsigned long a2, afp_fsctl_t *a3, unsigned int a4);

int	 my_fsctl(const char * a1, unsigned long a2, afp_fsctl_t *io, unsigned int a4)
{
    if (a2 == 0xC0307A1D) {
        int cmd1 = io->cmd1;
        int result = fsctl(a1, a2, io, a4); // Doesn't cause infinite recursion.. Maybe it should?.. Naah!
        if (result != 0 && errno == 45) {// com.apple.backupd: afpfs fsctl failed to read settings: 45 Operation not supported
            fprintf(stderr, "backupd_anyfs[%u]: Faking AFP fsctl support for a non-AFP volume .. cmd=[%u]\n", getpid(), cmd1);
            fflush(stderr);
            memset(io, 0, sizeof(afp_fsctl_t));
            io->cmd1 = cmd1;
            result = 0;
        }
        if (
            (result == 0) && 
            (cmd1 == 1) && // 1 means get flags, 2 means set .. I guess
            ((io->flags1 & 3) != 3)
            ) 
        {
            if (0 == (io->flags1 & 1)) {                
                io->flags1 |= 1;
                fprintf(stderr, "backupd_anyfs[%u]: Faking TM Lock Stealing support!\n", getpid());
                fflush(stderr);
            }
            if (0 == (io->flags1 & 2)) {                
                io->flags1 |= 2;
                fprintf(stderr, "backupd_anyfs[%u]: Faking Server Reply Cache support!\n", getpid());
                fflush(stderr);
            }
        } else {
            fprintf(stderr, "backupd_anyfs[%u]: result=%u, flags1=%u\n", getpid(), result, io->flags1);
            fflush(stderr);
        }
        return result;
    }

    fprintf(stderr, "backupd_anyfs[%u]: my_fsctl(0x%lX) ignored\n", getpid(), a2);
    fflush(stderr);        
    return fsctl(a1, a2, io, a4);
}


void dl_main(void);

void dl_main()
{
    const char* procImagePath = _dyld_get_image_name(0);
    const char* procName = strrchr(procImagePath, '/');
    if (procName != NULL)
        procName++;
    else
        procName = procImagePath;
    if (0 != strcasecmp(procName, "backupd")) {
        fprintf(stderr, "backupd_anyfs: loaded into %s (PID=%u), feeling unwelcome and unappreciated. Hibernating...\n", procName, getpid());
        fflush(stderr);
        return;        
    }
    fprintf(stderr, "backupd_anyfs: Helloooo backupd! (PID=%u)\n", getpid());
    fflush(stderr);
    const struct mach_header* mainHeader = _dyld_get_image_header(0);
    volatile uintptr_t* pFsctlImp = get_import_ptr((const macho_header*)mainHeader, "_fsctl");
    if (pFsctlImp == NULL) {
        fprintf(stderr, "backupd_anyfs: Trolled by backupd: _fsctl not impor... Did I just see a pig fly by???\n");
        fflush(stderr);
        return;
    }

    const struct mach_header* procImageHeader = _dyld_get_image_header(0);
    _dyld_bind_fully_image_containing_address(procImageHeader);
    s_origFsctl = (pfn_fsctl_t)*pFsctlImp;
    *pFsctlImp = (uintptr_t)my_fsctl;
    fprintf(stderr, "backupd_anyfs: _fsctl hooked in %s (PID=%u)\n", _dyld_get_image_name(0), getpid());
    fflush(stderr);
}