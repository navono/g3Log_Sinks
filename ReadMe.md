这是[__g3log__](https://github.com/KjellKod/g3log.git)的sink的一个socket版本扩展。
代码整体结构和g3log保持一致。只需将本目录下的文件拷贝到g3log目录下即可。
socket使用的[__libuv__](https://github.com/libuv/libuv.git)库。
此目录下包含的libuv版本为1.11.1

后续计划增加kafka实现


# 注意事项
__1.需要在使用的工程中加入以下库：__
Iphlpapi.lib
Userenv.lib
Psapi.lib
ws2_32.lib

__2.g3log源码修改点：__
logworker.cpp中加入：
```cpp
void LogWorker::addSocketLogger(
   const std::string& svrIP,
   const unsigned int port, 
   const std::string& default_id /*= "g3log"*/) {
   addSink(std::make_unique<g3::socketSink>(svrIP, port, default_id), &socketSink::sendMessage);
   }
```

# 使用方法
类似和增加默认的文件log一样
```cpp
worker->addSocketLogger("127.0.0.1", 8899);
```
其他的log初始化等设置可以参照g3log工程中的示例代码。