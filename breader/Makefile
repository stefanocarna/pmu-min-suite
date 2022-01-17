MODULE_NAME = breader
KROOT = /lib/modules/$(shell uname -r)/build

EXTRA_CFLAGS :=
EXTRA_CFLAGS +=	-std=gnu99					\
		-fno-builtin-memset				\
		-Werror						\
		-Wframe-larger-than=400				\
		-Wno-declaration-after-statement		\
		$(INCLUDES)


obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs := breader_module.o
$(MODULE_NAME)-objs += breader_seq.o
$(MODULE_NAME)-objs += breader_raw.o
$(MODULE_NAME)-objs += breader_mmap.o

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
