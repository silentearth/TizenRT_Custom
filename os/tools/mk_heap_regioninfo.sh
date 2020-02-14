#!/bin/bash
###########################################################################
#
# Copyright 2018 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################
#
# File : mk_heap_regioninfo.sh

OSTOOLDIR=`test -d ${0%/*} && cd ${0%/*}; pwd`
OSDIR="${OSTOOLDIR}/.."
source ${OSDIR}/.config

REGION_INFO_C_FILE_PATH=${OSDIR}/mm/mm_heap/mm_heap_regioninfo.c

REGIONx_START=${CONFIG_RAM_REGIONx_START}
REGIONx_SIZE=${CONFIG_RAM_REGIONx_SIZE}
REGIONx_HEAP_IDX=${CONFIG_RAM_REGIONx_HEAP_INDEX}

ENABLE_KERNEL_HEAP=${CONFIG_MM_KERNEL_HEAP}

KREGIONx_START=${CONFIG_RAM_KREGIONx_START}
KREGIONx_SIZE=${CONFIG_RAM_KREGIONx_SIZE}
KREGIONx_HEAP_IDX=${CONFIG_RAM_KREGIONx_HEAP_INDEX}

REGION_NUM=${CONFIG_MM_REGIONS}
HEAP_NUM=${CONFIG_MM_NHEAPS}

KREGION_NUM=${CONFIG_KMM_REGIONS}
KHEAP_NUM=${CONFIG_KMM_NHEAPS}

printf '%s\n' "/* Automatically generated file; DO NOT EDIT. */">${REGION_INFO_C_FILE_PATH}
printf '%s\n%s\n\n%s' "#include <tinyara/config.h>" "#include <sys/types.h>" "void *regionx_start[CONFIG_MM_REGIONS] = {">> ${REGION_INFO_C_FILE_PATH}

# REGIONx_START
for ((idx=0; idx<${REGION_NUM}; idx++))
do
	START_ADDR=$(printf ${REGIONx_START} | awk '{ split($0, start_arr, ","); print start_arr['$(($idx+1))'] }')
	printf '(void *)%s' "${START_ADDR}">> ${REGION_INFO_C_FILE_PATH}
	if [ $idx -lt $((REGION_NUM-1)) ];then
		printf '%s' ", ">> ${REGION_INFO_C_FILE_PATH}
	fi
done
printf '%s\n' "};">> ${REGION_INFO_C_FILE_PATH}

# REGIONx_SIZE
printf '%s' "size_t regionx_size[CONFIG_MM_REGIONS] = {">> ${REGION_INFO_C_FILE_PATH}
for ((idx=0; idx<${REGION_NUM}; idx++))
do
	SIZE=$(printf ${REGIONx_SIZE} | awk '{ split($0, size_arr, ","); print size_arr['$(($idx+1))'] }')
	printf '%s' "${SIZE}">> ${REGION_INFO_C_FILE_PATH}
	if [ $idx -lt $((REGION_NUM-1)) ];then
		printf '%s' ", ">> ${REGION_INFO_C_FILE_PATH}
	fi
done
printf '%s\n' "};">> ${REGION_INFO_C_FILE_PATH}

# REGIONx_HEAP_IDX
printf '%s' "int regionx_heap_idx[CONFIG_MM_REGIONS] = {">> ${REGION_INFO_C_FILE_PATH}
if [ ${HEAP_NUM} -eq 1 ];then
	printf '%d%s\n\n' 0 "};">> ${REGION_INFO_C_FILE_PATH}
else
for ((idx=0; idx<${REGION_NUM}; idx++))
do
	INDEX=$(printf ${REGIONx_HEAP_IDX} | awk '{ split($0, index_arr, ","); print index_arr['$(($idx+1))'] }')
	printf '%s' "${INDEX}">> ${REGION_INFO_C_FILE_PATH}
	if [ $idx -lt $((REGION_NUM-1)) ];then
		printf '%s' ", ">> ${REGION_INFO_C_FILE_PATH}
	fi
done
printf '%s\n\n' "};">> ${REGION_INFO_C_FILE_PATH}
fi

if [ "${ENABLE_KERNEL_HEAP}" == "y" ];then
###### Kernel Heap

printf '\n%s' "void *kregionx_start[CONFIG_KMM_REGIONS] = {">> ${REGION_INFO_C_FILE_PATH}
# KREGIONx_START
for ((idx=0; idx<${KREGION_NUM}; idx++))
do
	KSTART_ADDR=$(printf ${KREGIONx_START} | awk '{ split($0, kstart_arr, ","); print kstart_arr['$(($idx+1))'] }')
	printf '(void *)%s' "${KSTART_ADDR}">> ${REGION_INFO_C_FILE_PATH}
	if [ $idx -lt $((KREGION_NUM-1)) ];then
		printf '%s' ", ">> ${REGION_INFO_C_FILE_PATH}
	fi
done
printf '%s\n' "};">> ${REGION_INFO_C_FILE_PATH}

# KREGIONx_SIZE
printf '%s' "size_t kregionx_size[CONFIG_KMM_REGIONS] = {">> ${REGION_INFO_C_FILE_PATH}
for ((idx=0; idx<${KREGION_NUM}; idx++))
do
	KSIZE=$(printf ${KREGIONx_SIZE} | awk '{ split($0, ksize_arr, ","); print ksize_arr['$(($idx+1))'] }')
	printf '%s' "${KSIZE}">> ${REGION_INFO_C_FILE_PATH}
	if [ $idx -lt $((KREGION_NUM-1)) ];then
		printf '%s' ", ">> ${REGION_INFO_C_FILE_PATH}
	fi
done
printf '%s\n' "};">> ${REGION_INFO_C_FILE_PATH}

# KREGIONx_HEAP_IDX
printf '%s' "int regionx_kheap_idx[CONFIG_KMM_REGIONS] = {">> ${REGION_INFO_C_FILE_PATH}
if [ ${KHEAP_NUM} -eq 1 ];then
	printf '%d%s\n\n' 0 "};">> ${REGION_INFO_C_FILE_PATH}
else
for ((idx=0; idx<${KREGION_NUM}; idx++))
do
	KINDEX=$(printf ${KREGIONx_HEAP_IDX} | awk '{ split($0, kindex_arr, ","); print kindex_arr['$(($idx+1))'] }')
	printf '%s' "${KINDEX}">> ${REGION_INFO_C_FILE_PATH}
	if [ $idx -lt $((KREGION_NUM-1)) ];then
		printf '%s' ", ">> ${REGION_INFO_C_FILE_PATH}
	fi
done
printf '%s\n\n' "};">> ${REGION_INFO_C_FILE_PATH}
fi

fi
