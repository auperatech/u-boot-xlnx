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

	run_command("i2c read 0x10000000 0x50 0x14 1; setenvram.b aup_rootfselect 0x10000000;", 0);
	run_command("setenv aup_imgname image.ub", 0);
	run_command("setenv aup_imgaddr 0x10000000", 0);
	run_command("setenv aup_imgsize 0x2fe0000", 0);
	run_command("setenv aup_imgofst 0x1000000", 0);
	run_command("setenv aup_sdboot   \"mmc dev 1 && mmcinfo && load mmc 1:1 ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_emmcboot \"mmc dev 0 && mmcinfo && load mmc 0:2 ${aup_imgaddr} ${aup_imgname} && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_qspi_recover_boot \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && bootm ${aup_imgaddr}\"", 0);
	run_command("setenv aup_qspiboot1 \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && bootm ${aup_imgaddr}#conf@1\"", 0);
	run_command("setenv aup_qspiboot2 \"sf probe && sf read ${aup_imgaddr} ${aup_imgofst} ${aup_imgsize} && bootm ${aup_imgaddr}#conf@2\"", 0);
	run_command("setenv aup_qspiboot  \"run aup_emmcboot || run aup_qspiboot1\"", 0);
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
