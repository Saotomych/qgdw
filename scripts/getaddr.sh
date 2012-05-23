#!/bin/sh
source /tmp/about/setenv.sh
home1=/rw/mx00
cd $home1/configs
sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg.md5 $home1/configs > /dev/null 2>&1
md5sum -c addr.cfg.md5 > /dev/null 2>&1
if [ $? -ne 0 ] 
  then
  rm $home1/configs/addr.cfg > /dev/null 2>&1
  sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg $home1/configs > /dev/null 2>&1
  stat $home1/configs/addr.cfg > /dev/null 2>&1
  if [ $? -eq 0 ]
    then
    cd $home1/bin
    echo >> $home1/cfgupdate.log
    date +%d.%m.%Y-%a-%H:%M:%S >> $home1/cfgupdate.log
    echo "addr.cfg download OK" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    crontab -u root /tmp/crontabs/update.cron
    ./manager
    date +%d.%m.%Y-%a-%H:%M:%S >> $home1/cfgupdate.log
    echo "Configuration READY" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    $home1/scripts/startconfig.sh
  fi
fi

