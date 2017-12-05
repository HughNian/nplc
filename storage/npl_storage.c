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

#include "storage/npl_cache.h"

npl_storage storage;

int
storage_start(unsigned long storage_size)
{
	unsigned long alloc_size;
	unsigned long node_max_data_size;
	unsigned int nums;

	if(storage_size < 0) return 0;

	if(storage_size < STORAGE_MIN_SIZE) storage_size = STORAGE_MIN_SIZE;

	node_max_data_size = NODE_MAX_DATA_SIZE;
	nums = storage_size / node_max_data_size;

	if(nums){
	    alloc_size = storage_size + NPL_ALIGNED_SIZE(sizeof(npl_node))* nums + NPL_ALIGNED_SIZE(sizeof(npl_kv_key)) * nums + NPL_ALIGNED_SIZE(sizeof(npl_kv_value)) * nums;
	}

	storage.total = nums;
	storage.nums  = nums;
	storage.size  = alloc_size;
	storage.used  = storage.left = 0;
	storage.used_size = storage.left_size = 0;
	storage.recyle = 0;
	storage.first = storage.last = NULL;

	storage.p = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

	if (storage.p == MAP_FAILED) {
		return 0;
	}

	return 1;
}

void
storage_off(void)
{
	if(storage.size){
		munmap(storage.p, storage.size);
	}

	storage.nums = storage.used = storage.left = 0;
	storage.p = NULL;
	storage.size = storage.used_size = storage.left_size = 0;
	storage.recyle = 0;

	npl_node *node;
	node = storage.first;
	while(node){
		if(node->type == 1){
			npl_kv_key *k;
			npl_kv_value *v;
			k = (npl_kv_key *)node->data;
			v = (npl_kv_value *)k->value;

			free(v);
			free(node->data);
			free(node);
		}

		node = (npl_node *)node->next;
	}

	return;
}

static inline unsigned long
get_node_size(void)
{
	return NODE_MAX_DATA_SIZE + NPL_ALIGNED_SIZE(sizeof(npl_node)) + NPL_ALIGNED_SIZE(sizeof(npl_kv_key)) + NPL_ALIGNED_SIZE(sizeof(npl_kv_value));
}

static void
append_node(npl_node *node)
{
	if(storage.used == 0 && storage.left == 0){
		node->prev = node->next = NULL;
		storage.first = storage.last = node;
	} else {
		node->next = NULL;
		node->prev = (struct npl_node *)(storage.last);
		storage.last->next = (struct npl_node *)node;
		storage.last = node;
	}

	return;
}

static npl_node *
node_init_mmap(unsigned long node_size)
{
	npl_node *node;
	if(storage.used == storage.nums && storage.left == 0){
		node = storage.first;
	} else {
		node = storage.p + storage.used_size;
	}

	node->index = 0;
	node->type  = 0;
	node->size  = node_size;
	node->d_size = node->d_len = 0;
	node->pos = node->out = 0;

	node->data = node + NPL_ALIGNED_SIZE(sizeof(npl_node));

	return node;
}

static npl_node *
node_init_malloc(unsigned long size)
{
	npl_node *node;
	npl_kv_key *k;
	npl_kv_value *v;

	node = (npl_node *)malloc(sizeof(*node));
	if(NULL == node) return NULL;

	memset(node, 0, sizeof(*node));

	node->index = 0;
	node->type  = 1;
	node->size  = size;
	node->d_size = node->d_len = 0;
	node->pos = node->out = 0;

	k = (npl_kv_key *)malloc(sizeof(*k));
	if(NULL == k){
		free(node);
		return NULL;
	}
	memset(k, 0, sizeof(*k));

	v = (npl_kv_value *)malloc(sizeof(*v) + size);
	if(NULL == v){
		free(k);
		free(node);
		return NULL;
	}
	memset(v, 0, sizeof(*v) + size);

	k->value = v;
	node->data = k;

	return node;
}

npl_node *
node_alloc(unsigned int size)
{
	unsigned long node_size;
	npl_node *node;
	if(size <= NODE_MAX_DATA_SIZE){
		node_size = get_node_size();
		if(storage.used == 0 && storage.left == 0){
			node = node_init_mmap(node_size);
			append_node(node);

			storage.used_size += node_size;
			storage.left_size = storage.size - storage.used_size;
			storage.used++;
			storage.left = storage.nums - storage.used;
		} else if(storage.used == storage.nums && storage.left == 0){
			node = node_init_mmap(node_size);
			append_node(node);

			storage.recyle++;
		} else {
			node = node_init_mmap(node_size);
			append_node(node);

			storage.used_size += node_size;
			storage.left_size = storage.size - storage.used_size;
			storage.used++;
			storage.left = storage.nums - storage.used;
		}

		return node;
	}
	else { //do malloc
		node = node_init_malloc(size);
		if(NULL == node) return NULL;

		append_node(node);

		storage.total++;

		return node;
	}
}
