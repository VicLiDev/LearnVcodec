==> cmod:
--> ijg
documents: https://www.ijg.org/
download: wget https://www.ijg.org/files/jpegsrc.v9f.tar.gz
--> libjpeg
documents: https://libjpeg.sourceforge.net/
download: wget https://sourceforge.net/projects/libjpeg/files/latest/download && tar -xvf download

---

# ijg 和 libjpeg 的区别和联系

1. IJG（Independent JPEG Group）
- 定义：IJG 是一个开发并维护 JPEG 标准开源实现的志愿者组织，成立于 1991 年。
- 作用：IJG 开发了最早的 JPEG 编解码器参考实现，并发布了著名的 `jpeg-6b`、`jpeg-8` 等版本。
- 特点：
  - 提供 C 语言编写的源码，兼容性广，但代码较老旧。
  - 功能基础，支持标准的 JPEG 压缩/解压缩。
  - 许可证为宽松的 IJG License（类似 BSD 许可证）。

2. libjpeg
- 定义：`libjpeg` 是 IJG 发布的 JPEG 编解码器的库形式，通常指官方实现的动态/静态库（如 `libjpeg.so` 或 `libjpeg.a`）。
- 作用：为应用程序提供 JPEG 图像处理的 API，开发者可通过调用其接口实现 JPEG 的读写功能。
- 版本：
  - libjpeg-turbo：基于 IJG 代码优化的高性能分支，支持 SIMD 指令加速（如 x86/ARM NEON），速度比原版快数倍。
  - mozjpeg：由 Mozilla 主导的衍生版本，专注于优化 Web 场景的压缩效率（更好的压缩率/质量平衡）。

主要区别
| 特性           | IJG               | libjpeg（含衍生版本）       |
|----------------|-------------------|-----------------------------|
| 性质           | 组织 + 原始代码   | 具体的库实现                |
| 代码来源       | IJG 官方发布      | 基于 IJG 代码的封装或优化   |
| 性能           | 基础实现，较慢    | 可能优化（如 libjpeg-turbo）|
| 使用场景       | 参考或历史项目    | 实际开发中的依赖库          |
| 衍生版本       | 无                | libjpeg-turbo、mozjpeg 等   |

常见问题
1. 应该用哪个？
   - 现代项目推荐 libjpeg-turbo（性能最优），或 mozjpeg（Web 优化）。
   - 原版 IJG 代码通常仅用于兼容旧项目或学习。

2. 兼容性
   libjpeg 的 API 与 IJG 代码基本一致，但衍生版本可能扩展功能（如色彩空间支持）。

3. 许可证
   IJG 和 libjpeg 均允许自由使用，但 libjpeg-turbo 部分代码可能涉及额外许可（如 SIMD 优化部分）。



# 源码下载

以下是主要的 **JPEG 编解码库**及其衍生版本的源码仓库和下载链接，包括官方实现和优化版本：

1. IJG (Independent JPEG Group) 官方版本
- **源码下载**：
  [http://www.ijg.org/](http://www.ijg.org/)
  （最新版本为 `jpeg-9e`，直接提供 `.tar.gz` 压缩包）
- **GitHub 镜像**：
  [https://github.com/LuaDist/libjpeg](https://github.com/LuaDist/libjpeg)
  （非官方，但方便查看）


2. libjpeg-turbo（高性能优化版）
- **官网**：
  [https://libjpeg-turbo.org/](https://libjpeg-turbo.org/)
- **源码仓库**：
  GitHub: [https://github.com/libjpeg-turbo/libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo)
  （支持 SIMD 加速，推荐用于生产环境）

3. mozjpeg（Web 图像优化版）
- **官网**：
  [https://github.com/mozilla/mozjpeg](https://github.com/mozilla/mozjpeg)
- **源码仓库**：
  GitHub: [https://github.com/mozilla/mozjpeg](https://github.com/mozilla/mozjpeg)
  （专注于更好的压缩率，适合网页图片）

4. OpenJPEG（JPEG 2000 实现）
- **注意**：这是 JPEG 2000（`.jp2`）标准的实现，与传统 JPEG 不同。
- **官网**：
  [https://www.openjpeg.org/](https://www.openjpeg.org/)
- **源码仓库**：
  GitHub: [https://github.com/uclouvain/openjpeg](https://github.com/uclouvain/openjpeg)

其他相关工具

jpeginfo（JPEG 文件检测工具）
- 仓库: [https://github.com/tjko/jpeginfo](https://github.com/tjko/jpeginfo)
  （用于检查 JPEG 文件完整性）

cjpeg/djpeg 命令行工具
- 包含在 **IJG** 或 **libjpeg-turbo** 的源码中，编译后即可使用。


总结建议
- 优先选择 libjpeg-turbo：性能最佳，兼容性好。
- 需要更高压缩率：尝试 mozjpeg。
- 学习或兼容旧代码：使用 IJG 官方版本。

如果需要具体平台的编译指导，可以告诉我你的操作系统（如 Windows/Linux/macOS）！
