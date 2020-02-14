/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
#ifndef __INCLUDE_MM_HEAP_REGIONINFO_H
#define __INCLUDE_MM_HEAP_REGIONINFO_H

#include <tinyara/config.h>
#include <sys/types.h>

extern void *regionx_start[CONFIG_MM_REGIONS];
extern size_t regionx_size[CONFIG_MM_REGIONS];
extern int regionx_heap_idx[CONFIG_MM_REGIONS];

#ifdef CONFIG_MM_KERNEL_HEAP
extern void *kregionx_start[CONFIG_KMM_REGIONS];
extern size_t kregionx_size[CONFIG_KMM_REGIONS];
extern int regionx_kheap_idx[CONFIG_KMM_REGIONS];
#endif

#endif
