source /tmp/about/setenv.sh
sftp $UPDATE@$SFTP:dev/$ID/configs/ieclevel.cid /rw/mx00/configs
stat /rw/mx00/configs/ieclevel.cid
if [ $? -eq 0 ]
    then
    cd /rw/mx00/scripts
    ./startconfig.sh
fi

