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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include "teei_id.h"
#include "teei_common.h"
#include "teei_smc_call.h"
#include "nt_smc_call.h"
#include "utdriver_macro.h"
#include "notify_queue.h"
#include "switch_queue.h"
#include "teei_client_main.h"
#include "teei_smc_struct.h"
#include "sched_status.h"
#include "teei_capi.h"
#include <imsg_log.h>


void set_sch_nq_cmd(void)
{
	struct message_head msg_head;

	memset((void *)(&msg_head), 0, sizeof(struct message_head));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = STANDARD_CALL_TYPE;
	msg_head.child_type = N_INVOKE_T_NQ;

	memcpy((void *)message_buff, (void *)(&msg_head), sizeof(struct message_head));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

}

/**
 * @brief
 *
 * @param cmd_addr phys address
 *
 * @return
 */

static u32 teei_smc(u32 cmd_addr, int size, int valid_flag)
{
	uint64_t smc_type = 2;

	add_nq_entry(cmd_addr, size, valid_flag);
	set_sch_nq_cmd();
	Flush_Dcache_By_Area((unsigned long)t_nt_buffer, (unsigned long)t_nt_buffer + 0x1000);

	n_invoke_t_nq(&smc_type, 0, 0);
	while (smc_type == 0x54)
		nt_sched_t(&smc_type);

	return 0;
}

/**
 * @brief
 *      call smc
 * @param svc_id  - service identifier
 * @param cmd_id  - command identifier
 * @param context - session context
 * @param enc_id - encoder identifier
 * @param cmd_buf - command buffer
 * @param cmd_len - command buffer length
 * @param resp_buf - response buffer
 * @param resp_len - response buffer length
 * @param meta_data
 * @param ret_resp_len
 *
 * @return
 */
int __teei_smc_call(unsigned long local_smc_cmd,
					u32 teei_cmd_type,
					u32 dev_file_id,
					u32 svc_id,
					u32 cmd_id,
					u32 context,
					u32 enc_id,
					const void *cmd_buf,
					size_t cmd_len,
					void *resp_buf,
					size_t resp_len,
					const void *meta_data,
					const void *info_data,
					size_t info_len,
					int *ret_resp_len,
					int *error_code,
					struct semaphore *psema)
{
	u32 ret = 50;
	void *smc_cmd_phys = 0;
	struct teei_smc_cmd *smc_cmd = NULL;

	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_context *temp_cont = NULL;

	smc_cmd = (struct teei_smc_cmd *)local_smc_cmd;

	if (ret_resp_len)
		*ret_resp_len = 0;


	smc_cmd->teei_cmd_type = teei_cmd_type;
	smc_cmd->dev_file_id = dev_file_id;
	smc_cmd->src_id = svc_id;
	smc_cmd->src_context = task_tgid_vnr(current);

	smc_cmd->id = cmd_id;
	smc_cmd->context = context;
	smc_cmd->enc_id = enc_id;
	smc_cmd->src_context = task_tgid_vnr(current);

	smc_cmd->req_buf_len = cmd_len;
	smc_cmd->resp_buf_len = resp_len;
	smc_cmd->info_buf_len = info_len;
	smc_cmd->ret_resp_buf_len = 0;

	if (psema == NULL)
		return -EINVAL;

	smc_cmd->teei_sema = psema;

	if (cmd_buf != NULL) {
		smc_cmd->req_buf_phys = virt_to_phys((void *)cmd_buf);
		Flush_Dcache_By_Area((unsigned long)cmd_buf, (unsigned long)cmd_buf + cmd_len);
		Flush_Dcache_By_Area((unsigned long)&cmd_buf, (unsigned long)&cmd_buf + sizeof(int));
	} else
		smc_cmd->req_buf_phys = 0;

	if (resp_buf) {
		smc_cmd->resp_buf_phys = virt_to_phys((void *)resp_buf);
		Flush_Dcache_By_Area((unsigned long)resp_buf, (unsigned long)resp_buf + resp_len);
	} else
		smc_cmd->resp_buf_phys = 0;

	if (meta_data) {
		smc_cmd->meta_data_phys = virt_to_phys(meta_data);
		Flush_Dcache_By_Area((unsigned long)meta_data, (unsigned long)meta_data +
			sizeof(struct teei_encode_meta) * (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));
	} else
		smc_cmd->meta_data_phys = 0;

	if (info_data) {
		smc_cmd->info_buf_phys = virt_to_phys(info_data);
		Flush_Dcache_By_Area((unsigned long)info_data, (unsigned long)info_data + info_len);
	} else
		smc_cmd->info_buf_phys = 0;

	smc_cmd_phys = (void *)((unsigned long)virt_to_phys((void *)smc_cmd));

	smc_cmd->error_code = 0;

	Flush_Dcache_By_Area((unsigned long)smc_cmd, (unsigned long)smc_cmd + sizeof(struct teei_smc_cmd));
	Flush_Dcache_By_Area((unsigned long)&smc_cmd, (unsigned long)&smc_cmd + sizeof(int));

	/* down(&smc_lock); */

	list_for_each_entry(temp_cont,
						&teei_contexts_head.context_list,
						link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_shared_mem,
								&temp_cont->shared_mem_list,
								head) {
				Flush_Dcache_By_Area((unsigned long)temp_shared_mem->k_addr,
					(unsigned long)temp_shared_mem->k_addr + temp_shared_mem->len);
			}
		}
	}

	forward_call_flag = GLSCH_LOW;
	ret = teei_smc((u32)(unsigned long)smc_cmd_phys, sizeof(struct teei_smc_cmd), NQ_VALID);

	/* down(psema); */

	return 0;
}

int teei_smc_call(u32 teei_cmd_type,
					u32 dev_file_id,
					u32 svc_id,
					u32 cmd_id,
					u32 context,
					u32 enc_id,
					void *cmd_buf,
					size_t cmd_len,
					void *resp_buf,
					size_t resp_len,
					void *meta_data,
					void *info_data,
					size_t info_len,
					int *ret_resp_len,
					int *error_code,
					struct semaphore *psema)
{
	int retVal = 0;

	struct teei_smc_cmd *local_smc_cmd =
				(struct teei_smc_cmd *)tz_malloc_shared_mem(sizeof(struct teei_smc_cmd), GFP_KERNEL);

	if (local_smc_cmd == NULL) {
		IMSG_ERROR("[%s][%d] tz_malloc_shared_mem failed!\n", __func__, __LINE__);
		return -1;
	}
	/*IMSG_DEBUG("[%s][%d] before smc_call sema = %p\n", __func__, __LINE__, psema);*/

	smc_call_entry.local_cmd = (unsigned long)local_smc_cmd;
	smc_call_entry.teei_cmd_type = teei_cmd_type;
	smc_call_entry.dev_file_id = dev_file_id;
	smc_call_entry.svc_id = svc_id;
	smc_call_entry.cmd_id = cmd_id;
	smc_call_entry.context = context;
	smc_call_entry.enc_id = enc_id;
	smc_call_entry.cmd_buf = cmd_buf;
	smc_call_entry.cmd_len = cmd_len;
	smc_call_entry.resp_buf = resp_buf;
	smc_call_entry.resp_len = resp_len;
	smc_call_entry.meta_data = meta_data;
	smc_call_entry.info_data = info_data;
	smc_call_entry.info_len = info_len;
	smc_call_entry.ret_resp_len = ret_resp_len;
	smc_call_entry.error_code = error_code;
	smc_call_entry.psema = psema;

	down(&smc_lock);

	if (teei_config_flag == 1)
		complete(&global_down_lock);

	/* with a wmb() */
	wmb();

	Flush_Dcache_By_Area((unsigned long)&smc_call_entry, (unsigned long)&smc_call_entry + sizeof(smc_call_entry));
	retVal = add_work_entry(CAPI_CALL, (unsigned char *)(&smc_call_entry));

	if (retVal != 0) {
		tz_free_shared_mem(local_smc_cmd, sizeof(struct teei_smc_cmd));
		return retVal;
	}

	down(psema);

	Invalidate_Dcache_By_Area((unsigned long)local_smc_cmd,
						(unsigned long)local_smc_cmd + sizeof(struct teei_smc_cmd));

	if (cmd_buf)
		Invalidate_Dcache_By_Area((unsigned long)cmd_buf, (unsigned long)cmd_buf + cmd_len);

	if (resp_buf)
		Invalidate_Dcache_By_Area((unsigned long)resp_buf, (unsigned long)resp_buf + resp_len);

	if (meta_data) {
		Invalidate_Dcache_By_Area((unsigned long)meta_data, (unsigned long)meta_data +
					sizeof(struct teei_encode_meta) * (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));
	}

	if (info_data)
		Invalidate_Dcache_By_Area((unsigned long)info_data, (unsigned long)info_data + info_len);


	/* with a rmb() */
	rmb();

	if (ret_resp_len)
		*ret_resp_len = local_smc_cmd->ret_resp_buf_len;

	tz_free_shared_mem(local_smc_cmd, sizeof(struct teei_smc_cmd));

	return smc_call_entry.retVal;
}
