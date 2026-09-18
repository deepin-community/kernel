#ifndef _DPP_HASH_CRC_H_
#define _DPP_HASH_CRC_H_

#define MAX_CRC_WIDTH     (20)

ZXIC_UINT32 dpp_crc32_calc(ZXIC_UINT8 *pInputKey,ZXIC_UINT32 dwByteNum,ZXIC_UINT32 dwCrcPoly);

ZXIC_UINT16 dpp_crc16_calc(ZXIC_UINT8 *pInputKey,ZXIC_UINT32 dwByteNum,ZXIC_UINT16 dwCrcPoly);

ZXIC_UINT16 dpp_crc16_get_idx(ZXIC_UINT16 crc_val);

ZXIC_UINT16 dpp_crc16_table_lookup(ZXIC_UINT8 *pInputKey, ZXIC_UINT32 dwByteNum, ZXIC_UINT16 dwCrcPoly);

#endif


