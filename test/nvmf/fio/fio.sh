#!/usr/bin/env bash

testdir=$(readlink -f $(dirname $0))
rootdir=$(readlink -f $testdir/../../..)
source $rootdir/scripts/autotest_common.sh
source $rootdir/test/nvmf/common.sh

set -e

if ! rdma_nic_available; then
	echo "no NIC for nvmf test"
	exit 0
fi

timing_enter fio

# Start up the NVMf target in another process
$rootdir/app/nvmf_tgt/nvmf_tgt -c $testdir/../nvmf.conf &
nvmfpid=$!

trap "killprocess $nvmfpid; exit 1" SIGINT SIGTERM EXIT

waitforlisten $nvmfpid ${RPC_PORT}

modprobe -v nvme-rdma

if [ -e "/dev/nvme-fabrics" ]; then
	chmod a+rw /dev/nvme-fabrics
fi

nvme connect -t rdma -n "nqn.2016-06.io.spdk:cnode1" -a "$NVMF_FIRST_TARGET_IP" -s "$NVMF_PORT"
nvme connect -t rdma -n "nqn.2016-06.io.spdk:cnode2" -a "$NVMF_FIRST_TARGET_IP" -s "$NVMF_PORT"

$testdir/nvmf_fio.py 4096 1 write 1 verify
$testdir/nvmf_fio.py 4096 1 randwrite 1 verify
$testdir/nvmf_fio.py 4096 128 write 1 verify
$testdir/nvmf_fio.py 4096 128 randwrite 1 verify

sync
nvme disconnect -n "nqn.2016-06.io.spdk:cnode1"
nvme disconnect -n "nqn.2016-06.io.spdk:cnode2"

rm -f ./local-job0-0-verify.state
rm -f ./local-job1-1-verify.state
rm -f ./local-job2-2-verify.state

trap - SIGINT SIGTERM EXIT

nvmfcleanup
killprocess $nvmfpid
timing_exit fio
