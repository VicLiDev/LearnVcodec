#!/bin/bash

# usage: build_run.sh <prot> [<b>] [<bt>] [<gdb>]

clear

# ============ base ============
curScriptPath=`dirname $0`
echo "curScriptPath: $curScriptPath"
curPath=`pwd`
echo "curPath: $curPath"
curDir=${curPath#*LearnVcodec/}
echo "curDir: $curDir"
rootPath=${curPath%/$curDir}
echo "rootPath: $rootPath"

cmd_prot="null"
cmd_build=false
cmd_debug=false

function procParas()
{
    for para in $*
    do
        case ${para} in
        "hevc" | "avc" | "avs2" | "av1" | "vp9")
            cmd_prot=${para}
            ;;
        "b")
            cmd_build=true
            ;;
        "gdb")
            cmd_debug=true
            ;;
        *)
            echo "unsupport cmd"
            ;;
        esac
    done

    if [ $cmd_prot == "null" ]; then
        echo "prot is null"
    fi

    echo "======> cmd <======"
    echo "prot: $cmd_prot"
    echo "build: $cmd_build"
    echo "debug: $cmd_debug"
    echo "======> cmd <======"
}

function create_dir()
{
    if [ ! -d $1 ]; then echo "create dir $1"; mkdir -p $1; fi
}

function cdDir()
{
    workDir=""
    case $1 in
        "hevc")
            workDir="${rootPath}/h265/HM/build"
            ;;
        "avc")
            workDir="${rootPath}/h264/JM/build"
            ;;
        "avs2")
            workDir="${rootPath}/avs2/RD17.0/build/linux"
            ;;
        "av1")
            workDir="${rootPath}/av1/aom/Lbuild"
            ;;
        "vp9")
            workDir="${rootPath}/vp9/libvpx/Lbuild"
            ;;
        *)
            echo "unsupport prot: $1"
            ;;
    esac

    create_dir $workDir
    if [ ! -d "$workDir" ]; then
        echo "dir $workDir is not exist"
        exit 0
    else
        cd $workDir
    fi
}

function build()
{
    cur_prot=$1
    cdDir $cur_prot

    case $cur_prot in
        "hevc")
            ;;
        "avc")
            ;;
        "avs2")
            ;;
        "av1")
            cmake -DCMAKE_BUILD_TYPE=Debug ..
            make
            ;;
        "vp9")
            ;;
        *)
            echo "unsupport prot: $1"
            ;;
    esac
}

function run()
{
    cur_prot=$1
    need_dbg=$2
    cdDir $cur_prot

    cur_prot=$1
    case $cur_prot in
        "hevc")
            hevcCmd=""
            ;;
        "avc")
            avcCmd=""
            ;;
        "avs2")
            avs2Cmd=""
            ;;
        "av1")
            av1Cmd="./aomdec -o output.yuv ${HOME}/test/testStrms/Sintel_360_10s_1MB.ivf"
            ;;
        "vp9")
            vp9Cmd=""
            ;;
        *)
            echo "unsupport prot: $1"
            ;;
    esac

    eval runCmd='$'${cur_prot}Cmd
    if [ "${need_dbg}" == true ];then
        gdb --command=debug.gdb --args $runCmd
    else
        $runCmd
    fi

    echo "cmd: $runCmd"
}

function main()
{
    procParas $@
    
    if [ $cmd_build == true ]; then
        build $cmd_prot
    fi
    if [ $? -eq 0 ]; then
        run $cmd_prot $cmd_debug
    fi
}

# ============ main ============

main $@
