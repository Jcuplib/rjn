#!/bin/sh

# Settings
date
mpiexec -np 3 ./comp2 : -np 1 ./comp1
date

