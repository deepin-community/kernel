
#include "sxe.h"
#include "sxe_pci.h"

bool sxe_check_cfg_fault(struct sxe_hw *hw, struct pci_dev *dev)
{
	u16 value;
	struct sxe_adapter *adapter = hw->adapter;

	pci_read_config_word(dev, PCI_VENDOR_ID, &value);
	if (value == SXE_READ_CFG_WORD_FAILED) {
		sxe_hw_fault_handle(hw);
		LOG_ERROR_BDF("pci vendorId:0x%x read pci config word fail,"
			  "remove adapter.\n", PCI_VENDOR_ID);
		return true;
	}

	return false;
}

u16 sxe_read_pci_cfg_word(struct pci_dev *pdev, struct sxe_hw *hw, u32 reg)
{
	u16 value = SXE_READ_CFG_WORD_FAILED;

	if (sxe_is_hw_fault(hw)) {
		goto l_end;
	}

	pci_read_config_word(pdev, reg, &value);
	if (SXE_READ_CFG_WORD_FAILED == value) {
		sxe_check_cfg_fault(hw, pdev);
	}

l_end:
	return value;
}


#ifdef CONFIG_PCI_IOV
u32 sxe_read_pci_cfg_dword(struct sxe_adapter *adapter, u32 reg)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 value = SXE_FAILED_READ_CFG_DWORD;

	if (sxe_is_hw_fault(hw)) {
		goto l_end;
	}

	pci_read_config_dword(adapter->pdev, reg, &value);
	if (SXE_FAILED_READ_CFG_DWORD == value) {
		sxe_check_cfg_fault(hw, adapter->pdev);
	}

l_end:
	return value;
}
#endif 

u32 sxe_pcie_timeout_poll(struct pci_dev *pdev, struct sxe_hw *hw)
{
	u16 devctl2;
	u32 pollcnt;

	devctl2 = sxe_read_pci_cfg_word(pdev, hw, SXE_PCI_DEVICE_CONTROL2);
	devctl2 &= SXE_PCIDEVCTRL2_TIMEO_MASK;

	switch (devctl2) {
	case SXE_PCIDEVCTRL2_65_130ms:
		pollcnt = 1300;         
		break;
	case SXE_PCIDEVCTRL2_260_520ms:
		pollcnt = 5200;         
		break;
	case SXE_PCIDEVCTRL2_1_2s:
		pollcnt = 20000;        
		break;
	case SXE_PCIDEVCTRL2_4_8s:
		pollcnt = 80000;        
		break;
	case SXE_PCIDEVCTRL2_17_34s:
		pollcnt = 34000;        
		break;
	case SXE_PCIDEVCTRL2_50_100us:        
	case SXE_PCIDEVCTRL2_1_2ms:           
	case SXE_PCIDEVCTRL2_16_32ms:         
	case SXE_PCIDEVCTRL2_16_32ms_def:     
	default:
		pollcnt = 800;          
		break;
	}

	return (pollcnt * 11) / 10;
}

unsigned long sxe_get_completion_timeout(struct sxe_adapter *adapter)
{
	u16 devctl2;

	pcie_capability_read_word(adapter->pdev, PCI_EXP_DEVCTL2, &devctl2);

	switch (devctl2 & SXE_PCIDEVCTRL2_TIMEO_MASK) {
	case SXE_PCIDEVCTRL2_17_34s:
	case SXE_PCIDEVCTRL2_4_8s:
	case SXE_PCIDEVCTRL2_1_2s:
		return 2000000ul;	
	case SXE_PCIDEVCTRL2_260_520ms:
		return 520000ul;	
	case SXE_PCIDEVCTRL2_65_130ms:
		return 130000ul;	
	case SXE_PCIDEVCTRL2_16_32ms:
		return 32000ul;		
	case SXE_PCIDEVCTRL2_1_2ms:
		return 2000ul;		
	case SXE_PCIDEVCTRL2_50_100us:
		return 100ul;		
	case SXE_PCIDEVCTRL2_16_32ms_def:
		return 32000ul;		
	default:
		break;
	}

	return 32000ul;
}

