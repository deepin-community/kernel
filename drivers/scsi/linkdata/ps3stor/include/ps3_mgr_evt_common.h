/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PS3_MGR_EVT_COMMON_H__
#define __PS3_MGR_EVT_COMMON_H__

#define MGR_EVT_TYPE_OFFSET (80)
#define MGR_EVT_LOG_INFO_MAX_SIZE    (116)

#if !(defined(PS3_PRODUCT_EXPANDER) || defined(PS3_PRODUCT_SWITCH))
#define MGR_EVT_TYPE_EXTEND_OFFSET (200)
#define MGR_EVT_EXTEND_TYPE_START (17)


#define MGR_EVT_TYPE_BASE_LOCAL(sn)  (sn < MGR_EVT_EXTEND_TYPE_START ? \
	(sn - 1) * MGR_EVT_TYPE_OFFSET : \
	MGR_EVT_TYPE_OFFSET * (MGR_EVT_EXTEND_TYPE_START - 1) \
	+ MGR_EVT_TYPE_EXTEND_OFFSET * (sn - MGR_EVT_EXTEND_TYPE_START))



#define MGR_EVENT_CODE_2_TYPE(code)  \
	(code <= MGR_EVT_TYPE_OFFSET * (MGR_EVT_EXTEND_TYPE_START - 1) ? \
	((code) / MGR_EVT_TYPE_OFFSET + 1) : \
	(code - MGR_EVT_TYPE_OFFSET * (MGR_EVT_EXTEND_TYPE_START - 1)) / \
	MGR_EVT_TYPE_EXTEND_OFFSET + MGR_EVT_EXTEND_TYPE_START)

#else
#define MGR_EVT_TYPE_BASE_LOCAL(sn) ((sn - 1) * MGR_EVT_TYPE_OFFSET)
#define MGR_EVENT_CODE_2_TYPE(code) ((code) / MGR_EVT_TYPE_OFFSET + 1)
#endif

#define PS3_EVT_TYPE_OFFSET      (30)
#define PS3_EVT_EXPOSE_OFFSET    (28)
#define PS3_EVT_BATCH_OFFSET     (27)
#define PS3_EVT_ATTR_OFFSET      (16)
#define PS3_EVT_LEVEL_OFFSET     (12)
#define PS3_EVT_CODE_OFFSET      (0)

#define PS3_MAKE_EVT_CODE(type, expose, batch, attr, level, code) \
	((((type)&0b11) << PS3_EVT_TYPE_OFFSET) | \
	(((expose)&0b11) << PS3_EVT_EXPOSE_OFFSET) | \
	(((batch)&0b1) << PS3_EVT_BATCH_OFFSET) | \
	(((attr)&0x3F) << PS3_EVT_ATTR_OFFSET) | \
	(((level)&0xF) << PS3_EVT_LEVEL_OFFSET) | \
	((code)&0xFFF) << PS3_EVT_CODE_OFFSET)
#define PS3_MK_EVT(type, expose, batch, attr, level, code)     \
	(PS3_MAKE_EVT_CODE(type, expose, batch, attr, level, code))

#define PS3_EVT_TYPE(evtcode) ((evtcode >> PS3_EVT_TYPE_OFFSET) & 0b11)
#define PS3_EVT_EXPOSE(evtcode) ((evtcode >> PS3_EVT_EXPOSE_OFFSET) & 0b11)
#define PS3_EVT_IS_BATCH(evtcode) ((evtcode >> PS3_EVT_BATCH_OFFSET) & 0b1)
#define PS3_EVT_ATTR_EXTEND(evtcode) ((U8)((evtcode >> PS3_EVT_ATTR_OFFSET) & 0x3F))
#define PS3_EVT_LEVEL(evtcode) ((evtcode >> PS3_EVT_LEVEL_OFFSET) & 0xF)
#define PS3_EVT_CODE(evtcode) (((unsigned int)evtcode >> PS3_EVT_CODE_OFFSET) & 0xFFF)

enum Ps3EventType {
	PS3_EVT_TYPE_UNKNOWN = 0b0000,
	PS3_EVT_TYPE_RAIDHBA = 0b0001,
	PS3_EVT_TYPE_EXPANDER = 0b0010,
	PS3_EVT_TYPE_PCIESWITCH = 0b0011
};

enum Ps3EventExpose {
	PS3_EVT_EXP_UNKNOWN = 0b0000,
	PS3_EVT_EXP_EXTERNAL = 0b0001,
	PS3_EVT_EXP_INTERNAL = 0b0010,
	PS3_EVT_EXP_MAX = 0b0011
};

enum MgrEventLevel {
	PS3_EVT_CLASS_UNKNOWN = 0b0000,
	PS3_EVT_CLASS_DEBUG = 0b0011,
	PS3_EVT_CLASS_PROCESS = 0b0101,
	PS3_EVT_CLASS_INFO = 0b0001,
	PS3_EVT_CLASS_WARNING = 0b0010,
	PS3_EVT_CLASS_CRITICAL = 0b0100,
	PS3_EVT_CLASS_FATAL = 0b1000,
	PS3_EVT_CLASS_MAX
};

enum MgrEventIsBatch {
	PS3_EVT_IS_BATCH_FALSE = 0b0,
	PS3_EVT_IS_BATCH_TRUE = 0b1,
};

#endif
