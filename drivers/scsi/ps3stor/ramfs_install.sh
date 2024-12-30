#!/bin/sh
#note  实现卡下系统安装脚本
#warn  该脚本仅在系统安装使用进行使用

#验证rpm安装是否正常，若是失败，退出
function check_return(){
	if [ $? -ne 0 ]; then
		exit 1;
	fi
}

function dirCheck(){
	driverDir=$1
	imageDir=$2
	serviceDir="/mnt/sysimage/usr/lib/systemd/system"
	serviceLnDir="/mnt/sysimage/etc/systemd/system"

	while ((1))
	do
		isImageDir="null"
		sleep 1
		if [ -d ${imageDir} ];then
			sleep 10;
			if [ -d ${imageDir} ];then
				isImageDir=`ls -dn $imageDir | grep "drw" | awk '{print $1}'`
				check_return
			fi
		fi
		if [ ${isImageDir} != "null" ]&&[ ! -f ${imageDir}/ps3stor.ko ];then
			echo "The image dir has been built!"
			cp ${driverDir}/ps3stor.ko ${imageDir}/
			check_return
			echo "mv ps3stor.ko finisded"
		fi
	done
}

#step1 基础变量赋值
echo "The rpm script is running!"
kernel=`uname -r`
driverSourDir=`pwd`
driverDesDir="/usr/lib/modules/${kernel}/kernel/drivers/scsi"
imageDir="/mnt/sysimage/usr/lib/modules/${kernel}/kernel/drivers/scsi"

#step2 安装基本驱动程序
if [ ! -e ${driverSourDir}/ps3stor.ko ]; then
	echo "The ps3stor.ko needs to be in the current dirctory!"
	exit 1;
fi

cp -rf ${driverSourDir}/ps3stor.ko ${driverDesDir}/ps3stor.ko
check_return
isload=`lsmod | grep -wm1 ps3stor | cut -d " " -f 1`
if [ ! ${isload} ]; then
	modprobe scsi_transport_sas
	check_return
	echo "Finish scsi_transport_sas.ko install!"
	insmod ${driverDesDir}/ps3stor.ko
	check_return
	echo "Finsih ps3stor.ko install!"
else
	if [ ${kernel} == "4.19.90-2305.1.0.0199.56.uel20.x86_64" ]; then
		imageDir="/mnt/sysimage/usr/lib/modules/5.10.0-46.uel20.x86_64/kernel/drivers/scsi"
	fi
	if [ ${kernel} == "4.19.90-2403.3.0.0270.84.uel20.x86_64" ]; then
		imageDir="/mnt/sysimage/usr/lib/modules/5.10.0-74.uel20.x86_64/kernel/drivers/scsi"
	fi
	echo "No need to insmod ps3stor.ko"
fi

#step3 判断安装目录是否存在，存在的话进行文件拷贝
dirCheck ${driverSourDir} ${imageDir} &
