#!/bin/bash
USER=ftpadmin
PASSWORD=s1l
URL=84.242.3.190/update
URL1=84.242.3.190/bin
site=/rw/mx00/
sourcce=/rw/mx00/update
accepted=/usr/update/accepted
soft=/rw/mx00/bin
bin=bin
d=`date`

$OLDPWD=`pwd`
cd $site

echo Download firmware with ftp
wget -m -nd --timestamping -P $site ftp://$USER:$PASSWORD@$URL -a log.upd # скачивание файлов с проверкой(если есть новые файлы то скачиваються только они)
echo Download software with ftp
wget -m -nd --timestamping -P $site ftp://$USER:$PASSWORD@$URL1 -a log.upd # скачивание файлов с проверкой(если есть новые файлы то скачиваються только они)

echo search new firmware 
cmp $site/rootfs.cramfs $sourcce/rootfs.cramfs &> /dev/null
case $?
in
0)
    echo latest version of the file rootfs.crammfs
;;
1) 
    echo found new version rootfs. Check md5.
    md5sum -c $site/rootfs.cramfs.md5 &> /dev/null; #проверка md5 суммы
    case $?
    in
    0)
	    echo md5 rootfs its OK
	    cp $site/rootfs.cramfs $sourcce/rootfs.cramfs
	    flash_eraseall /dev/mtd3 # очистка нужного раздела
	    echo $d write rootfs.cramfs >> nand.stat
	    (nandwrite -p /dev/mtd3 $sourcce/rootfs.cramfs | grep bad) >> nand.stat # запись образа
	    case $?
	    in
	    0)
		    echo write rootfs.cramfs fail >> nand.stat
    	;;
    	1)
    		echo write rootfs.cramfs OK >> nand.stat
    	;;
    	esac
    ;;
    1)
    	echo md5 rootfs its bad
    ;;
    esac
;;
2)
echo old rootfs.cramfs not found
cp $site/rootfs.cramfs $sourcce/rootfs.cramfs
;;
esac
rm -f $site/rootfs.cramfs
rm -f $site/rootfs.cramfs.md5

cmp $site/uImage $sourcce/uImage &> /dev/null
case $?
in
0)
    echo latest version of the file uImage
;;
1) 
    echo found new version kernel. Check md5.
    md5sum -c $site/uImage.md5 &> /dev/null; #проверка md5 суммы
    case $?
    in
    0)
	    echo md5 uImage its OK
	    cp $site/uImage $sourcce/uImage
	    flash_eraseall /dev/mtd2 # очистка нужного раздела
	    echo $d write uImage >> nand.stat
	    (nandwrite -p /dev/mtd2 $sourcce/uImage | grep bad) >> nand.stat # запись образа
    	case $?
    	in
    	0)
    		echo write uImage fail >> nand.stat
    	;;
    	1)
    		echo write uImage OK >> nand.stat
    	;;
    	esac
    ;;
    1)
        echo md5 it bad
    ;;
    esac
;;
2)
echo old uImage not found
cp $site/uImage $sourcce/uImage
;;
esac
rm -f $site/uImage
rm -f $site/uImage.md5

cmp $site/u-boot.bin $sourcce/u-boot.bin &> /dev/null
case $?
in
0)
    echo latest version of the file u-boot.bin
;;
1) 
    echo found new version u-boot. Check md5.
    md5sum $site/u-boot.bin.md5; #проверка md5 суммы
    case $?
    in
    0)
	    echo md5 u-boot its OK
	    cp $site/u-boot.bin $sourcce/u-boot.bin
	    flash_eraseall /dev/mtd1 # очистка нужного раздела
	    echo $d write u-boot >> nand.stat
	    ( nandwrite -p /dev/mtd1 $target/u-boot.bin | grep bad) >> nand.stat # запись образа
	    case $?
	    in
    	0)
    		echo write u-boot fail >> nand.stat
    	;;
    	1)
    		echo write u-boot OK >> nand.stat
    	;;
    	esac
    ;;
    1)
	    echo md5 u-boot it bad
	;;
	esac
;;
2)
echo old u-boot.bin not found
cp $site/u-boot.bin $sourcce/u-boot.bin
;;
esac
rm -f $site/u-boot.bin
rm -f $site/u-boot.bin.md5

#update users software
echo search new software
cd $soft
for file in *
do
echo check $file
cmp $file $site/$file &> /dev/null
case $?
in
0)
    echo latest version of the $file
;;
1)
    echo found new version $file. Check md5.
    cd $site
    md5sum -c $site/$file.md5 &> /dev/null; #проверка md5 суммы
    case $?
    in
    0)
	    echo md5 $site/$file its OK
	    cp -f $site/$file $soft/$file
	    case $?
	    in
	    0)
	    	echo Move $file OK
	    ;;
	    1)
	    	echo Move $file failed
	    ;;
	    esac
    ;;
    1)
	    echo md5 $site/$file it bad
	;;
	esac
	cd $soft
;;
2)
echo $file not found
cp -f $site/$file $soft/$file
;;
esac
rm $site/$file
rm $site/$file.md5
done;

cd $OLDPWD

