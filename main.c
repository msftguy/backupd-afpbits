//
//  main.c
//  backupd_anyfs
//
//  Created by msftguy on 7/10/11.
//  Copyright 2011 msftguy. All rights reserved.
//
#include <stdio.h>
#include "../impHook/impHookApi.h"
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#include <sys/unistd.h>

typedef int	 (*pfn_fsctl_t)(const char *,unsigned long,void*,unsigned int);

static pfn_fsctl_t s_origFsctl;

int	 my_fsctl(const char * a1, unsigned long a2, void*a3, unsigned int a4);

int	 my_fsctl(const char * a1, unsigned long a2, void*a3, unsigned int a4)
{
    if (a2 == 0xC0307A1D) {
        int result = fsctl(a1, a2, a3, a4); // Doesn't cause infinite recursion.. Maybe it should?.. Naah!
        unsigned char* pOptByte = ((unsigned char*)a3) + 8;
        if (result == 0 && ((*pOptByte & 3) != 3)) {
            if (0 == (*pOptByte & 1)) {                
                *pOptByte |= 1;
                fprintf(stderr, "backupd_anyfs[%u]: Faking TM Lock Stealing support!\n", getpid());
                fflush(stderr);
            }
            if (0 == (*pOptByte & 2)) {                
                *pOptByte |= 2;
                fprintf(stderr, "backupd_anyfs[%u]: Faking Server Reply Cache support!\n", getpid());
                fflush(stderr);
            }
        } else {
            fprintf(stderr, "backupd_anyfs[%u]: result=%u, out[8]=%u\n", getpid(), result, ((char*)a3)[8]);
            fflush(stderr);
        }
        return result;
    }

    fprintf(stderr, "backupd_anyfs[%u]: my_fsctl(0x%lX) ignored\n", getpid(), a2);
    fflush(stderr);        
    return s_origFsctl(a1, a2, a3, a4);
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
    pfn_fsctl_t origFsctl;

    origFsctl = (pfn_fsctl_t)*pFsctlImp;
    const struct mach_header* procImageHeader = _dyld_get_image_header(0);
    _dyld_bind_fully_image_containing_address(procImageHeader);
    s_origFsctl = (pfn_fsctl_t)*pFsctlImp;
    if (s_origFsctl != origFsctl) {
        fprintf(stderr, "backupd_anyfs: A strange and mysterious thing has just happened.. %p->%p, just like that!\n", origFsctl, s_origFsctl);
        fflush(stderr);
    }
    *pFsctlImp = (uintptr_t)my_fsctl;
    fprintf(stderr, "backupd_anyfs: _fsctl hooked in %s (PID=%u)\n", _dyld_get_image_name(0), getpid());
    fflush(stderr);
}