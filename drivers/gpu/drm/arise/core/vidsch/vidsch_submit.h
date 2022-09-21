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

#ifndef __VIDSCH_SUBMIT_H__
#define __VIDSCH_SUBMIT_H__

extern int  vidschi_prepare_and_submit_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma);
extern void vidschi_fake_submit_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma, int discard);

extern void vidschi_release_allocated_tasks(vidsch_mgr_t *sch_mgr);
extern task_desc_t *vidschi_uncompleted_task_exceed_wait_time(vidsch_mgr_t *sch_mgr, unsigned long long max_wait_time);

extern void vidschi_set_dma_task_submitted(vidsch_mgr_t *sch_mgr, task_desc_t *task);
extern void vidschi_set_dma_task_fake_submitted(vidsch_mgr_t *sch_mgr, task_desc_t *task);
extern void vidschi_set_uncompleted_task_dropped(vidsch_mgr_t *sch_mgr, unsigned long long dropped_task);

#endif

