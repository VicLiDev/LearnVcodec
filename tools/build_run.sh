#!/bin/bash

clear

# single run cmd
sig_hevc_run=""
sig_avc_run=""
sig_avs2_run=""
sig_av1_run="./aomdec -o output.yuv ${HOME}/test/testStrms/Sintel_360_10s_1MB.ivf"
sig_vp9_run=""

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

cmd_prot="null"
cmd_build=false
cmd_sig_test=false
cmd_batch_test=false
cmd_debug=false

function help()
{
    echo "usage: build_run.sh <-p prot> <-b|-r|-bt> [<-gdb>]"
    echo "    -p:   prot, hevc|avc|avs2|av1|vp9"
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
            -p)   cmd_prot=${2}; shift ;;
            -b)   cmd_build=true ;;
            -r)   cmd_sig_test=true ;;
            -bt)  cmd_batch_test=true ;;
            -gdb) cmd_debug=true ;;
            -h)   help; exit 0 ;;
            *)    echo "unsupport cmd ${key}"; help; exit 1 ;;
        esac
        shift # move to next para
    done


    [ $cmd_prot == "null" ] && { echo "Error: prot is null"; help; exit;}

    echo "============> cmd <============"
    echo "prot:       ${cmd_prot}"
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
        hevc) workDir="${rootPath}/h265/HM/build" ;;
        avc)  workDir="${rootPath}/h264/JM/build" ;;
        avs2) workDir="${rootPath}/avs2/RD17.0/build/linux" ;;
        av1)  workDir="${rootPath}/av1/aom/Lbuild" ;;
        vp9)  workDir="${rootPath}/vp9/libvpx/Lbuild" ;;
        *)      echo "unsupport prot: $1"; help ;;
    esac

    create_dir ${workDir} && cd ${workDir} || { echo "dir ${workDir} is not exist"; exit 0;}
}

function build()
{
    cur_prot=$1
    cdDir $cur_prot

    case ${cur_prot} in
        hevc) ;;
        avc)  ;;
        avs2) ;;
        av1)  cmake -DCMAKE_BUILD_TYPE=Debug ..; make ;;
        vp9)  ;;
        *)      echo "unsupport prot: $1"; help ;;
    esac
}

function run()
{
    cur_prot=$1
    need_dbg=$2
    cdDir $cur_prot

    cur_prot=$1
    case ${cur_prot} in
        hevc) hevcCmd=${sig_hevc_run} ;;
        avc)  avcCmd=${sig_avc_run}  ;;
        avs2) avs2Cmd=${sig_avs2_run} ;;
        av1)  av1Cmd=${sig_av1_run} ;;
        vp9)  vp9Cmd=${sig_vp9_run}  ;;
        *)    echo "unsupport prot: $1"; help ;;
    esac

    eval runCmd='$'${cur_prot}Cmd
    [ "${need_dbg}" == true ] && gdb --command=debug.gdb --args ${runCmd} || ${runCmd}

    echo "sigle test cmd: ${runCmd}"
}

function run_batch()
{
    cur_prot=$1
    cdDir $cur_prot

    echo "need finish"
    return 0

    cur_prot=$1
    case ${cur_prot} in
        hevc) hevcCmd="" ;;
        avc)  avcCmd=""  ;;
        avs2) avs2Cmd="" ;;
        av1)  av1Cmd=""  ;;
        vp9)  vp9Cmd=""  ;;
        *)    echo "unsupport prot: $1"; help ;;
    esac

    eval runCmd='$'${cur_prot}Cmd
    [ "${need_dbg}" == true ] && gdb --command=debug.gdb --args ${runCmd} || ${runCmd}

    echo "batch test cmd: ${runCmd}"
}

function main()
{
    procParas $@
    
    [ ${cmd_build} == true ] && build ${cmd_prot}
    [ ${cmd_sig_test} == true ] && run ${cmd_prot} ${cmd_debug}
    [ ${cmd_batch_test} == true ] && run_batch ${cmd_prot}
}

# ============ main ============

main $@
