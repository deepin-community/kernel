#ifndef GRTNIC_NVM_H
#define GRTNIC_NVM_H

#define be32(x) ((x<<24 & 0xff000000) | (x<<8 & 0x00ff0000) | (x>>8 & 0x0000ff00) | (x>>24 & 0x000000ff))
#define be16(x) ((x<<8 & 0xff00) | (x>>8 & 0x00ff))

//////////////////////////////////////////////////
#define PAGE_READ_CMD         0x00
#define SECTOR_ERASE_CMD      0x01
#define SUBSECTOR_ERASE_CMD   0x02
#define PAGE_PROG_CMD         0x03


#define MAX_FLASH_LOAD_POLL_TIME  10

#define SPI_CMD                   0x0400
#define SPI_CMD_CMD(_v)           (((_v) & 0x7) << 28)
#define SPI_CMD_ADDR(_v)          (((_v) & 0xFFFFFF))

#define SPI_DATA                  0x0404

#define SPI_STATUS                0x0408
#define SPI_STATUS_OPDONE         ((0x1))
#define SPI_DATA_OP_DONE          ((0x2))

#define SPI_ERASE_TIMEOUT  2000000
#define SPI_TIMEOUT  20000

////////////////////////////////////////////////////////////////
#define VPD_OFFSET 			0xEFF000
#define MAC_ADDR_OFFSET 0x100
#define VERSION_OFFSET 	0x200

#define PXE_OFFSET 			0xE00000

#define FLASH_SECTOR_SIZE 0x10000 //64k
#define FLASH_SUBSECTOR_SIZE 0x1000 //4k
////////////////////////////////////////////////////////////////

static inline int po32m(uint8_t* hw, u32 reg, u32 mask, u32 field, int usecs, int count)
{
  int loop;

  loop = (count ? count : (usecs + 9) / 10);
  usecs = (loop ? (usecs + loop - 1) / loop : 0);

  count = loop;
//  printf("loop = %d, usecs = %d\n", loop, usecs);
  do {
    u32 value = readl(hw + reg);

    if ((value & mask) == (field & mask)) {
      break;
    }

    if (loop-- <= 0)
      break;

//    udelay(20);
    usleep_range(20,20);
  } while (1);

  return (count - loop <= count ? 0 : 1);
}

int erase_sector_flash(struct grtnic_adapter *adapter, u32 offset);
int erase_subsector_flash(struct grtnic_adapter *adapter, u32 offset);
int write_flash_buffer(struct grtnic_adapter *adapter, u32 offset, u32 dwords, u32 *data);
int read_flash_buffer(struct grtnic_adapter *adapter, u32 offset, u32 dwords, u32 *data);
void write_flash_macaddr(struct net_device *netdev);

#endif /* GRTNIC_NVM_H */
