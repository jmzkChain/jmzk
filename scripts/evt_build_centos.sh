	OS_VER=$( cat /etc/os-release | grep VERSION_ID | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' \
	| cut -d'.' -f1 )

	MEM_MEG=$( free -m | grep Mem | tr -s ' ' | cut -d\  -f2 )
	CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu | grep "^CPU(s)" | tr -s ' ' | cut -d\  -f2 )
	MEM_GIG=$(( (($MEM_MEG / 1000) / 2) ))
	JOBS=$(( ${MEM_GIG} > ${CPU_CORE} ? ${CPU_CORE} : ${MEM_GIG} ))

	DISK_INSTALL=`df -h . | tail -1 | tr -s ' ' | cut -d\  -f1`
	DISK_TOTAL_KB=`df . | tail -1 | awk '{print $2}'`
	DISK_AVAIL_KB=`df . | tail -1 | awk '{print $4}'`
	DISK_TOTAL=$(( $DISK_TOTAL_KB / 1048576 ))
	DISK_AVAIL=$(( $DISK_AVAIL_KB / 1048576 ))

	printf "\n\tOS name: $OS_NAME\n"
	printf "\tOS Version: ${OS_VER}\n"
	printf "\tCPU speed: ${CPU_SPEED}Mhz\n"
	printf "\tCPU cores: $CPU_CORE\n"
	printf "\tPhysical Memory: $MEM_MEG Mgb\n"
	printf "\tDisk install: ${DISK_INSTALL}\n"
	printf "\tDisk space total: ${DISK_TOTAL%.*}G\n"
	printf "\tDisk space available: ${DISK_AVAIL%.*}G\n"

	if [ $MEM_MEG -lt 7000 ]; then
		echo "Your system must have 7 or more Gigabytes of physical memory installed."
		echo "exiting now."
		exit 1
	fi

	if [ $OS_VER -lt 7 ]; then
		echo "You must be running Centos 7 or higher to install everiToken."
		echo "exiting now."
		exit 1
	fi

	if [ ${DISK_AVAIL%.*} -lt $DISK_MIN ]; then
		echo "You must have at least ${DISK_MIN}GB of available storage to install everiToken."
		echo "exiting now."
		exit 1
	fi
	printf "\n\tChecking Yum installation\n"
	
	YUM=$( which yum 2>/dev/null )
	if [ $? -ne 0 ]; then
		printf "\n\tYum must be installed to compile everiToken.\n"
		printf "\n\tExiting now.\n"
		exit 0
	fi
	
	printf "\tYum installation found at ${YUM}.\n"
	printf "\n\tChecking installation of Centos Software Collections Repository.\n"
	
	SCL=$( which scl 2>/dev/null )
	if [ -z $SCL ]; then
		printf "\n\tThe Centos Software Collections Repository, devtoolset-7 and Python3 are required to install everiToken.\n"
		printf "\tDo you wish to install and enable this repository, devtoolset-7 and Python3 packages?\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\n\n\tInstalling SCL.\n\n"
					sudo yum -y --enablerepo=extras install centos-release-scl 2>/dev/null
					if [ $? -ne 0 ]; then
						printf "\n\tCentos Software Collections Repository installation failed.\n"
						printf "\n\tExiting now.\n"
						exit 1
					else
						printf "\n\tCentos Software Collections Repository installed successfully.\n"
					fi
					printf "\n\n\tInstalling devtoolset-7.\n\n"
					sudo yum install -y devtoolset-7 2>/dev/null
					if [ $? -ne 0 ]; then
						printf "\n\tCentos devtoolset-7 installation failed.\n"
						printf "\n\tExiting now.\n"
						exit 1
					else
						printf "\n\tCentos devtoolset installed successfully.\n"
					fi
					printf "\n\n\tInstalling Python3.\n\n"
					sudo yum install -y python33.x86_64 2>/dev/null
					if [ $? -ne 0 ]; then
						printf "\n\tCentos Python3 installation failed.\n"
						printf "\n\tExiting now.\n"
						exit 1
					else
						printf "\n\tCentos Python3 installed successfully.\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required Centos Software Collections Repository, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\n\tCentos Software Collections Repository found.\n"
	fi

	printf "\n\tEnabling Centos devtoolset-7.\n"
	source /opt/rh/devtoolset-7/enable
	if [ $? -ne 0 ]; then
		printf "\n\tUnable to enable Centos devtoolset-7 at this time.\n"
		printf "\n\tExiting now.\n"
		exit 1
	fi
	printf "\tCentos devtoolset-7 successfully enabled.\n"

# 	printf "\n\tEnabling Centos python3 installation.\n"
# 	source /opt/rh/python33/enable
# 	if [ $? -ne 0 ]; then
# 		printf "\n\tUnable to enable Centos python3 at this time.\n"
# 		printf "\n\tExiting now.\n"
# 		exit 1
# 	fi
# 	printf "\tCentos python3 successfully enabled.\n"
	
	printf "\n\tUpdating YUM repository.\n"

	sudo yum -y update 2>/dev/null
	
	if [ $? -ne 0 ]; then
		printf "\n\tYUM update failed.\n"
		printf "\n\tExiting now.\n"
		exit 1
	fi

	printf "\n\tYUM repository successfully updated.\n"

	DEP_ARRAY=( git autoconf automake libtool ocaml.x86_64 doxygen libicu-devel.x86_64 \
	bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 python-devel.x86_64 gettext-devel.x86_64 \
    lz4 lz4-devel libzstd libzstd-devel readline-devel readline)
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\n\tChecking YUM for installed dependencies.\n\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( sudo $YUM info ${DEP_ARRAY[$i]} 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )

		if [ "$pkg" != "installed" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\n\t"
			printf "\tPackage ${DEP_ARRAY[$i]} ${bldred} NOT ${txtrst} found.\n"
			let COUNT++
		else
			printf "\tPackage ${DEP_ARRAY[$i]} found.\n"
			continue
		fi
	done		

	if [ ${COUNT} -gt 1 ]; then
		printf "\n\tThe following dependencies are required to install everiToken.\n"
		printf "\n\t$DISPLAY\n\n"
		printf "\tDo you wish to install these dependencies?\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\n\n\tInstalling dependencies\n\n"
					sudo yum -y install ${DEP}
					if [ $? -ne 0 ]; then
						printf "\n\tYUM dependency installation failed.\n"
						printf "\n\tExiting now.\n"
						exit 1
					else
						printf "\n\tYUM dependencies installed successfully.\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\n\tNo required YUM dependencies to install.\n"
	fi

	printf "\n\tChecking CMAKE installation.\n"
    if [ ! -e ${CMAKE} ]; then
		printf "\tInstalling CMAKE\n"
		mkdir -p ${HOME}/opt/ 2>/dev/null
		cd ${HOME}/opt
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz)
		if [ "${STATUS}" -ne 200 ]; then
			printf "\tUnable to download CMAKE at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		tar xf cmake-3.10.2.tar.gz
		rm -f cmake-3.10.2.tar.gz
		ln -s ${HOME}/opt/cmake-3.10.2/ cmake
		cd cmake
		./bootstrap
		if [ $? -ne 0 ]; then
			printf "\tError running bootstrap for CMAKE.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make -j${JOBS}
		if [ $? -ne 0 ]; then
			printf "\tError compiling CMAKE.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
	else
		printf "\tCMAKE found\n"
	fi

	printf "\n\tChecking boost library installation.\n"
	BVERSION=`cat "${BOOST_ROOT}/include/boost/version.hpp" 2>/dev/null | grep BOOST_LIB_VERSION \
	| tail -1 | tr -s ' ' | cut -d\  -f3 | sed 's/[^0-9\._]//gI'`
	if [ "${BVERSION}" != "1_66" ]; then
		printf "\tRemoving existing boost libraries in ${HOME}/opt/boost* .\n"
		rm -rf ${HOME}/opt/boost*
		if [ $? -ne 0 ]; then
			printf "\n\tUnable to remove deprecated boost libraries at this time.\n"
			printf "\n\tExiting now.\n"
			exit 1
		fi
		printf "\tInstalling boost libraries.\n"
		cd ${TEMP_DIR}
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2)
		if [ "${STATUS}" -ne 200 ]; then
			printf "\tUnable to download Boost libraries at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		tar xf ${TEMP_DIR}/boost_1_66_0.tar.bz2
		rm -f  ${TEMP_DIR}/boost_1_66_0.tar.bz2
		cd ${TEMP_DIR}/boost_1_66_0/
		./bootstrap.sh "--prefix=$BOOST_ROOT"
		if [ $? -ne 0 ]; then
			printf "\n\tInstallation of boost libraries failed. 0\n"
			printf "\n\tExiting now.\n"
			exit 1
		fi
		./b2 install
		if [ $? -ne 0 ]; then
			printf "\n\tInstallation of boost libraries failed. 1\n"
			printf "\n\tExiting now.\n"
			exit 1
		fi
		rm -rf ${TEMP_DIR}/boost_1_66_0/
	else
		printf "\tBoost 1.66.0 found at ${HOME}/opt/boost_1_66_0.\n"
	fi

	printf "\n\tChecking secp256k1-zkp installation.\n"
    if [ ! -e /usr/local/lib/libsecp256k1.a ]; then
		printf "\tInstalling secp256k1-zkp (Cryptonomex branch)\n"
		cd ${TEMP_DIR}
		git clone https://github.com/cryptonomex/secp256k1-zkp.git
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone repo secp256k1-zkp @ https://github.com/cryptonomex/secp256k1-zkp.git.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd secp256k1-zkp
		./autogen.sh
		if [ $? -ne 0 ]; then
			printf "\tError running autogen for secp256k1-zkp.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		./configure
		make -j${JOBS}
		if [ $? -ne 0 ]; then
			printf "\tError compiling secp256k1-zkp.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		rm -rf cd ${TEMP_DIR}/secp256k1-zkp
	else
		printf "\tsecp256k1 found\n"
	fi
	
	printf "\n\tChecking gflags installation.\n"
    if [ ! -e /usr/local/lib/libgflags.a ]; then
		printf "\tInstalling gflags\n"
		cd ${TEMP_DIR}
		git clone https://github.com/gflags/gflags.git
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone repo gflags @ https://github.com/gflags/gflags.git.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd gflags
		mkdir build
        cd build
        cmake ../
		if [ $? -ne 0 ]; then
			printf "\tError running CMAKE for gflags.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make -j${JOBS}
		if [ $? -ne 0 ]; then
			printf "\tError compiling gflags.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		rm -rf cd ${TEMP_DIR}/gflags
	else
		printf "\tgflags found\n"
	fi

	function print_instructions()
	{
		printf "\tsource /opt/rh/python33/enable\n"
		printf "\tcd ${HOME}/evt/build; make test\n\n"
	return 0
	}