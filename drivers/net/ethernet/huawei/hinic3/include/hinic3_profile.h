/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_PROFILE_H
#define HINIC3_PROFILE_H

typedef bool (*hinic3_is_match_prof)(void *device);
typedef void *(*hinic3_init_prof_attr)(void *device);
typedef void (*hinic3_deinit_prof_attr)(void *porf_attr);

enum prof_adapter_type {
	PROF_ADAP_TYPE_INVALID,
	PROF_ADAP_TYPE_PANGEA = 1,

	/* Add prof adapter type before default */
	PROF_ADAP_TYPE_DEFAULT,
};

/**
 * struct hinic3_prof_adapter - custom scene's profile adapter
 * @type: adapter type
 * @match: Check whether the current function is used in the custom scene.
 *	Implemented in the current source file
 * @init: When @match return true, the initialization function called in probe.
 *	Implemented in the source file of the custom scene
 * @deinit: When @match return true, the deinitialization function called when
 *	remove. Implemented in the source file of the custom scene
 */
struct hinic3_prof_adapter {
	enum prof_adapter_type	type;
	hinic3_is_match_prof	match;
	hinic3_init_prof_attr	init;
	hinic3_deinit_prof_attr	deinit;
};

#ifdef static
#undef static
#define LLT_STATIC_DEF_SAVED
#endif

static inline struct hinic3_prof_adapter *hinic3_prof_init(void *device,
							   struct hinic3_prof_adapter *adap_objs,
							   int num_adap, void **prof_attr)
{
	struct hinic3_prof_adapter *prof_obj = NULL;
	int i;

	for (i = 0; i < num_adap; i++) {
		prof_obj = &adap_objs[i];
		if (!(prof_obj->match && prof_obj->match(device)))
			continue;

		*prof_attr = prof_obj->init ? prof_obj->init(device) : NULL;

		return prof_obj;
	}

	return NULL;
}

static inline void hinic3_prof_deinit(struct hinic3_prof_adapter *prof_obj, void *prof_attr)
{
	if (!prof_obj)
		return;

	if (prof_obj->deinit)
		prof_obj->deinit(prof_attr);
}

/* module-level interface */
#ifdef CONFIG_MODULE_PROF
struct hinic3_module_ops {
	int (*module_prof_init)(void);
	void (*module_prof_exit)(void);
	void (*probe_fault_process)(void *pdev, u16 level);
	int (*probe_pre_process)(void *pdev);
	void (*probe_pre_unprocess)(void *pdev);
};

struct hinic3_module_ops *hinic3_get_module_prof_ops(void);

static inline void hinic3_probe_fault_process(void *pdev, u16 level)
{
	struct hinic3_module_ops *ops = hinic3_get_module_prof_ops();

	if (ops && ops->probe_fault_process)
		ops->probe_fault_process(pdev, level);
}

static inline int hinic3_module_pre_init(void)
{
	struct hinic3_module_ops *ops = hinic3_get_module_prof_ops();

	if (!ops || !ops->module_prof_init)
		return -EINVAL;

	return ops->module_prof_init();
}

static inline void hinic3_module_post_exit(void)
{
	struct hinic3_module_ops *ops = hinic3_get_module_prof_ops();

	if (ops && ops->module_prof_exit)
		ops->module_prof_exit();
}

static inline int hinic3_probe_pre_process(void *pdev)
{
	struct hinic3_module_ops *ops = hinic3_get_module_prof_ops();

	if (!ops || !ops->probe_pre_process)
		return -EINVAL;

	return ops->probe_pre_process(pdev);
}

static inline void hinic3_probe_pre_unprocess(void *pdev)
{
	struct hinic3_module_ops *ops = hinic3_get_module_prof_ops();

	if (ops && ops->probe_pre_unprocess)
		ops->probe_pre_unprocess(pdev);
}
#else
static inline void hinic3_probe_fault_process(void *pdev, u16 level) { };

static inline int hinic3_module_pre_init(void)
{
	return 0;
}

static inline void hinic3_module_post_exit(void) { };

static inline int hinic3_probe_pre_process(void *pdev)
{
	return 0;
}

static inline void hinic3_probe_pre_unprocess(void *pdev) { };
#endif

#ifdef LLT_STATIC_DEF_SAVED
#define static
#undef LLT_STATIC_DEF_SAVED
#endif

#endif
