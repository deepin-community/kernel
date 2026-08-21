ccflags-y += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
ccflags-y += -I$(objtree)/security/selinux
obj-y += selinuxp.o

ifdef CONFIG_IEE_SELINUX_P
SELINUX_GENHDRS := $(objtree)/security/selinux/flask.h \
		   $(objtree)/security/selinux/av_permissions.h

quiet_cmd_haoc_genhdrs = GEN     $(SELINUX_GENHDRS)
      cmd_haoc_genhdrs = mkdir -p $(objtree)/security/selinux; \
				 $< $(SELINUX_GENHDRS)

$(SELINUX_GENHDRS): scripts/selinux/genheaders/genheaders FORCE
	$(call if_changed,haoc_genhdrs)

$(obj)/selinuxp.o: $(SELINUX_GENHDRS)
endif
