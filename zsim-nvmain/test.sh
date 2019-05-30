#!/bin/bash
rm mem*
if [ -z $PINPATH ];then
    echo -e "[H] 环境变量为空，开始设置环境变量"
    source env.sh
    if [ $? -ne 0 ];then
        echo "[H] 设置环境变量failed!"
        exit
    else
	echo "[H] 设置环境变量done!"
    fi
else 
    echo "[H] 环境变量已存在，无需设置!"
fi

./run.sh
if [ $? -eq 0 ]; then
    echo -e "[H] 编译done!\n"
else
    echo "[H] 编译failed!"
    exit
fi
echo "----------------------------------------------------------------------"
./bin/zsim ./config/flat.cfg
if [ $? -eq 0 ]; then
    echo "----------------------------------------------------------------------"
    echo "[H] 执行done!"
else
    echo "[H] 执行failed!"
    exit
fi
