/*
 *  hda_ft.h - Implementation of primary alsa driver code base
 *                for Intel HD Audio of Phytium.
 *
 *  Copyright(c) 2018 Phytium Corporation. All rights reserved.
 *
 *  Copyright(c) 2018 Leo Hou <houyuefei@phytium.com.cn>
 *
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __SOUND_HDA_PHYTIUM_H__
#define __SOUND_HDA_PHYTIUM_H__

#include "hda_controller.h"

struct hda_ft {
	struct azx chip;
	struct snd_pcm_substream *substream;
	struct device *dev;
	void __iomem *regs;

	/* for pending irqs */
	struct work_struct irq_pending_work;

	/* sync probing */
	struct completion probe_wait;
	struct work_struct probe_work;

	/* card list (for power_save trigger) */
	struct list_head list;

	/* extra flags */
	unsigned int irq_pending_warned:1;
	unsigned int probe_continued:1;

};

#endif
