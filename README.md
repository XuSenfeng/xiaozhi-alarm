# 小智Alarm

主项目: [XuSenfeng/ai-chat-local: 使用esp32+ollama实现本地模型的对话以及联网+工具调用](https://github.com/XuSenfeng/ai-chat-local)

在小智AI上面进行的改版, 添加了一个可以语音控制的闹钟功能

> 测试阶段!!! 只实现基础功能, 稳定性待定 !!! 不要用于重要信息的提醒
>
> 只在c3的板子实现, 其他板子没有, 可以自己在对应的board文件里面添加iot模块即可

## 实现

### 闹钟部分

![image-20250313095223239](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503130952377.png)

首先闹钟部分最重要的是到时间以后进行提醒, 我这里使用的实现方式是计算所有的时钟里面会最先响应的那一个, 然后使用esp32的时钟在一定时间以后进入回调函数, 回调函数中显示一下注册时候的时钟的名字, 同时开始播放本地的音频(目前是那个6的语音)

同时考虑到这个闹钟可能在运行到一半的时候断电, 所以需要在再次开机的时候可以恢复运行, esp32的nvs可以实现这个功能, 小智的代码里面把他抽象为Setting这个组件, 所以在时钟开启、插入、删除的时候会同步更新一下nvs, 这样可以在下一次上电的时候获取到之前的时钟运行情况

使用这种方式的时候时间就不可以是相对时间, 所以我使用最新版的小智获取的时间作为基准时间, 加上时间差值作为每一个闹钟的响应时间, 由于有的模型不可以获取时间, 所以实际控制的时候告诉他相对时间的效果会更好(示例: 十分钟后提醒我吃饭)

### 语音控制

使用小智的iot部分代码, 添加一个thing模块

## 自定义音频

不建议使用比较大的音频文件!!!, esp32的性能还是比较差的

我的示例音频是[Free 闹钟铃声 Sound Effects Download - Pixabay](https://pixabay.com/zh/sound-effects/search/闹钟铃声/)网站的

![image-20250302150627639](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503021506028.png)

### 音频处理

下载的音频需要转为opus格式, 可以使用虾哥的脚本进行转换, 但是原本的脚本有特定的分辨率限制, 我这里做了一点改动, 增加了分辨率转换, 使它可以支持所有的分辨率

![image-20250302150838754](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503021508841.png)

使用的时候需要安装对应的库

```bash
conda create -n xiaozhi-alarm python=3.10
conda activate xiaozhi-alarm
pip install librosa opuslib tqdm numpy
```

编码

```bash
python convert_audio_to_p3.py ringtone-249206.mp3 alarm_ring.p3
```

获取到的文件放在

![image-20250302151157686](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503021511760.png)

之后再lang_config.h文件里面添加你的配置(也可以使用脚本gen_lang.py)

```python
extern const char p3_alarm_ring_start[] asm("_binary_alarm_ring_p3_start");
extern const char p3_alarm_ring_end[] asm("_binary_alarm_ring_p3_end");
static const std::string_view P3_ALARM_RING {
static_cast<const char*>(p3_alarm_ring_start),
static_cast<size_t>(p3_alarm_ring_end - p3_alarm_ring_start)
};
```

![image-20250302151902508](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503021519584.png)

最后改一下实际播放的音频即可

![image-20250302152058165](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503021520348.png)

## 驱动移植

### 触摸屏移植

使用`espressif/esp_lcd_touch_ft5x06: "~1.0.7"  # 触摸屏驱动`这个驱动, 嘉立创默认的驱动程序, 但是原本的驱动使用的i2c版本比较低, 所以在初始化的时候使用`esp_lcd_new_panel_io_i2c_v2`进行初始化

### 照相机移植

同样是i2c驱动的问题, 默认的camera在5.3.2的时候使用的还是老版的i2c驱动, 把他的CMake文件中的5.4改成5.3, 之后去除找不到的一个头文件即可正常使用(我记得好像是一个`i2c_platform`之类的文件, 没有使用到)

![image-20250329103455270](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503291034445.png)

### 代码逻辑

目前做的功能比较简陋, 只是把驱动最简单的使用起来的, 之后会进一步开发

### 姿态检测

目前使用的检测比较简单, 在小智本身的时钟驱动上边每秒一次进行检测, 如果检测到角度大于一定阈值, 发送一段信息给服务器

### 照相机

照相机服务非常占用内存, 所以只在拍照的时候启用一下, 由于图片的格式不适合使用iot原本的格式进行传输, 所以这里使用esp32作为一个http server, 在有访问请求的时候加载照相机返回一帧数据

服务器这部分使用插件的形式, 判断出来使用照相机的请求的时候首先访问开发板的http服务器获取当前的图片, 之后使用coze的图片接口进行图片的识别

![image-20250329104825927](https://picture-01-1316374204.cos.ap-beijing.myqcloud.com/lenovo-picture/202503291048116.png)

> 插件文件放在 /server-plugins下边
