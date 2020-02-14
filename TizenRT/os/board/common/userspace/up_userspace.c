/*****************************************************************
*
* Copyright 2019 NXP Semiconductors All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************/
/****************************************************************************
 * os/board/imxrt1050-evk/userspace/imxrt_userspace.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name TinyARA nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdlib.h>

#include <tinyara/userspace.h>
#include <tinyara/wqueue.h>
#include <tinyara/mm/mm.h>
#include <tinyara/init.h>

#if defined(CONFIG_BUILD_PROTECTED) && !defined(__KERNEL__)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* These 'addresses' of these values are setup by the linker script.  They are
 * not actual uint32_t storage locations! They are only used meaningfully in the
 * following way:
 *
 *  - The linker script defines, for example, the symbol_sdata.
 *  - The declareion extern uint32_t _sdata; makes C happy.  C will believe
 *    that the value _sdata is the address of a uint32_t variable _data (it is
 *    not!).
 *  - We can recoved the linker value then by simply taking the address of
 *    of _data.  like:  uint32_t *pdata = &_sdata;
 */
extern uint32_t _stext;			/* Start of .text */
extern uint32_t _etext;			/* End_1 of .text + .rodata */
extern const uint32_t _eronly;	/* End+1 of read only section (.text + .rodata) */
extern uint32_t _sdata;			/* Start of .data */
extern uint32_t _edata;			/* End+1 of .data */
extern uint32_t _sbss;			/* Start of .bss */
extern uint32_t _ebss;			/* End+1 of .bss */

const struct userspace_s userspace __attribute__((section(".userspace"))) = {
	/* General memory map */
#ifndef __APP_BUILD__
#ifdef CONFIG_USER_ENTRYPOINT
	.us_entrypoint = (main_t)CONFIG_USER_ENTRYPOINT,
#else
	.us_entrypoint = (main_t)NULL,
#endif
	.us_textstart = (uintptr_t)&_stext,
	.us_textend = (uintptr_t)&_etext,
	.us_datasource = (uintptr_t)&_eronly,
	.us_datastart = (uintptr_t)&_sdata,
	.us_dataend = (uintptr_t)&_edata,
	.us_bssstart = (uintptr_t)&_sbss,
	.us_bssend = (uintptr_t)&_ebss,

	/* pre-application entry points (declared in include/tinyara/init.h) */

#ifdef CONFIG_SYSTEM_PREAPP_INIT
	.preapp_start    = preapp_start,
#endif
#endif
	/* Task/thread startup routines */
	.task_startup = task_startup,
#ifndef CONFIG_DISABLE_PTHREAD
	.pthread_startup = pthread_startup,
#endif
	/* Signal handler trampoline */
#ifndef CONFIG_DISABLE_SIGNALS
	.signal_handler = up_signal_handler,
#endif

};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#endif							/* CONFIG_BUILD_PROTECTED && !__KERNEL__ */
