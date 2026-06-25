# DFrameworkDBus-Qt6
## 介绍
Standalone package of current `libdframeworkdbus` building against Qt6.

`libdframeworkdbus`的Qt6特别单独包。

Note that the package has been renamed:

请注意，最后的产物被重命名了：

* **Project name · 项目名称**: `DFrameworkDBus`  → `DFrameworkDBusQt6`
* **SO Version · SO库版本**: 2.0.0  → 6.0.0
* **Library name · 库名称**: `libdframeworkdbus.so`  → `libdframeworkdbus-qt6.so`
* **Header file path · 头文件路径**: `libdframeworkdbus-2.0`  → `libdframeworkdbus-qt6-6.0`
* **CMake configuration · CMake配置**: `cmake/DFrameworkdbus`  → `cmake/DFrameworkdbusQt6`
* **CMake target · CMake目标**: `DFrameworkdbus::dframeworkdbus`  → `DFrameworkdbusQt6::dframeworkdbus_qt6`

## Building | 构建
```bash
$ chmod a+x ./build-deb
$ ./build-deb -d
```

The artifacts will be in parent folder of project root. You may use `./build-deb -c` to do cleanup.

构建产物在项目根目录的上级目录，对于项目目录，执行`./build-deb -c`以清理。

## License | 许可证
This project is licensed under GNU GENERAL PUBLIC LICENSE Version 3, you may find it [here](./LICENSE).

本项目使用GNU GENERAL PUBLIC LICENSE Version 3许可授权，可以在[这里](./LICENSE)找到一份许可证副本。
