// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <env.h>
#include <version.h>

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
	char *p;

	p = env_get("preboot");
	if (p != NULL) {
		int prev = 0;

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			prev = disable_ctrlc(1); /* disable Ctrl-C checking */

		run_command_list(p, -1, 0);

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			disable_ctrlc(prev);	/* restore Ctrl-C checking */
	}
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

	if (IS_ENABLED(CONFIG_VERSION_VARIABLE))
		env_set("ver", version_string);  /* set version variable */

	cli_init();

	if (IS_ENABLED(CONFIG_USE_PREBOOT))
		run_preboot_environment_command();

	if (IS_ENABLED(CONFIG_UPDATE_TFTP))
		update_tftp(0UL, NULL, NULL);

//#ifdef CONFIG_V205_WORKAROUND_PETA2019_1_UBOOTENV
	printf("notice: hardcode workaround for uboot env change since 2018.3\n");

#ifdef CONFIG_CMD_AUP_RAM_ENV
	env_set("aup_get_rootfselect", "setenv aup_rootfselect 2; i2c dev 0 && i2c read 0x50 0x14 1 0x10000000 && setenvram.bd aup_rootfselect 0x10000000 && if test ${aup_rootfselect} = 3; then echo 'rootfselect mmcblk0p3'; setenv aup_rootfselect 3; else if test ${aup_rootfselect} = 2;then echo 'rootfselect mmcblk0p2'; else echo 'no rootfselect, use default mmcblk0p2. (set it 2 by cmd: i2c mw 0x50 0x14 2 1)'; fi; fi;");
	run_command("run aup_get_rootfselect", 0);
#else
	env_set("aup_rootfselect", "2");
	env_set("aup_get_rootfselect", "none");
#endif

#ifdef CONFIG_CMD_AUP_UART_ENV
	run_command("setenvuart uartconf 1 115200; setenvuart moduletype 1 aup_module_type || setenv aup_module_type XU30 && mmc dev 0 && setenv aup_module_type V305;", 0);
	run_command("if test ${aup_module_type} = V205B2; then echo 'aup_module_type V205B2'; else if test ${aup_module_type} = V205A1;then echo 'aup_module_type V205A1'; else echo 'aup_module_type unknow';fi; fi;", 0);
	run_command("setenvuart bootinfo 1 aup_CpuMAC aup_ManageMAC aup_SlotID aup_NodeID aup_ChassType;", 0);
	run_command("setenv ethaddr ${aup_CpuMAC};", 0);
#else
	run_command("setenv aup_module_type XU30 && mmc dev 0 && setenv aup_module_type V305;", 0);
#endif

#ifdef CONFIG_CMD_AUP_NODE_BOOT_CLIENT
	env_set("aup_get_nodebootinfo", "nodebootclient nodebootinfo ${aup_SlotID} ${aup_NodeID} ${aup_ManageMAC} ${aup_CpuMAC} aup_nodeip aup_dl_address aup_mask aup_gateway aup_dns1 aup_dns2 aup_hostip aup_ntp_server aup_syslog_server aup_syslog_server_port aup_dhcp; setenv ipaddr ${aup_nodeip}; setenv serverip ${aup_hostip};");
	run_command("run aup_get_nodebootinfo;", 0);
#else
	env_set("aup_get_nodebootinfo", "none");
#endif

	run_command("setenv aup_imgname image.ub", 0);
	run_command("setenv aup_imgaddr 0x10000000", 0);
	run_command("setenv aup_imgofst 0x1000000", 0);
	//run_command("setenv aup_imgsize 0x2fe0000", 0);
	run_command("if test ${aup_module_type} = XU30; then setenv aup_imgsize 0x6fe0000; else setenv aup_imgsize 0x2fe0000; fi;", 0);
	run_command("setenv aup_sdboot   \"mmc dev 1 && mmcinfo && load mmc 1:1 ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
#ifdef CONFIG_CMD_AUP_IMG_INFO
	run_command("setenv aup_emmcboot \"mmc dev 0 && mmcinfo && load mmc 0:$\"{aup_rootfselect}\" ${aup_imgaddr} ${aup_imgname} && getimginfo ${aup_imgaddr} fdt@1 aup_fdt1addr aup_fdt1len && echo aup_fdt1addr=$\"{aup_fdt1addr}\" && cp $\"{aup_fdt1addr}\" ${fdt_addr} 0x100000 && fdt addr $fdt_addr && fdt get value bootargs /chosen bootargs; if test ${aup_rootfselect} = 3; then setexpr bootargs sub 'root=/dev/mmcblk0p2' 'root=/dev/mmcblk0p3'; fi; if test ${aup_module_type} = V205B2; then echo 'V205B? dts 2'; bootm ${aup_imgaddr}#conf@2; else echo 'V205A? dts 1'; bootm ${aup_imgaddr}#conf@1; fi \"", 0);
//	run_command("setenv aup_emmcboot \"echo 'fixme: 2020.1 use emmcboot'; mmc dev 0 && mmcinfo && load mmc 0:$\"{aup_rootfselect}\" ${aup_imgaddr} ${aup_imgname} && setenv bootargs \"earlycon console=ttyPS0,115200 clk_ignore_unused root=/dev/mmcblk0p$\"{aup_rootfselect}\" rw rootwait\"; if test ${aup_module_type} = V205B2; then bootm ${aup_imgaddr}#conf@2; else bootm ${aup_imgaddr}#conf@1; fi\"", 0);
//	run_command("setenv aup_emmcboot \"echo 'fixme: 2020.1 use emmcboot'; mmc dev 0 && mmcinfo && load mmc 0:$\"{aup_rootfselect}\" ${aup_imgaddr} ${aup_imgname} && setenv bootargs \"earlycon console=ttyPS0,115200 clk_ignore_unused root=/dev/mmcblk0p$\"{aup_rootfselect}\" rw rootwait\"; bootm ${aup_imgaddr}\"", 0);
#else
	run_command("setenv aup_emmcboot \"mmc dev 0 && mmcinfo && load mmc 0:$\"{aup_rootfselect}\" ${aup_imgaddr} ${aup_imgname} && cp 0x11143bfc $fdt_addr 0x100000 && fdt addr $fdt_addr && fdt get value bootargs /chosen bootargs; if test ${aup_rootfselect} = 3; then setexpr bootargs sub 'root=/dev/mmcblk0p2' 'root=/dev/mmcblk0p3'; fi; if test ${aup_module_type} = V205B2; then bootm ${aup_imgaddr}#conf@2; else bootm ${aup_imgaddr}#conf@1; fi \"", 0);
#endif
	run_command("setenv aup_qspi_recover_boot \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} $\"{aup_imgsize}\" && setexpr bootargs sub 'root=/dev/mmcblk0p2' 'root=/dev/ram'; setexpr bootargs sub 'root=/dev/mmcblk0p3' 'root=/dev/ram'; if test ${aup_module_type} = V205B2; then echo 'V205B? dts 2'; bootm ${aup_imgaddr}#conf@2; else echo 'V205A? dts 1'; bootm ${aup_imgaddr}#conf@1; fi \"", 0);
	run_command("setenv aup_qspi_boot \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} $\"{aup_imgsize}\" && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_qspiboot  \"if test $\"{aup_module_type}\" = XU30; then run aup_qspi_boot; else run aup_emmcboot || run aup_qspi_recover_boot; fi;\"", 0);
	run_command("setenv aup_usbboot \"usb start && load usb 0 ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr} \"", 0);
	run_command("setenv aup_tftpboot \"setenv ethact eth0 && setenv serverip 192.168.1.254 && setenv ipaddr 192.168.1.250 && tftpboot ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_boottry \"for target in  qspi sd emmc usb tftp; do run aup_${target}boot; done\"", 0);
	run_command("setenv bootcmd \"run aup_${modeboot}\"", 0);

	//sdbootdev=0 or 1
	//run_command("if test \"${modeboot}\" = \"sdboot\"; then setenv sdboot \"mmc dev 1 && mmcinfo && load mmc 1:1 0x10000000 image.ub && bootm 0x10000000\"; fi;", 0);
//#endif
	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
