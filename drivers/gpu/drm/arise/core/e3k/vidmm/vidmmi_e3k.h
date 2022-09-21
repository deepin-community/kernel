//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#ifndef __VIDMMI_E3K_H__
#define __VIDMMI_E3K_H__

extern void vidmm_query_segment_info_e3k(adapter_t *, vidmm_chip_segment_info_t *);
extern int  vidmm_describe_allocation_e3k(adapter_t*, vidmm_describe_allocation_t*);
extern int  vidmm_build_page_buffer_e3k(adapter_t*, vidmm_private_build_paging_buffer_arg_t*);
extern int  vidmm_query_gart_table_info_e3k(adapter_t*);
extern void vidmm_init_mem_settings_e3k(adapter_t *);
extern void vidmm_init_gart_table_e3k(adapter_t*);
extern void vidmm_init_svm_gart_table_e3k(adapter_t*);
extern void vidmm_deinit_gart_table_e3k(adapter_t*);
extern void vidmm_deinit_svm_gart_table_e3k(adapter_t*);
extern void vidmm_map_gart_table_e3k(adapter_t*, vidmm_allocation_t *allocation, int snooping);
extern void vidmm_map_svm_gart_table_e3k(adapter_t*, vidmm_allocation_t *allocation, int snooping);
extern void vidmm_unmap_gart_table_e3k(adapter_t*, vidmm_allocation_t *allocation);
extern void vidmm_unmap_svm_gart_table_e3k(adapter_t*, vidmm_allocation_t *allocation);
extern void vidmm_gart_table_set_snoop_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, int snooping);
extern int  vidmm_get_allocation_info_e3k(adapter_t *adapter, vidmm_get_allocation_info_t *info);


#endif

