# libevent_study

一个以示例驱动的 C++ libevent 学习仓库。内容从 `event_base` 和事件循环开始，逐步覆盖定时器、信号、文件描述符、TCP 监听、`evbuffer`、`bufferevent`、过滤器、zlib 压缩、HTTP、事件驱动线程池和 FTP 服务端。

仓库同时保留 Visual Studio 工程和 Linux Makefile。每个 `src` 子目录都是相对独立的实验，可以单独构建和调试。

## 你可以学到什么

- Reactor 事件驱动模型如何把“等待 I/O”和“处理业务”分开。
- `event_base`、`event`、pending、active 和 callback 的关系。
- 定时器、信号、文件和网络事件如何使用同一套分发机制。
- 水平触发与边缘触发的主要差异。
- `evconnlistener` 如何封装 `socket/bind/listen/accept`。
- `evbuffer` 如何管理可增长的字节队列。
- `bufferevent` 如何封装输入缓冲、输出缓冲、超时和连接事件。
- `bufferevent_filter` 如何在收发路径中透明转换数据。
- 一个线程一个 `event_base` 的线程池如何接收主线程分发的连接。
- libevent HTTP API 和 FTP 控制连接/数据连接的基本结构。

## 仓库结构

```text
libevent_study/
├── bin/       Windows 可执行程序、调试文件和 zlib1.dll
├── include/   libevent 与 zlib 头文件
├── lib/       Windows 静态库、导入库和调试符号
├── src/
│   ├── first_libevent/
│   ├── test_timer/
│   ├── test_signal/
│   ├── test_signal_loop/
│   ├── test_file/
│   ├── test_conf/
│   ├── test_server/
│   ├── test_event_server/
│   ├── test_buffer/
│   ├── test_buffer_client/
│   ├── test_buffer_filter/
│   ├── test_buffer_filter_zlib/
│   ├── test_http_client/
│   ├── test_http_server/
│   ├── test_thread_pool/
│   └── ev_ftp_server/
└── README.md
```

## 构建环境

### Windows

当前 Visual Studio 工程主要使用：

- C++20
- MSVC 平台工具集 `v145`
- `x64`
- 仓库内 `include/` 和 `lib/`
- `libevent.lib`
- zlib 示例额外链接 `zlib.lib`
- Winsock 系统库

包含解决方案的示例：

```text
src/first_libevent/first_libevent.slnx
src/test_server/test_server.slnx
src/test_conf/test_conf.slnx
src/test_buffer_filter_zlib/test_buffer_filter_zlib.slnx
src/test_http_client/test_client.slnx
src/test_http_server/test_server.slnx
src/test_thread_pool/test_thread_pool.slnx
src/ev_ftp_server/ev_ftp_server.slnx
```

构建步骤：

1. 打开目标 `.slnx`。
2. 选择 `Debug|x64`。
3. 构建并运行。
4. 如果本机没有 `v145`，在项目属性中重新定向平台工具集。
5. zlib 示例运行时确保 `zlib1.dll` 位于可执行文件目录或可搜索路径。

只有 Makefile、没有 Visual Studio 解决方案的目录，可以参考邻近项目创建工程，或在 Linux 环境构建。

### Linux

每个示例目录都有 `makefile`，主要使用 C++17。基础示例：

```bash
cd src/first_libevent
make
make run
make clean
```

常见依赖：

```bash
# Debian/Ubuntu 示例
sudo apt install g++ make libevent-dev zlib1g-dev
```

主要链接选项：

- 普通示例：`-levent`
- zlib 过滤示例：`-levent -lz`
- FTP 和线程池示例：`-levent -pthread`

不同发行版的包名可能不同。

## 示例总览

| 示例 | 源码实际内容 | 默认资源 | 重点 |
| --- | --- | --- | --- |
| [`first_libevent`](src/first_libevent) | 创建 `event_base` | 无 | 最小 libevent 上下文 |
| [`test_timer`](src/test_timer) | 非持久、持久和 common timeout 定时器 | 1s、1.2s、3s | 回调阻塞会拖延整个事件循环 |
| [`test_signal`](src/test_signal) | `SIGINT` 持久事件和 `SIGTERM` 非持久事件 | 系统信号 | pending 状态和重新添加 |
| [`test_signal_loop`](src/test_signal_loop) | 退出事件循环及不同 loop 标志 | `Ctrl+C` | `loopbreak` 与 `loopexit` |
| [`test_file`](src/test_file) | 监听普通文件描述符 | `/var/log/auth.log` | Linux 文件事件与 `EV_FEATURE_FDS` |
| [`test_conf`](src/test_conf) | 后端、特征、IOCP 和 CPU 提示 | TCP 5001 | event_config 能力选择 |
| [`test_server`](src/test_server) | `evconnlistener` 接受连接 | TCP 5001 | 高层监听器封装 |
| [`test_event_server`](src/test_event_server) | 原始 Socket 事件服务端 | TCP 5001 | accept、客户端事件、超时、LT/ET |
| [`test_buffer`](src/test_buffer) | `bufferevent` 服务端 | TCP 5001 | 输入输出缓冲、水位和超时 |
| [`test_buffer_client`](src/test_buffer_client) | 同一事件循环中的服务端与文件发送客户端 | TCP 5001 | 异步连接和回调驱动发送 |
| [`test_buffer_filter`](src/test_buffer_filter) | 输入/输出过滤器 | TCP 5001 | 在缓冲链中变换数据 |
| [`test_buffer_filter_zlib`](src/test_buffer_filter_zlib) | 客户端压缩、服务端解压并写文件 | TCP 5001、`001.txt` | zlib 状态、filter 与底层缓冲 |
| [`test_http_client`](src/test_http_client) | GET 下载和 POST 请求 | 外部 HTTP、127.0.0.1 | URL 解析、请求头、响应体 |
| [`test_http_server`](src/test_http_server) | 静态文件 HTTP 服务端 | TCP 8080、当前目录 | 请求方法、MIME、文件响应 |
| [`test_thread_pool`](src/test_thread_pool) | 将连接轮询分发到 10 个事件线程 | TCP 5001 | socketpair/pipe 唤醒工作线程 |
| [`ev_ftp_server`](src/ev_ftp_server) | 主动模式 FTP 教学服务端 | TCP 21 | 命令分发、PORT、LIST、RETR、STOR |

## libevent 核心模型

一个事件通常经历：

```text
创建 event_base
      |
      v
创建 event，并指定 fd/信号/超时、标志、回调和参数
      |
      v
event_add()：进入 pending 状态
      |
      v
内核后端等待事件
      |
      v
条件满足：事件变为 active
      |
      v
libevent 调用 callback
      |
      v
非持久事件退出 pending；持久事件继续等待
```

常用对象：

| 对象 | 作用 |
| --- | --- |
| `event_base` | 一个事件循环及其底层 I/O 后端 |
| `event` | 一个文件描述符、信号或超时的监听规则 |
| `evconnlistener` | TCP 监听和接受连接的高层封装 |
| `evbuffer` | 可增长的字节缓冲队列 |
| `bufferevent` | 带输入/输出缓冲的异步 I/O 对象 |
| `evhttp` | 构建在 event_base 上的 HTTP 层 |

一个 `event_base` 通常由一个线程运行。跨线程操作前需要按 libevent 的线程规则初始化和设计，不能默认任意对象都可并发访问。

## first_libevent

最小示例只执行：

```cpp
event_base* base = event_base_new();
```

`event_base_new()` 会选择当前平台可用的最佳事件后端。Linux 常见 epoll，Windows 常见 win32/IOCP 相关实现。当前示例只验证创建成功，没有进入事件循环，也没有释放 `base`；完整程序应调用 `event_base_free(base)`。

Windows Socket 相关示例在使用网络 API 前调用 `WSAStartup()`，结束时应调用 `WSACleanup()`。

## 定时器

`test_timer` 同时创建三类定时器：

### 非持久定时器

`evtimer_new()` 创建的事件触发一次后不再 pending。`time1()` 回调检查状态并再次调用 `evtimer_add()`，从而形成周期效果。

### 持久定时器

```cpp
event_new(base, -1, EV_PERSIST, time2, nullptr);
```

`EV_PERSIST` 让事件触发后继续保持。示例周期约 1.2 秒，但回调内部休眠 3 秒。因为回调就在事件循环线程中运行，这 3 秒期间其他定时器和 I/O 回调都不能执行。

重要结论：

- libevent 是异步等待，不代表回调自动并行。
- 回调应尽量短。
- 耗时计算应交给工作线程或任务队列。

### common timeout

`event_base_init_common_timeout()` 可让大量相同超时时间的事件共享优化后的超时表示。返回指针由 `event_base` 管理，调用者不要自行释放。

示例同时打印系统时间和 `steady_clock` 经过时间。计算耗时应使用单调的 `steady_clock`，避免系统时间调整影响。

## 信号和事件循环退出

`test_signal` 使用：

```cpp
evsignal_new(base, SIGINT, Ctrl_C, arg);
event_new(base, SIGTERM, EV_SIGNAL, Kill, arg);
```

`evsignal_new()` 隐含 `EV_SIGNAL | EV_PERSIST`。非持久信号事件触发后离开 pending，需要再次 `event_add()` 才能继续监听。

`event_self_cbarg()` 允许回调收到当前事件对象本身，便于检查状态或重新注册。

### `loopbreak` 与 `loopexit`

| 方法 | 行为 |
| --- | --- |
| `event_base_loopbreak(base)` | 当前回调结束后尽快退出循环 |
| `event_base_loopexit(base, nullptr)` | 允许当前一批 active 事件处理后退出 |
| `event_base_loopexit(base, &tv)` | 计划在指定延迟后退出 |

事件循环形式：

| 写法 | 是否阻塞等待 | 没有 pending 事件时 |
| --- | ---: | --- |
| `event_base_dispatch(base)` | 是 | 退出 |
| `event_base_loop(base, 0)` | 是 | 退出 |
| `event_base_loop(base, EVLOOP_ONCE)` | 是 | 处理一轮后退出 |
| `event_base_loop(base, EVLOOP_NONBLOCK)` | 否 | 立即返回 |
| `event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY)` | 是 | 保持循环直到显式退出 |

## 文件描述符事件

`test_file` 是 Linux 导向的示例：

1. 使用 `event_config_require_features(conf, EV_FEATURE_FDS)` 要求后端支持普通文件描述符。
2. 以非阻塞方式打开 `/var/log/auth.log`。
3. `lseek()` 到文件尾部。
4. 创建 `EV_READ | EV_PERSIST` 事件。
5. 新日志可读时执行回调。

普通磁盘文件在不同事件后端上的可监听能力不同。Windows 部分虽然保留初始化代码，但 `open/read/lseek/close` 路径和 `EV_FEATURE_FDS` 主要面向 POSIX 环境。

日志文件路径和权限因发行版而异。某些系统使用 journald，并不存在 `/var/log/auth.log`。

## event_config 与后端选择

`test_conf` 会：

- 列出 `event_get_supported_methods()`。
- 创建 `event_config`。
- 可选排除某些后端。
- 可要求 ET、O(1)、任意 FD、early close 等能力。
- Windows 下尝试启用 IOCP 和线程支持。
- 打印实际选择的后端和特征。

特征要求必须由同一个后端满足。设置不兼容组合时，`event_base_new_with_config()` 可能失败。示例失败后回退到普通 `event_base_new()`。

Windows 的：

```cpp
evthread_use_windows_threads();
```

需要链接对应 libevent 线程支持库，且应在创建相关 event_base 之前完成线程初始化。

## TCP 监听：两种层次

### 原始 event 方式

`test_event_server` 手工完成：

```text
socket
  -> 设置非阻塞和地址复用
  -> bind
  -> listen
  -> 为监听 Socket 创建 EV_READ 事件
  -> 回调中 accept
  -> 为客户端 Socket 创建 EV_READ|EV_PERSIST 事件
```

客户端事件带 10 秒超时。`recv()` 返回正数时打印并回复 `ok`；返回 0 或错误时释放事件并关闭 Socket。

水平触发下，只要缓冲区仍有数据就会继续通知。边缘触发 `EV_ET` 通常要求非阻塞，并在回调中一直读取到 `EAGAIN`，否则未读数据可能不再触发新边沿。

### `evconnlistener` 方式

`test_server` 使用：

```cpp
evconnlistener_new_bind(
    base,
    listen_cb,
    arg,
    LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
    backlog,
    address,
    address_length);
```

它一次完成 Socket 创建、绑定、监听和事件注册。回调直接收到已接受的客户端 Socket。示例只打印连接回调，没有继续处理或关闭客户端，因此主要用于理解监听器接口。

## evbuffer 与 bufferevent

### `evbuffer`

`evbuffer` 是字节队列，常见操作：

| API | 作用 |
| --- | --- |
| `evbuffer_get_length` | 查询当前字节数 |
| `evbuffer_add` | 复制数据到尾部 |
| `evbuffer_remove` | 从头部取出并移除数据 |
| `evbuffer_drain` | 丢弃前 N 字节 |
| `evbuffer_peek` | 查看内部片段而不移除 |
| `evbuffer_reserve_space` | 预留可直接写入的空间 |
| `evbuffer_commit_space` | 提交实际写入长度 |

### `bufferevent`

`test_buffer` 为每个客户端创建 Socket bufferevent：

```text
网络收到数据
  -> bufferevent 输入缓冲区
  -> read callback

业务调用 bufferevent_write
  -> bufferevent 输出缓冲区
  -> Socket 可写时自动发送
  -> 输出下降到低水位时 write callback
```

示例设置：

- 读取低水位 5：输入达到至少 5 字节才触发读回调。
- 读取高水位 10：输入达到上限后暂停继续读取。
- 写低水位 5：待发送数据下降到阈值时触发写回调。
- 读取超时 3 秒。

`BEV_OPT_CLOSE_ON_FREE` 表示释放 bufferevent 时同时关闭底层 Socket。释放后不能再访问该对象。

## 异步文件发送客户端

`test_buffer_client` 在同一个 `event_base` 中同时创建：

- 5001 端口服务端；
- 连接 `127.0.0.1:5001` 的客户端。

客户端连接成功后手动触发第一次写回调。每次输出缓冲下降时，从 `test_buffer_client.cpp` 再读一块并调用 `bufferevent_write()`，形成回调驱动的文件发送。

文件发送完成后关闭文件并禁用写事件，但保留读事件，以接收服务端回复。读回调主动 drain 输入，避免回复持续堆积。

运行依赖工作目录中存在：

```text
test_buffer_client.cpp
```

如果从其他输出目录启动，需要调整工作目录或文件路径。

## bufferevent 过滤器

过滤器位于上层 bufferevent 和底层 bufferevent 之间：

```text
应用写入
  -> filter_out
  -> 底层输出缓冲
  -> Socket

Socket
  -> 底层输入缓冲
  -> filter_in
  -> 应用输入缓冲
  -> read callback
```

过滤函数返回值：

| 返回值 | 含义 |
| --- | --- |
| `BEV_OK` | 成功处理，可能还有数据 |
| `BEV_NEED_MORE` | 当前数据不足，等待更多输入 |
| `BEV_ERROR` | 转换失败，触发错误流程 |

`test_buffer_filter` 的输入过滤器原样复制数据，输出过滤器给响应增加分隔文本，用于观察双向转换位置。

## zlib 压缩传输

`test_buffer_filter_zlib` 在一个事件循环中启动服务端和客户端：

```text
客户端发送文件名
        |
        v
服务端创建 out/文件并回复 OK
        |
        v
客户端读取 001.txt
        |
        v
filter_out 使用 deflate 压缩
        |
        v
TCP 传输
        |
        v
服务端 filter_in 使用 inflate 解压
        |
        v
服务端写入 out/001.txt
```

客户端状态记录原始读取字节数和压缩发送字节数；服务端记录压缩接收量和解压写出量。

过滤器使用 `evbuffer_peek()` 取得输入片段，并用 `reserve_space/commit_space` 直接向目标缓冲写入，避免额外中间复制。

运行前准备：

```text
src/test_buffer_filter_zlib/
├── 001.txt
└── out/
```

如果使用 Visual Studio 输出目录运行，相对路径会相对于调试工作目录，需要把测试文件复制过去或修改工作目录。

结束发送时不能只看过滤层输出缓冲，还要确认底层 bufferevent 输出缓冲已经清空，否则立即释放会丢掉尚未真正写入 Socket 的数据。

## HTTP 客户端

`test_http_client` 包含 GET 和 POST 两组流程。

GET：

1. `evhttp_uri_parse()` 解析 URL。
2. 取得 scheme、host、port、path 和 query。
3. 创建 Socket bufferevent。
4. 创建 `evhttp_connection`。
5. 创建请求并设置 `Host` 头。
6. `evhttp_make_request(..., EVHTTP_REQ_GET, path)`。
7. 回调中读取状态码和响应缓冲。
8. 按 URL 路径创建本地目录并保存响应体。
9. 调用 `event_base_loopbreak()` 退出。

当前 GET URL 是源码中的外部 HTTP 地址，运行结果依赖网络、DNS 和目标站点。

POST 向 `127.0.0.1:8080/index.html` 发送：

```text
name=zhangsan&age=20
```

当前请求没有完整设置内容类型等生产级头部，只用于观察输出缓冲和请求方法。

## HTTP 服务端

`test_http_server` 在 `0.0.0.0:8080` 监听：

1. `evhttp_new(base)` 创建 HTTP 上下文。
2. `evhttp_bind_socket()` 绑定端口。
3. `evhttp_set_gencb()` 注册通用请求回调。
4. 回调读取 URI、方法、请求头和正文。
5. 将 URI 拼到当前工作目录。
6. 根据后缀设置简单 MIME 类型。
7. 文件存在时返回 200，否则返回 404。

启动目录中应准备：

```text
index.html
其他静态资源
```

测试：

```bash
curl http://127.0.0.1:8080/
curl -X POST -d "name=test" http://127.0.0.1:8080/index.html
```

当前代码直接把 URI 拼入文件路径，没有路径规范化和目录穿越防护，不能暴露到不可信网络。查询字符串也可能影响文件路径判断。

## 事件线程池

`test_thread_pool` 的设计不是“工作线程执行阻塞业务”，而是每个工作线程维护独立 `event_base`：

```text
主线程 event_base
  |
  | accept 新连接
  v
XThreadPool::Dispatch()
  |
  | round-robin
  v
工作线程任务队列
  |
  | socketpair(Windows) / pipe(Linux) 写 1 字节
  v
工作线程 event_base 收到通知
  |
  v
取出 XTask，令其在本线程 event_base 上注册 bufferevent
```

### 为什么需要通知管道

工作线程通常阻塞在 `event_base_dispatch()`。主线程只把任务放进 C++ 队列，并不会自动让 event loop 醒来。向工作线程监听的 socketpair 或 pipe 写一个字节，可以制造一个可读事件，让它立刻执行 `Notify()`。

### `EVENT_BASE_FLAG_NOLOCK`

每个 `event_base` 只由所属工作线程使用，因此配置为无内部锁。主线程不直接修改工作线程的 event_base，而是通过受互斥锁保护的任务队列和通知 FD 交接任务。

### 调度

线程池使用轮询：

```cpp
tid = (lastThread + 1) % threadCount;
```

它只按连接数量平均分配，不考虑每个连接的实际负载。

当前线程使用 `detach()`，没有完整停止、资源回收和异常处理协议，是教学结构而非完整线程池库。

## FTP 服务端

`ev_ftp_server` 建立在事件线程池之上，默认监听 TCP 21。Linux 绑定 21 端口通常需要 root 或 `CAP_NET_BIND_SERVICE`；学习时可以把 `SPORT` 改为 2121。

### 控制连接

客户端连接后创建 `XFtpServerCMD`，它维护：

- 控制连接 bufferevent；
- 当前目录和根目录；
- 命令接收缓冲；
- 命令字符串到 `XFtpTask` 的映射；
- 主动模式数据连接的 IP 和端口。

接收缓冲按 `\r\n` 拆分命令，解决一次读取包含多条命令或半条命令的情况。

### 已注册命令

| 命令 | 处理类 | 行为 |
| --- | --- | --- |
| `USER` | `XFtpUSER` | 简化登录，直接回复成功 |
| `PWD` | `XFtpLIST` | 返回当前目录 |
| `LIST` | `XFtpLIST` | 建立数据连接并发送目录列表 |
| `CWD` | `XFtpLIST` | 修改当前目录 |
| `CDUP` | `XFtpLIST` | 返回上级目录 |
| `PORT` | `XFtpPORT` | 解析主动模式客户端 IP 和端口 |
| `RETR` | `XFtpRETR` | 读取文件并发送给客户端 |
| `STOR` | `XFtpSTOR` | 接收数据并写入文件 |

### 主动模式

客户端发送：

```text
PORT n1,n2,n3,n4,n5,n6
```

服务端解析：

```text
IP   = n1.n2.n3.n4
端口 = n5 * 256 + n6
```

随后服务端主动连接客户端的数据端口。控制连接继续传输 FTP 状态码，目录列表和文件内容走独立数据连接。

### 文件传输

`RETR`：

1. 打开文件。
2. 建立数据连接。
3. 回复 150。
4. 输出缓冲可写时继续读取文件并发送。
5. 完成后回复 226。

`STOR`：

1. 创建输出文件。
2. 建立数据连接。
3. 回复 150。
4. 读回调持续把输入缓冲写入文件。
5. EOF 时处理剩余数据、关闭文件并回复 226。

### FTP 示例限制

- `USER` 没有真实认证。
- 主要实现主动模式，没有 PASV。
- 路径拼接缺少可靠规范化，可能越过根目录。
- Linux 列目录通过 `popen("ls -l ...")`，路径未经安全转义。
- 没有 TLS、权限模型、传输限速和磁盘配额。
- 命令支持不完整，不能视为标准 FTP 服务端。

只应在隔离的本地环境学习。

## 端口与文件速查

| 示例 | 端口/文件 |
| --- | --- |
| TCP、bufferevent、线程池示例 | TCP 5001 |
| HTTP 服务端与本地 POST | TCP 8080 |
| FTP 服务端 | TCP 21 |
| 文件监听 | `/var/log/auth.log` |
| buffer client | 工作目录中的 `test_buffer_client.cpp` |
| zlib 文件传输 | `001.txt` 和 `out/` |
| HTTP 服务端 Web 根目录 | 当前工作目录 |

端口冲突时先确认没有同时运行多个监听相同端口的示例。

## 常见问题

### `evconnlistener_new_bind failed`

检查端口是否占用、端口权限、防火墙、地址结构是否初始化，以及错误输出：

```cpp
evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())
```

### 程序进入 `event_base_dispatch()` 后不返回

事件循环会持续等待已注册事件。这通常是正确行为。需要通过信号回调、`loopbreak()`、`loopexit()` 或释放所有事件退出。

### 定时器没有按预期时间触发

检查其他回调是否执行了阻塞操作。一个 3 秒的回调会让同一 event_base 上的其他事件全部延后。

### 文件或网页资源找不到

相对路径相对于进程当前工作目录，不是源码目录。检查 Visual Studio 项目属性中的“调试 -> 工作目录”。

### zlib 示例找不到 DLL

确保 `zlib1.dll` 与 `.exe` 同目录，或位于 `PATH` 可搜索目录。链接成功只说明构建时找到了 `.lib`。

### Linux 写 Socket 时进程突然退出

对端关闭后继续写可能触发 `SIGPIPE`。多个示例使用：

```cpp
signal(SIGPIPE, SIG_IGN);
```

忽略信号后仍要检查发送函数的错误返回值。

### ET 模式只收到一次回调

边缘触发要求非阻塞并持续读取到 `EAGAIN`。如果只读取一小部分，剩余数据可能不会再次通知。

## 推荐学习路线

### 第一阶段：事件循环

1. `first_libevent`
2. `test_timer`
3. `test_signal`
4. `test_signal_loop`
5. `test_conf`

### 第二阶段：文件和网络事件

1. `test_file`
2. `test_server`
3. `test_event_server`

### 第三阶段：缓冲 I/O

1. `test_buffer`
2. `test_buffer_client`
3. `test_buffer_filter`
4. `test_buffer_filter_zlib`

### 第四阶段：应用协议和并发架构

1. `test_http_client`
2. `test_http_server`
3. `test_thread_pool`
4. `ev_ftp_server`

## 建议实验

- 给 `first_libevent` 补充 `event_base_get_method()` 和资源释放。
- 在定时器回调中去掉休眠，比较其他事件的触发时间。
- 分别使用 `loopbreak()` 和延迟 `loopexit()`，观察退出顺序。
- 把 `test_event_server` 改成 ET，并循环读取到 `EAGAIN`。
- 调整 bufferevent 高低水位，观察回调频率。
- 给 filter 添加大小统计或简单字符转换。
- 比较压缩前后字节数，分别测试文本和已经压缩的图片。
- 为线程池增加停止通知、线程 join 和资源回收。
- 把 FTP 端口改为 2121，并用本地客户端观察控制命令和数据连接。

## 代码边界

这些代码用于理解 libevent API 和事件驱动架构，部分错误检查、所有权管理和协议处理经过简化。真实服务还需要补充资源上限、超时策略、背压、线程安全、连接关闭状态机、输入边界、路径规范化、认证、TLS、日志和压力测试。
