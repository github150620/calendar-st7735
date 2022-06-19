# calendar-st7735

## 安装库
* Arduino -> Tools -> Manage Libraries...
* 搜索st7735
* 安装Adafruit ST7735 and ST7789 Library

## 原点偏移问题解决方法
### 第1步，修改库文件
找到Adafrui_ST77xx.h（通常位于Documents/Arduino/libraries/下），把函数setColRowStart()改为公有函数。修改前
```
protected:
...
  void setColRowStart(int8_t col, int8_t row);
...
```
修改后
```
public:
...
  void setColRowStart(int8_t col, int8_t row);
...
```

### 第2步，添加设置原点代码
```
...
  tft.initR(INITR_BLACKTAB);
  tft.setColRowStart(2,1); // 设置原点坐标为(2,1)。一定要在initR()之后，否则无效。
...
```
