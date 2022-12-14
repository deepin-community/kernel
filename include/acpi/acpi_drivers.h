/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  acpi_drivers.h  ($Revision: 31 $)
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 */

#ifndef __ACPI_DRIVERS_H__
#define __ACPI_DRIVERS_H__

#define ACPI_MAX_STRING			80

/*
 * Please update drivers/acpi/debug.c and Documentation/firmware-guide/acpi/debug.rst
 * if you add to this list.
 */
#define ACPI_BUS_COMPONENT		0x00010000
#define ACPI_AC_COMPONENT		0x00020000
#define ACPI_BATTERY_COMPONENT		0x00040000
#define ACPI_BUTTON_COMPONENT		0x00080000
#define ACPI_SBS_COMPONENT		0x00100000
#define ACPI_FAN_COMPONENT		0x00200000
#define ACPI_PCI_COMPONENT		0x00400000
#define ACPI_POWER_COMPONENT		0x00800000
#define ACPI_CONTAINER_COMPONENT	0x01000000
#define ACPI_SYSTEM_COMPONENT		0x02000000
#define ACPI_THERMAL_COMPONENT		0x04000000
#define ACPI_MEMORY_DEVICE_COMPONENT	0x08000000
#define ACPI_VIDEO_COMPONENT		0x10000000
#define ACPI_PROCESSOR_COMPONENT	0x20000000

/*
 * _HID definitions
 * HIDs must conform to ACPI spec(6.1.4)
 * Linux specific HIDs do not apply to this and begin with LNX:
 */

#define ACPI_POWER_HID			"LNXPOWER"
#define ACPI_PROCESSOR_OBJECT_HID	"LNXCPU"
#define ACPI_SYSTEM_HID			"LNXSYSTM"
#define ACPI_THERMAL_HID		"LNXTHERM"
#define ACPI_BUTTON_HID_POWERF		"LNXPWRBN"
#define ACPI_BUTTON_HID_SLEEPF		"LNXSLPBN"
#define ACPI_VIDEO_HID			"LNXVIDEO"
#define ACPI_BAY_HID			"LNXIOBAY"
#define ACPI_DOCK_HID			"LNXDOCK"
#define ACPI_ECDT_HID			"LNXEC"
/* Quirk for broken IBM BIOSes */
#define ACPI_SMBUS_IBM_HID		"SMBUSIBM"

/*
 * For fixed hardware buttons, we fabricate acpi_devices with HID
 * ACPI_BUTTON_HID_POWERF or ACPI_BUTTON_HID_SLEEPF.  Fixed hardware
 * signals only an event; it doesn't supply a notification value.
 * To allow drivers to treat notifications from fixed hardware the
 * same as those from real devices, we turn the events into this
 * notification value.
 */
#define ACPI_FIXED_HARDWARE_EVENT	0x100

/* --------------------------------------------------------------------------
                                       PCI
   -------------------------------------------------------------------------- */


/* ACPI PCI Interrupt Link (pci_link.c) */

int acpi_irq_penalty_init(void);
int acpi_pci_link_allocate_irq(acpi_handle handle, int index, int *triggering,
			       int *polarity, char **name, struct fwnode_handle **rs_fwnode);
int acpi_pci_link_free_irq(acpi_handle handle);

/* ACPI PCI Device Binding (pci_bind.c) */

struct pci_bus;

#ifdef CONFIG_PCI
struct pci_dev *acpi_get_pci_dev(acpi_handle);
#else
static inline struct pci_dev *acpi_get_pci_dev(acpi_handle handle)
{
	return NULL;
}
#endif

/* Arch-defined function to add a bus to the system */

struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root);

#ifdef CONFIG_X86
void pci_acpi_crs_quirks(void);
#else
static inline void pci_acpi_crs_quirks(void) { }
#endif

/* --------------------------------------------------------------------------
                                    Processor
   -------------------------------------------------------------------------- */

#define ACPI_PROCESSOR_LIMIT_NONE	0x00
#define ACPI_PROCESSOR_LIMIT_INCREMENT	0x01
#define ACPI_PROCESSOR_LIMIT_DECREMENT	0x02

/*--------------------------------------------------------------------------
                                  Dock Station
  -------------------------------------------------------------------------- */

#ifdef CONFIG_ACPI_DOCK
extern int is_dock_device(struct acpi_device *adev);
#else
static inline int is_dock_device(struct acpi_device *adev)
{
	return 0;
}
#endif /* CONFIG_ACPI_DOCK */

#endif /*__ACPI_DRIVERS_H__*/
