#!/bin/sh
home1=/rw/mx00
sourcce=/tmp

source $sourcce/about/setenv.sh

mkdir recycle 2>&1 > /dev/null
rm $home1/update.log 2>&1 > /dev/null

d=`date`

# SOFTWARE UPDATE
cd $home1
echo "Download upfiles.list"
sftp $SOFTWARE@$SFTP:upfiles.list $home1 2>&1 > /dev/null
dir=`cat $home1/upfiles.list`
               
echo search new software
dir_md5=""
for filemd5 in $dir
	do

	a=`echo $filemd5 | grep ./`
	if [ ${#a} -ne 0 ]
		then
		if [ ! -d "$filemd5" ]
			then
			echo "::=> Directory does not exist :: $filenew -> Create Directory"
			echo "::=> Directory does not exist :: $filenew -> Create Directory" >> $home1/update.log
			mkdir $home1/$filemd5 2>&1 > /dev/null
		else
			echo "Directory $home1/$filemd5 OK $filenew"
      		fi
		dir_md5=$filemd5
		sftp $SOFTWARE@$SFTP:$dir_md5/*.md5 $home1/$dir_md5
		cd $home1/$dir_md5 2>&1 > /dev/null
	else
		md5sum -c $filemd5.md5
		if [ $? -ne 0 ] 
			then
		    	echo "found new version of $filemd5.md5 -> download $filemd5 from sftp-server"
		      	echo "found new version of $filemd5.md5 -> download $filemd5 from sftp-server" >> $home1/update.log
			sftp $SOFTWARE@$SFTP:$dir_md5/$filemd5 $home1/$dir_md5/$filemd5 2>&1 > /dev/null
			stat $filemd5.md5
			if [ $? -eq 0 ]
				then
				md5sum -c $filemd5.md5
				if [ $? -eq 0 ]
					then
					echo $filemd5 download OK >> $home1/update.log
					echo $filemd5 download OK
					mv $home1/$filemd5.md5 $home1/$dir_md5/$filemd5.md5 2>&1 > /dev/null
				else
					echo $filemd5 download error >> $home1/update.log
					echo $filemd5 download error
				     	mv -f $home1/$dir_md5/$filemd5 $home1/recycle/$filemd5 2>&1 > /dev/null	# don't rm - don't epic fails in future
					rm $home1/$dir_md5/$filemd5.md5 2>&1 > /dev/null
				fi
			else
				echo $filemd5.md5 not found >> $home1/update.log
				echo $filemd5.md5 not found
				mv -f $home1/$dir_md5/$filemd5 $home1/recycle/$filemd5 2>&1 > /dev/null	# don't rm - don't epic fails in future				
			fi
		else
			echo "latest version of the $filemd5.md5 -> $filemd5 - OK"
			echo "latest version of the $filemd5.md5 -> $filemd5 - OK" >> $home1/update.log
		fi
	fi

done;

# FIRMWARE UPDATE
cd $sourcce
echo Firmware update
echo Download md5files firmware with ftp
sftp $SOFTWARE@$SFTP:firmware/*.md5 $sourcce

sysreboot=0

# Update system and complex environment
cmp $home1/firmware/m700env.0.md5 $sourcce/m700env.0.md5
case $?
in
0)
    echo "latest version of the file m700env.0"
;;
1) 
    echo "found new version file"
    echo "download new version file m700env.0"
    sftp $SOFTWARE@$SFTP:firmware/m700env.0 $sourcce
	md5sum -c m700env.0.md5
	if [ $? -eq 0 ]
		then
		echo m700env.0 download OK > $home1/update.log
		echo erase flash
    		flash_erase /dev/mtd0 # очистка нужного раздела
    		echo "$d write m700env.0" > $home1/update.log
   		nandwrite -p /dev/mtd0 $sourcce/m700env.0 | grep bad > $home1/update.log # запись образа
    		case $?
		in
    		0)
    			echo "write m700env.0 fail" > $home1/update.log
			# WDT reset for recovery in future
    		;;
    		1)
    			echo "write m700env.0 OK" > $home1/update.log
			cp $sourcce/m700env.0.md5 $home1/firmware/m700env.0.md5
			sysreboot=1
		esac
	else
		echo m700env.0 download error > $home1/update.log
	fi
    ;;
2)
    echo m700env.0 not found
    ;;
esac
rm -f $sourcce/m700env.0 > /dev/null
rm -f $sourcce/m700env.0.md5 2>&1 > /dev/null
#
# Update kernel
cmp $home1/firmware/uImage.md5 $sourcce/uImage.md5
case $?
in
0)
    echo "latest version of the file uImage"
;;
1) 
    echo "found new version file"
    echo "download new version file uImage"
    sftp $SOFTWARE@$SFTP:firmware/uImage $sourcce
	md5sum -c uImage.md5
	if [ $? -eq 0 ]
		then
		echo uImage download OK > $home1/update.log
		echo erase flash
    		flash_eraseall /dev/mtd2 # очистка нужного раздела
    		echo "$d write uImage" > $home1/update.log
   		nandwrite -p /dev/mtd2 $sourcce/uImage | grep bad > $home1/update.log # запись образа
    		case $?
		in
    		0)
    			echo "write uImage fail" > $home1/update.log
			# WDT reset for recovery in future
    		;;
    		1)
    			echo "write uImage OK" > $home1/update.log
			cp $sourcce/uImage.md5 $home1/firmware/uImage.md5
			sysreboot=1
		esac
	else
		echo uImage download error > $home1/update.log
	fi
    ;;
2)
    echo uImage not found
    ;;
esac
rm -f $sourcce/uImage > /dev/null
rm -f $sourcce/uImage.md5 2>&1 > /dev/null

cmp $home1/firmware/u-boot.bin.md5 $sourcce/u-boot.bin.md5
case $?
in
0)
    echo "latest version of the file u-boot.bin"
;;
1) 
    echo "found new version file"
    echo "download new version file u-boot.bin"
    sftp $SOFTWARE@$SFTP:firmware/u-boot.bin $sourcce
	md5sum -c u-boot.bin.md5
	if [ $? -eq 0 ]
		then
		echo u-boot.bin download OK > $home1/update.log
		echo erase flash
    		flash_eraseall /dev/mtd1 # очистка нужного раздела
    		echo "$d write u-boot.bin" > $home1/update.log
   		nandwrite -p /dev/mtd1 $sourcce/u-boot.bin | grep bad > $home1/update.log # запись образа
    		case $?
		in
    		0)
    			echo "write u-boot.bin fail" > $home1/update.log
			# WDT reset for recovery in future
    		;;
    		1)
    			echo "write u-boot.bin OK" > $home1/update.log
			cp $sourcce/u-boot.bin.md5 $home1/firmware/u-boot.bin.md5
		esac
	else
		echo u-boot.bin download error > $home1/update.log
	fi
    ;;
2)
    echo u-boot.bin not found
    ;;
esac
rm -f $sourcce/u-boot.bin > /dev/null
rm -f $sourcce/u-boot.bin.md5 2>&1 > /dev/null

# Update rootfs
cmp $home1/firmware/rootfs.cramfs.md5 $sourcce/rootfs.cramfs.md5
case $?
in
0)
    echo "latest version of the file rootfs.cramfs"
;;
1) 
    echo "found new version file"
    echo "download new version file rootfs.cramfs"
    sftp $SOFTWARE@$SFTP:firmware/rootfs.cramfs $sourcce
	md5sum -c rootfs.cramfs.md5
	if [ $? -eq 0 ]
		then
		echo rootfs.cramfs download OK > $home1/update.log
		echo erase flash
		cp /usr/sbin/nandwrite $sourcce		# save nandwrite for working without rootfs
    		flash_eraseall /dev/mtd3 # очистка нужного раздела
    		echo "$d write rootfs.cramfs" > $home1/update.log
 		$sourcce/nandwrite -p /dev/mtd3 $sourcce/rootfs.cramfs | grep bad > $home1/update.log # запись образа
    		case $?
		in
    		0)
    			echo "write rootfs.cramfs fail" > $home1/update.log
			# WDT reset for recovery in future
    		;;
    		1)
    			echo "write rootfs.cramfs OK" > $home1/update.log
			cp $sourcce/rootfs.cramfs.md5 $home1/firmware/rootfs.cramfs.md5
			sysreboot=1
		esac
	else
		echo rootfs.cramfs download error > $home1/update.log
	fi
    ;;
2)
    echo rootfs.cramfs not found
    ;;
esac
rm -f $sourcce/rootfs.cramfs > /dev/null
rm -f $sourcce/rootfs.cramfs.md5 2>&1 > /dev/null

cd $home1
echo "put $home1/update.log $ID/$home1/update.log" | sftp $SOFTWARE@$SFTP 2>&1 > /dev/null

if [ $sysreboot -ne 0 ]
	then
	reboot
fi

exit 0

