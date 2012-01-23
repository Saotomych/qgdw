sftp $UPDATE@$SFTP:dev/$ID/cfgs/ieclevel.cid /rw/mx00/configs
stat /rw/mx00/configs/ieclevel.cid
if [ $? -eq 0 ]
    then
    cd /rw/mx00/bin
    ./startiec&
fi

