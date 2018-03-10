/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef SECMEM_PLATFORM_H
#define SECMEM_PLATFORM_H

/* Enable the support of secure memory dump detail */
#define SECMEM_DEBUG_DUMP

/* Enable secure memory test interface */
/* #define SECMEM_DEBUG_INTERFACE */

/* Enable secure memory API */
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) \
	|| defined(CONFIG_BLOWFISH_TEE_SUPPORT)
#define SECMEM_KERNEL_API
#endif

#endif				/* end of SECMEM_PLATFORM_H */
