// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*
 * DW9800WAF voice coil motor driver
 * 兼容 CN3927EAF
 */
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include "lens_info.h"

#define AF_DRVNAME "DW9800WAF_DRV"
#define AF_I2C_SLAVE_ADDR_CN 0x18
#define AF_I2C_SLAVE_ADDR_DW 0x1c

#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...) \
	pr_debug(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

static struct i2c_client *g_pstAF_I2Cclient;
static int *g_pAF_Opened;
static spinlock_t *g_pAF_SpinLock;

static unsigned long g_u4AF_INF;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4TargetPosition;
static unsigned long g_u4CurrPosition;

static int g_motor_type = -1; /* -1:Not detected, 0:CN3927, 1:DW9800 */

static u8 cn_init_regs_data[][2] = {
	{0xec, 0xa3},
	{0xa1, 0x14},
	{0xf2, 0xf0},
	{0xdc, 0x51}
};


static u8 dw_init_regs_data[][2] = {
	{0x02, 0x01},
	{0x02, 0x00},
	{0x06, 0x40},
	{0x07, 0x60},
	{0x08, 0x49},
};

static int af_simple_ack_probe(void)
{
	int i;
	int ret;
	u16 old_addr = g_pstAF_I2Cclient->addr;

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_CN >> 1;
	for (i = 0; i < 2; i++) {
		ret = i2c_master_send(g_pstAF_I2Cclient, NULL, 0);
		if (ret == 0) {
			printk("CN3927EAF ACK detected (attempt %d)\n", i+1);
			g_pstAF_I2Cclient->addr = old_addr;
			return 0;
		}
		msleep(5);
	}

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_DW >> 1;
	for (i = 0; i < 2; i++) {
		ret = i2c_master_send(g_pstAF_I2Cclient, NULL, 0);
		if (ret == 0) {
			printk("DW9800WAF ACK detected (attempt %d)\n", i+1);
			g_pstAF_I2Cclient->addr = old_addr;
			return 1;
		}
		msleep(5);
	}
	
	g_pstAF_I2Cclient->addr = old_addr;
	printk("MAINAF No ACK from either motor\n");
	return -1;
}


static int test_cn_register_write(void)
{
	int ret;
	u16 old_addr = g_pstAF_I2Cclient->addr;
	char test_cmd[2] = {0x00, 0x00};

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_CN >> 1;
	
	ret = i2c_master_send(g_pstAF_I2Cclient, test_cmd, 2);
	g_pstAF_I2Cclient->addr = old_addr;
	
	if (ret == 2) {
		printk("CN3927 write test PASSED\n");
		return 0;
	} else {
		printk("CN3927 write test FAILED, ret=%d\n", ret);
		return -1;
	}
}

static int test_dw_register_write(void)
{
	int ret;
	u16 old_addr = g_pstAF_I2Cclient->addr;

	char test_cmd[3] = {0x03, 0x00, 0x00};

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_DW >> 1;

	ret = i2c_master_send(g_pstAF_I2Cclient, test_cmd, 3);
	g_pstAF_I2Cclient->addr = old_addr;
	
	if (ret == 3) {
		printk("DW9800 write test PASSED\n");
		return 0;
	} else {
		printk("DW9800 write test FAILED, ret=%d\n", ret);
		return -1;
	}
}

static int af_enhanced_probe_motor(void)
{
	int detected_type = -1;
	detected_type = af_simple_ack_probe();
	
	if (detected_type == 0) {
		if (test_cn_register_write() == 0) {
			printk("MAINAF CN3927EAF\n");
		} else {
			printk("MAINAF CN3927 register test failed\n");
			detected_type = -1;
		}
	} else if (detected_type == 1) {
		if (test_dw_register_write() == 0) {
			printk("MAINAF DW9800WAF\n");
		} else {
			printk("MAINAF DW9800 register test failed\n");
			detected_type = -1;
		}
	}
	
	if (detected_type == -1) {
		/* CN3927 format */
		if (test_cn_register_write() == 0) {
			printk("MAINAF CN3927 format works\n");
			detected_type = 0;
		} else {
			printk("MAINAF CN3927 format failed\n");
		}
		
		/* DW9800 format */
		if (detected_type == -1) {
			if (test_dw_register_write() == 0) {
				printk("MAINAF DW9800 format works\n");
				detected_type = 1;
			} else {
				printk("MAINAF DW9800 format failed\n");
			}
		}
	}

	if (detected_type == -1) {
		detected_type = 1;
		printk("MAINAF Defaulting to DW9800WAF\n");
	}

	g_motor_type = detected_type;

	if (detected_type == 0) {
		printk("MAINAF === FINAL RESULT: CN3927EAF Motor ===\n");
	} else {
		printk("MAINAF === FINAL RESULT: DW9800WAF Motor ===\n");
	}

	return detected_type;
}

int af_get_motor_type(void)
{
	if (g_motor_type == -1) {
		af_enhanced_probe_motor();
	}
	return g_motor_type;
}

static int cn_init_regs(void)
{
	int i;
	int ret;
	u16 old_addr = g_pstAF_I2Cclient->addr;

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_CN >> 1;
	
	for (i = 0; i < ARRAY_SIZE(cn_init_regs_data); i++) {
		ret = i2c_master_send(g_pstAF_I2Cclient, cn_init_regs_data[i], 2);
		if (ret != 2) {
			printk("MAINAF CN3927 init failed at reg 0x%02X, ret=%d\n", 
				cn_init_regs_data[i][0], ret);
		}
		udelay(100);
	}
	
	g_pstAF_I2Cclient->addr = old_addr;
	printk("MAINAF CN3927EAF initialization complete\n");
	return 0;
}

static int dw_initdrv(void)
{
	int i;
	int ret;
	u16 old_addr = g_pstAF_I2Cclient->addr;

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_DW >> 1;
	
	for (i = 0; i < ARRAY_SIZE(dw_init_regs_data); i++) {
		if (dw_init_regs_data[i][0] == 0xFE && dw_init_regs_data[i][1] == 0xFE) {
			udelay(100);
			continue;
		}
		
		ret = i2c_master_send(g_pstAF_I2Cclient, dw_init_regs_data[i], 2);
		if (ret != 2) {
			printk("DW9800 init failed at step %d (reg 0x%02X=0x%02X), ret=%d\n", 
				i, dw_init_regs_data[i][0], dw_init_regs_data[i][1], ret);
		}
		udelay(100);
	}
	
	g_pstAF_I2Cclient->addr = old_addr;
	printk("MAINAF DW9800WAF initialization complete\n");
	return 0;
}

static int af_init_by_type(void)
{
	int type = 0;

	if (g_motor_type == -1) {
		type = af_enhanced_probe_motor();
		if (type == -1) {
			printk("MAINAF Motor detection failed, cannot proceed\n");
			return -ENODEV;
		}
		printk("MAINAF Motor detection complete: %s\n", type == 0 ? "CN3927EAF" : "DW9800WAF");
	}

	if (g_motor_type == 0) {
		return cn_init_regs();
	} else if (g_motor_type == 1) {
		return dw_initdrv();
	}
	return -1;
}

static int s4AF_WriteReg_by_type(u16 a_u2Data)
{
	int ret = 0;
	u16 old_addr = g_pstAF_I2Cclient->addr;
	
	if (g_motor_type == 0) {
		char puSendCmd[2] = {(char)(a_u2Data >> 4),
				     (char)((a_u2Data & 0xF) << 4)};
		g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_CN >> 1;
		ret = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2);
		if (ret == 2) {
			ret = 0;
		} else {
			ret = -EIO;
		}
	} else {
		char puSendCmd[3] = {0x03, (char)(a_u2Data >> 8),
				     (char)(a_u2Data & 0xFF)};
		g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_DW >> 1;
		ret = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 3);
		if (ret == 3) {
			ret = 0;
		} else {
			ret = -EIO;
		}
	}

	g_pstAF_I2Cclient->addr = old_addr;
	
	return ret;
}

static int s4DW9800WAF_ReadReg(unsigned short *a_pu2Result)
{
	u8 data[2];
	int ret = 0;
	u16 old_addr = g_pstAF_I2Cclient->addr;
	
	if (g_motor_type == 0) {
		char read_cmd[1] = {0x00}; 
		g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_CN >> 1;

		ret = i2c_master_send(g_pstAF_I2Cclient, read_cmd, 1);
		if (ret == 1) {
			ret = i2c_master_recv(g_pstAF_I2Cclient, (char *)data, 2);
			if (ret == 2) {
				*a_pu2Result = (((u16)data[0]) << 4) + (data[1] >> 4);
			} else {
				printk("MAINAF CN3927 read data failed, ret=%d\n", ret);
				ret = -1;
			}
		} else {
			printk("MAINAF CN3927 read command failed, ret=%d\n", ret);
			ret = -1;
		}
	} else {
		char read_cmd[1] = {0x03};
		g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR_DW >> 1;
		
		ret = i2c_master_send(g_pstAF_I2Cclient, read_cmd, 1);
		if (ret == 1) {
			ret = i2c_master_recv(g_pstAF_I2Cclient, (char *)data, 2);
			if (ret == 2) {
				*a_pu2Result = (data[0] << 8) | data[1];
			} else {
				printk("MAINAF DW9800 read data failed, ret=%d\n", ret);
				ret = -1;
			}
		} else {
			printk("MAINAF DW9800 read command failed, ret=%d\n", ret);
			ret = -1;
		}
	}
	
	g_pstAF_I2Cclient->addr = old_addr;
	
	return (ret == 2) ? 0 : -1;
}

static inline int getAFInfo(__user struct stAF_MotorInfo *pstMotorInfo)
{
	struct stAF_MotorInfo stMotorInfo;

	stMotorInfo.u4MacroPosition = g_u4AF_MACRO;
	stMotorInfo.u4InfPosition = g_u4AF_INF;
	stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	stMotorInfo.bIsSupportSR = 1;

	stMotorInfo.bIsMotorMoving = 1;

	if (*g_pAF_Opened >= 1)
		stMotorInfo.bIsMotorOpen = 1;
	else
		stMotorInfo.bIsMotorOpen = 0;

	if (copy_to_user(pstMotorInfo, &stMotorInfo,
		sizeof(struct stAF_MotorInfo)))
		printk("copy to user failed when getting motor information\n");

	return 0;
}

static inline int moveAF(unsigned long a_u4Position)
{
	int ret = 0;

	if ((a_u4Position > g_u4AF_MACRO) || (a_u4Position < g_u4AF_INF)) {
		printk("Position out of range: %lu (INF=%lu, MACRO=%lu)\n",
			a_u4Position, g_u4AF_INF, g_u4AF_MACRO);
		return -EINVAL;
	}

	if (g_u4CurrPosition == a_u4Position)
		return 0;

	spin_lock(g_pAF_SpinLock);
	g_u4TargetPosition = a_u4Position;
	spin_unlock(g_pAF_SpinLock);

	ret = s4AF_WriteReg_by_type((unsigned short)g_u4TargetPosition);
	if (ret == 0) {
		spin_lock(g_pAF_SpinLock);
		g_u4CurrPosition = g_u4TargetPosition;
		spin_unlock(g_pAF_SpinLock);
	} else {
		printk("Failed to move motor to position: %lu\n", a_u4Position);
		ret = -1;
	}

	return ret;
}

static inline int setAFInf(unsigned long a_u4Position)
{
	spin_lock(g_pAF_SpinLock);
	g_u4AF_INF = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
	return 0;
}

static inline int setAFMacro(unsigned long a_u4Position)
{
	spin_lock(g_pAF_SpinLock);
	g_u4AF_MACRO = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
	return 0;
}

/* ////////////////////////////////////////////////////////////// */
long DW9800WAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
		unsigned long a_u4Param)
{
	long i4RetValue = 0;

	switch (a_u4Command) {
	case AFIOC_G_MOTORINFO:
		i4RetValue = getAFInfo((__user struct stAF_MotorInfo *)
				(a_u4Param));
		break;

	case AFIOC_T_MOVETO:
		i4RetValue = moveAF(a_u4Param);
		break;

	case AFIOC_T_SETINFPOS:
		i4RetValue = setAFInf(a_u4Param);
		break;

	case AFIOC_T_SETMACROPOS:
		i4RetValue = setAFMacro(a_u4Param);
		break;

	default:
		LOG_INF("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int DW9800WAF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	printk("Start release, motor type: %s\n",
		g_motor_type == 0 ? "CN3927EAF" : "DW9800WAF");

	if (*g_pAF_Opened == 2)
		LOG_INF("Wait\n");

	if (*g_pAF_Opened) {
		LOG_INF("Free\n");

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 0;
		spin_unlock(g_pAF_SpinLock);
	}

	LOG_INF("End\n");

	return 0;
}

int DW9800WAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
	spinlock_t *pAF_SpinLock, int *pAF_Opened)
{
	int ret = 0;
	unsigned short InitPos = 0;

	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_SpinLock = pAF_SpinLock;
	g_pAF_Opened = pAF_Opened;
	
	/* Detect motor type */
	if (g_motor_type == -1) {
		g_motor_type = af_enhanced_probe_motor();
		
		if (g_motor_type == -1) {
			LOG_INF("MAINAF Probe failed, defaulting to DW9800\n");
		}
	}
	
	printk("MAINAF Motor type detected: %s   g_motor_type:%d\n", 
		g_motor_type == 0 ? "CN3927EAF" : "DW9800WAF",g_motor_type);
	
	if (*g_pAF_Opened == 1) {
		if (af_init_by_type() != 0) {
			printk("ERROR: MAINAF Motor initialization failed!\n");
		}
		
		ret = s4DW9800WAF_ReadReg(&InitPos);

		if (ret == 0) {
			LOG_INF("Init Pos %6d\n", InitPos);

			spin_lock(g_pAF_SpinLock);
			g_u4CurrPosition = (unsigned long)InitPos;
			spin_unlock(g_pAF_SpinLock);

		} else {
			spin_lock(g_pAF_SpinLock);
			g_u4CurrPosition = 0;
			spin_unlock(g_pAF_SpinLock);
		}

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 2;
		spin_unlock(g_pAF_SpinLock);
	}

	return 1;
}

int DW9800WAF_GetFileName(unsigned char *pFileName)
{
	#if SUPPORT_GETTING_LENS_FOLDER_NAME
	char FilePath[256];
	char *FileString;

	sprintf(FilePath, "%s", __FILE__);
	FileString = strrchr(FilePath, '/');
	*FileString = '\0';
	FileString = (strrchr(FilePath, '/') + 1);
	strncpy(pFileName, FileString, AF_MOTOR_NAME);
	LOG_INF("FileName : %s\n", pFileName);
	#else
	pFileName[0] = '\0';
	#endif
	return 1;
}
