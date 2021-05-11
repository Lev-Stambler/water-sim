#!/bin/sh
xhost +local:
mpirun -np $1 watermpi 
# mpirun -np $1 --hostfile hostfile watermpi
