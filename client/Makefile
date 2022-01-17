MODULE_NAME = client_mod
KROOT = /lib/modules/$(shell uname -r)/build

EXTRA_CFLAGS :=
EXTRA_CFLAGS +=	-std=gnu99					\
		-fno-builtin-memset				\
		-Werror						\
		-Wframe-larger-than=400				\
		-Wno-declaration-after-statement		\
		$(INCLUDES)


PARAMS:=""
export PLUGIN :=
export TRACKER :=
export CLIST :=
export BUFFERP :=

obj-m := ../ $(MODULE_NAME).o

$(MODULE_NAME)-objs := client_module.o
$(MODULE_NAME)-objs += params.o
$(MODULE_NAME)-objs += pmu.o
$(MODULE_NAME)-objs += pmi.o
$(MODULE_NAME)-objs += device/proc.o
$(MODULE_NAME)-objs += device/proc_processes.o

$(MODULE_NAME)-objs += buffering/buffering.o
$(MODULE_NAME)-objs += clist/clist.o

ifeq ($(CLIST), ARRAY)
	$(MODULE_NAME)-objs += clist/carray.o
endif
ifeq ($(CLIST), NBLIST)
	$(MODULE_NAME)-objs += clist/nbboundedlist.o
endif

ifeq ($(BUFFERP), SYSTEM)
	$(MODULE_NAME)-objs += buffering/bsystem.o
endif
ifeq ($(BUFFERP), THREAD)
	$(MODULE_NAME)-objs += buffering/bthread.o
endif
ifeq ($(BUFFERP), CPU)
	$(MODULE_NAME)-objs += buffering/bcpu.o
endif


.PHONY: modules modules_install clean


modules:
	@$(MAKE) -w -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

clean: 
	@$(MAKE) -C $(KROOT) M=$(PWD) clean
	rm -rf   Module.symvers modules.order

load:
	for mod in $(shell cat modules.order); do sudo insmod $$mod $(PARAMS); done

unload:
	for mod in $(shell tac modules.order); do \
		sudo rmmod $(basename $$mod) || (echo "rmmod $$mod failed $$?"; exit 1); \
	done
