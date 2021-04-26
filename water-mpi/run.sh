#!/bin/sh

# mpirun -np $1 wsp $2
mpirun -np $1 --hostfile hostfile watermpi