ccflags-y += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
obj-y += selinuxp.o

FLASK_HDR :=  $(srctree)/security/selinux/flask.h
$(obj)/selinuxp.o: $(FLASK_HDR)
