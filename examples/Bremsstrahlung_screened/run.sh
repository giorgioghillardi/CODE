#!/usr/bin/bash

source ../config.sh 

###########
# Run SOFT
###########
mkdir -p data

time $SOFT pi

###################
# Generate figures
###################
../plotimage.py data/image.mat data/image.pdf
../plottopview.py data/topview.mat data/topview.pdf

