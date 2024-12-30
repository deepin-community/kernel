#!/bin/bash
#
# 检查参数
num=0
while getopts :i:o:h opt
do
  case "$opt" in
  "o") VERSION_FILE=$OPTARG
          num=$(($num+1))
      ;;
  "i") TOOLCHAIN_ID=$OPTARG
          num=$(($num+1))
      ;;
  "h") echo "Usage: ps3_drv_readme.sh -o <headfile> [-i <toolchain_id>]"
      ;;
  *) echo "Unknown option: $opt. Usage: ps3_drv_readme.sh -o <headfile> [-i <toolchain_id>]" 
      ;;
  esac
done

if [ $num -ne 1 ]&&[ $num -ne 2 ]; then
       echo "Invalid command.  Usage: ps3_drv_readme.sh -o <headfile> [-i <toolchain_id>]"
       exit 1
fi

# 将toolchain id 写入版本信息头文件
if [ "$TOOLCHAIN_ID" == "no" ]; then
	TOOLCHAIN_ID='' 
fi

DRV_TOOLCHAIN="#define PS3_DRV_TOOLCHAIN_ID            \"${TOOLCHAIN_ID}\""
sed -i "/PS3_DRV_TOOLCHAIN_ID/c ${DRV_TOOLCHAIN}" $VERSION_FILE

# 从已生成的版本信息头文件中获取版本信息
CUR_DIR=$(dirname $0)
README_FILE=${CUR_DIR}/ps3_drv_readme.txt

VERSION=$(grep PS3_DRV_VERSION ${VERSION_FILE} | cut -d '"' -f 2- |cut -d '"' -f 1)
COMMIT_ID=$(grep PS3_DRV_COMMIT_ID ${VERSION_FILE} | cut -d '"' -f 2- |cut -d '"' -f 1)
BRANCH=$(grep PS3_DRV_BRANCH ${VERSION_FILE} | cut -d '"' -f 2- |cut -d '"' -f 1)
BUILD_TIME=$(grep PS3_DRV_BUILD_TIME ${VERSION_FILE} | cut -d '"' -f 2- |cut -d '"' -f 1)
ARCH=$(grep PS3_DRV_ARCH ${VERSION_FILE} | cut -d '"' -f 2- |cut -d '"' -f 1)
PRODUCT_SUPPORT=$(grep PS3_DRV_PRODUCT_SUPPORT ${VERSION_FILE} | cut -d '"' -f 2- |cut -d '"' -f 1)

# 将版本信息写入 readme.txt
echo "version         : " ${VERSION} > ${README_FILE}      #第一行，覆盖写
echo "commit_id       : " ${COMMIT_ID} >> ${README_FILE} #第二行开始，用追加写
echo "toolchain_id    : " ${TOOLCHAIN_ID} >> ${README_FILE} #第二行开始，用追加写
echo "build_time      : " ${BUILD_TIME} >> ${README_FILE} #第二行开始，用追加写
echo "arch            : " ${ARCH} >> ${README_FILE} #第二行开始，用追加写
echo "product_support : " ${PRODUCT_SUPPORT} >> ${README_FILE} #第二行开始，用追加写
