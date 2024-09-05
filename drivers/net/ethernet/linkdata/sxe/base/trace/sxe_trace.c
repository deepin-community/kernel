
#ifdef SXE_DRIVER_TRACE

#include <linux/device.h>
#include <linux/netdevice.h>

#include "sxe_trace.h"
#include "sxe_ring.h"
#include "sxe_log.h"

#define SXE_FILE_NAME_LEN (256)
#define SXE_TRACE_NS_MASK (0xFFFFFFFF)

#define SXE_TRACE_BUF_CLEAN(buf, buf_size, len) \
            memset(buf, 0, buf_size); \
            len = 0;

struct sxe_trace_tx_ring g_sxe_trace_tx[SXE_TXRX_RING_NUM_MAX] = {{ 0 }};
struct sxe_trace_rx_ring g_sxe_trace_rx[SXE_TXRX_RING_NUM_MAX] = {{ 0 }};

void sxe_file_close(struct file **file)
{
    filp_close(*file, NULL);
    *file = NULL;
}

void sxe_trace_tx_add(u8 ring_idx, enum sxe_trace_lab_tx lab)
{
    if (unlikely(ring_idx >= SXE_TXRX_RING_NUM_MAX) || unlikely(lab >= SXE_TRACE_LAB_TX_MAX)) {
        return;
    }

    if (unlikely(lab == 0)) {
        g_sxe_trace_tx[ring_idx].next++;
        g_sxe_trace_tx[ring_idx].next &= SXE_TRACE_PER_RING_MASK;
        memset(&g_sxe_trace_tx[ring_idx].timestamp[g_sxe_trace_tx[ring_idx].next], 0,
               sizeof(g_sxe_trace_tx[ring_idx].timestamp[0]));
    }

    g_sxe_trace_tx[ring_idx].timestamp[g_sxe_trace_tx[ring_idx].next][lab] = ktime_get_real_ns() & SXE_TRACE_NS_MASK;
}

void sxe_trace_rx_add(u8 ring_idx, enum sxe_trace_lab_rx lab)
{
    if (unlikely(ring_idx >= SXE_TXRX_RING_NUM_MAX) || unlikely(lab >= SXE_TRACE_LAB_RX_MAX)) {
        return;
    }

    if (unlikely(lab == 0)) {
        g_sxe_trace_rx[ring_idx].next++;
        g_sxe_trace_rx[ring_idx].next &= SXE_TRACE_PER_RING_MASK;
        memset(&g_sxe_trace_rx[ring_idx].timestamp[g_sxe_trace_rx[ring_idx].next], 0,
               sizeof(g_sxe_trace_rx[ring_idx].timestamp[0]));
    }

    g_sxe_trace_rx[ring_idx].timestamp[g_sxe_trace_rx[ring_idx].next][lab] = ktime_get_real_ns() & SXE_TRACE_NS_MASK;
}

static int sxe_trace_create_file(struct file **pp_file)
{
    char         file_name[SXE_FILE_NAME_LEN] = {};
    int          flags_new                    = O_CREAT | O_RDWR | O_APPEND | O_LARGEFILE;
    int          len                          = 0;
    int          rc                           = 0;
    struct file *file;

    len += snprintf(file_name, sizeof(file_name), "%s.", SXE_TRACE_DUMP_FILE_NAME);
    time_for_file_name(file_name + len, sizeof(file_name) - len);

    file = filp_open(file_name, flags_new, 0666);
    if (IS_ERR(file)) {
        rc = (int)PTR_ERR(file);
        sxe_print(KERN_ERR, NULL, "open file:%s failed[errno:%d]\n", file_name, rc);
        goto l_out;
    }
    *pp_file = file;

l_out:
    return rc;
}

static int sxe_trace_write_file(struct file *file)
{
    char * buff;
    size_t buff_size   = 2048;
    int    rc 		   = 0;
    int    len         = 0;
    u64    spend       = 0;
    u64    times       = 0;
    u64    spend_total = 0;
    u64    times_total = 0;
    u64    start;
    u64    end;
    u32    i;
    u32    j;
    u32    k;

    buff = kzalloc(buff_size, GFP_KERNEL);
    if (buff == NULL) {
        rc = -ENOMEM;
        sxe_print(KERN_ERR, NULL, "kzalloc %lu failed.\n", buff_size);
        goto l_out;
    }

    len += snprintf(buff + len, buff_size - len, "tx trace dump:\n");
    rc = sxe_file_write(file, buff, len);
    if (rc < 0) {
        goto l_out;
    }
    for (i = 0; i < ARRAY_SIZE(g_sxe_trace_tx); i++) {
        spend = 0;
        times = 0;
        for (j = 0; j < SXE_TRACE_NUM_PER_RING; j++) {
			start = g_sxe_trace_tx[i].timestamp[j][SXE_TRACE_LAB_TX_START];
			end = g_sxe_trace_tx[i].timestamp[j][SXE_TRACE_LAB_TX_END];
            if (start == 0 || end == 0) {
                continue;
            }
			SXE_TRACE_BUF_CLEAN(buff, buff_size, len);
            len += snprintf(buff + len, buff_size - len, "\ttx ring %d trace %d dump:", i, j);
            for (k = 0; k < SXE_TRACE_LAB_TX_MAX; k++) {
                len += snprintf(buff + len, buff_size - len, "%llu ", g_sxe_trace_tx[i].timestamp[j][k]);
            }
            len += snprintf(buff + len, buff_size - len, "spend: %llu\n", end - start);
            rc = sxe_file_write(file, buff, len);
            if (rc < 0) {
                goto l_out;
            }
            spend += end - start;
            times++;
        }

		SXE_TRACE_BUF_CLEAN(buff, buff_size, len);
        len += snprintf(buff + len, buff_size - len, "tx ring %d, spend %llu, times:%llu.\n", i, spend, times);
        spend_total += spend;
        times_total += times;
        rc    = sxe_file_write(file, buff, len);
        if (rc < 0) {
            goto l_out;
        }
    }

	SXE_TRACE_BUF_CLEAN(buff, buff_size, len);
    len += snprintf(buff + len, buff_size - len, "tx trace dump, spend_total: %llu, times_total: %llu.\n",
                    spend_total, times_total);

    len += snprintf(buff + len, buff_size - len, "rx trace dump:\n");
    rc = sxe_file_write(file, buff, len);
    if (rc < 0) {
        goto l_out;
    }
    spend_total = 0;
    times_total = 0;
    for (i = 0; i < ARRAY_SIZE(g_sxe_trace_rx); i++) {
        spend = 0;
        times = 0;
        for (j = 0; j < SXE_TRACE_NUM_PER_RING; j++) {
			start = g_sxe_trace_rx[i].timestamp[j][SXE_TRACE_LAB_RX_START];
			end = g_sxe_trace_rx[i].timestamp[j][SXE_TRACE_LAB_RX_END];
            if (start == 0 || end == 0) {
                continue;
            }
			SXE_TRACE_BUF_CLEAN(buff, buff_size, len);
            len += snprintf(buff + len, buff_size - len, "\trx ring %d trace %d dump:", i, j);
            for (k = 0; k < SXE_TRACE_LAB_RX_MAX; k++) {
                len += snprintf(buff + len, buff_size - len, "%llu ", g_sxe_trace_rx[i].timestamp[j][k]);
            }
            len += snprintf(buff + len, buff_size - len, "spend: %llu\n", end - start);
            rc = sxe_file_write(file, buff, len);
            if (rc < 0) {
                goto l_out;
            }
            spend += end - start;
            times++;
        }
		SXE_TRACE_BUF_CLEAN(buff, buff_size, len);
        len += snprintf(buff + len, buff_size - len, "rx ring %d, spend %llu, times:%llu:\n", i, spend, times);
        spend_total += spend;
        times_total += times;
        rc    = sxe_file_write(file, buff, len);
        if (rc < 0) {
            goto l_out;
        }
    }

	SXE_TRACE_BUF_CLEAN(buff, buff_size, len);
    len += snprintf(buff + len, buff_size - len, "rx trace dump, spend_total: %llu, times_total: %llu.\n",
                    spend_total, times_total);
    rc = sxe_file_write(file, buff, len);
    if (rc < 0) {
        goto l_out;
    }

l_out:
    if (buff) {
        kfree(buff);
    }
    if (rc < 0) {
        sxe_print(KERN_ERR, NULL, "write file error %d\n", rc);
    }
    return rc;
}

void sxe_trace_dump(void)
{
    struct file *file;
    int          rc = 0;

    rc = sxe_trace_create_file(&file);
    if (file == NULL) {
        goto l_out;
    }

    rc = sxe_trace_write_file(file);
    if (rc < 0) {
        goto l_out;
    }

l_out:
    if (file) {
        sxe_file_close(&file);
    }
    return;
}

#endif 
