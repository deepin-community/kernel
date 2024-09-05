ccflags-y += \
    -I${GFGPU_FULL_PATH}/core/include \
    -I${GFGPU_FULL_PATH}/core/context \
    -I${GFGPU_FULL_PATH}/core/global \
    -I${GFGPU_FULL_PATH}/core/powermgr \
    -I${GFGPU_FULL_PATH}/core/vidmm \
    -I${GFGPU_FULL_PATH}/core/vidsch \
    -I${GFGPU_FULL_PATH}/core/util \
    -I${GFGPU_FULL_PATH}/core/isr \
    -I${GFGPU_FULL_PATH}/core/perfevent \
    -I${GFGPU_FULL_PATH}/shared

CORE_OBJ := kernel_interface.o

UTIL_OBJ := \
	util/util.o \
	util/handle_manager.o \
	util/heap_manager.o \
	util/queue.o \
	util/ring_buffer.o

VIDMM_OBJ := \
	vidmm/vidmm.o \
	vidmm/vidmm_allocate.o \
	vidmm/vidmm_paging.o \
	vidmm/vidmm_lock.o

VIDSCH_OBJ := \
	vidsch/vidsch.o \
	vidsch/vidsch_task.o \
	vidsch/vidsch_submit.o \
	vidsch/vidsch_workerthread.o \
	vidsch/vidsch_render.o \
	vidsch/vidsch_sync.o \
	vidsch/vidsch_gating.o \
	vidsch/vidsch_daemon_thread.o

PERF_EVENT_OBJ := \
	perfevent/perfevent.o

GLOBAL_OBJ := global/global.o

CONTEXT_OBJ := \
	context/context.o \
	context/di_context.o

POWERMGR_OBJ := powermgr/powermgr.o

core-objs := \
	$(UTIL_OBJ) \
	$(SHAREDAREA_OBJ) \
	$(GLOBAL_OBJ) \
	$(CONTEXT_OBJ) \
	$(POWERMGR_OBJ) \
	$(VIDMM_OBJ) \
	$(VIDSCH_OBJ) \
	$(SOFT_FLOAT_OBJ) \
	$(CORE_OBJ) \
	$(PERF_EVENT_OBJ)

$(DRIVER_NAME)-objs += $(addprefix core/, $(core-objs))

ifeq ($(CHIP), E3k)
	include $(GFGPU_FULL_PATH)/core/e3k/Makefile
endif
