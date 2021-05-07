#!/bin/sh

mpirun --plm_rsh_agent badSSH:ssh:rsh --mca oob_tcp_static_ports 22 -np $1 --hostfile hostfile watermpi 
# mpirun -np $1 --hostfile hostfile watermpi