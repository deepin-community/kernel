# SPDX-License-Identifier: GPL-2.0-only
# scmi-bus-y = bus.o
#scmi-core-objs := $(scmi-bus-y)

#scmi-driver-y = driver.o
#scmi-transport-$(CONFIG_ARM_SCMI_TRANSPORT_MAILBOX) += mailbox.o
#scmi-protocols-y = base.o clock.o perf.o
#scmi-module-objs := $(scmi-driver-y) $(scmi-protocols-y) $(scmi-transport-y)

#obj-$(CONFIG_ARM_SCMI_PROTOCOL) += scmi-core.o
#obj-$(CONFIG_ARM_SCMI_PROTOCOL) += scmi-module.o

obj-y += bus.o
obj-y += driver.o
obj-y += mailbox.o
obj-y += base.o
obj-y += clock.o
obj-y += perf.o
obj-y += shmem.o
obj-y += domain.o
obj-y += scmi_perf_domain.o
obj-y += clk-scmi.o
obj-y += cix_scmi_em.o
