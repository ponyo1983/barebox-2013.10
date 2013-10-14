/*
 * reginfo.c - print information about SoC specifc registers
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>

static int do_reginfo(int argc, char *argv[])
{
	reginfo();
	return 0;
}

BAREBOX_CMD_START(reginfo)
	.cmd		= do_reginfo,
	.usage		= "print register information",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
