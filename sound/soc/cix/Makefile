# SPDX-License-Identifier: GPL-2.0
# Cixtech Platform Support
cix-snd-soc-cdns-i2s-sc-objs := cdns_i2s_sc.o
cix-snd-soc-cdns-i2s-mc-objs := cdns_i2s_mc.o
cix-snd-soc-hda-objs := hdac.o hda_pcm.o ipb_hda.o
cix-snd-hda-codec-realtek-objs := hdacodec.o patch_realtek.o
snd-soc-sky1-sound-card-objs := card-utils.o sky1-card.o

obj-$(CONFIG_SND_SOC_CDNS_I2S_SC_CIX) += cix-snd-soc-cdns-i2s-sc.o
obj-$(CONFIG_SND_SOC_CDNS_I2S_MC_CIX) += cix-snd-soc-cdns-i2s-mc.o
obj-$(CONFIG_SND_SOC_IPBLOQ_HDA_CIX) += cix-snd-soc-hda.o
obj-$(CONFIG_SND_HDACODEC_REALTEK_CIX) += cix-snd-hda-codec-realtek.o
obj-${CONFIG_SND_SOC_CIX} += snd-soc-sky1-sound-card.o
