#ifndef _EN_ZUX_H_
#define _EN_ZUX_H_

#ifdef __cplusplus
extern "C" {
#endif


struct eth_stats
{
    uint64_t rx_pkts;            /* gorc */
    uint64_t rx_bytes;           /* gorc */
    uint64_t rx_unicast;         /* uprc */
    uint64_t rx_multicast;       /* mprc */
    uint64_t rx_broadcast;       /* bprc */
    uint64_t rx_discards;        /* rdpc */
    uint64_t rx_errors;          /* rupp */
    uint64_t tx_pkts;            /* gorc */
    uint64_t tx_bytes;           /* gotc */
    uint64_t tx_unicast;         /* uptc */
    uint64_t tx_multicast;       /* mptc */
    uint64_t tx_broadcast;       /* bptc */
    uint64_t tx_discards;        /* tdpc */
    uint64_t tx_errors;          /* tepc */
    uint64_t rx_size_64;         /* prc64 */
    uint64_t rx_size_127;        /* prc127 */
    uint64_t rx_size_255;        /* prc255 */
    uint64_t rx_size_511;        /* prc511 */
    uint64_t rx_size_1023;       /* prc1023 */
    uint64_t rx_size_1518;
    uint64_t rx_size_1522;       /* prc1522 */
    uint64_t rx_size_1548;
    uint64_t rx_size_2047;
    uint64_t rx_size_4095;
    uint64_t rx_size_8191;
    uint64_t rx_size_9215;
    uint64_t rx_undersize;       /* ruc */
    uint64_t rx_oversize;        /* roc */
    uint64_t tx_size_64;         /* ptc64 */
    uint64_t tx_size_127;        /* ptc127 */
    uint64_t tx_size_255;        /* ptc255 */
    uint64_t tx_size_511;        /* ptc511 */
    uint64_t tx_size_1023;       /* ptc1023 */
    uint64_t tx_size_1518;
    uint64_t tx_size_1522;       /* prc1522 */
    uint64_t tx_size_1548;
    uint64_t tx_size_2047;
    uint64_t tx_size_4095;
    uint64_t tx_size_8191;
    uint64_t tx_size_9215;
    uint64_t tx_undersize;       /* ruc */
    uint64_t tx_oversize;        /* roc */
};

struct zxdh_pf_eth_stats
{
    struct eth_stats eth_total_stat;
    struct eth_stats mac0_stat;
    struct eth_stats mac1_stat;
    struct eth_stats mac2_stat;
    struct eth_stats mac3_stat;
};

int32_t zxdh_en_driver_register(void);
void zxdh_en_driver_unregister(void);

#ifdef __cplusplus
}
#endif

#endif
