/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2016 MediaTek Inc.
*/

#ifndef _FW_LOG_WIFI_H_
#define _FW_LOG_WIFI_H_

#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
int fw_log_wifi_init(void);
int fw_log_wifi_deinit(void);
#endif /* CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH */

#if (CFG_ANDORID_CONNINFRA_SUPPORT == 1)
int fw_log_wifi_irq_handler(void);
#endif /* CFG_ANDORID_CONNINFRA_SUPPORT */

#if (CFG_ANDORID_CONNINFRA_COREDUMP_SUPPORT == 1)
void fw_log_connsys_coredump_init(void);
void fw_log_connsys_coredump_deinit(void);
void fw_log_connsys_coredump_start(unsigned int, char *);
int fw_log_reg_readable(void);
#endif

#endif /*_FW_LOG_WIFI_H_*/
