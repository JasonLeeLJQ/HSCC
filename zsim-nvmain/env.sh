#!/bin/sh
PINPATH=/home/jason/dev_code/HSCC/zsim-nvmain/pin_kit
NVMAINPATH=/home/jason/dev_code/HSCC/zsim-nvmain/nvmain
ZSIMPATH=/home/jason/dev_code/HSCC/zsim-nvmain
BOOST=/usr/local/lib/boost
LIBCONFIG=/usr/local/lib/
HDF5=/usr/local/lib
#SIMPOINT_DIR=path of simpoint
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PINPATH/intel64/lib:$PINPATH/intel64/runtime:$PINPATH/intel64/lib:$PINPATH/intel64/lib-ext:$BOOST/lib:$HDF5/lib
INCLUDE=$INCLUDE:$HDF5/include
LIBRARY_PATH=$LIBRARY_PATH:$HDF5/lib
CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$HDF5/include
export ZSIMPATH PINPATH NVMAINPATH LD_LIBRARY_PATH BOOST CPLUS_INCLUDE_PATH LIBRARY_PATH LIBCONFIG #SIMPOINT_DIR
