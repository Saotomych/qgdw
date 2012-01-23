#!/bin/sh
source /tmp/about/setenv.sh
cd /rw/mx00/configs
stat addr.md5
adr=$?
stat ieclevel.icd
icd=$?
stat ieclevel.cid
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
    cd /rw/mx00/bin
    ./startiec&
    exit 0
  fi

  if [ $icd -eq 0 ]
    then
# no addr.cfg, exist *.icd, no *.cid
    echo "Manager has making ICD file. CID waiting."
    echo put ../configs/ieclevel.icd dev/$ID/cfgs/ieclevel.icd | sftp $UPDATE@$SFTP
    crontab -u root /tmp/crontabs/getcid.cron
    exit 1
  fi

# no addr.cfg, no *.icd, no *.cid
  rm /rw/mx00/configs/*.icd
  rm /rw/mx00/configs/*.cid

  sftp $UPDATE@$SFTP:dev/$ID/cfgs/addr.cfg
  stat addr.md5
  if [ $? -ne 0 ]
    then
# no addr.cfg, no *.icd, no *.cid
    echo Addr configuration waiting
    crontab -u root /tmp/crontabs/getaddr.cron
    exit 3
  else
# exist addr.cfg, no *.icd, no *.cid
    echo Start manager
    cd /rw/mx00/bin
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
    cd /rw/mx00/bin
    ./startiec&
    exit 0
  fi

  if [ $icd -ne 0 ]
    then
    echo Start Manager
    crontab -u root /tmp/crontabs/update.cron
    cd /rw/mx00/bin
    ./manager
  fi
fi

icd=`stat ieclevel.icd`
if [ $icd -eq 0 ]
  then
# exist *.icd, no *.cid
  echo "Manager has making ICD file. CID waiting."
  echo put ../configs/ieclevel.icd dev/$ID/cfgs/ieclevel.icd | sftp $UPDATE@$SFTP
  crontab -u root /tmp/crontabs/getcid.cron
  exit 1
fi

# no *.icd, no *.cid
echo "ERROR: Manager don't has making ICD file"
crontab -u root /tmp/crontabs/getaddrnew.cron
exit 2
