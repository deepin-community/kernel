#include "grtnic.h"
#include "grtnic_nvm.h"

/////////////////////////////////////////////////////////////////////////////////////////////
int erase_sector_flash(struct grtnic_adapter *adapter, u32 offset) //erase 0x10000(64k) every time
{
	struct grtnic_hw *hw = &adapter->hw;
	int status = 0;

	writel( (SPI_CMD_ADDR(offset) | SPI_CMD_CMD(SECTOR_ERASE_CMD)), hw->user_bar + SPI_CMD);

	status = po32m(hw->user_bar, SPI_STATUS,
		SPI_STATUS_OPDONE, SPI_STATUS_OPDONE,
		SPI_ERASE_TIMEOUT, 0);

	if (status) {
		printk("FLASH erase timed out\n");
	}

	return status;
}


int erase_subsector_flash(struct grtnic_adapter *adapter, u32 offset) //erase 0x1000(4k) every time
{
	struct grtnic_hw *hw = &adapter->hw;
	int status = 0;

	writel( (SPI_CMD_ADDR(offset) | SPI_CMD_CMD(SUBSECTOR_ERASE_CMD)), hw->user_bar + SPI_CMD);

	status = po32m(hw->user_bar, SPI_STATUS, SPI_STATUS_OPDONE, SPI_STATUS_OPDONE, SPI_ERASE_TIMEOUT, 0);
	if (status) {
		printk("FLASH erase timed out\n");
	}

	return status;
}

/**
 *  ngbe_read_flash_buffer - Read FLASH dword(s) using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  dword in EEPROM to read
 *  @dwords: number of dwords
 *  @data: dword(s) read from the EEPROM
 *
 *  Retrieves 32 bit dword(s) read from EEPROM
 **/
int write_flash_buffer(struct grtnic_adapter *adapter, u32 offset, u32 dwords, u32 *data)
{
	struct grtnic_hw *hw = &adapter->hw;
	int status = 0;
	u32 i;

	for (i = 0; i < dwords; i++) {
		writel(be32(data[i]), hw->user_bar + SPI_DATA);
		writel( (SPI_CMD_ADDR(offset + (i << 2)) | SPI_CMD_CMD(PAGE_PROG_CMD)), hw->user_bar + SPI_CMD);

		status = po32m(hw->user_bar, SPI_STATUS,
			SPI_STATUS_OPDONE, SPI_STATUS_OPDONE,
			SPI_TIMEOUT, 0);
		if (status) {
			printk("FLASH write timed out\n");
			break;
		}
	}

	return status;
}

/**
 *  ngbe_write_flash_buffer - Write FLASH dword(s) using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  dword in EEPROM to write
 *  @dwords: number of dwords
 *  @data: dword(s) write from to EEPROM
 *
 **/
int read_flash_buffer(struct grtnic_adapter *adapter, u32 offset, u32 dwords, u32 *data)
{
	struct grtnic_hw *hw = &adapter->hw;
	int status = 0;
	u32 i;

	for (i = 0; i < dwords; i++) {
		writel( (SPI_CMD_ADDR(offset + (i << 2)) | SPI_CMD_CMD(PAGE_READ_CMD)), hw->user_bar + SPI_CMD);

		status = po32m(hw->user_bar, SPI_STATUS,
			SPI_DATA_OP_DONE, SPI_DATA_OP_DONE,
			SPI_TIMEOUT, 0);
		if (status != 0) {
			printk("FLASH read timed out\n");
			break;
		}

    data[i] = readl(hw->user_bar + SPI_DATA);
	}

	return status;
}

void write_flash_macaddr(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
  u32 *temp;

	int firmware_offset = adapter->speed;
	int port = adapter->func;
	u32 offset = VPD_OFFSET - (firmware_offset * 0x100000);

  temp = vmalloc(FLASH_SUBSECTOR_SIZE);
  memset(temp, 0x00, FLASH_SUBSECTOR_SIZE);

	read_flash_buffer(adapter, offset, FLASH_SUBSECTOR_SIZE>>2, temp); //subsector is 4K
	erase_subsector_flash(adapter, offset);

	temp[(MAC_ADDR_OFFSET>>2) + port*2] 		=  (netdev->dev_addr[2] << 24 | netdev->dev_addr[3] << 16 | netdev->dev_addr[4] << 8 | netdev->dev_addr[5]);
	temp[(MAC_ADDR_OFFSET>>2) + port*2+1] 	=  (netdev->dev_addr[0] << 8 | netdev->dev_addr[1]);

	write_flash_buffer(adapter, offset, FLASH_SUBSECTOR_SIZE>>2, temp);
  vfree(temp);
}
