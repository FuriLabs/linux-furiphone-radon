export KERNEL_SRC := /lib/modules/$(shell uname -r)/build
#################### Configurations ####################
# Compile Options for bt driver configuration.
CONFIG_SUPPORT_BT_DL_WIFI_PATCH=y
CONFIG_SUPPORT_BLUEZ=n
CONFIG_SUPPORT_DVT=n
CONFIG_SUPPORT_MULTI_DEV_NODE=n

ifeq ($(CONFIG_SUPPORT_BT_DL_WIFI_PATCH), y)
    ccflags-y += -DCFG_SUPPORT_BT_DL_WIFI_PATCH=1
else
    ccflags-y += -DCFG_SUPPORT_BT_DL_WIFI_PATCH=0
endif

ifeq ($(CONFIG_SUPPORT_BLUEZ), y)
    ccflags-y += -DCFG_SUPPORT_BLUEZ=1
    ccflags-y += -DCFG_SUPPORT_HW_DVT=0
else
    ccflags-y += -DCFG_SUPPORT_BLUEZ=0
    ccflags-y += -DCFG_SUPPORT_HW_DVT=1
endif

ifeq ($(CONFIG_SUPPORT_DVT), y)
    ccflags-y += -DCFG_SUPPORT_DVT=1
else
    ccflags-y += -DCFG_SUPPORT_DVT=0
endif

ifeq ($(CONFIG_SUPPORT_DVT), y)
    ccflags-y += -DCFG_SUPPORT_DVT=1
else
    ccflags-y += -DCFG_SUPPORT_DVT=0
endif

ifeq ($(CONFIG_SUPPORT_MULTI_DEV_NODE), y)
    ccflags-y += -DCFG_SUPPORT_MULTI_DEV_NODE=1
else
    ccflags-y += -DCFG_SUPPORT_MULTI_DEV_NODE=0
endif

#################### Configurations ####################
# For chip interface, driver supports "usb", "sdio", "uart" and "btif"
MTK_CHIP_IF := usb

ifeq ($(MTK_CHIP_IF), sdio)
    MOD_NAME = btmtk_sdio_unify
    CFILES := sdio/btmtksdio.c btmtk_woble.c btmtk_buffer_mode.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_SDIO
    ccflags-y += -DSDIO_DEBUG=0
    ccflags-y += -I$(src)/include/sdio
else ifeq ($(MTK_CHIP_IF), usb)
    MOD_NAME = btmtk_usb_unify
    CFILES := usb/btmtkusb.c btmtk_woble.c btmtk_chip_reset.c
    ccflags-y += -DCHIP_IF_USB
    ccflags-y += -I$(src)/include/usb
else ifeq ($(MTK_CHIP_IF), uart)
    MOD_NAME = btmtk_uart_unify
    CFILES := uart/btmtk_uart_main.c
    ccflags-y += -DCHIP_IF_UART
    ccflags-y += -I$(src)/include/uart
else
    MOD_NAME = btmtkbtif_unify
    CFILES := btif/btmtk_btif.c
    ccflags-y += -DCHIP_IF_BTIF
    ccflags-y += -I$(src)/include/btif
endif

CFILES += btmtk_main.c btmtk_fw_log.c

ccflags-y += -I$(src)/include/ -I$(src)/

$(MOD_NAME)-objs := $(CFILES:.c=.o)

obj-m += $(MOD_NAME).o
