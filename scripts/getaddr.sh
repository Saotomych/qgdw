sftp $UPDATE@$SFTP:$ID/cfgs/addr.cfg /rw/mx00/configs
stat /rw/mx00/configs/addr.cfg
if [ $? -eq 0 ]
    then
    cd /rw/mx00/bin
    ./manager
    cd ..
    ./startconfig.sh
fi