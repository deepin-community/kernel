#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/printk.h>

#include "dh_procfs.h"
#include "lag.h"

void *lag_info_seq_start(struct seq_file *seq, loff_t *pos)
{
    if (*pos == 0)
    {
        return SEQ_START_TOKEN;
    }

    return NULL;
}

void *lag_info_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
#if 0
    struct test *tst = PDE_DATA(file_inode(seq->file));

    ++*pos;
    if (v == SEQ_START_TOKEN)
        pr_info("%s first *pos = %u\n", __FUNCTION__, (uint32_t)*pos);

    pr_info("%s *pos = %u\n", __FUNCTION__, (uint32_t)*pos);
    if (*pos < 5)
    {
        return tst;
    }
#endif
    return NULL;
}

void lag_info_seq_stop(struct seq_file *seq, void *v)
{
    pr_info("%s \n", __FUNCTION__);
}

int lag_info_seq_show(struct seq_file *seq, void *v)
{
#if 0
    struct zxdh_lag *lag = PDE_DATA(file_inode(seq->file));

    if (v == SEQ_START_TOKEN)
    {
        seq_printf(seq, "Port Mode: %u", tst->port);
        seq_printf(seq, "Num  Mode: %u", tst->age);
        seq_printf(seq, "\n");
    }
#endif
    return 0;
}

struct seq_operations lag_info_seq_ops =
{
    .start = lag_info_seq_start,
    .next  = lag_info_seq_next,
    .stop  = lag_info_seq_stop,
    .show  = lag_info_seq_show,
};