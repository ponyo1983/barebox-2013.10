/*
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Boot support
 */
#include <common.h>
#include <driver.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <xfuncs.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>
#include <errno.h>
#include <boot.h>
#include <of.h>
#include <rtc.h>
#include <init.h>
#include <of.h>
#include <magicvar.h>
#include <uncompress.h>
#include <memory.h>
#include <filetype.h>
#include <binfmt.h>
#include <globalvar.h>
#include <magicvar.h>
#include <asm-generic/memory_layout.h>

/*
 * Additional oftree size for the fixed tree
 */
#define OFTREE_SIZE_INCREASE	0x8000

static char *bootm_image_name_and_no(const char *name, int *no)
{
	char *at, *ret;

	if (!name || !*name)
		return NULL;

	*no = 0;

	ret = xstrdup(name);
	at = strchr(ret, '@');
	if (!at)
		return ret;

	*at++ = 0;

	*no = simple_strtoul(at, NULL, 10);

	return ret;
}

#define BOOTM_OPTS_COMMON "ca:e:vo:f"

#ifdef CONFIG_CMD_BOOTM_INITRD
#define BOOTM_OPTS BOOTM_OPTS_COMMON "L:r:"
#else
#define BOOTM_OPTS BOOTM_OPTS_COMMON
#endif

unsigned long long getenv_loadaddr(const char *name)
{
	const char *valstr = getenv(name);

	if (!valstr)
		return UIMAGE_SOME_ADDRESS;

	if (valstr[0] == '\0')
		return UIMAGE_SOME_ADDRESS;

	return simple_strtoull(valstr, NULL, 0);
}

static int do_bootm(int argc, char *argv[])
{
	int opt;
	struct image_data data;
	int ret = 1;
	const char *oftree = NULL, *initrd_file = NULL, *os_file = NULL;

	memset(&data, 0, sizeof(struct image_data));

	data.initrd_address = UIMAGE_INVALID_ADDRESS;
	data.os_address = UIMAGE_SOME_ADDRESS;
	data.verify = 0;
	data.verbose = 0;

	oftree = getenv("global.bootm.oftree");
	os_file = getenv("global.bootm.image");
	data.os_address = getenv_loadaddr("global.bootm.image.loadaddr");
	data.initrd_address = getenv_loadaddr("global.bootm.initrd.loadaddr");
	if (IS_ENABLED(CONFIG_CMD_BOOTM_INITRD))
		initrd_file = getenv("global.bootm.initrd");

	while ((opt = getopt(argc, argv, BOOTM_OPTS)) > 0) {
		switch(opt) {
		case 'c':
			data.verify = 1;
			break;
#ifdef CONFIG_CMD_BOOTM_INITRD
		case 'L':
			data.initrd_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'r':
			initrd_file = optarg;
			break;
#endif
		case 'a':
			data.os_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'e':
			data.os_entry = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			data.verbose++;
			break;
		case 'o':
			oftree = optarg;
			break;
		case 'f':
			data.force = 1;
			break;
		default:
			break;
		}
	}

	if (optind != argc)
		os_file = argv[optind];

	if (!os_file || !*os_file) {
		printf("no boot image given\n");
		goto err_out;
	}

	if (initrd_file && !*initrd_file)
		initrd_file = NULL;

	if (oftree && !*oftree)
		oftree = NULL;

	data.os_file = bootm_image_name_and_no(os_file, &data.os_num);
	data.oftree_file = bootm_image_name_and_no(oftree, &data.oftree_num);
	data.initrd_file = bootm_image_name_and_no(initrd_file, &data.initrd_num);

	ret = bootm_boot(&data);

	printf("handler failed with %s\n", strerror(-ret));

err_out:
	free(data.initrd_file);
	free(data.os_file);

	return 1;
}

static int bootm_init(void)
{
	globalvar_add_simple("bootm.image", NULL);
	globalvar_add_simple("bootm.image.loadaddr", NULL);
	globalvar_add_simple("bootm.oftree", NULL);
	if (IS_ENABLED(CONFIG_CMD_BOOTM_INITRD)) {
		globalvar_add_simple("bootm.initrd", NULL);
		globalvar_add_simple("bootm.initrd.loadaddr", NULL);
	}

	return 0;
}
late_initcall(bootm_init);

BAREBOX_CMD_HELP_START(bootm)
BAREBOX_CMD_HELP_USAGE("bootm [OPTIONS] image\n")
BAREBOX_CMD_HELP_SHORT("Boot an application image.\n")
BAREBOX_CMD_HELP_OPT  ("-c",  "crc check uImage data\n")
#ifdef CONFIG_CMD_BOOTM_INITRD
BAREBOX_CMD_HELP_OPT  ("-r <initrd>","specify an initrd image\n")
BAREBOX_CMD_HELP_OPT  ("-L <load addr>","specify initrd load address\n")
#endif
BAREBOX_CMD_HELP_OPT  ("-a <load addr>","specify os load address\n")
BAREBOX_CMD_HELP_OPT  ("-e <ofs>","entry point to the image relative to start (0)\n")
#ifdef CONFIG_OFTREE
BAREBOX_CMD_HELP_OPT  ("-o <oftree>","specify oftree\n")
#endif
#ifdef CONFIG_CMD_BOOTM_VERBOSE
BAREBOX_CMD_HELP_OPT  ("-v","verbose\n")
#endif
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootm)
	.cmd		= do_bootm,
	.usage		= "boot an application image",
	BAREBOX_CMD_HELP(cmd_bootm_help)
BAREBOX_CMD_END

BAREBOX_MAGICVAR(bootargs, "Linux Kernel parameters");
BAREBOX_MAGICVAR_NAMED(global_bootm_image, global.bootm.image, "bootm default boot image");
BAREBOX_MAGICVAR_NAMED(global_bootm_image_loadaddr, global.bootm.image.loadaddr, "bootm default boot image loadaddr");
BAREBOX_MAGICVAR_NAMED(global_bootm_initrd, global.bootm.initrd, "bootm default initrd");
BAREBOX_MAGICVAR_NAMED(global_bootm_initrd_loadaddr, global.bootm.initrd.loadaddr, "bootm default initrd loadaddr");

static struct binfmt_hook binfmt_uimage_hook = {
	.type = filetype_uimage,
	.exec = "bootm",
};

static int binfmt_uimage_init(void)
{
	return binfmt_register(&binfmt_uimage_hook);
}
fs_initcall(binfmt_uimage_init);

/**
 * @file
 * @brief Boot support for Linux
 */

/**
 * @page boot_preparation Preparing for Boot
 *
 * This chapter describes what's to be done to forward the control from
 * barebox to Linux. This part describes the generic part, below you can find
 * the architecture specific part.
 *
 * - @subpage arm_boot_preparation
 * - @subpage ppc_boot_preparation
 * - @subpage x86_boot_preparation
 */
