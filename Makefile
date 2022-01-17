MODULE_NAME = shook
KROOT = /lib/modules/$(shell uname -r)/build

EXTRA_CFLAGS :=
EXTRA_CFLAGS +=	-std=gnu99					\
		-fno-builtin-memset				\
		-Werror						\
		-Wframe-larger-than=400				\
		-Wno-declaration-after-statement		\
		$(INCLUDES)

obj-m := $(MODULE_NAME).o

PLUGIN := DUMMY
TRACKER := HASH

$(MODULE_NAME)-objs := system_hook.o
$(MODULE_NAME)-objs += ptracker.o

ifeq ($(PLUGIN), KPROBE)
	$(MODULE_NAME)-objs += system_hook_kprobe.o
endif
ifeq ($(PLUGIN), TRACEP)
	$(MODULE_NAME)-objs += hooks.o
	$(MODULE_NAME)-objs += system_hook_tracep.o
endif

ifeq ($(TRACKER), HASH)
	$(MODULE_NAME)-objs += ptracker_hash.o
endif
ifeq ($(TRACKER), STACK)
	$(MODULE_NAME)-objs += ptracker_stack.o
endif
ifeq ($(TRACKER), PERF)
	$(MODULE_NAME)-objs += ptracker_perf.o
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
	for mod in $(shell cat modules.order); do sudo insmod $$mod; done

unload:
	for mod in $(shell tac modules.order); do \
		sudo rmmod $(basename $$mod) || (echo "rmmod $$mod failed $$?"; exit 1); \
	done
