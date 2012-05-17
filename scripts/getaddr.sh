#!/bin/sh
source /tmp/about/setenv.sh
home1=/rw/mx00
sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg $home1/configs  > /dev/null 2>&1
stat $home1/configs/addr.cfg  > /dev/null 2>&1
if [ $? -eq 0 ]
    then
    cd $home1/bin
    echo "addr.cfg download OK" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    ./manager
    echo "Configuration READY" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    cd ..
    $home1/scripts/startconfig.sh
fi
