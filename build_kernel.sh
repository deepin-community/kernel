#!/bin/sh

#$1为"--help"或" -h"时，显示使用方法
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
	echo "./build_kernel.sh  [桌面或者服务器版] [编译线程数] [编译ID号] [代码提交commit-id] [分支名称]"
	echo "[桌面或者服务器版]: 没有此参数时，默认为0 (0：桌面版本，1：服务器版本，2：新专业版5.10，3：社区版5.10)"
	echo "[编译线程数]:      没有此参数时，默认为4"
	echo "[编译ID号]:         没有此参数时，默认为1  (桌面版编译ID为编译ID号，服务器编译ID为编译ID号+1)"
	echo "[代码提交commit-id]：没有此参数时，抓去分支分最新代码"
	echo "[分支名称branch_name]：没有此参数时，抓取master分支"
	echo "[abi检查开关abi_check]: 没有此参数时，不开启abi检查"
	echo ""
	exit
fi

### 默认变量赋值 ###


#默认的gerrit获取地址：arm-kernel
git_url_arm="ssh://uta00001@gerrit.uniontech.com:29418/kernel/arm-kernel"

#默认的gerrit获取地址：x86
git_url_x86="ssh://uta00001@gerrit.uniontech.com:29418/kernel/x86-kernel"

#默认的gerrit获取地址：5.10
git_url_510="ssh://uta00001@gerrit.uniontech.com:29418/uos/kernel"

#默认的gerrit获取地址：loongarch
git_url_loongarch="ssh://uta00001@gerrit.uniontech.com:29418/Loongarch-kernel"

arm_lines=$(uname -a | grep -c arm)
x86_lines=$(uname -a | grep -c amd)
loongarch_lines=$(uname -a | grep -c loongarch) # 注意：loongarch使用need_merge分支




### 脚本参数导入 ###


#$1为空，默认编译桌面版本
#$2为空，默认采用4个线程进行编译
#$3为空，说明编译ID为空，使用默认值；如果不为空，设置为编译ID；
#$4为空，默认用HEAD
#$5为空，默认用master
#$6为空， 默认为0，不开启abi检查
build_edition=0
build_thread_num=4
build_version_id=1
build_commit_id="HEAD"
build_branch_name="master"
abi_check="0"

if [ "$1" ]; then
	build_edition=$1
fi

if [ "$2" ]; then
	build_thread_num=$2
fi

if [ "$3" ]; then
	build_version_id=$3
fi

if [ "$4" ]; then
	build_commit_id=$4
fi

if [ "$5" ]; then
	build_branch_name=$5
fi

if [ "$6" ];then
	abi_check=$6
fi

### 配置过程 ###


if [ "$arm_lines" = "1" ]; then
	export ARCH=arm64
	if [ "$build_edition" = "0" ]; then
		REMOTE_ADDR=${git_url_arm}
		KERNEL_DIR=arm-kernel
		KERNEL_CONFIG=armdesktop_defconfig
	elif [ "$build_edition" = "1"  ]; then
		REMOTE_ADDR=${git_url_arm}
		KERNEL_DIR=arm-kernel
		KERNEL_CONFIG=armserver_defconfig
	elif [ "$build_edition" = "2"  ]; then
		REMOTE_ADDR=${git_url_510}
		KERNEL_DIR=kernel
		KERNEL_CONFIG=armdesktop_defconfig
	elif [ "$build_edition" = "3"  ]; then
		REMOTE_ADDR=${git_url_510}
		KERNEL_DIR=kernel
		KERNEL_CONFIG=armdesktop_community_defconfig
	 elif [ "$build_edition" = "4"  ]; then
		 REMOTE_ADDR=${git_url_510}
		 KERNEL_DIR=kernel
		 KERNEL_CONFIG=armdesktop_community_defconfig
	fi
elif [ "$x86_lines" = "1" ]; then
	export ARCH=x86
	if [ "$build_edition" = "0" ]; then
		REMOTE_ADDR=${git_url_x86}
		KERNEL_DIR=x86-kernel
		KERNEL_CONFIG=x86_desktop_defconfig
		if [ "$build_branch_name" = "linux-5.7" ];then
			KERNEL_CONFIG=x86_5.7_desktop_defconfig
		fi
	elif [ "$build_edition" = "1"  ]; then
		REMOTE_ADDR=${git_url_x86}
		KERNEL_DIR=x86-kernel
		KERNEL_CONFIG=x86_server_defconfig
	elif [ "$build_edition" = "2"  ]; then
		REMOTE_ADDR=${git_url_510}
		KERNEL_DIR=kernel
		KERNEL_CONFIG=x86_desktop_professional_defconfig
	elif [ "$build_edition" = "3"  ]; then
		REMOTE_ADDR=${git_url_510}
		KERNEL_DIR=kernel
		KERNEL_CONFIG=x86_desktop_defconfig
	elif [ "$build_edition" = "4"  ]; then
		REMOTE_ADDR=${git_url_510}
		KERNEL_DIR=kernel
		KERNEL_CONFIG=x86_desktop_professional_defconfig
	elif [ "$build_edition" = "5" ]||[ "$build_edition" = "6" ]; then
		REMOTE_ADDR=${git_url_510}
		KERNEL_DIR=kernel
		KERNEL_CONFIG=hwe_desktop_defconfig
	fi
elif [ "$loongarch_lines" = "1" ]; then
	export ARCH=loongarch
	if [ "$build_edition" = "0" ]; then
		REMOTE_ADDR=${git_url_loongarch}
		KERNEL_DIR=Loongarch-kernel
		KERNEL_CONFIG=loongson5000_uosdesktop_defconfig
	elif [ "$build_edition" = "1"  ]; then
		REMOTE_ADDR=${git_url_loongarch}
		KERNEL_DIR=Loongarch-kernel
		KERNEL_CONFIG=loongson5000_uosserver_defconfig
	fi
fi


### 编译过程 ###


echo "arch: $ARCH"
echo "build_edition: ${build_edition}"
echo "build_thread_num: ${build_thread_num}"
echo "build_version_id: ${build_version_id}"
echo "build_commit_id: ${build_commit_id}"
echo "build_branch_name: ${build_branch_name}"
echo "abi_check: ${abi_check}"

rm -rf $KERNEL_DIR
git clone $REMOTE_ADDR
cd $KERNEL_DIR || exit 1

git checkout "$build_branch_name"
git reset --hard "$build_commit_id"
echo "$build_version_id" > .version
git rev-parse --short HEAD > ../.commit_info
git show -s  | sed '1D' > ../.current_commit_msg.info

if [ "x$build_edition" = "x0" ]||[ "x$build_edition" = "x1" ]||[ "x$build_edition" = "x2" ]||[ "x$build_edition" = "x3" ]; then
	touch auto_sign_kernel || exit 1
elif [ "x$build_edition" = "x4" ]; then
	sed -i "s/CONFIG_BUILD_SALT/KERNELVERSION/g" Makefile
	sed -i 's/\$revision/\$version-\$revision/g' scripts/package/mkdebian
	echo "CONFIG_LOCALVERSION=\"-amd64-desktop\"" >> arch/x86/configs/x86_desktop_professional_defconfig
	sed -i "s/5.10.0-amd64-desktop//g" arch/x86/configs/x86_desktop_professional_defconfig
	sed -i "s/elfverify,//g" arch/x86/configs/x86_desktop_professional_defconfig
	touch auto_deepin_sign_kernel || exit 1
elif [ "x$build_edition" = "x5" ]; then
	touch auto_sign_kernel || exit 1
elif [ "x$build_edition" = "x6" ]; then
	sed -i "s/CONFIG_BUILD_SALT/KERNELVERSION/g" Makefile
	sed -i 's/\$revision/\$version-\$revision/g' scripts/package/mkdebian
	echo "CONFIG_LOCALVERSION=\"-amd64-desktop-hwe\"" >> arch/x86/configs/hwe_desktop_defconfig
	sed -i "s/5.17.0-amd64-desktop-hwe//g" arch/x86/configs/hwe_desktop_defconfig
	sed -i "s/elfverify,//g" arch/x86/configs/hwe_desktop_defconfig
	touch auto_deepin_sign_kernel || exit 1
fi
make $KERNEL_CONFIG
make bindeb-pkg -j "${build_thread_num}"

if [ "${abi_check}" = "1" ];then
	ABI_DIR=scripts/uos/abicheck
	ABI_TOOL=uos_abi_check.py
	echo "================================================="
	echo " Comparing ABI against baseline"
	ABI_TOOL_SHELL="${ABI_DIR}/${ABI_TOOL}"
	[ -d "${ABI_DIR}" ] || mkdir -p ${ABI_DIR}
	wget -P ${ABI_DIR} https://gitlabwh.uniontech.com/deepin-auto-tools1/gerrit-pipeline/-/raw/kernel/kabi-check-tools/uos_abi_check.py
	wget -P ${ABI_DIR} https://gitlabwh.uniontech.com/deepin-auto-tools1/gerrit-pipeline/-/raw/kernel/kabi-check-tools/uos_diff_abi.py
	wget -P ${ABI_DIR} https://gitlabwh.uniontech.com/deepin-auto-tools1/gerrit-pipeline/-/raw/kernel/kabi-check-tools/uos_dump_abi.py
	wget -P ${ABI_DIR} https://gitlabwh.uniontech.com/deepin-auto-tools1/gerrit-pipeline/-/raw/kernel/kabi-check-tools/uos_gen_whitelist.py
	chmod 755 ${ABI_TOOL_SHELL}
	if [ -x "${ABI_TOOL_SHELL}" ];then
		set +e
		./${ABI_TOOL_SHELL} --kernel-dir . --data-dir ${ABI_DIR} --action 1
		set -e
		## 复制abi检查的日志文件到外层分析
		echo "--------------------------------------------"
		echo "get generate  files "
		ls -la ${ABI_DIR}
		[ -d "../log" ] || mkdir -p "../log"
		cp ${ABI_DIR}/*.txt ../log/ || true
	else
		echo "uos_abi_check: miss the tool"
	fi
fi

cd ..
rm -rf $KERNEL_DIR
