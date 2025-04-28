/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_PHYTIUM_USB_OTG_H__
#define __LINUX_PHYTIUM_USB_OTG_H__


struct phytium_usb_otg_regs {
	__le32	did;			/*0x0*/
	__le32	rid;			/*0x4*/
	__le32	cfgs1;			/*0x8*/
	__le32	cfgs2;			/*0xC*/
	__le32	cmd;			/*0x10*/
	__le32	sts;			/*0x14*/
	__le32	state;			/*0x18*/
	__le32	ien;			/*0x1C*/
	__le32	ivect;			/*0x20*/
	__le32	tmr;			/*0x24*/
	__le32	simulate;		/*0x28*/
	__le32	adpbc_sts;		/*0x2c*/
	__le32	adp_ramp_time;		/*0x30*/
	__le32	adpbc_ctrl1;		/*0x34*/
	__le32	adpbc_ctrl2;		/*0x38*/
	__le32	override;		/*0x3C*/
	__le32	vbusvalid_dbnc_cfg;	/*0x40*/
	__le32	sessvalid_dbnc_cfg;	/*0x44*/
	__le32	susp_timing_ctrl;	/*0x48*/
};

int phytium_usb_otg_init(void *data);
void phytium_usb_otg_uninit(void *data);
int phytium_usb_otg_enable_iddig(void *data);
int phytium_usb_otg_disable_irq(void *data);
int phytium_usb_otg_clear_irq(void *data);
int phytium_usb_otg_enable_irq(void *data);
int phytium_usb_otg_host_on(void *data);
int phytium_usb_otg_host_off(void *data);
int phytium_usb_otg_gadget_on(void *data);
int phytium_usb_otg_gadget_off(void *data);
int phytium_usb_otg_get_id(void *data);
int phytium_usb_otg_get_vbus(void *data);
int phytium_usb_otg_is_host(void *data);
int phytium_usb_otg_is_device(void *data);
bool phytium_usb_otg_power_is_lost(void *data);

#endif
