//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#include "os_interface.h"

#define GF_TRACK_OK    0
#define GF_TRACK_FAIL  1

typedef enum
{
    GF_TYPE_MALLOC_TRACK       = 0x01,
    GF_TYPE_ALLOC_PAGES_TRACK  = 0x02,
    GF_TYPE_MAP_PAGES_TRACK    = 0x03,
    GF_TYPE_MAP_IO_TRACK       = 0x04,
    GF_TYPE_MEM_TRACK_LAST     = 0x05,

}gf_mem_track_type;

typedef struct __gf_mem_track* gf_mem_track_ptr;

typedef struct __gf_mem_track
{
   gf_mem_track_ptr next;
   void*             addr;
   const char*       file;
   unsigned int      size;
   unsigned int      line;
   unsigned int      times;
}gf_mem_track;

static struct os_mutex *memory_track_lock = NULL;
static gf_mem_track gf_memory_tracks[GF_TYPE_MEM_TRACK_LAST-1] = {{0},};

static inline void gf_mem_track_add(gf_mem_track_type type, void* addr, const char* file, int line, unsigned long size)
{
    gf_mem_track *pMemList  = &gf_memory_tracks[type - 1];
    gf_mem_track *pMemTrack = NULL;

    if(addr == NULL)
    {
        return;
    }

    pMemTrack = gf_malloc_priv(sizeof(gf_mem_track));

    if(pMemTrack == NULL)
    {
        gf_error("malloc struct gf_mem_track failed.\n");
        return;
    }

    if(memory_track_lock != NULL)
    {
        gf_mutex_lock(memory_track_lock);
    }

    pMemTrack->addr = addr;
    pMemTrack->file = file;
    pMemTrack->line = line;
    pMemTrack->size = size;
    pMemTrack->next = pMemList->next;
    pMemTrack->times = pMemList->times++;
    

    pMemList->next  = pMemTrack;
    pMemList->size++;

    if(memory_track_lock != NULL)
    {
        gf_mutex_unlock(memory_track_lock);
    }
}

static inline int gf_mem_track_remove(gf_mem_track_type type, int check_overflow, void* addr, const char* file, int line)
{
    gf_mem_track *pMemList  = &gf_memory_tracks[type - 1];
    gf_mem_track *pTemp, *pPre;
    int status = GF_TRACK_OK;

    gf_mutex_lock(memory_track_lock);

    pPre  = pMemList;
    pTemp = pMemList->next;

    while((pTemp != NULL) && (pTemp->addr != addr))
    {
        pPre  = pTemp;
        pTemp = pTemp->next;
    }

    if(pTemp != NULL)
    {
        if(check_overflow)
        {
            char *head = (char*)pTemp->addr;
            char *tail = (char*)pTemp->addr + pTemp->size - 32;
            int  i;

            for(i=0; i < 32; i++)
            {
                if(head[i] != 0x55 || tail[i] != 0x55)
                {
                    gf_info("OVERFLOW %s: %d, size = %x, times = %d.\n", 
                             pTemp->file, pTemp->line, pTemp->size, pTemp->times);
                    break;
                }
            }
            gf_memset(pTemp->addr, 0, pTemp->size);
        }
        pPre->next = pTemp->next;
        gf_free_priv(pTemp);
      
        pMemList->size--;
    }
    else
    {
        gf_info("type:%d-memory:0x%p: %s line:%d, no found in track list.\n", pMemList->line, addr, file, line);
        status = GF_TRACK_FAIL;
    }
    gf_mutex_unlock(memory_track_lock);

    return status;
}

static inline void gf_mem_track_list(gf_mem_track* pMemList, struct os_file *file)
{
    char msg[512];
    gf_mem_track *pList = pMemList->next;
    
    gf_info("memory type: %d, leaks number: %d\n", pMemList->line, pMemList->size);

    if(file != NULL)
    {
        gf_vsnprintf(msg, 512, "memory type: %d, leaks number: %d\n", pMemList->line, pMemList->size);
        gf_file_write(file, msg, gf_strlen(msg));
    }

    while(pList)
    {
        gf_info("%s: %d, size = %u, times = %d.\n", pList->file, pList->line, pList->size, pList->times);
        if(file != NULL)
        {
            gf_vsnprintf(msg, 512, "%s: %d, size = %u, times = %d.\n", pList->file, pList->line, pList->size, pList->times);
            gf_file_write(file, msg, gf_strlen(msg));
        }
        pList = pList->next;
    }

}

void gf_mem_track_init(void)
{
    gf_mem_track_type type;
    gf_mem_track  *header = NULL;

    memory_track_lock = gf_create_mutex();

    for(type = GF_TYPE_MALLOC_TRACK; type < GF_TYPE_MEM_TRACK_LAST; type++)
    {
        header = &gf_memory_tracks[type-1];
        gf_memset(header, 0, sizeof(*header));
        header->line = type;
    }
}

void gf_mem_track_list_result(void)
{
    gf_mem_track_type type;
    gf_mem_track  *header = NULL;
    struct os_file *file = NULL;
#if GF_MEM_TRACK_RESULT_TO_FILE
    file = gf_file_open(gf_mem_track_result_path, OS_WRONLY | OS_CREAT | OS_APPEND,  0644);
#endif

    gf_info("************leak************\n");

    for(type = GF_TYPE_MALLOC_TRACK; type < GF_TYPE_MEM_TRACK_LAST; type++)
    {
        header = &gf_memory_tracks[type-1];

        if(header->next)
        {
            gf_mem_track_list(header, file);
        }
    }

#if GF_MEM_TRACK_RESULT_TO_FILE
    gf_file_close(file);
#endif

    gf_destroy_mutex(memory_track_lock);

    memory_track_lock = NULL;
}

void *gf_malloc_track(unsigned long size, const char *file, unsigned int line)
{
    unsigned long track_size     = size;
    void          *returned_addr = NULL;
    char          *malloc_addr   = NULL;

#if GF_CHECK_MALLOC_OVERFLOW
    track_size += 2 * 32; 
#endif

    malloc_addr = gf_malloc_priv(track_size);

#if GF_CHECK_MALLOC_OVERFLOW
    gf_memset(malloc_addr, 0x55, 32);
    gf_memset(malloc_addr + track_size - 32, 0x55, 32);
    returned_addr = malloc_addr + 32;
#else
    returned_addr = malloc_addr;
#endif

    gf_mem_track_add(GF_TYPE_MALLOC_TRACK, malloc_addr, file, line, track_size);

    return returned_addr;
}

void *gf_calloc_track(unsigned long size, const char *file, unsigned int line)
{
    unsigned long track_size     = size;
    void          *returned_addr = NULL;
    char          *malloc_addr   = NULL;

#if GF_CHECK_MALLOC_OVERFLOW
    track_size += 2 * 32; 
#endif

    malloc_addr = gf_calloc_priv(track_size);

#if GF_CHECK_MALLOC_OVERFLOW
    gf_memset(malloc_addr, 0x55, 32);
    gf_memset(malloc_addr + track_size - 32, 0x55, 32);
    returned_addr = malloc_addr + 32;
#else
    returned_addr = malloc_addr;
#endif

    gf_mem_track_add(GF_TYPE_MALLOC_TRACK, malloc_addr, file, line, track_size);

    return returned_addr;
}

void gf_free_track(void *addr, const char *file, unsigned int line)
{
    void *malloc_addr   = addr;
    int  overflow_check = 0;
#if GF_CHECK_MALLOC_OVERFLOW
    overflow_check = 1;
    malloc_addr    = (char*)addr - 32;
#endif
    if(addr == NULL)
    {
        gf_error("free a NULL pointer: %s, %d.\n", file, line);
        return;
    }

    if(gf_mem_track_remove(GF_TYPE_MALLOC_TRACK, overflow_check, malloc_addr, file, line) == GF_TRACK_OK)
    {
        gf_free_priv(malloc_addr);
    }
}

struct os_pages_memory * 
gf_allocate_pages_memory_track(void *pdev, int size, int page_size, alloc_pages_flags_t alloc_flags, const char *file, unsigned int line)
{
    struct os_pages_memory *memory = NULL;

    memory = gf_allocate_pages_memory_priv(pdev, size, page_size, alloc_flags);

    gf_mem_track_add(GF_TYPE_ALLOC_PAGES_TRACK, memory, file, line, size);

    return memory;
}

void gf_free_pages_memory_track(void *pdev, struct os_pages_memory *memory, const char *file, unsigned int line)
{
    if(memory == NULL)
    {
        gf_error("free a NULL pages memory: %s, %d.\n", file, line);
        return;
    }
    gf_mem_track_remove(GF_TYPE_ALLOC_PAGES_TRACK, 0, memory, file, line);
    gf_free_pages_memory_priv(pdev, memory);
}

gf_vm_area_t *gf_map_pages_memory_track(void *process_context, gf_map_argu_t *map,
                                      const char *file, unsigned int line)
{
    gf_vm_area_t *vm_area = gf_map_pages_memory_priv(process_context, map);

    gf_mem_track_add(GF_TYPE_MAP_PAGES_TRACK, vm_area->virt_addr, file, line, vm_area->size);

    return vm_area;
}

void gf_unmap_pages_memory_track(gf_vm_area_t *vm_area,
                                  const char *file, unsigned int line)
{
    if(vm_area->virt_addr == NULL)
    {
        gf_error("unmap a NULL pages memory: %s, %d.\n", file, line);
        return;
    }
    gf_mem_track_remove(GF_TYPE_MAP_PAGES_TRACK, 0, vm_area->virt_addr, file, line);
    gf_unmap_pages_memory_priv(vm_area);
}

gf_vm_area_t *gf_map_io_memory_track(void *process_context, gf_map_argu_t *map,
                              const char *file, unsigned int line)
{
    gf_vm_area_t *vm_area = gf_map_io_memory_priv(process_context, map);

    gf_mem_track_add(GF_TYPE_MAP_IO_TRACK, vm_area->virt_addr, file, line, vm_area->size);

    return vm_area;
}

void gf_unmap_io_memory_track(gf_vm_area_t *vm_area,
                               const char *file, unsigned int line)
{
    if(vm_area->virt_addr == NULL)
    {
        gf_error("unmap a NULL io memory: %s, %d.\n", file, line);
        return;
    }
    gf_mem_track_remove(GF_TYPE_MAP_IO_TRACK, 0, vm_area->virt_addr, file, line);
    gf_unmap_io_memory_priv(vm_area);
}
