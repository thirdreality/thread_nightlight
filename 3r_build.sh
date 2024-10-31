#!/bin/bash
###
### Usage:
###   3r_build.sh [target_name]
###
### Options:
###   [target_name]   bl706-night-light
###   [clean_build]   clean
###
### Examples:
###   Clean build   :   ./3r_build.sh bl706-night-light clean
###   Dirty build   :   ./3r_build.sh bl706-night-light
###   -h        Show this message.
###   ?         Show this message.

help() {
	pnt $0
	sed -rn 's/^### ?//;T;p;' "$0"
}

pnt(){
	echo ---------------- $1 ---
}

if [[ "$1" == "-h" ]] || [[ "$1" == "?" ]]; then
	    help
	    exit 1
fi

set -e
# set -x

# clean
if [[ "$2"x == "clean"x ]]; then
	echo "rm -rf ./out"
	rm -rf ./out
fi

proj_name=bl706-night-light
if [ $# -ge 1 ];then
	proj_name=$1
fi

date_now=`date +%Y%m%d_%H%M%S`
root_dir=`pwd`

# build
echo ---------------- Build $proj_name project at $date_now in $root_dir ----------------
cd $root_dir

echo -------------------------------- Build $proj_name Start --------------------------------

source ./scripts/activate.sh -p bouffalolab
export BOUFFALOLAB_SDK_ROOT=/opt/bouffalolab_sdk
# ./scripts/build/build_examples.py --target bouffalolab-bl706-night-light-light-easyflash build
./scripts/build/build_examples.py --target bouffalolab-$proj_name-light-easyflash build

echo -------------------------------- Build $proj_name Done--------------------------------

# set +x
exit 0
