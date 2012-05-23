#!/bin/sh
export LD_LIBRARY_PATH=/rw/mx00/armlib
source /tmp/about/setenv.sh
home1=/rw/mx00

cd $home1/configs
stat ieclevel.icd > /dev/null 2>&1
icd=$?
stat ieclevel.cid > /dev/null 2>&1
cid=$?

if [ $cid -eq 0 ] 
    then
# exist *.cid
    crontab -u root /tmp/crontabs/getaddrnew.cron
    echo Start IEC software
    cd $home1/bin
    ./hmi700&
    ./nano-X&
    exit 0
fi

if [ $icd -eq 0 ] 
    then
# exist *.icd, no *.cid
    echo "Manager has making ICD file. CID waiting."
    echo put ../configs/ieclevel.icd dev/$ID/configs/ieclevel.icd | sftp $UPDATE@$SFTP  > /dev/null 2>&1
    echo >> $home1/cfgupdate.log
    date +%d.%m.%Y-%a-%H:%M:%S >> $home1/cfgupdate.log    
    echo "ieclevel.icd upload OK" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs/cfgupdate.log" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    crontab -u root /tmp/crontabs/getcid.cron
    exit 1
fi

# no *.icd, no *.cid
rm $home1/configs/addr.cfg > /dev/null 2>&1
echo Start addr configuration waiting >> $home1/cfgupdate.log    
echo "put $home1/cfgupdate.log dev/$ID/logs/cfgupdate.log" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
crontab -u root /tmp/crontabs/getaddr.cron
$home1/scripts/getaddr.sh
exit 2
