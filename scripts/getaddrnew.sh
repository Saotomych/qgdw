source /tmp/about/setenv.sh
rm /rw/mx00/configs/addr.cfg
sftp $UPDATE@$SFTP:dev/$ID/configs/addr.cfg /rw/mx00/configs
stat /rw/mx00/configs/addr.cfg
if [ $? -eq 0 ]
    then
    rm /rw/mx00/configs/*.icd
    rm /rw/mx00/configs/*.cid
    reboot
fi
