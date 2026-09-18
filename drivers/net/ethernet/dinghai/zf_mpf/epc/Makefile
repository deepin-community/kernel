KDIR=/home/ls/kernel-5.10.y

obj-m  += zte-epc.o
zte-epc-y += pcie-zte-zf-hdma.o pcie-zte-zf-epc.o virt-dma.o


all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
