/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __TEEI_SMC_STRUCT_H__
#define __TEEI_SMC_STRUCT_H__

#include <linux/semaphore.h>
#define TEE_NAME_SIZE   (255)

struct teei_encode {
	struct list_head head;
	int encode_id;
	void *ker_req_data_addr;
	void *ker_res_data_addr;
	u32  enc_req_offset;
	u32  enc_res_offset;
	u32  enc_req_pos;
	u32  enc_res_pos;
	u32  dec_res_pos;
	u32  dec_offset;
	struct teei_encode_meta *meta;
};

struct teei_shared_mem {
	struct list_head head;
	struct list_head s_head;
	void *index;
	void *k_addr;
	void *u_addr;
	u32  len;
};

struct teei_context {
	unsigned long cont_id;                  /* ID */
	char tee_name[TEE_NAME_SIZE];           /* Name */
	unsigned long sess_cnt;                 /* session counter */
	unsigned long shared_mem_cnt;
	struct list_head link;
	struct list_head sess_link;
	struct list_head shared_mem_list;
	struct semaphore cont_lock;
};

struct teei_session {
	int sess_id;
	struct teei_context *parent_cont;
	struct list_head link;
	struct list_head encode_list;
	struct list_head shared_mem_list;
};

#endif
