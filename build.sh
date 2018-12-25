set -x

SOURCE_DIR=`dirname $(readlink -f $0)`
BUILD_DIR=${BUILD_DIR:-./build}
mkdir -p ${BUILD_DIR}\
    && cd ${BUILD_DIR}\
    && cmake ${SOURCE_DIR}\
    && make
cd ${SOURCE_DIR}
