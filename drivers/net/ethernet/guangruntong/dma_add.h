#ifndef GRTDMA_H
#define GRTDMA_H

// Target
#define TARGET_H2C          0
#define TARGET_C2H          1
#define TARGET_IRQ          2
#define TARGET_CONFIG       3

#define TARGET_MSIX         8

//TARGET_H2C & TARGET_C2H
// Writable addresses
#define ADDR_SG_SWPT      0
#define ADDR_SG_ADDRLO    1
#define ADDR_SG_ADDRHI    2
#define ADDR_SG_MAXNUM    3
#define ADDR_ENGINE_CTRL  4

#define ADDR_DESC_CTRL    5  //WTHRESH, PTHRESH, HTHRESH
#define ADDR_INT_DELAY    6  //Write back & Interrupt Delay Value TIDV RDTR

#define ADDR_SG_WBADDRLO  7
#define ADDR_SG_WBADDRHI  8

#define ADDR_DCA_RXTXCTL  9

// Readable Addresses
#define ADDR_SG_HWPT      16


//TARGET_IRQ
// Writable addresses
#define ADDR_INTR_ICS       0 //Interrupt Cause Set Register     1:This registers allows triggering an immediate interrupt by software 1:对应的中断激活
#define ADDR_INTR_IMS       1 //interrupt mask set     1:对应的中断激活，0：没影响。如果要想屏蔽中断，应该用IMC
#define ADDR_INTR_IMC       2 //interrupt mask clean   1:对应的中断禁止，0：没影响。
#define ADDR_INTR_IAM       3 //interrupt auto mask    当IAME（中断应答自动屏蔽使能）位置1时，对ICR寄存器的读或写会产生将IAM寄存器中的值写入IMC寄存器的副作用。 该位为0b时，该功能被禁用
#define ADDR_INTR_MODE      4 //中断模式c2s:s2c 前16位的4位为channel，后面的16位的2位为模式c2s:s2c 1:c2s eop interrupt 0:s2c normal interrupt

#define ADDR_INTR_ITR       5 //msix,多少个中断就有多少个,channel*2(RXTX) + 1(Other) ,32bit数据，前面16位（用了4位，总共32个中断）为vector，后面为ITR数据。如果不支持msix，那么数据就在第一个vector上面
                              //The interval is specified in 256 ns increments. Zero disables interrupt throttling logic.

#define ADD_INTR_IVAR				6
#define ADD_INTR_IVAR_MISC	7


// Readable Addresses
#define ADDR_INTR_VECTOR    8

//TARGET_CONF
#define ADDR_CORESETTINGS   0
#define ADDR_FPGA_NAME      1

#define ADDR_DCA_GTCL      	3
#define ADDR_FUNC_RST      	4

#endif /* GRTDMA_H */
