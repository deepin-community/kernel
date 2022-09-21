/* SPDX-License-Identifier: GPL-2.0 */
/* Phytium X100 display drm driver
 *
 * Copyright (c) 2021 Phytium Limited.
 *
 * Author:
 *	Yang Xun <yangxun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PHYTIUM_DP_H__
#define __PHYTIUM_DP_H__

#include <drm/drm_dp_helper.h>
#include <drm/drm_encoder.h>
#include <sound/hdmi-codec.h>

struct phytium_dp_device;

#include "phytium_panel.h"

struct audio_info {
	int sample_rate;
	int channels;
	int sample_width;
};

struct dp_audio_n_m {
	int sample_rate;
	int link_rate;
	u16 m;
	u16 n;
};

struct phytium_dp_compliance {
	unsigned long test_type;
	uint32_t test_link_rate;
	u8 test_lane_count;
	bool test_active;
	u8 reserve[2];
};

struct phytium_dp_func {
	void (*dp_hw_set_test_pattern)(struct phytium_dp_device *phytium_dp, uint8_t test_pattern,
				       uint8_t lane_count, uint8_t *custom_pattern,
				       uint32_t custom_pattern_size);
	void (*dp_hw_set_link)(struct phytium_dp_device *phytium_dp, uint8_t link_lane_count,
			       uint32_t link_rate);
	void (*dp_hw_set_lane_setting)(struct phytium_dp_device *phytium_dp,
				       uint32_t link_rate, uint8_t train_set);
	void (*dp_hw_set_train_pattern)(struct phytium_dp_device *phytium_dp,
					uint8_t train_pattern);
	void (*dp_hw_init)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_enable_output)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_disable_output)(struct phytium_dp_device *phytium_dp);
	bool (*dp_hw_output_is_enable)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_enable_input_source)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_disable_input_source)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_get_hpd_state)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_hpd_irq_setup)(struct phytium_dp_device *phytium_dp, bool enable);
	void (*dp_hw_disable_video)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_enable_video)(struct phytium_dp_device *phytium_dp);
	bool (*dp_hw_video_is_enable)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_config_video)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_disable_audio)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_enable_audio)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_audio_shutdown)(struct phytium_dp_device *phytium_dp);
	int (*dp_hw_audio_digital_mute)(struct phytium_dp_device *phytium_dp);
	int (*dp_hw_audio_hw_params)(struct phytium_dp_device *phytium_dp,
				     struct audio_info audio_info);
	void (*dp_hw_enable_backlight)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_disable_backlight)(struct phytium_dp_device *phytium_dp);
	uint32_t (*dp_hw_get_backlight)(struct phytium_dp_device *phytium_dp);
	int (*dp_hw_set_backlight)(struct phytium_dp_device *phytium_dp, uint32_t level);
	void (*dp_hw_poweron_panel)(struct phytium_dp_device *phytium_dp);
	void (*dp_hw_poweroff_panel)(struct phytium_dp_device *phytium_dp);
};

struct phytium_dp_hpd_state {
	bool hpd_event_state;
	bool hpd_irq_state;
	bool hpd_raw_state;
	bool hpd_irq_enable;
};

struct phytium_dp_device {
	struct drm_device *dev;
	struct drm_encoder encoder;
	struct drm_connector connector;
	int port;
	struct drm_display_mode mode;
	bool	link_trained;
	bool	detect_done;
	bool	is_edp;
	bool	reserve0;
	struct drm_dp_aux aux;
	unsigned char dpcd[DP_RECEIVER_CAP_SIZE];
	uint8_t edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE];
	unsigned char downstream_ports[DP_MAX_DOWNSTREAM_PORTS];
	unsigned char sink_count;

	int *source_rates;
	int num_source_rates;
	int sink_rates[DP_MAX_SUPPORTED_RATES];
	int num_sink_rates;
	int common_rates[DP_MAX_SUPPORTED_RATES];
	int num_common_rates;

	int source_max_lane_count;
	int sink_max_lane_count;
	int common_max_lane_count;

	int max_link_rate;
	int max_link_lane_count;
	int link_rate;
	int link_lane_count;
	struct work_struct train_retry_work;
	int train_retry_count;
	uint32_t trigger_train_fail;

	unsigned char train_set[4];
	struct edid *detect_edid;
	bool has_audio;
	bool fast_train_support;
	bool reserve[2];
	struct platform_device *audio_pdev;
	struct audio_info audio_info;
	hdmi_codec_plugged_cb plugged_cb;
	struct device *codec_dev;
	struct phytium_dp_compliance compliance;
	struct phytium_dp_func *funcs;
	struct phytium_dp_hpd_state dp_hpd_state;

	struct phytium_panel panel;
	struct drm_display_mode native_mode;
};

union phytium_phy_tp {
	struct {
		/* DpcdPhyTestPatterns. This field is 2 bits for DP1.1
		 * and 3 bits for DP1.2.
		 */
		uint8_t PATTERN	:3;
		uint8_t RESERVED	:5;
	} bits;
	uint8_t raw;
};

/* PHY test patterns
 * The order of test patterns follows DPCD register PHY_TEST_PATTERN (0x248)
 */
enum phytium_dpcd_phy_tp {
	PHYTIUM_PHY_TP_NONE = 0,
	PHYTIUM_PHY_TP_D10_2,
	PHYTIUM_PHY_TP_SYMBOL_ERROR,
	PHYTIUM_PHY_TP_PRBS7,
	PHYTIUM_PHY_TP_80BIT_CUSTOM,
	PHYTIUM_PHY_TP_CP2520_1,
	PHYTIUM_PHY_TP_CP2520_2,
	PHYTIUM_PHY_TP_CP2520_3,
};
#define encoder_to_dp_device(x) container_of(x, struct phytium_dp_device, encoder)
#define connector_to_dp_device(x) container_of(x, struct phytium_dp_device, connector)
#define panel_to_dp_device(x) container_of(x, struct phytium_dp_device, panel)
#define train_retry_to_dp_device(x)	container_of(x, struct phytium_dp_device, train_retry_work)
int phytium_dp_init(struct drm_device *dev, int pipe);
void phytium_dp_hw_init(struct phytium_dp_device *phytium_dp);
void phytium_dp_hpd_irq_setup(struct drm_device *dev, bool enable);
irqreturn_t phytium_dp_hpd_irq_handler(struct phytium_display_drm_private *priv);
void phytium_dp_hpd_work_func(struct work_struct *work);
const struct dp_audio_n_m *phytium_dp_audio_get_n_m(int link_rate, int sample_rate);
#endif /* __PHYTIUM_DP_H__ */
