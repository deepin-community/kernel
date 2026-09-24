#include <linux/proc_fs.h>

#include "dh_procfs.h"

#define DRV_NAME "dinghai"

static struct fs_entry_desc fs_entry_table[] = {
    { FS_ENTRY_BOND, "lag" },
    {0}
};

uint32_t find_fs_entry(uint32_t type)
{
    uint32_t idx = 0;

    while (fs_entry_table[idx].file_name)
    {
        if (type == fs_entry_table[idx].type)
        {
            break;
        }
        idx++;
    }

    return idx;
}

void zxdh_create_proc_dir(struct zxdh_proc_fs *procfs)
{
    if (!procfs->proc_dir)
    {
        procfs->proc_dir = proc_mkdir(DRV_NAME, NULL);
        if (!procfs->proc_dir)
        {
            pr_warn("Warning: Cannot create /proc/%s\n", DRV_NAME);
        }
    }
}

void zxdh_destroy_proc_dir(struct zxdh_proc_fs *procfs)
{
    if (procfs->proc_dir)
    {
        remove_proc_entry(DRV_NAME, NULL);
        procfs->proc_dir = NULL;
    }
}

#ifndef CGS_V5_693
void zxdh_create_proc_entry(struct zxdh_proc_fs *procfs,
        uint32_t type, struct seq_operations *seq_ops, void *data)
{
    uint32_t idx = 0;
    char *file_name = NULL;

    if (procfs->proc_dir)
    {
        idx = find_fs_entry(type);
        file_name = fs_entry_table[idx].file_name;
        if (file_name && (idx < PROC_ENTRY_MAX))
        {
            procfs->proc_entry[idx] = proc_create_seq_data(file_name, 0444, procfs->proc_dir, seq_ops, data);
            if (procfs->proc_entry[idx] == NULL)
            {
                pr_info("Cannot create /proc/%s/%s\n", DRV_NAME, file_name);
            }
        }
    }
}
#endif

void zxdh_remove_proc_entry(struct zxdh_proc_fs *procfs, uint32_t type)
{
    uint32_t idx = 0;
    char *file_name = NULL;

    if (procfs->proc_dir)
    {
        idx = find_fs_entry(type);
        file_name = fs_entry_table[idx].file_name;
        if (file_name && (idx < PROC_ENTRY_MAX))
        {
            remove_proc_entry(file_name, procfs->proc_dir);
            procfs->proc_entry[idx] = NULL;
        }
    }
}
