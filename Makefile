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

$(MODULE_NAME)-objs := system_hook.o
$(MODULE_NAME)-objs += ptracker.o

.PHONY: modules modules_install clean


modules:
	@$(MAKE) -w -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

clean: 
	@$(MAKE) -C $(KROOT) M=$(PWD) clean
	rm -rf   Module.symvers modules.order
