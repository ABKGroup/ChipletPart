#!/bin/bash
job_id="$$"
host_name="$HOSTNAME"
session="${host_name}_${job_id}"
screen -S ${session} -d -m bash -c "tcsh /home/sakundu/SCRIPT/GNU_PARALLEL/run_gnu_parallel.csh $1 $2; exit"

