#!/bin/bash
rm mem*

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
