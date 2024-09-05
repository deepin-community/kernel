#! /bin/bash

# 传入的内核树路径
kernel_path=$1

###############################################
# 兼容性列表：LIST与MACRO中的元素一一对应
# 如需增加其他操作系统，新增LIST与对应MACRO数组
###############################################
# NFS
NFS_LIST=("4.19.113-14.1.nfs4.x86_64" "4.19.113-40.nfs4.x86_64")
NFS_MACRO=("NFS_4_0_0613" "NFS_4_0_0612")
# UOS
UOS_LIST=("4.19.90-2201.4.0.0135.up1.uel20.x86_64" "4.19.90-2305.1.0.0199.56.uel20.x86_64" \
    "5.10.0-46.uel20.x86_64" "4.19.90-2403.3.0.0270.84.uel20.x86_64" "5.10.0-74.uel20.x86_64")
UOS_MACRO=("UOS_1050" "UOS_1060_4_19" "UOS_1060_5_10" "UOS_1070_4_19" "UOS_1070_5_10")
# kylin linux
KYLIN_LIST=("4.19.90-24.4.v2101.ky10.x86_64" "4.19.90-vhulk2001.1.0.0026.ns7.15.x86_64" \
    "4.19.90-21.2.9.wa.x86_64")
KYLIN_MACRO=("KYLIN_10_SP2" "KYLIN_0429" "KYLIN_0721")
# anolis
ANOLIS_LIST=("5.10.134-13.an8.x86_64")
ANOLIS_MACRO=("ANOLIS_8_8")
# openeuler
EULER_LIST=("5.10.0-60.18.0.50.oe2203.x86_64")
EULER_MACRO=("EULER_2203_LTS")
# bc-linux
BCLINUX_LIST=("4.19.0-240.23.11.el8_2.bclinux.x86_64" "5.10.0-200.el8_2.bclinux.x86_64")
BCLINUX_MACRO=("BCLINUX_8_2_4_19" "BCLINUX_8_2_5_10")
# culinux
CULINUX_LIST=("5.10.0-60.67.0.107.ule3.x86_64")
CULINUX_MACRO=("CULINUX_3_0")

KERNEL_LIST=(NFS_LIST UOS_LIST KYLIN_LIST ANOLIS_LIST EULER_LIST BCLINUX_LIST CULINUX_LIST)
MACRO_LIST=(NFS_MACRO UOS_MACRO KYLIN_MACRO ANOLIS_MACRO EULER_MACRO BCLINUX_MACRO CULINUX_MACRO)

###############################################
# 获取元素在数组中的下标
###############################################
function getArrItemIdx(){
    local arr=$1
    local item=$2
    local index=0

    for i in ${arr[*]}; do
        if [[ $item == $i ]]
            then
            echo $index
            return
        fi
        index=$(($index + 1))
    done

    echo -1
    return
}

###############################################
# 获取要编译驱动的内核版本
###############################################
function getKernelVersion(){
    local uts_h="/include/generated/utsrelease.h"
    version_path=$1$uts_h
    if [ ! -f $version_path ];then
        return
    fi
    cat $version_path | grep UTS_RELEASE | awk '{ print $3 }' | sed 's/\"//g'
    return
}

##############################################
# main 返回当前内核版本对应的宏
##############################################
function main(){
    local build_kernel=$(getKernelVersion $kernel_path)
    local row=0
    for OS_TYPE in ${KERNEL_LIST[*]}; do
        kernel_tmp=$OS_TYPE[*]
        macro_tmp=${MACRO_LIST[row]}[*]
        KERNELS=(${!kernel_tmp})
        MACROS=(${!macro_tmp})
        col=$(getArrItemIdx "${KERNELS[*]}" $build_kernel)
        if [ $col != -1 ]; then
            echo ${MACROS[col]}
            return
        fi
        row=$(($row + 1))
    done
}

main