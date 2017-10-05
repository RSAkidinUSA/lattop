obj-m := lattop.o
lattop-objs := p03_main.o p03_kprobe.o p03_rb.o

CONFIG_MODULE_SIG=n
KDIR := ~/linux

PWD := $(shell pwd)

all: module
	
module:
	make -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean
