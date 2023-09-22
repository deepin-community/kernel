#ifndef __GSGPU_ZIP_H__
#define __GSGPU_ZIP_H__

#define     ZIP_ENABLE    0x01 // zip enable
#define     ZIP_DISABLE   0x02 // zip disable
#define     ZIP_SET_BASE  0x03 // zip_base = ret[1:0]
#define     ZIP_SET_MASK  0x04 // zip_mask = ret[1:0]
#define     ZIP_FLUSH     0x05 // zip_flush

extern const struct gsgpu_ip_block_version zip_ip_block;

#endif /*__GSGPU_ZIP_H__*/
