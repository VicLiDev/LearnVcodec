#!/opt/local/bin/bash
#########################################################################
# File Name: subrepo.sh
# Author: LiHongjin
# mail: 872648180@qq.com
# Created Time: Fri May 31 11:07:05 2024
#########################################################################

# add sub repo
# 注意目录需要是空的，或者是未创建的，git已经在管理的非空目录是无法添加子仓库的
git submodule add https://aomedia.googlesource.com/aom av1/aom
git submodule add https://vcgit.hhi.fraunhofer.de/jvet/HM.git h265/HM
git submodule add https://chromium.googlesource.com/webm/libvpx vp9/libvpx
git submodule add https://github.com/LuaDist/libjpeg jpeg/libjpeg
git submodule add https://github.com/libjpeg-turbo/libjpeg-turbo jpeg/libjpeg-turbo
git submodule add https://github.com/mozilla/mozjpeg jpeg/mozjpeg
git submodule add https://github.com/uclouvain/openjpeg jpeg/openjpeg
git submodule add https://github.com/tjko/jpeginfo jpeg/jpeginfo

# 当克隆一个包含子模块的仓库时，子模块目录会是空的（只包含 .git 文件）。要初始化并
# 更新子模块，需要执行以下命令：
git submodule init
git submodule update

# 也可以使用 --recursive 选项在克隆时直接初始化并更新所有子模块
git clone --recursive https://github.com/your-username/your-repo.git

# 更新子模块
git submodule update --remote
# 也可以进入子模块目录并执行 git pull
