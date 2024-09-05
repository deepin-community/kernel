#
# Makefile for GF kernel driver
#

ccflags-y += \
	-I$(GFGPU_FULL_PATH)/core/e3k/include

VIDMM_OBJ := \
	vidmm/vidmm_allocate_e3k.o \
	vidmm/vidmm_build_page_buffer_e3k.o \
	vidmm/vidmm_e3k.o \
	vidmm/vidmm_gart_table_e3k.o \

VIDSCH_OBJ := \
	vidsch/vidsch_setup_e3k.o \
	vidsch/vidsch_patch_e3k.o \
	vidsch/vidsch_render_e3k.o \
	vidsch/vidsch_blt_e3k.o \
	vidsch/vidsch_engine_submit_e3k.o \
	vidsch/vidsch_engine_setup_e3k.o \
	vidsch/ContextSwitch_e3k.o \
	vidsch/vidsch_debug_hang_e3k.o \
	vidsch/vidsch_debug_hang_compatible_e3k.o \
	vidsch/vidsch_dfs_e3k.o 

PERFEVENT_OBJ := \
	perfevent/perfevent_e3k.o

GLOBAL_OBJ := \
	global/global_e3k.o

core_e3k-objs := \
	$(VIDMM_OBJ) \
	$(VIDSCH_OBJ) \
    $(PERFEVENT_OBJ) \
	$(GLOBAL_OBJ) 

$(DRIVER_NAME)-objs += $(addprefix core/e3k/, $(core_e3k-objs))
