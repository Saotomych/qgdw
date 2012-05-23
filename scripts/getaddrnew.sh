#!/bin/sh
source /tmp/about/setenv.sh
home1=/rw/mx00
cd $home1/configs
rm addr.cfg.md5
sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg.md5 $home1/configs > /dev/null 2>&1
md5sum -c addr.cfg.md5 > /dev/null 2>&1
stat $home1/configs/addr.cfg.md5 > /dev/null 2>&1
if [ $? -eq 0 ]
  then
 if [ $? -ne 0 ] 
  then
  rm $home1/configs/addr.cfg > /dev/null 2>&1
  sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg $home1/configs > /dev/null 2>&1
  stat $home1/configs/addr.cfg > /dev/null 2>&1
  if [ $? -eq 0 ]
    then
    rm $home1/configs/*.icd > /dev/null 2>&1
    rm $home1/configs/*.cid > /dev/null 2>&1
    echo >> $home1/cfgupdate.log
    date +%d.%m.%Y-%a-%H:%M:%S >> $home1/cfgupdate.log
    echo "new addr.cfg download OK" >> $home1/cfgupdate.log
    echo "REBOOT" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    reboot
  fi
 fi
fi

