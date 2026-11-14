# DreamMediaHub bug / UB 检查报告

检查日期：2026-11-10。范围：Qt 播放器、Qt 录制器，以及子模块中的 DreamPlayer、DreamRecorder 和公共代码。

初查基于主仓库 HEAD `63483919033e564039707f684883dd1456fa8beb` 和 native HEAD `84f261e30ea1d7274af18916484cab17963569ff`。2026-11-11 复验了 Qt 条目并落实 QT-09。2026-11-12 复核了 QT-04 至 QT-08。2026-11-13 复核了 QT-05 至 QT-08。2026-11-14 复核了 QT-05 至 QT-07。✔ 表示已修改、待复核，✅ 表示验收通过。

本文共记录 34 项实现问题，其中 Qt 层 9 项，native 层 25 项。复核重新打开的条目附有解决思路和验证方法。

## 阅读说明

- **P1**：可能造成内存破坏、崩溃、永久等待，或主要功能明显失效。
- **P2**：确定的功能、同步、数据完整性、资源或诊断错误。
- **实测**：用临时验证程序调用当前实现，或验证对应的 Qt 语义、数值表达式。具体边界见文末。
- **静态确认**：由代码中的生命周期、线程访问或状态转换直接确定；不表示已经在完整 GUI 中复现。
- **条件性**：代码缺陷确定，但需要文中所列的媒体、设备或操作条件才会触发。

不同问题可能互相遮蔽。例如，解码器的栈对象越生命周期访问可能先于其他播放问题出现；未 seek 的首次播放又会遮蔽部分正常结束后的问题。不能把“当前先在别处崩溃”理解为后面的缺陷不存在。

## 问题索引

| 编号 | 等级 | 问题 | 性质 |
|---|---|---|---|
| QT-01 | P1 | 缺少启动参数仍继续取 `argsList[1]` ✅ | 越界 / UB |
| QT-02 | P1 | 无音频设备的降级路径仍解引用音频对象 ✅ | 空指针 / UB |
| QT-03 | P1 | 打开媒体失败后仍进入正常播放流程 ✅ | 空指针 / UB |
| QT-04 | P1 | 队列信号传递录音缓冲区裸指针 ✅ | 悬空数据 / UB |
| QT-05 | P1 | 音频 EOF 仍空转，暂停时关闭可能卡死（重新打开） | 空转 / 退出挂起 |
| QT-06 | P1 | 满队列时 seek 可导致空指针访问，分段仍有重叠拷贝（重新打开） | 崩溃 / 音频数据错误 / UB |
| QT-07 | P2 | 预缓存目标曾因整帧入队而无法达到 ✅ | 条件性超时 |
| QT-08 | P2 | 摄像头限帧间隔因整数除法变为 0 ✅ | 帧率错误 |
| QT-09 | P1 | 录制路径错误处理 Windows 盘符和 URL 编码 ✅ | 路径错误 / 退出 |
| N-01 | P1 | 解码包的局部对象已经销毁，仍通过指针访问 | UB，ASan 确认 |
| N-02 | P1 | planar 音频只准备一个平面指针 | 数组越界 / UB |
| N-03 | P1 | FFmpeg 对象池被多个线程无同步读写 | 数据竞争 / UB |
| N-04 | P1 | native 工作线程在所用成员销毁后才 join | 生命周期 / UB |
| N-05 | P1 | 缺少某类流仍启动对应解码器、操作音频滤镜 | 空指针 / UB |
| N-06 | P1 | 播放状态、序号、时钟和回调有跨线程数据竞争 | 数据竞争 / UB |
| N-07 | P2 | 初始序号 0 同时代表“未结束”和实际播放代次 | 完成判定错误 |
| N-08 | P1 | 暂停时持续取出并丢弃音频帧 | 音频丢失 |
| N-09 | P2 | 暂停/恢复没有冻结和重定位时钟基准 | 进度跳变 |
| N-10 | P1 | 音频 PTS 直接除采样率，忽略流时间基 | 时间戳错误 |
| N-11 | P2 | 音频段的起始 PTS 被赋成帧结束时间 | 同步偏移 |
| N-12 | P1 | 音频采样补偿计算错误，正常输入可被全部丢弃 | 音频失效 |
| N-13 | P1 | 分辨率变化后复用旧转换上下文或旧 stride | 条件性内存错误 |
| N-14 | P1 | 奇数高度的 YUV420 色度平面少分配一行 | 越界写 / UB |
| N-15 | P1 | 未知时长转无符号整数超出可表示范围 | UB，表达式实测 |
| N-16 | P1 | 第二次 open 沿用旧解码器和流状态 | 重开错误 / 线程冲突 |
| N-17 | P2 | VideoMaster 模式的视频调度没有应用倍速 | 倍速错误 |
| N-18 | P2 | seek 只刷新 codec，旧音频转换状态继续保留 | seek 数据污染 |
| N-19 | P2 | 音频 EOF 没有排空重采样器和 atempo | 尾音丢失 |
| N-20 | P1 | 编码器初始化失败后仍访问 `stream_` 等对象 | 空指针 / UB |
| N-21 | P1 | stop 直接退出编码和封装循环，没有完成排空 | 录制尾部丢失 |
| N-22 | P2 | 录制上下文和部分在途帧没有释放 | 资源泄漏 |
| N-23 | P2 | 成功取得 FFmpeg 错误文本后仍返回空字符串 | 诊断错误 |
| N-24 | P2 | 把 3 元素数组交给按 4 元素读取的 `sws_scale` | 越界读 / UB |
| N-25 | P2 | `isPlaying()` 直接返回“是否暂停” | 状态反转 |

## 一、Qt 层

### QT-01 — 缺少启动参数时仍越界访问参数列表 ✅

**P1 · 越界 / UB · 静态确认**

位置：[main.cpp:16](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/main.cpp:16)、[playerController.cpp:49](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:49)。

`main()` 在 `argc < 2` 时只打印 Usage，没有退出。随后 QML 创建 `PlayerController`，构造函数直接读取 `argsList[1]`。没有媒体参数时，列表只有程序名，索引 1 越界。

触发方式：直接运行 `playerApp`，不传媒体路径。Debug Qt 通常会触发断言；关闭相应检查的构建可能出现非法访问。打印 Usage 并没有阻止错误路径继续执行。

#### 复验

**通过。** 当前入口在 `argc < 2` 时调用 `qFatal()`。运行重新编译的 `playerApp` 且省略媒体参数，实测打印 Usage 后以 `SIGABRT` 结束，控制器构造尚未执行。

### QT-02 — 无声卡或格式不支持时，播放控制仍使用空音频指针 ✅

**P1 · 空指针 / UB · 静态确认**

位置：[playerController.cpp:59](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:59)、[playerController.cpp:149](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:149)、[playerController.cpp:171](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:171)。

构造函数发现默认输出设备不支持指定格式时，只切换成 `VideoMaster`；`audioBufferDevice_` 保持 `nullptr`。但 `play()`、`pause()` 无条件调用该对象的方法，`seek()` 还会调用 `play()` 和 `clearQueue()`。

触发条件：无音频输出设备，或设备不支持 44100 Hz / 双声道 / Int16，然后按空格或方向键、操作播放按钮或进度条。

影响：本应允许无音频播放的路径会调用空对象。这里对 `player_` 的检查不能保护 `audioBufferDevice_`。

#### 复验

**通过。** 对照当前 `play()`、`pause()` 和 `seek()`，三个入口访问 `audioBufferDevice_` 前均检查了指针。验收依据是当前调用链。

### QT-03 — 媒体打开失败后继续读取未初始化的 native 时钟 ✅

**P1 · 空指针 / UB · 静态确认**

位置：[playerController.cpp:138](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:138)、[playerController.cpp:238](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:238)、[FFmpegPlayer.cpp:40](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:40)、[mediaClock.cpp:21](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/mediaClock.cpp:21)。

Qt 层丢弃 `player_->open()` 的布尔返回值，继续把界面标为播放中、创建音频拉取任务并启动进度定时器。native 在打开失败时会提前返回，正常路径末尾的 `initClock()` 没有执行。

如果音频后端没有先阻塞初始化，后续定时器会调用 `getCurrentTime()`，最终进入 `Clock::getTime()`，其中第一步就是解引用 `queSerial`；该指针初始为 `nullptr`。无音频输出设备的分支没有启动音频后端，会直接走到这个定时器路径。

触发方式：首次启动时传入不存在、无权限或无法打开的媒体路径。影响不只是文件打不开，还可能在后续定时器更新时崩溃。

#### 复验

**通过。** 传入不存在的媒体路径，实测进入 `open file failed` 的 `qFatal()` 分支。此时构造函数尚未启动音频或进度定时器。

### QT-04 — 录音数据跨线程排队时只复制了指针 ✅

**P1 · 数据失效 / 潜在悬空访问 · Qt 语义实测，调用链静态确认**

位置：[audioCapture.cpp:63](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/src/recorderComp/audioCapture.cpp:63)、[recorderController.cpp:66](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/src/recorderComp/recorderController.cpp:66)、[recorderWorker.cpp:53](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/src/recorderComp/recorderWorker.cpp:53)。

初查时 `AudioCapture` 发出的是 `audioBuffer_.constData()`，参数类型为 `const char*`。连接明确指定 `Qt::QueuedConnection`，接收方稍后才会在音频工作线程中读取这块内存。信号发出后，发送方立刻执行 `audioBuffer_.remove(0, validBytes)`，后续采集还会继续追加、移动或复用底层存储。

队列事件保存了指针值，没有保存指向的那段录音数据。实际读取时，内容可能已变成下一批数据，也可能因缓冲区重分配而失效。停止录制时先删除 `AudioCapture`、之后才等音频工作线程退出，也会使尚未处理的事件持有指向已释放缓冲区的指针。

影响：录音数据混批、杂音、音频损坏；涉及底层内存释放时存在 use-after-free。

验证：独立的 Qt 队列信号实验在发送 `'A'` 后、派发事件前把原缓冲区改为 `'Z'`，接收端读到的是 `'Z'`。这是对传参语义的验证，没有调用麦克风。

### QT-05 — 音频 EOF 仍空转，暂停时关闭可能卡死（重新打开）

**P1 · CPU 空转 / 退出挂起 · 当前实现局部实测**

空队列和暂停时，`QIODevice::read()` 都立即返回 0 字节。用替身先送一帧无效 PCM、再送一帧有效 PCM，恢复播放后，设备所属线程发出 `readyRead()`，接收槽收到正确的 4096 字节。 [readData()](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:150) · [postReadyRead()](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:212)

- **条件变量在取到有效帧后才生效。** `getAudioFrame()` 在 native 队列已关闭且为空时立即返回无效帧。暂停期间，native 也可能先消费一帧，再返回 `isUpdated == false`。这两种返回都直接走 `continue`，跳过 `cvPause_`。替身持续返回 EOF 时，50 ms 内调用了约 49 万次。若线程已经通过暂停等待、随后卡在容量等待，暂停状态也不会在入队前重新检查。 [pull()](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:54) · [native 取帧](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:114)
- **停止没有解除所有等待。** 拿到有效帧后在暂停状态析构设备，实测 2 秒内析构未返回。`cvPause_` 只检查 `!isPause_`，析构也只通知了 `cvBuffer_`。满队列用例中，请求 stop 后 50 ms 内线程仍在等，读走 4096 字节唤醒线程后，它又入队了 4096 字节才退出。成员逆序析构时，两个条件变量的销毁还早于 `thread_` 自动 join。 [停止与析构](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:25) · [成员顺序](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/include/videoComp/audioBufferDevice.h:72)

#### 解决思路

Qt 用一个条件变量等待暂停恢复和队列腾出空间。取帧前先等，取到帧后在入队前再等一次，覆盖等待 native 返回期间发生的暂停。native 的取帧接口接收同一个 `stop_token`，在音频帧队列上等待可读、seek 恢复或停止。两个层次的等待都能被同一次停止请求解除。

1. **native 取帧。** 在音频帧队列的锁下维护可读帧、当前 `serial`、暂停和关闭状态。正常播放时有当前代次的帧才取。暂停时先等恢复，避免继续出队（N-08）。当前代次排空后继续阻塞等待，吸收现在的「队列关闭且为空」立即返回分支。切换代次后唤醒取帧，并等新代次第一帧。结束整个播放器或收到 stop token 时返回空指针。转换失败则继续等下一帧。可用 `condition_variable_any::wait(lock, st, predicate)` 把停止纳入同一次等待。状态变化和通知对应到帧入队、暂停恢复、成功 seek 和关闭。首轮 EOF 的判定见 N-07。 [当前空队列返回条件](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/include/common/concurrentQueue.hpp:106)
2. **恢复与关闭。** 暂停时先关闭 Qt 的拉取和读取入口，再暂停 native。恢复时先让 native 可取帧，再将 Qt 的 `isPause_` 置为 false，解锁后通知条件变量并投递 `readyRead()`，使已有 PCM 也能恢复读取。关闭时先停止 `QAudioSink`，再对 Qt 拉取线程请求停止并显式 join。随后停止并等待 native 的 seek 回调线程退出，注销回调，再销毁设备和播放器。当前 `player_->stop()` 只关闭队列和请求 native 停止，还需要等实际线程结束（N-04）。 [当前控制器清理](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:215) · [native 线程退出接口](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/include/common/dreamThread.hpp:32)

下面的 Qt 示例依赖上述 native 取帧契约，所需声明和接线如下。

- `DreamPlayer`、`FFmpegPlayer` 和 `AudioDecodec` 的取帧声明与实现都接收 `std::stop_token`，返回类型为 `const Dream::AudioFrame*`。native 在下一次取帧或销毁播放器前保证返回值可读，seek 通过序号失效旧帧。
- 增加音频专用回调类型 `using OnAudioSeekCallback = std::function<void(uint32_t)>`，并贯穿三个类的音频回调注册及转发。解码器处理音频 flush 时传入对应的 `decodecSerial_`。`setPlayer()` 注册 `[this](uint32_t serial) { onSeek(serial); }`。视频回调继续使用其现有接口。
- `AudioBufferDevice` 用 `std::condition_variable_any cvBuffer_` 承担 Qt 的等待，移除 `cvPause_` 和成员 `audioFrame_`，增加 `uint32_t acceptedSerial_{}`，把 `onSeek()` 声明改为 `onSeek(uint32_t serial)`。当前首次音频序号为 0，若支持复用播放器，需要在拉取启动前同步其当前音频序号。
- `pause()` 在队列锁下设置 `isPause_`。`play()` 在同一把锁下复位，解锁后调用 `cvBuffer_.notify_all()` 和 `postReadyRead()`。`readData()` 读走 PCM 后通知 `cvBuffer_`。`PlayerController::seek()` 提交请求，清队列接入成功回调，具体顺序见 QT-06。

```cpp
void AudioBufferDevice::pull(std::stop_token st) {
    constexpr uint32_t sampleFrameBytes = CHANNEL_COUNT * SAMPLE_SIZE;
    const Dream::AudioFrame* frame = nullptr;
    uint32_t offset = 0;

    std::unique_lock lock(mtx_);
    while (true) {
        cvBuffer_.wait(lock, st, [&] {
            return isClose_ ||
                   (frame && frame->serial != acceptedSerial_) ||
                   (!isPause_ && MAX_BUFFER_SIZE - dataLen_ >= sampleFrameBytes);
        });

        if (st.stop_requested() || isClose_) {
            return;
        }

        if (frame && frame->serial != acceptedSerial_) {
            frame = nullptr;
            continue;
        }

        if (!frame) {
            // 取帧可能等待 native 出帧，先释放 Qt 缓冲区锁。
            lock.unlock();
            frame = player_->getAudioFrame(st);
            // 回到 Qt 共享状态的保护范围。
            lock.lock();
            offset = 0;
            if (!frame) {
                return;
            }
            // 等待期间可能发生暂停、seek 或停止，回到循环重新检查。
            continue;
        }

        const uint32_t frameLen = static_cast<uint32_t>(frame->len);
        const uint32_t freeBytes = MAX_BUFFER_SIZE - dataLen_;
        const uint32_t count = std::min(
            frameLen - offset, freeBytes - freeBytes % sampleFrameBytes);
        AudioSegment segment{};
        segment.pts = frame->pts;
        segment.serial = frame->serial;
        segment.data.assign(frame->data + offset, frame->data + offset + count);
        segQue_.push_back(std::move(segment));
        dataLen_ += count;
        offset += count;
        if (offset == frameLen) {
            frame = nullptr;
        }

        lock.unlock();
        cvBuffer_.notify_all();
        postReadyRead();
        lock.lock();
    }
}

void AudioBufferDevice::onSeek(uint32_t serial) {
    {
        std::lock_guard lock(mtx_);
        acceptedSerial_ = serial;
        segQue_.clear();
        dataLen_ = 0;
    }
    cvBuffer_.notify_all();
}

void AudioBufferDevice::stopPull() {
    {
        std::lock_guard lock(mtx_);
        isClose_ = true;
    }
    thread_.request_stop();
    cvBuffer_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}
```

这里的 `lock` 持有 `AudioBufferDevice::mtx_`，保护 Qt 的队列、暂停状态和允许入队的序号。前面的 `cvBuffer_.wait()` 在等待时临时释放锁，返回时已经重新持锁。接下来的 `getAudioFrame()` 会在 native 队列上等待下一帧，其内部等待使用 native 自己的锁。如果取帧期间继续持有 Qt 锁，`readData()`、`pause()`、`play()`、`onSeek()` 和示例中的 `stopPull()` 都无法取得它。例如，拉取线程持有 Qt 锁等新帧，native 解码线程处理 seek 时同步调用 `onSeek()`，回调又等 Qt 锁，该解码线程无法继续处理新帧，就形成相互等待。`unlock()` 让这些入口可以推进。取帧结束后 `lock()` 恢复对共享状态的保护，再由 `continue` 回到循环重新检查停止、暂停、容量和帧序号。`frame` 与 `offset` 属于拉取线程的局部状态，底层 PCM 的有效期由前述 native 接口保证。

`st` 来自 Qt 拉取线程的 `std::jthread`。示例将同一个停止令牌沿 `pull(st) → getAudioFrame(st) → acquireFrame(st) → 音频队列等待` 传下去，使 `thread_.request_stop()` 也能结束正在 native 内部等待的这次取帧。只在外层循环检查停止状态时，已经进入取帧等待的线程需要先返回才能检查，`join()` 就可能一直等。native 的所有相关等待需接入 `condition_variable_any::wait(nativeLock, st, predicate)` 或等效的取消机制，停止后逐层返回，最终让示例中的 `getAudioFrame(st)` 返回空指针。传入令牌本身不会自动中断普通条件变量等待。当前无参 `getAudioFrame()` 需要按这条调用链配套修改。若由播放器停止并关闭队列来解除等待，也应保证该操作覆盖所有取帧等待，且发生在 join 之前。这里选择传令牌，使 Qt 拉取线程自身的停止请求能直接解除取帧等待。

示例里的 `getAudioFrame(st)` 只交付长度大于零、按 4 字节采样帧对齐的有效 PCM。析构调用 `stopPull()`，控制器也可以在销毁设备前先调用一次。 上述 Qt 示例已在临时副本中编译，并用满足该接口契约的替身验证暂停、满队列和 native 等帧时停止、49152 字节分段比对、满队列 seek 及取帧跨 seek。native 的实现需要按前述步骤配套修改。验收时覆盖 EOF 后 seek、音轨早于视频排空后 seek、暂停恢复，以及在暂停、满队列、native 等帧三个位置停止。每种停止都应在不依赖消费者读数据的情况下完成 join，停止后的队列不再增加 PCM。

### QT-06 — 满队列时 seek 可导致空指针访问，分段仍有重叠拷贝（重新打开）

**P1 · 崩溃 / 音频数据错误 · 当前实现局部实测**

- **满队列时 seek 可崩溃。** 49152 字节的帧填满 40960 字节队列后，`pull()` 在容量条件变量上等待。seek 把共享的 `audioFrame_` 清零并通知，生产者醒来直接读 `audioFrame_->len`，UBSan 报空指针成员访问，随后 ASan 报段错误。生产者也在锁外检查和赋值这个成员，与 seek 的写入存在数据竞争。 [容量等待和解引用](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:54) · [onSeek()](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:133)
- **取帧跨越 seek 时仍会接收旧音频。** 阻塞替身的 `getAudioFrame()`，先执行 seek 回调，再让取帧返回旧 `serial=1`。当前实现将旧代次的 4096 字节 PCM 入队并读出。控制器在请求 seek 前清队列一次，native 确认时再清一次，两个清理点都没有校验入队帧的序号。 [控制器的 seek 路径](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:171) · [native 的 seek 回调](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:124)
- **分段后搬移 PCM 仍有 UB。** 先入队 40956 字节，再送 8192 字节，队列只剩 4 字节。ASan 在 `memcpy(data, data + 4, 8188)` 报 `memcpy-param-overlap`。 [重叠拷贝](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:80)

#### 解决思路

1. **帧与偏移归生产者。** 按 QT-05 示例，把帧指针和已入队偏移放在 `pull()` 的局部变量。每批只拷贝本次能放入队列的范围，剩余空间按双声道 S16 的 4 字节采样帧对齐。只推进本地偏移，保留 native 的 PCM、原始长度和时间戳。无须在 seek 回调里碰生产者的指针。
2. **以成功 seek 的音频序号切换。** 控制器提交请求，成功 seek 时 demux 向音频解码器排入 flush，并为它分配新的音频包队列序号。即使音轨已先于视频排空，这次成功 seek 也要重开音频队列。切换代次时清理旧队列并唤醒卡在旧代次入队的解码器，使其能处理 flush。`Decodec` 处理 flush 后，音频回调携带该 flush 对应的 `serial`，在发布首个新代次帧前调用 Qt 的 `onSeek(serial)`。Qt 在同一把锁下更新允许入队的序号、清空旧段并唤醒生产者。保留当前「seek 会恢复播放」的交互，清队列由成功回调统一执行，seek 失败时仍使用现有代次。 [demux seek](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/demuxer.cpp:100) · [序号递增](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/packetQueue.cpp:9) · [当前队列重开条件](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:181)
3. **每批入队前校验。** 在 Qt 队列锁下检查停止状态和实际帧序号。发现旧序号就丢掉整帧剩余部分，回到取帧步骤。这样覆盖满队列等待、等待暂停恢复和取帧跨越 seek。取帧期间释放 Qt 队列锁，native 回调才能完成清队列。验收用容量只剩 4 字节、跨段读取、满队列 seek、取帧跨 seek 和连续两次 seek，逐字节比对 PCM，并确认每次成功回调后只读到对应代次的帧。分段 PTS 继续等 N-11 明确原帧起点后处理。

### QT-07 — 预缓存目标超过可入队容量 ✅

**P2 · 条件性超时 · 2026-11-14 复验通过**

当前 100 ms 预缓存目标为 `44100 × 2 × 2 × 0.1 = 17640` 字节，缓冲上限为 40960 字节，PCM 可以分批入队。用三帧 8192 字节调用当前 `waitForBuffered(100ms)`，实测成功返回，队列中有 24576 字节。 [预缓存入口](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/playerController.cpp:72) · [等待条件](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:117)

预缓存让 `QAudioSink` 启动前先积累一小段 PCM，给解码和调度波动留出余量，启动也会因此延后。解码能及时供给数据时，去掉这次显式等待可能听不出区别。当前 `readData()` 空读立即返回，后续 PCM 通过 `readyRead()` 通知音频后端继续读取。

### QT-08 — 摄像头限帧间隔因整数除法变为 0 ✅

**P2 · 帧率错误 · 静态确认**

位置：[cameraCapture.cpp:59](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/src/recorderComp/cameraCapture.cpp:59)。

初查时 `interval = 1000 / fps_` 的单位是毫秒，`now = nsecsElapsed() / 1000` 的单位是微秒，随后直接比较。`fps_ = 30` 时，实际阈值是 33 微秒。

触发条件：摄像头实际输出帧率高于设定值。例如 60 fps 摄像头的两帧间隔约 16667 微秒，始终大于 33，所有帧都会放行。

影响：设置的采集限帧不起作用，录制链路收到的帧数与声明的目标帧率不符，并增加转换、队列和编码负担。

### QT-09 — Windows 目录和包含 URL 转义的目录被转换为错误路径 ✅

**P1 · 路径错误，失败会导致进程退出 · QJSEngine 实测**

位置：[Main.qml:16](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/resource/Main.qml:16)、[Main.qml:35](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/resource/Main.qml:35)、[recorderController.cpp:51](/run/media/Workplace/CPPWorkplace/DreamMediaHub/recorder/src/recorderComp/recorderController.cpp:51)。

存在两条独立的正常输入触发路径：

1. 手动输入 `C:/Videos` 时，`isLocalPath()` 把 `C:` 识别成未知 URL scheme，返回 false；`generateFileName()` 因此直接返回目录字符串，没有追加 MP4 文件名。
2. 文件夹 URL 如 `file:///tmp/Video%20Files` 只被去掉 `file://` 前缀，`%20` 没有解码，最终路径指向带字面 `%20` 的目录，而不是 `Video Files`。

验证：直接用 `QJSEngine` 执行当前 QML 中的函数，分别得到 `Windows directory local? 0` 和 `/tmp/Video%20Files`。

影响：录制输出路径错误，通常会令 `avio_open` 失败。native 对这个失败直接调用 `exit(1)`，所以界面会随之退出。

#### 指令

界面问题，你直接落地解决，要求最小化修改。

##### 处理结果

QML 在判断 URL scheme 前识别 Windows 绝对盘符路径，并给本地目录或文件 URL 追加录制文件名。Qt 控制器用 `QUrl::toLocalFile()` 转换文件 URL，再将 UTF-8 路径交给 native。流媒体地址仍按原文传递。 [QUrl::toLocalFile](https://doc.qt.io/qt-6/qurl.html#toLocalFile)

Linux 上重新编译了播放器和录制器。路径探针执行当前 QML 函数和实际 `RecorderController::start()`，用替身录制器检查传入 native 接口的字符串。覆盖 Windows 盘符的正反斜杠、带空格或中文的文件 URL、转义的 `#` 和 `%`、UNC 路径以及 RTMP。Windows 文件 URL 去掉盘符前斜杠的行为由 Qt 的 Windows 分支处理。 [Qt 路径转换实现](https://github.com/qt/qtbase/blob/v6.12.0/src/corelib/io/qurl.cpp#L1423-L1446)

## 二、native 层

### N-01 — `prePkt` 指向已离开作用域的 `packetOpt`

**P1 · 明确 UB · ASan 实测确认**

位置：[decodec.cpp:103](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:103)、[decodec.cpp:125](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:125)。

`packetOpt` 是 `else` 代码块内的局部 `std::optional<Packet>`。结构化绑定得到的 `pkt` 就是该 optional 里面的成员，随后 `prePkt = &pkt`。离开 `else` 块时，optional 及其中的 `Packet` 生命周期结束；后面的 flush 判定、`avcodec_send_packet()`、`av_packet_unref()` 仍通过 `prePkt` 访问它。

这个缺陷不依赖罕见 EAGAIN 分支：正常从队列拿包后就会发生。`AVPacket` 是普通结构体、栈内字节暂时还在，都不能延长其生命周期。

验证：向实际 `Decodec` 的队列压入 PCM 包，再调用 `decodeFrame()`，ASan 在第 125 行报告：

```text
ERROR: AddressSanitizer: stack-use-after-scope
Decodec::decodeFrame(AVFrame*) .../decodec.cpp:125
'packetOpt' (line 109) <== Memory access ... is inside this variable
```

影响：正常音视频解码都存在 UB，可能表现为随机解码错误、栈数据污染或崩溃。

### N-02 — planar 音频的平面指针数组长度不足

**P1 · 数组越界 / UB · 输出路径实测确认**

位置：[audioEncoder.h:34](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/include/FFmpegRecorder/audioEncoder.h:34)、[audioEncoder.cpp:44](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/audioEncoder.cpp:44)、[audioEncoder.cpp:70](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/audioEncoder.cpp:70)。

编码器固定把输出转换成 `AV_SAMPLE_FMT_FLTP`，但 `converted_` 只有一个指针。planar 格式每个声道都有独立平面，双声道调用 `av_samples_alloc()` 时会写入 `converted_[0]` 和 `converted_[1]`；后者已经越界。

随仓库附带的 FFmpeg 实现直接证明了这个访问：[samplefmt.c:163](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/third/src/ffmpeg/libavutil/samplefmt.c:163)。代码按声道数写 `audio_data[ch]`。

验证：调用实际 `AudioEncoder::addStream()`，输入参数为双声道 S16。`converted_` 的数组大小是 8 字节，而所需指针空间为 16 字节；相邻的 `convertedSampleCount_` 从应有的 1024 变成了 1161705728。这是一次运行中的地址相关结果，不是稳定的错误数值。

输入侧还存在同类问题：`inBuf[1]` 也只提供一个指针，而公开 `AudioParam` 的默认采样格式是双声道 FLTP。按默认参数传入 planar 数据时，`swr_convert()` 会按多个输入平面读取这个只有一个元素的数组。Qt 当前传 packed 格式不会触发输入侧这一分支，但不能避免输出侧越界。

影响：默认双声道录制即可破坏对象内存；后续音频转换、FIFO 操作和析构都可能异常。

### N-03 — FFmpeg 对象池没有同步，却实际跨多个线程使用

**P1 · 数据竞争 / UB · 静态线程调用链确认**

位置：[FFmpegPool.hpp:55](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/include/common/FFmpegPool.hpp:55)、[encoder.cpp:107](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/encoder.cpp:107)、[videoEncoder.cpp:80](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/videoEncoder.cpp:80)、[muxer.cpp:118](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/muxer.cpp:118)。

池内部是无锁保护的 `std::vector<T*>`。当前代码并没有把所有访问限制在同一线程：

| 对象池 | 一个访问方 | 并发访问方 |
|---|---|---|
| 视频编码器 `framePool_` | Qt 视频工作线程在 `commitVideoBuffer()` 中 `get()` | 视频编码线程在 `encodePacket()` 中 `put()` |
| 音频编码器 `framePool_` | Qt 音频工作线程在 `pushData()` 中 `get()` | 音频编码线程在 `encodePacket()` 中 `put()` |
| muxer `pktPool_` | 两个编码线程调用 `Muxer::handle()` 并 `get()` | muxer 线程写完包后 `put()` |

`empty/back/pop_back/push_back` 之间没有共同互斥保护。帧队列本身线程安全，并不能同步旁边的对象池。

影响：vector 元数据和元素发生数据竞争，属于 C++ UB，可能造成重复取出同一个对象、对象丢失、堆损坏或崩溃。这里记录的是已经存在的并发调用路径，不是对“对象池应如何设计”的评价。

### N-04 — 工作线程在其依赖的成员销毁之后才被 join

**P1 · 生命周期 / 潜在 use-after-free · 静态确认，线程交错触发**

位置：[dreamThread.hpp:15](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/include/common/dreamThread.hpp:15)、[dreamThread.hpp:32](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/include/common/dreamThread.hpp:32)、[FFmpegRecorder.cpp:45](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/FFmpegRecorder.cpp:45)、[decodec.h:26](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/include/FFmpegPlayer/decodec.h:26)、[FFmpegPlayer.cpp:33](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:33)。

`DreamThread::exit()` 只 `request_stop()`，没有等待线程结束。真正的自动 join 在基类里的 `std::jthread` 成员析构时发生；但派生类析构函数和派生类成员的销毁发生在这之前。

现有调用链中有多处直接受影响：

- 录制 stop 在 `exit()/close()` 后立即置空 encoder 指针。`AudioEncoder` 的 FIFO、转换缓冲区以及 `Encoder` 的对象池、互斥量、帧队列可能先被销毁，编码线程仍在使用它们。
- muxer 指针同样立即销毁，其队列和包池先于线程 join 被销毁。
- 播放器销毁时先删除音视频 decoder；decoder 会释放 codec 等成员，最后才轮到各自基类线程 join。播放器自己的监控线程还要等 decoder、demuxer 成员销毁后才 join。
- `FFmpegPlayer` 析构体只关闭队列，甚至没有主动停止自己的监控线程。

影响：停止录制、关闭播放器、释放 native 对象时可能崩溃、死锁或访问已经结束生命周期的对象。停止请求已经发出，不代表工作线程已经退出。

### N-05 — 单流媒体仍启动不存在的解码器，倍速也无条件操作音频滤镜

**P1 · 空指针 / UB · 对应实现路径实测**

位置：[FFmpegPlayer.cpp:59](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:59)、[FFmpegPlayer.cpp:69](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:69)、[videoDecodec.cpp:95](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:95)、[audioDecodec.cpp:145](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:145)。

`open()` 只给实际存在的流调用 `openCodec()`，但之后无条件启动音频和视频两个解码线程。

- 纯音频文件：视频 decoder 的 `stream_ == nullptr`，`VideoDecodec::run()` 读取 `stream_->time_base`。
- 无音轨视频：音频 decoder 的 `codecCtx_ == nullptr`，`AudioDecodec::run()` 进入 `avcodec_receive_frame(nullptr, ...)`。
- 没有音频解码器时改变倍速：`audioSpeedFilter_` 没有创建，`AudioDecodec::setSpeed()` 仍调用它的 `addFrame()`。

验证：分别直接运行与缺流状态一致的未打开 decoder，前两条路径分别得到 UBSan 的空 `AVStream` 成员访问和 `avcodec_receive_frame` 空上下文崩溃；对尚无音频滤镜的播放器设置 1.5 倍速，UBSan 指向 `audioDecodec.cpp:145` 的空对象调用。

这些是针对原实现入口的局部验证，没有声称完成了纯音频/纯视频 GUI 端到端测试。

### N-06 — 播放控制和工作线程之间存在多组未同步共享状态

**P1 · 数据竞争 / UB · 静态确认**

位置：[packetQueue.cpp:13](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/packetQueue.cpp:13)、[decodec.h:56](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/include/FFmpegPlayer/decodec.h:56)、[FFmpegPlayer.cpp:169](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:169)、[mediaClock.h:25](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/include/FFmpegPlayer/mediaClock.h:25)。

以下成员使用普通标量或 `std::function`，读写两侧没有共同锁或原子操作：

| 共享成员 | 写入线程 | 读取线程 / 操作 |
|---|---|---|
| `PacketQueue::serial_` | demux 线程 seek 时递增 | 解码线程、帧消费线程、时钟读取方解引用返回的指针 |
| `Decodec::isPause_` | GUI 线程 play/pause | 音频拉取线程、视频渲染线程 |
| `finishedSerial_` | 解码线程 | 监控线程检查 EOF |
| `decodecSerial_` | 解码线程 | 视频消费线程判断丢帧代次 |
| `Demuxer::isEof_` | demux 线程 | 监控线程 |
| `playbackCompleted_` | 监控线程、demux 线程 | GUI 线程读进度，以及彼此读取 |
| `Clock::speed / paused`、`FFInstance::syncType_` | GUI 或监控线程 | 渲染、音频和其他时钟调用线程 |
| seek / completion 回调 | GUI 在线程已经启动后赋值 | 解码或监控线程判断并调用 |
| 音频 `decodecTimer_` | 音频解码线程和 Qt 拉取线程 | 倍速尾帧提交等路径 |

`getSerial()` 在“返回地址”时加锁，不会保护锁释放后对该地址的读取；对普通 bool 的读写也不是线程同步。`pts` 等少数时钟字段使用 atomic，不能顺带保护同一个结构里的其他字段。

影响：这些都是实际多线程播放路径上的数据竞争，属于 C++ UB。表现可以是偶现状态错乱、暂停失效、完成通知异常，也可能因回调对象并发修改而崩溃。这里未运行 ThreadSanitizer，不把具体竞争时序写成已动态复现。

### N-07 — 第一次播放的完成序号为 0，永远不被认作完成

**P2 · 完成判定错误 · 状态判定实测**

位置：[packetQueue.h:48](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/include/FFmpegPlayer/packetQueue.h:48)、[decodec.cpp:95](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:95)、[decodec.cpp:188](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:188)。

包队列的初始 serial 是 0，只有 seek 的 flush 包才会递增它。正常首次播放的解码代次因此就是 0。到 EOF 时 `finishedSerial_ = decodecSerial_`，结果还是 0；但 `isDecodeFinished()` 又把 0 无条件当成“还没结束”。

验证：保持初始队列序号，把 `finishedSerial_` 设成与当前序号一致，得到 `serial=0, finished=0, isDecodeFinished=0`。这正是没有 seek 的首轮 EOF 状态。

影响：媒体不经 seek 播放到末尾时，监控线程无法通过该流的完成判定，完成回调及相关收尾不会正常执行。

### N-08 — 暂停只抑制音频返回标记，却持续消费并丢弃音频

**P1 · 音频数据丢失 · 静态确认，取决于暂停时的拉取位置**

位置：[audioDecodec.cpp:114](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:114)、[audioBufferDevice.cpp:52](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:52)、[audioBufferDevice.cpp:92](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:92)。

`AudioDecodec::getAudioFrame()` 先从帧队列取出数据，执行 `fillData()`，最后仅在 `!isPause_` 时把 `isUpdated` 设为 true。不管是否暂停，该帧最终都被释放。Qt 拉取循环收到 false 后继续取下一帧。`cvPause_` 的等待位于有效帧分支，这些无效帧会跳过暂停等待并继续取帧，见 QT-05。

触发方式：播放含音轨媒体，暂停一段时间，再继续播放。

若暂停时拉取线程没有被一份尚未入队的旧音频帧挡在 Qt 容量等待处，它会进入上述持续取帧、丢帧路径。恢复后可能出现音频跳跃、静音或音画严重错位；较长暂停甚至可以把剩余音轨消耗到末尾。Qt 队列恰好已满时可能暂时挡住拉取，所以不能断言每次暂停都产生相同数量的丢帧。

### N-09 — 暂停和恢复时钟时没有处理经过的系统时间

**P2 · 时间跳变 · 当前 `MediaClock` 实测**

位置：[mediaClock.cpp:21](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/mediaClock.cpp:21)、[mediaClock.cpp:117](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/mediaClock.cpp:117)、[mediaClock.cpp:133](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/mediaClock.cpp:133)。

运行中的时间由 `pts + (now - lastUpdatedTime) * speed` 得到。暂停只把 `paused` 设为 true，返回值变为旧 `pts`，没有保存暂停瞬间已经走过的时间；恢复只把 paused 清掉，`lastUpdatedTime` 仍是暂停前的值。

验证：时钟设为 10 秒，立即暂停，等待约 150 ms 再恢复，得到 `paused=10, resumed=10.1502`。暂停的时间被计入了恢复后的进度。如果从最后一次 setTime 到按暂停之间已有时间差，暂停瞬间还会先退回旧 `pts`。

影响：暂停/继续可使进度跳变，并扰乱视频延迟计算和音画同步。其他线程若恰好重设时钟，可能暂时掩盖这个问题。

### N-10 — 音频 PTS 使用了错误的时间基

**P1 · 时间戳错误 · 静态确认，特定容器触发**

位置：[decodec.cpp:92](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:92)、[audioDecodec.cpp:98](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:98)、[audioDecodec.cpp:370](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:370)。

公共解码函数把 `frame->pts` 替换为 `best_effort_timestamp`；该时间戳以输入流时间基表示，随仓库的 [frame.h:559](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/third/src/ffmpeg/libavutil/frame.h:559) 明确注明了这一点。但音频线程直接计算 `frame->pts / frame->sample_rate`，等价于假定流时间基一定是 `1 / sample_rate`。重采样转换里的 `av_rescale_q()` 也作出了同一个假定。

例如音频流时间基为 `1/1000`、采样率为 48000 时，表示 1 秒的 PTS 1000 被换算成约 0.02083 秒。

触发条件：音频流时间基与采样率倒数不一致，常见于使用毫秒时间基的封装场景。影响：音频时钟量级错误，进度、seek 后同步、音画追赶都可能异常。时间基恰好是 `1/sample_rate` 的文件不暴露这个问题。

### N-11 — `AudioFrame::pts` 标成了帧末尾，Qt 又重复加了一次帧内偏移

**P2 · 同步偏移 · 静态调用链确认**

位置：[audioDecodec.cpp:123](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:123)、[audioBufferDevice.cpp:74](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:74)、[audioBufferDevice.cpp:152](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/audioBufferDevice.cpp:152)。

native 取到音频帧后，把 `ap.pts + ap.duration` 赋给 `audioFrame_.pts`。Qt 却把这个值存为音频段的起始 PTS，再计算 `segment.pts + 已读取字节对应的时长`。

例如 44100 Hz 下 1024 个采样/声道的帧，时长约 23.22 ms。帧实际从 10 秒开始时，段起点已经被标成约 10.02322 秒；读完整段后，又推进到约 10.04644 秒，重复计算了一帧时长。

影响：回传给 native 的音频时钟偏前，且偏移随音频块大小和倍速变化，视频同步会据此做错误的延迟或丢帧判断。这里讨论的是软件时间戳自身的重复累计，未把音频硬件延迟混在一起。

### N-12 — 采样补偿算出的差值恒为 0，部分正常音频直接被拒绝

**P1 · 音频失效 · 当前实现局部实测**

位置：[audioDecodec.cpp:217](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:217)。

`wantedNbSamples` 初始化为 0，只有在已积累足够测量且偏差超过阈值时才更新。其他正常分支里它仍为 0，于是对任何非空帧，`wantedNbSamples != frame->nb_samples` 都成立。

更直接的算术错误是后面计算 `sampleDelta` 时使用 `nbSamples - frame->nb_samples`；`nbSamples` 恰好就是本函数开头保存的 `frame->nb_samples`，所以差值恒为 0，没有使用前面计算出的目标样本数。

对本来就已经是 44100 Hz / 双声道 S16 的输入，`convertFrame()` 可以直接返回引用，不创建 `swrCtx_`。VideoMaster 模式仍进入补偿分支，`swr_set_compensation(nullptr, ...)` 返回错误，`fillData()` 因而返回 false，当前音频帧被丢弃。

验证：给原 `fillData()` 传入上述正常格式的 1024 采样帧，设置 VideoMaster，实际得到 `fillData=0, swrCtx=0` 和 `swr_set_compensation() failed`。FFmpeg 对空上下文返回错误；本文不把这里误写成该函数必然空指针崩溃。

### N-13 — 分辨率变化后，转换上下文和部分 stride 仍对应旧尺寸

**P1 · 条件性错误结果 / 越界访问 · 静态确认**

位置：[videoDecodec.cpp:21](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:21)、[videoDecodec.cpp:253](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:253)。

非 YUV420P 帧只有在 `frame->format != cachedFormat_` 时才更新 `SwsContext`。如果像素格式相同、分辨率发生变化，当前帧的输出缓冲区按新尺寸分配，转换上下文却仍持有旧的源/目标尺寸。

结果可能是转换返回错误但仍被当作成功帧，或旧转换尺寸大于新缓冲区可容纳的范围，产生越界读写。

另一个同场景问题在 `updateVideoFrameSize()`：只要 `linesize × planeHeight` 总字节数没变，就提前 continue，连新的 stride 也不记录。相同容量不等于相同的行布局，后续 Qt 上传纹理会按旧 stride 读行。

触发条件：媒体流中途切换分辨率，尤其同像素格式下切换。固定尺寸视频不触发此项；本次没有使用动态分辨率媒体做端到端复现。

### N-14 — 奇数高度 YUV420 的色度平面容量不足

**P1 · 越界写 / UB · 容量计算与原转换路径实测**

位置：[videoDecodec.cpp:22](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:22)、[videoDecodec.cpp:271](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:271)、[videoArea.cpp:318](/run/media/Workplace/CPPWorkplace/DreamMediaHub/player/src/videoComp/videoArea.cpp:318)。

YUV420 对奇数高度需要向上取整的色度行数，而当前分配使用 `height / 2`。例如高为 3 的图像，U/V 各需 2 行，代码只给 1 行。非 YUV420P 输入经 `sws_scale()` 转换时，转换器仍会生成完整色度数据，超出分配范围。

验证：把 64×3 RGB24 帧送入原 `VideoDecodec::fillData()`，观测到色度平面分配 32 字节，而实际应容纳 64 字节。该转换后的析构还出现了 ASan invalid-free 报告；此处能确定的根因证据是缓冲区容量不足，不把未插桩的 FFmpeg 库内每一次越界写都声称为 ASan 已逐条定位。

原生 YUV420P 的直接复制分支以及 Qt 的纹理尺寸计算也使用向下取整，会漏掉最后色度行/列。奇数尺寸本身是可表示的输入，不能由这一段代码直接假设为偶数。

### N-15 — 未知时长经过浮点转换后再转 `uint32_t` 会产生 UB

**P1 · 浮点转整数越界 / UB · 对应表达式 UBSan 实测**

位置：[demuxer.cpp:40](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/demuxer.cpp:40)。

未知时长可以用 `AV_NOPTS_VALUE` 表示，值为 `INT64_MIN`。代码先把它转成 double 并除以 `AV_TIME_BASE`，得到约 `-9.22337e12`，再转成 `uint32_t`。这个浮点数的截断结果超出目标类型的可表示范围，属于 UB；不能用“整数转 unsigned 会取模”来解释这条浮点转换链。

触发条件：直播、部分原始流或其他没有有效 duration 的输入。影响：无定义的时长、巨大进度范围、seek 边界异常，或 sanitizer 直接报错。

验证：单独执行与第 40 行完全相同的转换表达式，UBSan 报告 `is outside the range of representable values of type 'unsigned int'`。这是表达式级验证，没有声称用直播输入复现。

### N-16 — 再次 open 并没有建立一套对应新文件的有效播放状态

**P1 · 重开错误 / 条件性线程冲突 · 静态确认**

位置：[FFmpegPlayer.cpp:37](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:37)、[demuxer.cpp:22](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/demuxer.cpp:22)、[decodec.cpp:69](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:69)。

`FFmpegPlayer::open()` 开头的 `shutdown()` 只关闭队列，没有先结束并等待旧 demux/decode 线程，也没有清理旧 codec。`Demuxer::open()` 把新流追加到 `streams_`，没有清掉旧流列表；已有 decoder 的 `isOpen()` 又仍为 true，遍历新流时直接跳过 `openCodec()`。

因此第二次 open 可以继续使用第一个文件的 codec 参数、`stream_` 和流索引。与此同时，旧线程还可能在读旧格式上下文时，主线程已经重新打开该上下文或重新开放队列，之后的 `start()` 才开始停止/替换旧线程。

触发方式：在同一个 native 播放器对象上打开 A，再打开格式或流布局不同的 B；即使先调用当前 `stop()`，也没有等待旧线程退出或把 decoder 的已打开状态重置。

影响：错误解码、旧流状态污染，以及并发访问格式上下文的 UB。当前 GUI 没有文件选择器不改变公开 `open()` 路径中的这个实现错误。

### N-17 — VideoMaster 模式的图像播放间隔没有随倍速变化

**P2 · 倍速错误 · 静态确认，VideoMaster 条件触发**

位置：[videoDecodec.cpp:208](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:208)、[videoDecodec.cpp:159](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:159)、[FFmpegPlayer.cpp:141](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:141)。

`calcDelay()` 在 VideoMaster 模式直接返回媒体帧时长。后面的 `decodecTimer_` 却用系统时间比较，并按这个未缩放时长逐帧推进，没有应用当前 speed。

例如 25 fps 视频的帧间媒体时长为 40 ms。设置 2 倍速后，VideoMaster 路径仍每约 40 ms 显示下一帧，而时钟的 speed 已经被设成 2，调度和时钟互不一致。

触发条件：通过公开接口切换成 VideoMaster 后调倍速。这里与 N-05 的“无音频滤镜直接崩溃”是不同问题：即便音视频 decoder 都有效，VideoMaster 的视频调度也有这个错误。

### N-18 — seek 后残留的滤镜和重采样状态会进入新代次音频

**P2 · seek 数据污染 · 静态确认，存在内部缓存时触发**

位置：[decodec.cpp:125](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:125)、[audioDecodec.cpp:274](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:274)、[audioDecodec.cpp:139](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:139)。

seek 的 flush 包只触发 `avcodec_flush_buffers()` 和外部 seek 回调。`AudioSpeedFilter`、`SwrContext` 的历史采样以及倍速切换留下的 `paddingFrames_` 没有随这个代次变化清空。

新位置的数据继续送入旧滤镜时，滤镜中旧位置的残留样本仍可输出；这些输出在 `pushFrame()` 时又被标记成当前的新 serial，后面的 serial 过滤无法识别其实际来源。

触发条件：音频重采样或 atempo 内存在历史数据时 seek，尤其非 1 倍速播放。影响：seek 后出现旧位置的短促声音、混入旧采样或短时音画不同步。Qt 清空外部音频队列不能清除 native 内部的这些缓存。

### N-19 — 音频解码到 EOF 后没有让滤镜和重采样器输出尾部

**P2 · 尾音丢失 · 静态确认，存在延迟样本时触发**

位置：[audioDecodec.cpp:90](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:90)、[audioDecodec.cpp:305](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/audioDecodec.cpp:305)。

`decodeFrame()` 返回 false 后，音频线程只 continue，不把 EOF 传给 `AudioSpeedFilter`，也不获取重采样器中剩余的延迟样本。正常收帧阶段遇到 EAGAIN 就结束本轮，因此留在 atempo 内部的尾部不会自行变成外部帧。

随仓库的 atempo 在收到输入 EOF 后有专门的尾部输出逻辑：[af_atempo.c:1125](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/third/src/ffmpeg/libavfilter/af_atempo.c:1125)。但当前播放结束路径没有向输入端发送这个 EOF。`addFrame(nullptr)` 只出现在切换倍速的另一条路径。

影响：音轨最后一小段数据被截掉，变速或重采样时更容易暴露。它与录制 stop 的未排空问题 N-21 属于不同的数据处理链路。

### N-20 — 编码器初始化失败仅从基函数返回，派生函数仍继续解引用

**P1 · 空指针 / UB · 静态确认，初始化失败触发**

位置：[encoder.cpp:15](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/encoder.cpp:15)、[videoEncoder.cpp:24](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/videoEncoder.cpp:24)、[audioEncoder.cpp:32](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/audioEncoder.cpp:32)。

`Encoder::addStream()` 遇到找不到编码器、`avcodec_open2()` 失败等情况，只在这个 void 函数里 return。派生类调用基函数后，仍立即写 `stream_->time_base`，音频路径还继续访问 codec、初始化 swr。失败发生在创建 stream 之前时，`stream_` 仍是空指针。

触发条件并不局限于内存耗尽：FFmpeg 没编入 H.264 编码器、摄像头没有有效尺寸、编码器不接受当前宽高或采样参数，都可能进入这些已有错误分支。

影响：录制启动失败后继续操作无效对象并崩溃。上层 `FFmpegRecorder::start()` 还会继续启动线程，错误状态进一步扩散。

### N-21 — 停止录制会直接退出循环，没有完成帧、编码器和包队列排空

**P1 · 录制数据丢失 · 静态确认**

位置：[FFmpegRecorder.cpp:45](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/FFmpegRecorder.cpp:45)、[encoder.cpp:51](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/encoder.cpp:51)、[audioEncoder.cpp:82](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/audioEncoder.cpp:82)、[muxer.cpp:79](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/muxer.cpp:79)。

stop 先设置停止请求，encoder 和 muxer 的主循环都以 `!st.stop_requested()` 为条件，因而可以在输入帧队列、编码器内部或待写包队列仍有数据时直接离开。代码没有向编码器发送用于排空的空帧，也没有继续收取所有延迟包；muxer 随后就写 trailer。

音频 FIFO 还有一个具体边界：只有 `fifo_size > frameSize` 时才提交帧，恰好凑满一帧时仍留在 FIFO；结束时这整帧连同不足一帧的余量一起被丢弃。编码器自身的延迟数据也不包含在这段 FIFO 里。

触发方式：正常点“停止录制”，尤其短录制、编码有延迟或停止时队列还有积压的情况。

影响：尾部视频/音频缺失、短片可能只有很少甚至没有有效视频包、两路尾部长度不一致。此项独立于 N-04：即使线程销毁没有碰巧崩溃，直接按停止标记跳出循环仍会丢数据。

### N-22 — native 录制上下文和部分在途帧未释放

**P2 · 资源泄漏 · 静态所有权确认**

位置：[encoder.cpp:23](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/encoder.cpp:23)、[encoder.h:29](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/include/FFmpegRecorder/encoder.h:29)、[muxer.h:21](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/include/FFmpegRecorder/muxer.h:21)、[muxer.cpp:63](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamRecorder/src/FFmpegRecorder/muxer.cpp:63)、[decodec.cpp:157](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/decodec.cpp:157)。

能直接从当前释放路径确认的泄漏包括：

- `Encoder::addStream()` 分配 `AVCodecContext`，但 `Encoder`、音视频派生类的析构均不释放它，也没有别处接管该上下文。
- `Muxer` 析构为空；`cleanup()` 虽然能释放 `AVFormatContext`，但只在同一 muxer 的 init 开头调用。上层每次 start 都新建 muxer，stop 则直接销毁它；线程结束仅关了 `pb`，没有释放 format context。
- `Decodec::pushFrame()` 取得一份 `AVFrame` 引用后，不检查 `frameQue_.push()` 是否因关闭而失败，失败时这份引用没有释放。
- decoder 帧队列和 encoder 帧队列中的裸帧指针，不能靠销毁 deque 自动释放所指向的帧；视频 `heldFrame_` 在正常析构体中也没有对应的 `releaseFrame()`。

触发条件：反复开始/停止录制、关闭仍有缓冲帧的播放器，或队列关闭恰好发生在提交帧期间。影响：内存和 FFmpeg 内部资源随使用次数累积。本文未把对象池有意保留可复用对象当成泄漏。

### N-23 — FFmpeg 已成功生成错误文本，返回值却还是空字符串

**P2 · 诊断错误 · 当前实现实测**

位置：[ffcore.cpp:8](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/ffcore.cpp:8)。

`av_strerror()` 成功时会把错误文本写入 `buf`，但代码只在它失败时才给 `ret` 赋值。正常成功分支最终返回默认的空 `std::string`。

验证：调用 `FFUtils::getError(AVERROR(ENOENT))`，实际输出 `error=[]`。

影响：媒体打开失败等日志虽然打印了“error message”，内容却为空，丢失原本已经成功取得的错误原因。这是确定的返回值错误，不涉及日志风格或设计偏好。

### N-24 — `sws_scale` 接收到的平面指针和 stride 数组只有 3 项

**P2 · 越界读 / UB · 依赖实现直接确认**

位置：[structs.h:27](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/include/DreamPlayer/structs.h:27)、[videoDecodec.cpp:272](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/videoDecodec.cpp:272)、[swscale.c:765](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/third/src/ffmpeg/libswscale/swscale.c:765)。

公开 `VideoFrame` 里的 `data`、`stride` 都是长度为 3 的数组。转换时直接把它们传给 `sws_scale()`。随仓库的 FFmpeg 在进入转换时，无条件复制 4 个 stride；随后又复制 4 个平面指针：[swscale.c:814](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/third/src/ffmpeg/libswscale/swscale.c:814)。

因此即使目标像素格式只实际使用三个平面，库也已经读取了数组的第 4 项。对应的字节可能落在结构体的其他成员或填充里，但这不使越过原数组边界的访问合法。

触发条件：输入视频不是 YUV420P，进入 `sws_scale()` 转换分支。影响：明确的越界读取；当前 YUV420P 目标通常不使用第 4 平面的值，所以未必立刻出现可见崩溃。此项与奇数高度导致的实际容量不足 N-14 分开记录。

### N-25 — `isPlaying()` 返回值与实际播放状态相反

**P2 · 状态反转 · 静态确认**

位置：[FFmpegPlayer.cpp:91](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/FFmpegPlayer.cpp:91)、[ffinstance.cpp:130](/run/media/Workplace/CPPWorkplace/DreamMediaHub/third/src/DreamMediaHubNative/DreamPlayer/src/FFmpegPlayer/ffinstance.cpp:130)。

`FFmpegPlayer::isPlaying()` 直接返回 `getMasterClockPause()`。该函数返回主时钟的 `paused` 字段，因此 pause 后查询结果是 true，play 后查询结果是 false。

影响：使用公开 native 接口查询或切换播放状态的调用方会得到反向结果。当前 Qt 控制器自己维护 `isPlaying_`，因此界面图标没有直接调用这个错误接口；这不影响 native 返回值本身是一个确定的 bug。

## 三、验证记录及结论边界

2026-11-10 的初查使用仓库自带 FFmpeg 4.3.x 开发包和 Linux Qt，编译和验证程序放在临时目录。2026-11-11 的录制路径探针用替身采集器和录制器核对接口入参。2026-11-12 在 Linux 上重新编译了播放器与录制器。音频复验用替身播放器驱动当前 `AudioBufferDevice`，比对读出的 PCM，并用 ASan 检测重叠拷贝。录音信号复验覆盖 Qt 事件派发和参数生命周期，摄像头限帧采用静态计算与调用链核对。2026-11-13 使用 Linux Qt 6.12 重新构建两款应用和 native 库，用替身复测音频空读、分段、seek 清队列和预缓存，用合成时间戳验证当前限帧表达式。

2026-11-14 在 Linux Qt 6.12 构建了 `playerApp`，并用替身播放器对当前 `AudioBufferDevice` 做了空读和通知、暂停及满队列退出、EOF、seek 和预缓存的局部验证。空指针与重叠拷贝分别由 UBSan 和 ASan 确认。

初查时 native 源码在临时构建目录中成功编译为带 AddressSanitizer / UndefinedBehaviorSanitizer 的库，主要编译参数如下。

```text
-fsanitize=address,undefined
-fsanitize-address-use-after-scope
-fno-omit-frame-pointer
```

表达式验证程序额外启用了 `float-cast-overflow`。仓库已有的 FFmpeg 动态库本身没有重新插桩，因此不把 FFmpeg 库内部所有访存都当成 sanitizer 已完整覆盖。

| 验证内容 | 结果 | 验证范围 |
|---|---|---|
| 解码局部包生命周期 | ASan 在 `decodec.cpp:125` 报 stack-use-after-scope | 调用原解码实现 |
| 双声道 planar 输出指针数组 | 8 字节数组需要 16 字节；相邻采样数成员被覆盖 | 调用原录音编码初始化 |
| 未打开的视频 decoder 执行 run | 空 `AVStream` 成员访问 | 对应缺视频流的局部状态 |
| 未打开的音频 decoder 执行 run | `avcodec_receive_frame` 空上下文崩溃 | 对应缺音频流的局部状态 |
| 没有音频滤镜时设置倍速 | UBSan 报空 `AudioSpeedFilter` 调用 | 原 setSpeed 路径 |
| 初始 serial 的 EOF 判定 | finished 与 serial 同为 0，仍返回未完成 | 注入与首轮 EOF 一致的状态 |
| 暂停/恢复时钟 | 等待约 150 ms 后，恢复读数增加约 150 ms | 原 MediaClock |
| 未知 duration 的转换 | UBSan 报浮点值超出 unsigned int 范围 | 与源码一致的孤立表达式 |
| VideoMaster 正常 S16 帧 | fillData 返回 false，打印补偿失败 | 原音频帧处理入口 |
| 64×3 RGB 转 YUV420 | 色度仅分配 32 字节，需容纳 64 字节；转换后析构报告 invalid-free | 原视频转换入口；未逐条定位库内写操作 |
| Qt 排队传 const char* | 发送后修改原数据，接收端读到修改后的数据 | 同样连接类型及参数类型的独立实验 |
| QML 路径函数 | Windows 目录被识别为非本地；保留 `%20` | 提取并执行当前函数文本 |
| 32768 字节音频帧 | 队列保持为空，拉取停在第一帧的入队等待 | 原 AudioBufferDevice 与替身播放器 |
| FFmpeg 错误文本包装 | 对 ENOENT 返回空字符串 | 原 FFUtils::getError |

#### 2026-11-14 音频复核

| 验证内容 | 结果 |
|---|---|
| 空读、无效帧后接有效帧、暂停恢复 | 空读和暂停读立即返回 0 字节，设备所属线程的 `readyRead()` 交付正确的 4096 字节 |
| EOF 连续返回无效帧 | 50 ms 内约 49 万次取帧，仍会空转 |
| 暂停后持续返回无效帧 | 50 ms 内约 71 万次取帧，未经过暂停等待 |
| 有效帧停在暂停等待时析构 | 2 秒超时，析构未返回 |
| 满队列时请求 stop | 50 ms 内未退出，消费者读走 4096 字节后又入队 4096 字节才退出 |
| 队列剩 4 字节时分段 | ASan 报 `memcpy-param-overlap` |
| 满队列时触发 seek 回调 | UBSan 报空指针成员访问，ASan 报段错误 |
| 取帧跨越 seek 回调 | 旧 `serial=1` 的 4096 字节仍能入队并读出 |
| 100 ms 预缓存 | 成功返回，三帧共入队 24576 字节 |

探针直接编译当前 `audioBufferDevice.cpp`，用替身控制 native 返回顺序。上述空读和通知用接收槽消费 PCM。播放器构建已通过，真实音频设备的 `IdleState → ActiveState` 恢复仍待设备回归。当前环境的 LeakSanitizer 受 ptrace 限制，本轮用 `ASAN_OPTIONS=detect_leaks=0` 运行，保留 ASan 和 UBSan 的访存检查。

这份报告不是一次完整的 GUI、Windows、真实摄像头/麦克风或所有媒体格式回归测试。并发问题由具体线程调用链和缺失的同步关系确认；动态分辨率、seek 残留、排空丢失等条目明确列出了触发条件，未冒充已做端到端复现。第三方 FFmpeg 源码仅用于核对项目调用的实际契约，没有把第三方库内部的潜在问题列入本项目缺陷数量。
