#!/bin/sh

mpirun --mca plm_rsh_agent badSSH.sh:ssh:rsh oob_tcp_static_ports 22 -np $1 --hostfile hostfile watermpi 
# mpirun -np $1 --hostfile hostfile watermpi
