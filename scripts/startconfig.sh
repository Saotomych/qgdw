#!/bin/sh
export LD_LIBRARY_PATH=/rw/mx00/armlib
source /tmp/about/setenv.sh
home1=/rw/mx00

cd $home1/configs
stat addr.md5 > /dev/null 2>&1
adr=$?
stat ieclevel.icd > /dev/null 2>&1
icd=$?
stat ieclevel.cid > /dev/null 2>&1
cid=$?

if [ $adr -ne 0 ] 
  then
# no addr.cfg

  if [ $cid -eq 0 ] 
    then
# no addr.cfg, exist *.cid
    echo Addr configuration not found
    crontab -u root /tmp/crontabs/getaddrnew.cron
    echo Start IEC software
    cd $home1/bin
    ./hmi700&
    ./nano-X&
    exit 0
  fi

  if [ $icd -eq 0 ] 
    then
# no addr.cfg, exist *.icd, no *.cid
    echo "Manager has making ICD file. CID waiting."
    echo put ../configs/ieclevel.icd dev/$ID/configs/ieclevel.icd | sftp $UPDATE@$SFTP  > /dev/null 2>&1
    echo "ieclevel.icd upload OK" >> $home1/cfgupdate.log
    echo "put $home1/cfgupdate.log dev/$ID/logs/cfgupdate.log" | sftp $UPDATE@$SFTP 2>&1 > /dev/null
    crontab -u root /tmp/crontabs/getcid.cron
    exit 1
  fi

# no addr.cfg, no *.icd, no *.cid
  rm $home1/configs/*.icd > /dev/null 2>&1
  rm $home1/configs/*.cid > /dev/null 2>&1

  sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg  > /dev/null 2>&1
  stat addr.md5 > /dev/null 2>&1
  if [ $? -ne 0 ] 
    then
# no addr.cfg, no *.icd, no *.cid
    echo Addr configuration waiting
    crontab -u root /tmp/crontabs/getaddr.cron
    exit 3
  else
# exist addr.cfg, no *.icd, no *.cid
    echo Start manager
    cd $home1/bin
    crontab -u root /tmp/crontabs/update.cron
    ./manager
  fi

else

# exist addr.cfg

  if [ $cid -eq 0 ] 
    then
# exist *.cid
    crontab -u root /tmp/crontabs/update.cron
    echo Start IEC software
    cd $home1/bin
    ./hmi700&
    ./nano-X&
    exit 0
  fi

  if [ $icd -ne 0 ] 
    then
    echo Start Manager
    crontab -u root /tmp/crontabs/update.cron
    cd $home1/bin
    ./manager
  fi

fi

icd=`stat ieclevel.icd > /dev/null 2>&1`
if [ $icd -eq 0 ] 
  then
# exist *.icd, no *.cid
  echo "Manager has making ICD file. CID waiting."
  echo put ../configs/ieclevel.icd dev/$ID/configs/ieclevel.icd | sftp $UPDATE@$SFTP  > /dev/null 2>&1
  echo "ieclevel.icd upload OK" >> $home1/cfgupdate.log
  echo "put $home1/cfgupdate.log dev/$ID/logs/cfgupdate.log" | sftp $UPDATE@$SFTP 2>&1 > /dev/null

  crontab -u root /tmp/crontabs/getcid.cron
  exit 1
fi

# no *.icd, no *.cid
echo "ERROR: Manager don't has making ICD file"
crontab -u root /tmp/crontabs/getaddrnew.cron
exit 2
