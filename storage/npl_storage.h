/*
+----------------------------------------------------------------------+
| Nplc means Niansong's PHP local cache                                |
+----------------------------------------------------------------------+
| Copyright (c) 2017 niansong                                          |
+----------------------------------------------------------------------+
| This source file is subject to version 3.01 of the PHP license,      |
| that is bundled with this package in the file LICENSE, and is        |
| available through the world-wide-web at the following url:           |
| http://www.php.net/license/3_01.txt                                  |
| If you did not receive a copy of the PHP license and are unable to   |
| obtain it through the world-wide-web, please send a note to          |
| license@php.net so we can mail you a copy immediately.               |
+----------------------------------------------------------------------+
| Author: niansong                                                     |
+----------------------------------------------------------------------+
*/
#ifndef NPL_STORAGE_H
#define NPL_STORAGE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define NPL_ALIGNMENT 8
#define NPL_ALIGNMENT_MASK ~(NPL_ALIGNMENT - 1)
#define NPL_ALIGNED_SIZE(x)	(((x) + NPL_ALIGNMENT - 1) & NPL_ALIGNMENT_MASK)
#define STORAGE_MIN_SIZE 64 * 1024 * 1024
#define NODE_MAX_DATA_SIZE 1 * 1024 * 1024

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_FAILED
#define MAP_FAILED (void *)-1
#endif

typedef struct {
	unsigned int type:1; //0-mmap,1-malloc
	uint32_t index;
	size_t size;
	size_t d_size;
	uint32_t d_len;

	uint32_t pos;
	uint32_t out;

	void *data;

	struct npl_node *prev;
	struct npl_node *next;
} npl_node;

typedef struct {
	unsigned int total;  //total node nums
	unsigned int nums;   //mmap's node nums, except malloc's node
	unsigned int recyle; //mmap 的空间是否已循环使用的次数
    unsigned int used;   // mmap 空间被使用了多少node数量
    unsigned int left;   // mmap 空间剩余的node数量

    void *p;
    unsigned long size;
    unsigned long used_size;
    unsigned long left_size;

	npl_node *first;
    npl_node *last;
} npl_storage;

extern npl_storage storage;

int  storage_start(unsigned long storage_size);
void storage_off(void);
npl_node *node_alloc(unsigned int size);

#endif
