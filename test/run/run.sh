#!/bin/sh

date
mpiexec -np 3 ./comp2_cpp : -np 1 ./comp1_cpp
date
