#ifndef _ZXDH_MSG_CHAN_LOCK_H_
#define _ZXDH_MSG_CHAN_LOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ARR_LEN(arr)             (sizeof(arr)/sizeof(arr[0]))

typedef void __iomem * spl_addr_t;

/* PCIEID位域掩码*/
#define PCIEID_IS_PF_MASK        (0x0800)
#define PCIEID_PF_IDX_MASK       (0x0700)
#define PCIEID_VF_IDX_MASK       (0x00ff)
#define PCIEID_EP_IDX_MASK       (0x7000)
#define PF0_PCIEID               (0x0800)

/* PCIEID位域偏移*/
#define PCIEID_PF_IDX_OFFSET     (8)
#define PCIEID_EP_IDX_OFFSET     (12)

#define MAX_HARD_SPINLOCK_NUM                (511)
#define MAX_HARD_SPINLOCK_ASK_TIMES          (1500)
#define BAR_CHAN_HARD_LOCK_POLLING_SPAN_MS   (1)



/* 硬件锁软件锁*/
#define LOCK_TYPE_HARD           1
#define LOCK_TYPE_SOFT           0

/* bar空间偏移*/
#define BAR0_CHAN_RISC_OFFSET                (0x2000)
#define BAR0_CHAN_PFVF_OFFSET                (0x3000)
#define BAR0_SPINLOCK_OFFSET                 (0x4000)

#define CHAN_RISC_SPINLOCK_OFFSET            (BAR0_SPINLOCK_OFFSET - BAR0_CHAN_RISC_OFFSET)
#define CHAN_PFVF_SPINLOCK_OFFSET            (BAR0_SPINLOCK_OFFSET - BAR0_CHAN_PFVF_OFFSET)

#define  MAX_EP_NUM         4
#define  PF_NUM_PER_EP      8
#define  VF_NUM_PER_PF      32

#define MULTIPLY_BY_8(x)   ((x) << 3)
#define MULTIPLY_BY_32(x)  ((x) << 5)
#define MULTIPLY_BY_256(x) ((x) << 8)

#define LOCK_ARR_LENGTH   (MAX_EP_NUM * PF_NUM_PER_EP * (3 + VF_NUM_PER_PF))

#define FW_SHRD_OFFSET                       (0x5000)
#define FW_SHRD_INNER_HW_LABEL_PAT           (0x800)
#define LOCK_MASTER_ID_MASK                  (0x8000)
#define HW_LABEL_OFFSET                      (FW_SHRD_OFFSET + FW_SHRD_INNER_HW_LABEL_PAT)
#define CHAN_RISC_LABEL_OFFSET               (HW_LABEL_OFFSET - BAR0_CHAN_RISC_OFFSET)
#define CHAN_PFVF_LABEL_OFFSET               (HW_LABEL_OFFSET - BAR0_CHAN_PFVF_OFFSET)


void bar_init_lock_arr(void);

int bar_chan_lock(uint8_t src, uint8_t dst, uint16_t src_pcieid, uint64_t virt_addr);
int bar_chan_unlock(uint8_t src, uint8_t dst, uint16_t src_pcieid, uint64_t virt_addr);

#ifdef __cplusplus
}
#endif

#endif /* _ZXDH_MSG_CHAN_LOCK_H_  */
