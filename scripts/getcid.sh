#!/bin/sh
export LD_LIBRARY_PATH=/rw/mx00/armlib
source /tmp/about/setenv.sh
home1=/rw/mx00
sftp $UPDATE@$SFTP:dev/$ID/configs/ieclevel.cid $home1/configs > /dev/null 2>&1
stat $home1/configs/ieclevel.cid > /dev/null 2>&1
if [ $? -eq 0 ]
    then
    cd $home1/bin > /dev/null 2>&1
    echo Start IEC software
    ./hmi700&
    ./nano-X&
    echo "ieclevel.cid download OK" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
fi
