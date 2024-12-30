#!/bin/bash
install_module=0
is_pkg_install=""
pkg_mode=""
ramfs_file=""
ramfs_bak_file=""
timestamp=""
rpm_initrd="/boot/initramfs-$(uname -r).img"
dpkg_initrd="/boot/initrd.img-$(uname -r)"
driverDesDir="/lib/modules/$(uname -r)/kernel/drivers/scsi/"
print_usage() {
	echo "Usage: $0 [-install|-auto] [-help]"
	echo ""
	echo "Options:"
	echo "  -install|-i: compile and install the driver immediately"
	echo "  -auto|-a: compile and install the driver, effective on next reboot"
	echo "  -help|-h: print usage message"
	echo ""
	echo "Performs the compilation and installation of a driver. If no options given then does compilation only."
	exit 1
}

find_rpm_or_deb() {
	is_pkg_install=$1
	pkg_mode=$2
	ramfs_file=$3

	if [ -f "$rpm_initrd" ]; then
		ramfs_file=$rpm_initrd
		pkg_mode="rpm"
		if rpm -qa |grep ps3stor &> /dev/null ; then
			is_pkg_install="pkg"
		elif lsinitrd "$ramfs_file" | grep -q "ps3stor" ; then
			is_pkg_install="auto"	
		elif lsmod | grep -q ps3stor; then
			is_pkg_install="install"
		else
			is_pkg_install="no"
		fi	
		
	elif [ -f "$dpkg_initrd" ]; then
		ramfs_file=$dpkg_initrd
		pkg_mode="deb"
		dpkg_status=$(dpkg -l | grep ps3stor  | awk '{ print $1 }')
		if [ "$dpkg_status" == "ii" ]; then
			is_pkg_install="pkg"
		elif lsinitramfs -l "$ramfs_file" | grep -q "ps3stor" ; then
			is_pkg_install="auto"
		elif lsmod | grep -q ps3stor; then
			is_pkg_install="install"
		else
			is_pkg_install="no"
		fi
	else
		is_pkg_install="no"
	fi
}

backup_restore(){
	if [ -e "${driverDesDir}/ps3stor.ko.$timestamp" ]; then
		cp -f ${driverDesDir}/ps3stor.ko.$timestamp ${driverDesDir}/ps3stor.ko
	fi
	if [ -e "${driverDesDir}/ps3stor.ko.xz.$timestamp" ]; then
		cp -f ${driverDesDir}/ps3stor.ko.xz.$timestamp ${driverDesDir}/ps3stor.ko.xz
	fi
	if [ -e "$ramfs_bak_file" ]; then
		echo "The backup file for boot is $ramfs_bak_file, please use the backup file to restore the system environment."
	fi
	echo "The ps3stor.ko deployed failed."
}

check_return(){
	if [ $? -ne 0 ]; then
		backup_restore
		exit 1;
	fi
}

auto_install(){
	if [ $pkg_mode != "rpm" ] && [ $pkg_mode != "deb" ]; then
		echo "Cannot find initrd file."
		exit 1
	fi
	timestamp=$(date +"%Y%m%d%H%M%S")
	if [ -e "${driverDesDir}/ps3stor.ko" ]; then
		cp -f ${driverDesDir}/ps3stor.ko ${driverDesDir}/ps3stor.ko.$timestamp
	fi
	if [ -e "${driverDesDir}/ps3stor.ko.xz" ]; then
	 cp -f ${driverDesDir}/ps3stor.ko.xz ${driverDesDir}/ps3stor.ko.xz.$timestamp
	fi
	ramfs_bak_file=$ramfs_file.${timestamp}
	if [ -e "$ramfs_file" ]; then
		cp -f "$ramfs_file" "$ramfs_bak_file"
	fi
	if [ $? -ne 0 ]; then
		echo "Initramfs backup failed, please check permissions and disk space."
		exit 1
	fi
	md5_sum_of_ramfs_back=$(md5sum "$ramfs_file" | awk '{print $1}')
	
	cp -f ps3stor.ko  $driverDesDir
	xz -f ps3stor.ko
	cp -f ps3stor.ko.xz $driverDesDir
	if [ $? -ne 0 ]; then
		echo "Failed to store ps3stor file, please check permissions and disk space."
		exit 1
	fi

	depmod -a
	check_return
	
	sleep 1s
	
	if [ "$pkg_mode" == "rpm" ]; then
		dracut -v -f
	elif [ "$pkg_mode" == "deb" ]; then
		mkinitramfs -o $ramfs_file
	fi
	check_return
	sleep 1s

	max_retries=30
	retry_interval=1
	attempt=0
	while [ $attempt -lt $max_retries ]; do
		md5_sum_of_ramfs=$(md5sum "$ramfs_file" | awk '{print $1}')
		echo "The ps3stor.ko is being deployed, please wait."
		if [ "$pkg_mode" == "rpm" ] ; then
			if lsinitrd "$ramfs_file" | grep -q "ps3stor" && [ $md5_sum_of_ramfs !=  $md5_sum_of_ramfs_back ]; then
				echo "The ps3stor.ko has been deployed. For immediate effect, please reboot or shutdown."
				break
			fi
		elif [ "$pkg_mode" == "deb" ] ; then
			if lsinitramfs -l "$ramfs_file" | grep -q "ps3stor" && [ $md5_sum_of_ramfs !=  $md5_sum_of_ramfs_back ]; then
				echo "The ps3stor.ko has been deployed. For immediate effect, please reboot or shutdown."
				break
			fi
		fi
		attempt=$((attempt + 1))
		sleep $retry_interval		
	done
	if [ $attempt -ge $max_retries ]; then
		backup_restore
		exit 1
	fi
	
}
while [[ $# -gt 0 ]]; do
	case "$1" in
		-install|-i)
			if [ $install_module -ne 0 ];then
				print_usage
				exit 1
			else
				install_module=1
				shift
			fi
			;;
		-auto|-a)
			if [ $install_module -ne 0 ];then
				print_usage
				exit 1
			else
				install_module=2
				shift
			fi
			;;
		-help|-h)
			print_usage
			exit 0
			shift
			;;
		*)
			echo "Invalid argument: $1"
			print_usage
			exit 1
			;;
		esac
done
output_filename="output.log"
if [ -e "$output_filename" ]; then
	rm -rf output_filename
fi
chmod a+x clean.sh
./clean.sh

if ! command -v gcc &> /dev/null; then
	echo "Error: The gcc compiler is not installed in the system."
	echo "Please install gcc before running this script."
	exit 1
fi

kernel_source_dir="/lib/modules/`uname -r`/build"

make -j4 -C $kernel_source_dir M=$PWD 2>&1 | tee output_filename

ko_filename="ps3stor.ko"
if [ ! -f "$ko_filename" ]; then
	echo "The compilation execution failed."
	exit 1
fi

if [ $install_module == 0 ]; then
	exit 0
fi
find_rpm_or_deb $is_pkg_install $pkg_mode $ramfs_file
if [ "$is_pkg_install" == "pkg" ]; then
	echo "The ps3stor.ko module has been loaded in rpm/deb format. Please unload the current driver pkg first."
	exit 1
elif [ "$is_pkg_install" = "auto" ]; then
	echo  "Already have a ps3stor.ko loaded in auto. Please unload first."
	exit 1
elif [ "$is_pkg_install" = "install" ]; then
	echo  "Already have a ps3stor.ko loaded. Please unload first."
	exit 1
elif [ "$is_pkg_install" = "no" ]; then
	if [ $install_module == 1 ]; then
		if !(lsmod |grep -q scsi_transport_sas); then
			modprobe scsi_transport_sas
		fi	
		if [ $? -eq 0 ]; then
			insmod ps3stor.ko
			if [ $? -eq 0 ]; then
				echo "The ps3stor.ko module has been loaded successfully."
			else
				echo "The ps3stor.ko module has been loaded failed."
				exit 1
			fi
		else
			echo "The ps3stor.ko module has been loaded failed."
			exit 1
		fi
	elif [ $install_module == 2 ]; then
		auto_install
	fi
else
	echo "unknown err"
	exit 1
fi

exit 0