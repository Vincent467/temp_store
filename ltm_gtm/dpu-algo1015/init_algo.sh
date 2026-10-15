
DPU_SW_DIR=../dpu-swalgo
DRV_DIR=`pwd`
ALGO_ARCH=dc9000l_0001
ALGO_SRC_PATH=algo_sources

function choose_cid(){
	CID="$1"
	CID_MS="${CID,,}"
	if [[ $CID_MS == 0x20000007 ]]; then
		ALGO_ARCH=sr2000_0002
		echo "ALGO_ARCH=sr2000_0002"
	elif [[ $CID_MS == 0x20000015 ]]; then
		ALGO_ARCH=dc9000l_0001
		echo "ALGO_ARCH=dc9000l_0001"
	elif [[ $CID_MS == 0x20000014 ]]; then
		ALGO_ARCH=dc9200l_0001
		echo "ALGO_ARCH=dc9200l_0001"
	elif [[ $CID_MS == 0x20000018 ]]; then
		ALGO_ARCH=dc9000l_0002
		echo "ALGO_ARCH=dc9000l_0002"
	elif [[ $CID_MS == 0x2000001b ]]; then
		ALGO_ARCH=dc9000l_0004
		echo "ALGO_ARCH=dc9000l_0004"
	elif [[ $CID_MS == 0x20000024 ]]; then
		ALGO_ARCH=dc9000l_1ppc
		echo "ALGO_ARCH=dc9000l_1ppc"
	elif [[ $CID_MS == 0x2000002d ]]; then
		ALGO_ARCH=dc9200l_1ppc_a2
		echo "ALGO_ARCH=dc9200l_1ppc_a2"
	elif [[ $CID_MS == 0x20000028 ]]; then
		ALGO_ARCH=dc9200l_0003
		echo "ALGO_ARCH=dc9200l_0003"
	elif [[ $CID_MS == 0x20000021 ]]; then
		ALGO_ARCH=dc9x00_improv_pdma
		echo "ALGO_ARCH=dc9x00_improv_pdma"
	elif [[ $CID_MS == 0x316 ]]; then
		ALGO_ARCH=dc9400_0001
		echo "ALGO_ARCH=dc9400_0001"
	elif [[ $CID_MS == 0x32a ]]; then
		ALGO_ARCH=dc9400_0003
		echo "ALGO_ARCH=dc9400_0003"
	elif [[ $CID_MS == 0x20000005 ]]; then
		ALGO_ARCH=dc9400_0003
		echo "ALGO_ARCH=dc9400_0003"
	elif [[ $CID_MS == 0x20000026 ]]; then
		ALGO_ARCH=dc9400_0011
		echo "ALGO_ARCH=dc9400_0011"
	elif [[ $CID_MS == 0x20000034 ]]; then
		ALGO_ARCH=dc9400_0012
		echo "ALGO_ARCH=dc9400_0012"
	elif [[ $CID_MS == 0x20000039 ]]; then
		ALGO_ARCH=dc9400l_0001
		echo "ALGO_ARCH=dc9400l_0001"
	elif [[ $CID_MS == 0x2000003f ]]; then
		ALGO_ARCH=dc9400l_0002
		echo "ALGO_ARCH=dc9400l_0002"
	elif [[ $CID_MS == 0x2000003a ]]; then
		ALGO_ARCH=dc9200_0005
		echo "ALGO_ARCH=dc9200_0005"
	elif [[ $CID_MS == 0x20000040 ]]; then
		ALGO_ARCH=dc9200_0006
		echo "ALGO_ARCH=dc9200_0006"
	elif [[ $CID_MS == 0x2000002b ]]; then
		ALGO_ARCH=dc9400_0013
		echo "ALGO_ARCH=dc9400_0013"
	elif [[ $CID_MS == 0x20000035 ]]; then
		ALGO_ARCH=dc9400_0014
		echo "ALGO_ARCH=dc9400_0014"
	elif [[ $CID_MS == 0x20000036 ]]; then
		ALGO_ARCH=dc9400_0015
		echo "ALGO_ARCH=dc9400_0015"
	elif [[ $CID_MS == 0x2000003b ]]; then
		ALGO_ARCH=dc9400_0016
		echo "ALGO_ARCH=dc9400_0016"
	elif [[ $CID_MS == 0x2000000b ]]; then
		ALGO_ARCH=dc9400_0009
		echo "ALGO_ARCH=dc9400_0009"
	elif [[ $CID_MS == 0x2000001a ]]; then
		ALGO_ARCH=dc9400_0010
		echo "ALGO_ARCH=dc9400_0010"
	elif [[ $CID_MS == 0x334 ]]; then
		ALGO_ARCH=dc9000
		echo "ALGO_ARCH=dc9000"
	elif [[ $CID_MS == 0x20000017 ]]; then
		ALGO_ARCH=dcnano0004
		echo "ALGO_ARCH=dcnano0004"
	elif [[ $CID_MS == 0x2000003c ]]; then
		ALGO_ARCH=dc9000l_0005
		echo "ALGO_ARCH=dc9000l_0005"
	elif [[ $CID_MS == 0x0 ]]; then
		ALGO_ARCH=Algo_Test
		echo "ALGO_ARCH=Algo_Test, only test"
	else
		echo "the CID "$CID" not use dpu soft algorithm"
		return -1
	fi
	return 0
}

function init_algo(){
	cd $DRV_DIR
	if [ -d "$ALGO_SRC_PATH" ];then
		rm -rf "$ALGO_SRC_PATH"
		echo "clear: $ALGO_SRC_PATH"
	fi
	mkdir $ALGO_SRC_PATH
	echo "mkdir: $ALGO_SRC_PATH"

	cp -r $DPU_SW_DIR/SW ./$ALGO_SRC_PATH
	cp -r $DPU_SW_DIR/common ./$ALGO_SRC_PATH
	cp -r $DPU_SW_DIR/algo_version_control.json ./$ALGO_SRC_PATH
	cp -r $DPU_SW_DIR/copy_sw_for_driver.py ./$ALGO_SRC_PATH

	echo "copy source code form algorithm..."
	python3 ./$ALGO_SRC_PATH/copy_sw_for_driver.py --arch $ALGO_ARCH --src_path ./$ALGO_SRC_PATH --dst_path ./ || return $?

	echo "copy mapper code form mapper..."
	python3 ./copy_mapper.py || return $?

	echo "delete $ALGO_SRC_PATH ..."
	rm -rf ./$ALGO_SRC_PATH
}


### main
CID=$1
choose_cid $CID
if [ $? -eq 0 ]; then
	init_algo || exit $?
fi
