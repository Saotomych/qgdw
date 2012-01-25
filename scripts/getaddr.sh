source /tmp/about/setenv.sh
sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg /rw/mx00/configs
stat /rw/mx00/configs/addr.cfg
if [ $? -eq 0 ]
    then
    cd /rw/mx00/bin
    ./manager
    cd /rw/mx00/scripts
    ./startconfig.sh
fi
