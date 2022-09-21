#ifndef _LIB_CLASS_TYPE_H
#define _LIB_CLASS_TYPE_H
// get cpu name by dmi_walk
char *get_cpu_name(void);

// get machine type by dmi_get_system_info
/* chassis types description
 * 0x01: // Other
 * 0x02: // Unknown
 * 0x03: // Desktop
 * 0x04: // Low Profile Desktop
 * 0x05: // Pizza Box
 * 0x06: // Mini Tower
 * 0x07: // Tower
 * 0x08: // Portable
 * 0x09: // Laptop
 * 0x0A: // Notebook
 * 0x0B: // Hand Held
 * 0x0C: // Docking Station
 * 0x0D: // All in One
 * 0x0E: // Sub Noteboo
 * 0x0F: // Space-saving
 * 0x10: // Lunch Box
 * 0x11: // Main Server Chassis
 ...
 * 0x1E: // Tablet
 * 0x22: // Embedded PC
  ...
 */
unsigned long get_chassis_types(void);
bool chassis_types_is_laptop(void);
bool chassis_types_is_desktop(void);
bool chassis_types_is_server(void);
bool chassis_types_is_allinone(void);
bool os_run_evn_is_virt(void);
#endif