# DFrameworkDBus-Qt6
> **WARNING · 警告**：This is just a MINIMAL library that is from `gxde-qt-dbus-factory`, we deleted the built in `tools` and `xml` folder for the purpose is to only build the `.so` files. `generate_code.py` may NOT work. - 本库裁剪自`gxde-qt-dbus-factory`，删除了`tools`与`xml`目录，因为最终目的是构建`.so`库。`generate_code.py`可能不会工作

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
