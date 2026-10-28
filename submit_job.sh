#!/bin/bash
# Site-specific helper: launch a GNU-parallel job list inside a detached screen.
# Set GNU_PARALLEL_RUNNER to the runner script on your cluster.
set -u
job_id="$$"
host_name="${HOSTNAME:-$(hostname)}"
session="${host_name}_${job_id}"
RUNNER="${GNU_PARALLEL_RUNNER:?set GNU_PARALLEL_RUNNER to the path of run_gnu_parallel.csh}"
screen -S "${session}" -d -m bash -c "tcsh ${RUNNER} $1 $2; exit"
