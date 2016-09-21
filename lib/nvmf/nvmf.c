/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <arpa/inet.h>

#include <rte_config.h>
#include <rte_mempool.h>
#include <rte_version.h>

#include "spdk/log.h"
#include "spdk/conf.h"
#include "subsystem.h"
#include "transport.h"
#include "spdk/trace.h"

SPDK_LOG_REGISTER_TRACE_FLAG("nvmf", SPDK_TRACE_NVMF)

#define MAX_SUBSYSTEMS 4

struct spdk_nvmf_globals g_nvmf_tgt;

extern struct rte_mempool *request_mempool;
static unsigned g_num_requests;

static int
spdk_nvmf_initialize_pools(void)
{
	SPDK_NOTICELOG("\n*** NVMf Pool Creation ***\n");

	g_num_requests = MAX_SUBSYSTEMS * g_nvmf_tgt.max_queues_per_session * g_nvmf_tgt.max_queue_depth;

	/* create NVMe backend request pool */
	request_mempool = rte_mempool_create("NVMe_Pool",
					     g_num_requests,
					     spdk_nvme_request_size(),
					     128, 0,
					     NULL, NULL, NULL, NULL,
					     SOCKET_ID_ANY, 0);
	if (!request_mempool) {
		SPDK_ERRLOG("create NVMe request pool failed\n");
		return -1;
	}

	SPDK_TRACELOG(SPDK_TRACE_DEBUG, "NVMe request_mempool %p, size %" PRIu64 " bytes\n",
		      request_mempool,
		      (uint64_t)g_num_requests * spdk_nvme_request_size());

	return 0;
}

/*
 * Wrapper to provide rte_mempool_avail_count() on older DPDK versions.
 * Drop this if the minimum DPDK version is raised to at least 16.07.
 */
#if RTE_VERSION < RTE_VERSION_NUM(16, 7, 0, 1)
static unsigned rte_mempool_avail_count(const struct rte_mempool *pool)
{
	return rte_mempool_count(pool);
}
#endif

static int spdk_nvmf_check_pool(struct rte_mempool *pool, uint32_t count)
{
	if (rte_mempool_avail_count(pool) != count) {
		SPDK_ERRLOG("rte_mempool_avail_count(%s) == %d, should be %d\n",
			    pool->name, rte_mempool_avail_count(pool), count);
		return -1;
	} else {
		return 0;
	}
}

int
spdk_nvmf_check_pools(void)
{
	int rc = 0;

	rc += spdk_nvmf_check_pool(request_mempool, g_num_requests);

	if (rc == 0) {
		return 0;
	} else {
		return -1;
	}
}

int
nvmf_tgt_init(uint16_t max_queue_depth, uint16_t max_queues_per_sess,
	      uint32_t in_capsule_data_size, uint32_t max_io_size)
{
	int rc;

	g_nvmf_tgt.max_queues_per_session = max_queues_per_sess;
	g_nvmf_tgt.max_queue_depth = max_queue_depth;
	g_nvmf_tgt.in_capsule_data_size = in_capsule_data_size;
	g_nvmf_tgt.max_io_size = max_io_size;

	SPDK_TRACELOG(SPDK_TRACE_NVMF, "Max Queues Per Session: %d\n", max_queues_per_sess);
	SPDK_TRACELOG(SPDK_TRACE_NVMF, "Max Queue Depth: %d\n", max_queue_depth);
	SPDK_TRACELOG(SPDK_TRACE_NVMF, "Max In Capsule Data: %d bytes\n", in_capsule_data_size);
	SPDK_TRACELOG(SPDK_TRACE_NVMF, "Max I/O Size: %d bytes\n", max_io_size);

	rc = spdk_nvmf_initialize_pools();
	if (rc != 0) {
		SPDK_ERRLOG("spdk_nvmf_initialize_pools() failed\n");
		return rc;
	}

	return 0;
}

SPDK_TRACE_REGISTER_FN(nvmf_trace)
{
	spdk_trace_register_object(OBJECT_NVMF_IO, 'r');
	spdk_trace_register_description("NVMF_IO_START", "", TRACE_NVMF_IO_START,
					OWNER_NONE, OBJECT_NVMF_IO, 1, 0, 0, "");
	spdk_trace_register_description("NVMF_RDMA_READ_START", "", TRACE_RDMA_READ_START,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_RDMA_WRITE_START", "", TRACE_RDMA_WRITE_START,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_RDMA_READ_COMPLETE", "", TRACE_RDMA_READ_COMPLETE,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_RDMA_WRITE_COMPLETE", "", TRACE_RDMA_WRITE_COMPLETE,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_LIB_READ_START", "", TRACE_NVMF_LIB_READ_START,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_LIB_WRITE_START", "", TRACE_NVMF_LIB_WRITE_START,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_LIB_COMPLETE", "", TRACE_NVMF_LIB_COMPLETE,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
	spdk_trace_register_description("NVMF_IO_COMPLETION_DONE", "", TRACE_NVMF_IO_COMPLETE,
					OWNER_NONE, OBJECT_NVMF_IO, 0, 0, 0, "");
}
