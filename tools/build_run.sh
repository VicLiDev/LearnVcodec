#!/bin/bash

clear

# single run cmd
sig_vvc_run=""

# exe:
# ../bin/MCTSExtractorStatic  ../bin/MCTSExtractorStaticd   ../bin/parcatStatic
# ../bin/parcatStaticd        ../bin/SEIFilmGrainAppStatic  ../bin/SEIFilmGrainAppStaticd
# ../bin/SEIRemovalAppStatic  ../bin/SEIRemovalAppStaticd   ../bin/TAppDecoderAnalyserStatic
# ../bin/TAppDecoderStatic    ../bin/TAppDecoderStaticd     ../bin/TAppEncoderStatic
# ../bin/TAppEncoderStaticd   ../bin/TAppDecoderAnalyserStaticd
sig_hevc_run="../bin/TAppDecoderStaticd"

# exe:
# bin/ldecod.exe      bin/lencod.exe      bin/rtp_loss.exe     bin/rtpdump.exe
# bin/ldecod.dbg.exe  bin/lencod.dbg.exe  bin/rtp_loss.dbg.exe bin/rtpdump.dbg.exe
sig_avc_run="bin/ldecod.dbg.exe"

# exe:
# ../../source/bin/ldecod.exe  ../../source/bin/lencod.exe
sig_avs2_run="../../source/bin/ldecod.exe"

# exe:
# aomdec aomenc test_aom_rc test_intra_pred_speed test_libaom tools/dump_obu
sig_av1_run="./aomdec -o output.yuv ${HOME}/test/testStrms/Sintel_360_10s_1MB.ivf"

# exe:
# test_intra_pred_speed test_libvpx test_rc_interface vpxdec vpxenc
sig_vp9_run="vpxdec"

# exe:
# cjpeg djpeg jpegtran libtool rdjpgcom wrjpgcom
sig_jpg1_run="wrjpgcom"

# exe:
# cjpeg djpeg jpegtran rdjpgcom wrjpgcom
sig_jpg2_run="wrjpgcom"

# ============ base ============
curScriptPath=`dirname $0`
curPath=`pwd`
curDir=${curPath#*LearnVcodec/}
rootPath=${curPath%/$curDir}
echo "============> path <============"
echo "curScriptPath: $curScriptPath"
echo "rootPath: $rootPath"
echo "curPath: $curPath"
echo "curDir: $curDir"
echo "============> path <============"

cmd_spec="null"
cmd_build=false
cmd_sig_test=false
cmd_batch_test=false
cmd_debug=false

function help()
{
    echo "usage: build_run.sh <-s spec> <-b|-r|-bt> [<-gdb>]"
    echo "    -s:   spec, vvc|hevc|avc|avs2|av1|vp9|jpg1|jpg2"
    echo "    -b:   build"
    echo "    -r:   run single test"
    echo "    -bt:  batch test"
    echo "    -gdb: debug"
}

function procParas()
{
    while [[ $# -gt 0 ]]; do
        key="$1"
        case ${key} in
            -s)   cmd_spec=${2}; shift ;;
            -b)   cmd_build=true ;;
            -r)   cmd_sig_test=true ;;
            -bt)  cmd_batch_test=true ;;
            -gdb) cmd_debug=true ;;
            -h)   help; exit 0 ;;
            *)    echo "unsupport cmd ${key}"; help; exit 1 ;;
        esac
        shift # move to next para
    done


    [ $cmd_spec == "null" ] && { echo "Error: spec is null"; help; exit;}

    echo "============> cmd <============"
    echo "spec:       ${cmd_spec}"
    echo "build:      ${cmd_build}"
    echo "sig_test:   ${cmd_sig_test}"
    echo "batch_test: ${cmd_batch_test}"
    echo "debug:      ${cmd_debug}"
    echo "============> cmd <============"
}

function create_dir()
{
    [ ! -d $1 ] && { echo "create dir $1"; mkdir -p $1;}
    [ ! -d $1 ] && { echo "create dir $1 faile"; return 1;} || return 0
}

function cdDir()
{
    workDir=""
    case $1 in
        vvc)  workDir="" ;;
        hevc) workDir="${rootPath}/h265/HM/Lbuild" ;;
        avc)  workDir="${rootPath}/h264/JM" ;;
        avs2) workDir="${rootPath}/avs2/RD17.0/build/linux" ;;
        av1)  workDir="${rootPath}/av1/aom/Lbuild" ;;
        vp9)  workDir="${rootPath}/vp9/libvpx/Lbuild" ;;
        jpg1) workDir="${rootPath}/jpeg/ijg_jpeg-9f/Lbuild" ;;
        jpg2) workDir="${rootPath}/jpeg/libjpeg_jpeg-6b/Lbuild" ;;
        *)    echo "unsupport spec: $1"; help ;;
    esac

    create_dir ${workDir} && cd ${workDir} || { echo "dir ${workDir} is not exist"; exit 0;}
    echo "work dir: ${workDir}"
}

function build()
{
    cur_spec=$1
    cdDir $cur_spec

    case ${cur_spec} in
        vvc)  ;;
        hevc) rm -rf ./*; cmake -DCMAKE_BUILD_TYPE=Debug ..; make -j ;;
        avc)  make clean; make CFLAGS="-fcommon -g" -j ;;
        avs2)
            bash ./clean.sh
            make CFLAGS="-fcommon -g" CC=gcc-9 CXX=g++-9 -j 10 -C ldecod
            make CFLAGS="-fcommon -g" CC=gcc-9 CXX=g++-9 -j 10 -C lencod
            ;;
        av1)  rm -rf ./*; cmake -DCMAKE_BUILD_TYPE=Debug ..; make ;;
        vp9)  rm -rf ./*; ../configure --enable-debug --disable-optimizations; make -j 10 ;;
        jpg1) rm -rf ./*; ../configure; make -j 10 ;;
        jpg2) rm -rf ./*; dos2unix ../configure; ../configure; make -j 10 ;;
        *)    echo "unsupport spec: $1"; help ;;
    esac
}

function run()
{
    cur_spec=$1
    need_dbg=$2
    cdDir $cur_spec

    cur_spec=$1
    case ${cur_spec} in
        vvc)  ;;
        hevc) hevcCmd=${sig_hevc_run} ;;
        avc)  avcCmd=${sig_avc_run}  ;;
        avs2) avs2Cmd=${sig_avs2_run} ;;
        av1)  av1Cmd=${sig_av1_run} ;;
        vp9)  vp9Cmd=${sig_vp9_run}  ;;
        jpg1) jpg1_Cmd=${sig_jpg1_run} ;;
        jpg2) jpg2_Cmd=${sig_jpg2_run} ;;
        *)    echo "unsupport spec: $1"; help ;;
    esac

    eval runCmd='$'${cur_spec}Cmd
    echo "pwd: $(pwd)"
    echo "sigle test cmd: ${runCmd}"
    [ "${need_dbg}" == true ] && gdb --command=debug.gdb --args ${runCmd} || ${runCmd}
    echo "pwd: $(pwd)"
    echo "sigle test cmd: ${runCmd}"
}

function run_batch()
{
    cur_spec=$1
    cdDir $cur_spec

    echo "need finish"
    return 0

    cur_spec=$1
    case ${cur_spec} in
        hevc) hevcCmd="" ;;
        avc)  avcCmd=""  ;;
        avs2) avs2Cmd="" ;;
        av1)  av1Cmd=""  ;;
        vp9)  vp9Cmd=""  ;;
        jpg1) jpg1Cmd="" ;;
        jpg2) jpg2Cmd="" ;;
        *)    echo "unsupport spec: $1"; help ;;
    esac

    eval runCmd='$'${cur_spec}Cmd
    [ "${need_dbg}" == true ] && gdb --command=debug.gdb --args ${runCmd} || ${runCmd}

    echo "batch test cmd: ${runCmd}"
}

function main()
{
    procParas $@
    
    [ ${cmd_build} == true ] && build ${cmd_spec}
    [ ${cmd_sig_test} == true ] && run ${cmd_spec} ${cmd_debug}
    [ ${cmd_batch_test} == true ] && run_batch ${cmd_spec}
}

# ============ main ============

main $@
