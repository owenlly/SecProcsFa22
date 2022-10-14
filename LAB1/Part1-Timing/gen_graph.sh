#!/bin/bash

make clean
make
for((i=0;i<3;i++));
do
python run.py;
python graph.py;
done
