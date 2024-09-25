#!/bin/bash
#
# load.sh : a helper script for loading the drivers
#

modprobe scsi_transport_sas

# loading the driver
insmod ps3stor.ko
if [ $? -eq 0 ]; then
	echo "The ps3stor.ko module has been loaded successfully."
else
	echo "The ps3stor.ko module has been loaded failed."
	exit 1
fi	
exit 0