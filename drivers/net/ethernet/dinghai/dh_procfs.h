#ifndef _ZXDH_PROC_FS_H_
#define _ZXDH_PROC_FS_H_

#define PROC_ENTRY_MAX      (16)

struct zxdh_proc_fs
{
    struct proc_dir_entry *proc_dir;
    struct proc_dir_entry *proc_entry[PROC_ENTRY_MAX];
};

struct fs_entry_desc
{
    uint32_t type;
    char *file_name;
};

enum
{
    FS_ENTRY_BOND = 0,
};

void zxdh_create_proc_dir(struct zxdh_proc_fs *procfs);
void zxdh_destroy_proc_dir(struct zxdh_proc_fs *procfs);
void zxdh_create_proc_entry(struct zxdh_proc_fs *procfs,
        uint32_t type, struct seq_operations *seq_ops, void *data);
void zxdh_remove_proc_entry(struct zxdh_proc_fs *procfs, uint32_t type);


#endif /* _ZXDH_PROC_FS_H_ */