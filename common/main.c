// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
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
#endif /* CONFIG_PREBOOT */
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

	if (IS_ENABLED(CONFIG_VERSION_VARIABLE))
		env_set("ver", version_string);  /* set version variable */

	cli_init();

	run_preboot_environment_command();

	if (IS_ENABLED(CONFIG_UPDATE_TFTP))
		update_tftp(0UL, NULL, NULL);

//#ifdef CONFIG_V205_WORKAROUND_PETA2019_1_UBOOTENV
	printf("notice: hardcode workaround for uboot env change since 2018.3\n");

#ifdef CONFIG_CMD_AUP_UART
	run_command("setenvuart uartconf 1 115200; setenvuart moduletype 1 aup_module_type;", 0);
	run_command("if test ${aup_module_type} = V205B2; then echo 'aup_module_type V205B2'; else if test ${aup_module_type} = V205A1;then echo 'aup_module_type V205A1'; else echo 'aup_module_type unknow';fi; fi;", 0);
	run_command("setenvuart bootinfo 1 aup_CpuMAC aup_ManageMAC aup_SlotID aup_NodeID aup_ChassType;", 0);
	run_command("setenv ethaddr ${aup_CpuMAC};", 0);
#endif

#ifdef CONFIG_CMD_AUP_NODE_BOOT_CLIENT
	//run_command("if test ${aup_module_type} = V205B2; then nodebootclient nodebootinfo ${aup_SlotID} ${aup_NodeID} ${aup_ManageMAC} ${aup_CpuMAC} aup_nodeip aup_dl_address aup_mask aup_gateway aup_dns1 aup_dns2 aup_hostip aup_ntp_server aup_syslog_server aup_syslog_server_port;else if test ${aup_module_type} = V205A1;then echo 'V205A1 board not support nodebootclient command.'; else echo 'unknow board not support nodebootclient command';fi; fi;", 0);
	//run_command("if test ${aup_module_type} = V205B2; then nodebootclient nodebootinfo ${aup_SlotID} ${aup_NodeID} ${aup_ManageMAC} ${aup_CpuMAC} aup_nodeip aup_dl_address aup_mask aup_gateway aup_dns1 aup_dns2 aup_hostip aup_ntp_server aup_syslog_server aup_syslog_server_port;else echo 'only V205B2 board support nodebootclient command.'; fi;", 0);
	env_set("aup_get_nodebootinfo", "\"if test ${aup_module_type} = V205B2; then nodebootclient nodebootinfo ${aup_SlotID} ${aup_NodeID} ${aup_ManageMAC} ${aup_CpuMAC} aup_nodeip aup_dl_address aup_mask aup_gateway aup_dns1 aup_dns2 aup_hostip aup_ntp_server aup_syslog_server aup_syslog_server_port; setenv ipaddr ${aup_nodeip}; setenv serverip ${aup_hostip}; else echo 'only V205B2 board support nodebootclient command.'; fi;\"");
#endif

#ifdef CONFIG_CMD_AUP_RAM
	run_command("i2c dev 0; i2c read 0x50 0x14 1 0x10000000; setenvram.bd aup_rootfselect 0x10000000;", 0);
#endif
	run_command("if test ${aup_rootfselect} = 3; then echo 'rootfselect mmcblk0p3'; else if test ${aup_rootfselect} = 2;then echo 'rootfselect mmcblk0p2'; else i2c mw 0x50 0x14 2 1; echo 'change rootfselect from '; printenv aup_rootfselect; echo ' to 2=mmcblk0p2'; setenv aup_rootfselect 2;fi; fi;", 0);
	run_command("setenv aup_imgname image.ub", 0);
	run_command("setenv aup_imgaddr 0x10000000", 0);
	run_command("setenv aup_imgsize 0x2fe0000", 0);
	run_command("setenv aup_imgofst 0x1000000", 0);
	run_command("setenv aup_sdboot   \"mmc dev 1 && mmcinfo && load mmc 1:1 ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
#ifdef CONFIG_CMD_AUP_IMG_INFO
	run_command("setenv aup_emmcboot \"mmc dev 0 && mmcinfo && load mmc 0:${aup_rootfselect} ${aup_imgaddr} ${aup_imgname} && getimginfo ${aup_imgaddr} fdt@1 aup_fdt1addr aup_fdt1len && echo aup_fdt1addr=$\"{aup_fdt1addr}\" && cp $\"{aup_fdt1addr}\" ${fdt_addr} 0x100000 && fdt addr $fdt_addr && fdt get value bootargs /chosen bootargs; if test ${aup_rootfselect} = 3; then setexpr bootargs sub 'root=/dev/mmcblk0p2' 'root=/dev/mmcblk0p3'; fi; if test ${aup_module_type} = V205B2; then echo 'V205B? dts 2'; bootm ${aup_imgaddr}#conf@2; else echo 'V205A? dts 1'; bootm ${aup_imgaddr}#conf@1; fi \"", 0);
	//run_command("mmc dev 1 && mmcinfo && load mmc 1:1 ${aup_imgaddr} ${aup_imgname};getimginfo ${aup_imgaddr} kernel@1 aup_kerneladdr aup_kernellen; getimginfo ${aup_imgaddr} ramdisk@1 aup_ramdiskaddr aup_ramdisklen;getimginfo ${aup_imgaddr} fdt@1 aup_fdt1addr aup_fdt1len;", 0);
#else
	run_command("setenv aup_emmcboot \"mmc dev 0 && mmcinfo && load mmc 0:${aup_rootfselect} ${aup_imgaddr} ${aup_imgname} && cp 0x11143bfc $fdt_addr 0x100000 && fdt addr $fdt_addr && fdt get value bootargs /chosen bootargs; if test ${aup_rootfselect} = 3; then setexpr bootargs sub 'root=/dev/mmcblk0p2' 'root=/dev/mmcblk0p3'; fi; if test ${aup_module_type} = V205B2; then bootm ${aup_imgaddr}#conf@2; else bootm ${aup_imgaddr}#conf@1; fi \"", 0);
#endif
	//load mmc 0:${aup_rootfselect} ${aup_imgaddr} ${aup_imgname} ; iminfo 0x10000000; (Data Start:   0x10f783ec); fdt addr 0x10f783ec; fdt get value bootargs /chosen bootargs;
	//mmc dev 0;load mmc 0:3 0x10000000 image.ub ; iminfo 0x10000000; cp 0x10f783ec $fdt_addr 0x100000; fdt addr $fdt_addr; fdt get value bootargs /chosen bootargs; fdt set /chosen bootargs "earlycon console=ttyPS0,115200 clk_ignore_unused root=/dev/mmcblk0p3 rw rootwait"; setexpr bootargs sub "root=/dev/mmcblk0p2" "root=/dev/mmcblk0p4" booti 0x100000e8 - $fdt_addr;
	run_command("setenv aup_qspi_recover_boot \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && setexpr bootargs sub 'root=/dev/mmcblk0p2' 'root=/dev/ram'; setexpr bootargs sub 'root=/dev/mmcblk0p3' 'root=/dev/ram'; bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_qspiboot \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && bootm ${aup_imgaddr}\"", 0);
	//run_command("setenv aup_qspiboot1 \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && bootm ${aup_imgaddr}#conf@1\"", 0);
	//run_command("setenv aup_qspiboot2 \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && bootm ${aup_imgaddr}#conf@2\"", 0);
	run_command("setenv aup_qspiboot  \"run aup_emmcboot || run aup_qspi_recover_boot\"", 0);
	run_command("setenv aup_usbboot \"usb start && load usb 0 ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_tftpboot \"setenv ethact eth0 && setenv serverip 192.168.1.254 && setenv ipaddr 192.168.1.250 && tftpboot ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_boottry \"for target in  qspi sd emmc usb tftp; do run aup_${target}boot; done\"", 0);
	run_command("setenv bootcmd \"run aup_${modeboot}\"", 0);
	//sdbootdev=0 or 1
	//run_command("if test \"${modeboot}\" = \"sdboot\"; then setenv sdboot \"mmc dev 1 && mmcinfo && load mmc 1:1 0x10000000 image.ub && bootm 0x10000000\"; fi;", 0);
	//run_command("if test \"${modeboot}\" = \"qspiboot\"; then setenv qspiboot \"sf probe && sf read 0x10000000 0x1000000 0x2fe0000 && bootm 0x10000000\"; fi;", 0);
//#endif
	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
