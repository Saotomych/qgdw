#!/bin/sh
export LD_LIBRARY_PATH=/rw/mx00/armlib
source /tmp/about/setenv.sh
home1=/rw/mx00
cd $home1/configs
sftp $UPDATE@$SFTP:dev/$ID/configs/ieclevel.cid.md5 $home1/configs > /dev/null 2>&1
md5sum -c ieclevel.cid.md5 > /dev/null 2>&1
if [ $? -ne 0 ] 
   then
   rm $home1/configs/ieclevel.cid > /dev/null 2>&1
   sftp $UPDATE@$SFTP:dev/$ID/configs/ieclevel.cid $home1/configs > /dev/null 2>&1
   stat $home1/configs/ieclevel.cid > /dev/null 2>&1
   if [ $? -eq 0 ]
      then
      echo >> $home1/cfgupdate.log
      date +%d.%m.%Y-%a-%H:%M:%S >> $home1/cfgupdate.log
      cd $home1/bin > /dev/null 2>&1
      crontab -u root /tmp/crontabs/getaddrnew.cron
      echo "ieclevel.cid download OK" >> $home1/cfgupdate.log
      echo "Start IEC software" >> $home1/cfgupdate.log
      echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
      ./hmi700&
      ./nano-X&
   fi
fi

