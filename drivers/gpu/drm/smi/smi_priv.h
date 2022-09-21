#ifndef __SMI_PRIV_H__
#define __SMI_PRIV_H__

struct smi_mode_info {
	bool mode_config_initialized;
	struct smi_crtc *crtc;
	/* pointer to fbdev info structure */
};

struct smi_crtc {
	struct drm_crtc base;
	u8 lut_r[256], lut_g[256], lut_b[256];
	int last_dpms;
	bool enabled;
	int crtc_index;
	int CursorOffset;
};

struct smi_mc {
	resource_size_t vram_size;
	resource_size_t vram_base;
};

#endif