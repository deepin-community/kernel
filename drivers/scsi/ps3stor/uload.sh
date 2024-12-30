#!/bin/bash
#
# uload.sh: a helper script for unloading the drivers
#
driverDesDir="/lib/modules/$(uname -r)/kernel/drivers/scsi/"
rpm_initrd="/boot/initramfs-$(uname -r).img"
dpkg_initrd="/boot/initrd.img-$(uname -r)"
install_mode=""
unload_module=""
pkg_mode=""
print_usage() {
	echo "Usage: $0 [-rpm|deb|-auto|-ko] [-help]"
	echo ""
	echo "Options:"
	echo "  -rpm|-r: uninstall the driver in rpm"
	echo "  -deb|-d: uninstall the driver in deb"
	echo "  -auto|-a: uninstall the driver in auto"
	echo "  -ko|-k: uninstall the driver in ko"
	echo "  -help|-h: print usage message"
	echo ""
	echo "Performs selective uninstallation of drivers based on the input provided. "
	echo "Please ensure to back up the driver and system boot files in advance. "
	exit 1
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		-rpm|-r)
			if [ "$unload_module" != "" ]; then
				print_usage
				exit 1
			else
				unload_module="rpm"
				shift
			fi
			;;
		-deb|-d)
			if [ "$unload_module" != "" ]; then
				print_usage
				exit 1
			else
				unload_module="deb"
				shift
			fi
			;;
		-auto|-a)
			if [ "$unload_module" != "" ]; then
				print_usage
				exit 1
			else
				unload_module="auto"
				shift
			fi
			;;
		-ko|-k)
			if [ "$unload_module" != "" ]; then
				print_usage
				exit 1
			else
				unload_module="ko"
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

find_rpm_or_deb() {
	install_mode=$1
	ramfs_file=$2

	if [ -f "$rpm_initrd" ]; then
		ramfs_file=$rpm_initrd
		if rpm -qa |grep ps3stor &> /dev/null ; then
			install_mode="pkg_rpm"
		elif lsinitrd "$ramfs_file" | grep -q "ps3stor" ; then
			install_mode="auto_rpm"	
		elif lsmod | grep -q ps3stor; then
			install_mode="ko"
		else
			install_mode="no"
		fi	
		
	elif [ -f "$dpkg_initrd" ]; then
		ramfs_file=$dpkg_initrd
		dpkg_status=$(dpkg -l | grep ps3stor  | awk '{ print $1 }')
		if [ "$dpkg_status" == "ii" ]; then
			install_mode="pkg_deb"
		elif lsinitramfs -l "$ramfs_file" | grep -q "ps3stor" ; then
			install_mode="auto_deb"
		elif lsmod | grep -q ps3stor; then
			install_mode="ko"
		else
			install_mode="no"
		fi
	else
		install_mode="no"
	fi
}
unload_ko(){
	reference_count=$(lsmod | grep "^ps3stor\s" | awk '{ print $3 }')

	if [ "$reference_count" -ne 0 ]; then
			echo "The driver is in use, please wait until it is no longer in use or seek technical support."
			exit 1
	else
			rmmod ps3stor.ko
			if [ $? -eq 0 ]; then
					echo "The ps3stor.ko module has been unloaded successfully."
			else
					echo "The ps3stor.ko module has been unloaded failed."
					exit 1
			fi
	fi
}

unload_auto(){
	if [ -f ${driverDesDir}/ps3stor.ko ]||[ -f ${driverDesDir}/ps3stor.ko.xz ]; then
		rm -f ${driverDesDir}/ps3stor.ko ${driverDesDir}/ps3stor.ko.xz
		if [ $? -ne 0 ]; then
			echo "ps3stor file delete failed, please check permissions."
			exit 1	
		fi
	fi
	echo "ps3stor.ko is being uninstall, please wait."
	depmod -a
	
	if [ "$install_mode" == "auto_rpm" ]; then
		dracut -v -f
	elif [ "$install_mode" == "auto_deb" ]; then
		mkinitramfs -o $ramfs_file
	fi
	sleep 1s
	
	if [ "$install_mode" == "auto_rpm" ] ; then
		if lsinitrd "$ramfs_file" | grep -q "ps3stor" ; then
			echo "The ps3stor.ko uninstall failed. Please use the backup file to restore the system environment."
		else
			echo "The ps3stor.ko has been uninstalled. For immediate effect, please reboot or shutdown."
		fi
	elif [ "$install_mode" == "auto_deb" ] ; then
		if lsinitramfs -l "$ramfs_file" | grep -q "ps3stor"; then
			echo "The ps3stor.ko uninstall failed. Please use the backup file to restore the system environment."
		else
			echo "The ps3stor.ko has been uninstalled. For immediate effect, please reboot or shutdown."
		fi
	fi
}

if [ -z $unload_module ]; then
	print_usage
fi

find_rpm_or_deb $install_mode $ramfs_file
case "$unload_module" in
    rpm)
       if [ "$install_mode" == "pkg_rpm" ]; then
            rpm -ev ps3stor
			if [ $? -ne 0 ]; then
				echo "The ps3stor.ko  unload failed."
				exit 1
			fi
        else
            echo "The ps3stor.ko module not found in RPM packages."
            exit 1
        fi
        ;;
    deb)
        if [ "$install_mode" == "pkg_deb" ]; then
            dpkg --purge ps3stor
			if [ $? -ne 0 ]; then
				echo "The ps3stor.ko unload failed."
				exit 1
			fi
        else
            echo "The ps3stor.ko module not found in DEB packages."
            exit 1
        fi
        ;;
    auto)
		if [ "$install_mode" == "auto_rpm" ]||[ "$install_mode" == "auto_deb" ]; then
			unload_auto
		else
		    echo "The ps3stor.ko moudule is not loaded in auto mode."
            exit 1
		fi
        ;;
    ko)
		if [ "$install_mode" == "ko" ]; then
			unload_ko
		else
		    echo "The ps3stor.ko moudule is not loaded in install mode."
            exit 1
		fi
        ;;
esac

exit 0
