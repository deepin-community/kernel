// SPDX-License-Identifier: GPL-2.0
#include <linux/delay.h>
#include <linux/pci.h>
#include "ngbe_pcierr.h"
#include "ngbe.h"
#define NGBE_ROOT_PORT_INTR_ON_MESG_MASK                          \
	(PCI_ERR_ROOT_CMD_COR_EN | PCI_ERR_ROOT_CMD_NONFATAL_EN | \
	 PCI_ERR_ROOT_CMD_FATAL_EN)

static const char *aer_correctable_error_string[16] = {
	"RxErr", /* Bit Position 0	*/
	NULL,	       NULL,	 NULL, NULL,
	NULL,	       "BadTLP", /* Bit Position 6	*/
	"BadDLLP", /* Bit Position 7	*/
	"Rollover", /* Bit Position 8	*/
	NULL,	       NULL,	 NULL, "Timeout", /* Bit Position 12	*/
	"NonFatalErr", /* Bit Position 13	*/
	"CorrIntErr", /* Bit Position 14	*/
	"HeaderOF", /* Bit Position 15	*/
};

static const char *aer_uncorrectable_error_string[27] = {
	"Undefined", /* Bit Position 0	*/
	NULL,
	NULL,
	NULL,
	"DLP", /* Bit Position 4	*/
	"SDES", /* Bit Position 5	*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"TLP", /* Bit Position 12	*/
	"FCP", /* Bit Position 13	*/
	"CmpltTO", /* Bit Position 14	*/
	"CmpltAbrt", /* Bit Position 15	*/
	"UnxCmplt", /* Bit Position 16	*/
	"RxOF", /* Bit Position 17	*/
	"MalfTLP", /* Bit Position 18	*/
	"ECRC", /* Bit Position 19	*/
	"UnsupReq", /* Bit Position 20	*/
	"ACSViol", /* Bit Position 21	*/
	"UncorrIntErr", /* Bit Position 22	*/
	"BlockedTLP", /* Bit Position 23	*/
	"AtomicOpBlocked", /* Bit Position 24	*/
	"TLPBlockedErr", /* Bit Position 25	*/
	"PoisonTLPBlocked", /* Bit Position 26	*/
};

static pci_ers_result_t merge_result(enum pci_ers_result orig,
				     enum pci_ers_result new)
{
	if (new == PCI_ERS_RESULT_NO_AER_DRIVER)
		return PCI_ERS_RESULT_NO_AER_DRIVER;
	if (new == PCI_ERS_RESULT_NONE)
		return orig;
	switch (orig) {
	case PCI_ERS_RESULT_CAN_RECOVER:
	case PCI_ERS_RESULT_RECOVERED:
		orig = new;
		break;
	case PCI_ERS_RESULT_DISCONNECT:
		if (new == PCI_ERS_RESULT_NEED_RESET)
			orig = PCI_ERS_RESULT_NEED_RESET;
		break;
	default:
		break;
	}
	return orig;
}

static int ngbe_report_error_detected(struct pci_dev *dev,
				      pci_channel_state_t state,
				      enum pci_ers_result *result)
{
	pci_ers_result_t vote;
	const struct pci_error_handlers *err_handler;

	device_lock(&dev->dev);
	if (!dev->driver || !dev->driver->err_handler ||
	    !dev->driver->err_handler->error_detected) {
		/* If any device in the subtree does not have an error_detected
		 * callback, PCI_ERS_RESULT_NO_AER_DRIVER prevents subsequent
		 * error callbacks of "any" device in the subtree, and will
		 * exit in the disconnected error state.
		 */
		if (dev->hdr_type != PCI_HEADER_TYPE_BRIDGE)
			vote = PCI_ERS_RESULT_NO_AER_DRIVER;
		else
			vote = PCI_ERS_RESULT_NONE;
	} else {
		err_handler = dev->driver->err_handler;
		vote = err_handler->error_detected(dev, state);
	}

	*result = merge_result(*result, vote);
	device_unlock(&dev->dev);
	return 0;
}

static int ngbe_report_frozen_detected(struct pci_dev *dev, void *data)
{
	return ngbe_report_error_detected(dev, pci_channel_io_frozen, data);
}

static int ngbe_report_mmio_enabled(struct pci_dev *dev, void *data)
{
	pci_ers_result_t vote, *result = data;
	const struct pci_error_handlers *err_handler;

	device_lock(&dev->dev);
	if (!dev->driver || !dev->driver->err_handler ||
	    !dev->driver->err_handler->mmio_enabled)
		goto out;

	err_handler = dev->driver->err_handler;
	vote = err_handler->mmio_enabled(dev);
	*result = merge_result(*result, vote);
out:
	device_unlock(&dev->dev);
	return 0;
}

static int ngbe_report_slot_reset(struct pci_dev *dev, void *data)
{
	pci_ers_result_t vote, *result = data;
	const struct pci_error_handlers *err_handler;

	device_lock(&dev->dev);
	if (!dev->driver || !dev->driver->err_handler ||
	    !dev->driver->err_handler->slot_reset)
		goto out;

	err_handler = dev->driver->err_handler;
	vote = err_handler->slot_reset(dev);
	*result = merge_result(*result, vote);
out:
	device_unlock(&dev->dev);
	return 0;
}

static int ngbe_report_resume(struct pci_dev *dev, void *data)
{
	const struct pci_error_handlers *err_handler;

	device_lock(&dev->dev);
	dev->error_state = pci_channel_io_normal;
	if (!dev->driver || !dev->driver->err_handler ||
	    !dev->driver->err_handler->resume)
		goto out;

	err_handler = dev->driver->err_handler;
	err_handler->resume(dev);
out:
	device_unlock(&dev->dev);
	return 0;
}

void ngbe_pcie_do_recovery(struct pci_dev *dev)
{
	pci_ers_result_t status = PCI_ERS_RESULT_CAN_RECOVER;
	struct pci_bus *bus;
	u32 reg32;
	int pos;
	int delay = 1;
	u32 id;
	u16 ctrl;
	/* Error recovery runs on all subordinates of the first downstream port.
	 * If the downstream port detected the error, it is cleared at the end.
	 */
	if (!(pci_pcie_type(dev) == PCI_EXP_TYPE_ROOT_PORT ||
	      pci_pcie_type(dev) == PCI_EXP_TYPE_DOWNSTREAM))
		dev = dev->bus->self;
	bus = dev->subordinate;

	pci_walk_bus(bus, ngbe_report_frozen_detected, &status);
	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ERR);
	if (pos) {
		/* Disable Root's interrupt in response to error messages */
		pci_read_config_dword(dev, pos + PCI_ERR_ROOT_COMMAND, &reg32);
		reg32 &= ~NGBE_ROOT_PORT_INTR_ON_MESG_MASK;
		pci_write_config_dword(dev, pos + PCI_ERR_ROOT_COMMAND, reg32);
	}

	pci_read_config_word(dev, PCI_BRIDGE_CONTROL, &ctrl);
	ctrl |= PCI_BRIDGE_CTL_BUS_RESET;
	pci_write_config_word(dev, PCI_BRIDGE_CONTROL, ctrl);

	/* PCI spec v3.0 7.6.4.2 requires minimum Trst of 1ms.  Double
	 * this to 2ms to ensure that we meet the minimum requirement.
	 */

	msleep(20);
	ctrl &= ~PCI_BRIDGE_CTL_BUS_RESET;
	pci_write_config_word(dev, PCI_BRIDGE_CONTROL, ctrl);

	/* Trhfa for conventional PCI is 2^25 clock cycles.
	 * Assuming a minimum 33MHz clock this results in a 1s
	 * delay before we can consider subordinate devices to
	 * be re-initialized.  PCIe has some ways to shorten this,
	 * but we don't make use of them yet.
	 */
	ssleep(1);

	pci_read_config_dword(dev, PCI_COMMAND, &id);
	while (id == ~0) {
		if (delay > 60000) {
			pci_warn(dev, "not ready %dms after %s; giving up\n",
				 delay - 1, "bus_reset");
			return;
		}

		if (delay > 1000)
			pci_info(dev, "not ready %dms after %s; waiting\n",
				 delay - 1, "bus_reset");

		msleep(delay);
		delay *= 2;
		pci_read_config_dword(dev, PCI_COMMAND, &id);
	}

	if (delay > 1000)
		pci_info(dev, "ready %dms after %s\n", delay - 1, "bus_reset");

	pci_info(dev, "Root Port link has been reset\n");

	if (pos) {
		/* Clear Root Error Status */
		pci_read_config_dword(dev, pos + PCI_ERR_ROOT_STATUS, &reg32);
		pci_write_config_dword(dev, pos + PCI_ERR_ROOT_STATUS, reg32);

		/* Enable Root Port's interrupt in response to error messages */
		pci_read_config_dword(dev, pos + PCI_ERR_ROOT_COMMAND, &reg32);
		reg32 |= NGBE_ROOT_PORT_INTR_ON_MESG_MASK;
		pci_write_config_dword(dev, pos + PCI_ERR_ROOT_COMMAND, reg32);
	}

	if (status == PCI_ERS_RESULT_CAN_RECOVER) {
		status = PCI_ERS_RESULT_RECOVERED;
		pci_dbg(dev, "broadcast mmio_enabled message\n");
		pci_walk_bus(bus, ngbe_report_mmio_enabled, &status);
	}

	if (status == PCI_ERS_RESULT_NEED_RESET) {
		/* TODO: Should call platform-specific
		 * functions to reset slot before calling
		 * drivers' slot_reset callbacks?
		 */
		status = PCI_ERS_RESULT_RECOVERED;
		pci_dbg(dev, "broadcast slot_reset message\n");
		pci_walk_bus(bus, ngbe_report_slot_reset, &status);
	}

	if (status != PCI_ERS_RESULT_RECOVERED)
		goto failed;

	pci_dbg(dev, "broadcast resume message\n");
	pci_walk_bus(bus, ngbe_report_resume, &status);

failed:
	return;
}

void ngbe_aer_print_error(struct ngbe_adapter *adapter, u32 severity,
			  u32 status)
{
	const char *errmsg = NULL;
	unsigned long i;
	unsigned long temp_status = status;
	struct pci_dev *pdev = adapter->pdev;

	for_each_set_bit(i, &temp_status, 32) {
		if (severity == NGBE_AER_CORRECTABLE) {
			errmsg = i < ARRAY_SIZE(aer_correctable_error_string) ?
					 aer_correctable_error_string[i] :
					 NULL;
		} else {
			errmsg =
				i < ARRAY_SIZE(aer_uncorrectable_error_string) ?
					aer_uncorrectable_error_string[i] :
					NULL;

			if (errmsg && i == 14)
				adapter->cmplt_to_dis = true;
		}
		if (errmsg)
			dev_info(&pdev->dev, "   [%2ld] %-22s\n", i, errmsg);
	}
}

bool ngbe_check_recovery_capability(struct pci_dev *dev)
{
#if defined(__i386__) || defined(__x86_64__)
	return true;
#else
	/* check upstream bridge is root or PLX bridge,
	 * or cpu is kupeng 920 or not
	 */
	if (dev->bus->self->vendor == 0x10b5 ||
	    dev->bus->self->vendor == 0x19e5)
		return true;
	else
		return false;
#endif
}
