/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_NPU_CMD_H
#define CQM_NPU_CMD_H

enum cqm_cmd_type {
	CQM_CMD_T_INVALID = 0, /* < Invalid command */
	CQM_CMD_T_BAT_UPDATE,  /* < Update the bat configuration of the funciton,
				* @see struct tag_cqm_cmdq_bat_update
				*/
	CQM_CMD_T_CLA_UPDATE,  /* < Update the cla configuration of the funciton,
				* @see struct tag_cqm_cla_update_cmd
				*/
	CQM_CMD_T_BLOOMFILTER_SET,   /* < Set the bloomfilter configuration of the funciton,
				      * @see struct tag_cqm_bloomfilter_cmd
				      */
	CQM_CMD_T_BLOOMFILTER_CLEAR, /* < Clear the bloomfilter configuration of the funciton,
				      * @see struct tag_cqm_bloomfilter_cmd
				      */
	CQM_CMD_T_RSVD,		/* < Unused */
	CQM_CMD_T_CLA_CACHE_INVALID, /* < Invalidate the cla cacheline,
				      * @see struct tag_cqm_cla_cache_invalid_cmd
				      */
	CQM_CMD_T_BLOOMFILTER_INIT,  /* < Init the bloomfilter configuration of the funciton,
				      * @see struct tag_cqm_bloomfilter_init_cmd
				      */
	CQM_CMD_T_MAX
};

#endif /* CQM_NPU_CMD_H */
