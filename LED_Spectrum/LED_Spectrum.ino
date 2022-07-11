#include <TimeLib.h>
#include <FastLED.h>
#include "arduinoFFT.h"


/********************FFT相关定义**********************************/
#define CHANNEL 4  //选择音频输入IO口序号为4

arduinoFFT FFT = arduinoFFT(); //创建FFT对象

const uint16_t samples = 64; //采样点数，必须为2的整数次幂
const double samplingFrequency = 4000; //Hz, 声音采样频率

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[samples]; //FFT采样输入样本数组
double vImag[samples]; //FFT运算输出数组

//FFT参数，保持默认即可
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03
/********************FFT相关定义*********************************/


/*******************灯板参数定义*********************************/
#define LED_PIN     23  //灯板输入IO口选择
#define NUM_LEDS    64  //灯珠数量
#define BRIGHTNESS  10  //默认背光亮度
#define LED_TYPE    WS2812  //灯珠类型
#define COLOR_ORDER GRB  //色彩顺序

CRGB leds[NUM_LEDS];  //定义LED对象
/*******************灯板参数定义*********************************/


void drawBar(int idx, int16_t value, uint8_t *flag)  //绘制函数，按序号和幅度值绘制条形块
{
  static int16_t volume[8]; //保存下降数据
  constrain(value,0,8); //幅度限制在0-8范围内

  if(volume[idx] < value)  //采集到的数据比之前大则更新，实现上冲效果
    volume[idx] = value;

  if(idx%2){ //余2运算判断序号是否为奇数
    for(int i = 0;i<8-volume[idx];i++) leds[idx*8+i] = CRGB::Black;
  }else{
    for(int i = volume[idx];i<8;i++) leds[idx*8+i] = CRGB::Black;
  }

  if(*flag){
    volume[idx] -= 1;  //达到时间则减小1，表示下落
    if(idx == 7) *flag = 0;  //第0-7列均更新完毕则清除标记
  }
}

void setup(){
  sampling_period_us = round(1000000*(1.0/samplingFrequency)); //计算采样频率
  pinMode(CHANNEL,INPUT); //初始化麦克风接口为输入模式，表示读取麦克风数据
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);//初始化LED灯条
  FastLED.setBrightness( BRIGHTNESS ); //LED亮度设置，取值范围为0-255
  
}

void loop() 
{
  static uint32_t t=0, dt = 70;
  static uint8_t flag=0;

  /*采样*/
  microseconds = micros();
  for(int i=0; i<samples; i++)
  {
      vReal[i] = analogRead(CHANNEL);  //读取模拟值，信号采样
      vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  }

  /*FFT运算*/
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
    
  fill_rainbow((leds), 64/*数量*/, 0/*开始色值*/, 4/*递增值*/); //设置彩虹渐变，先填充满，然后根据取值大小填充黑色，表示熄灭灯
  for(int i = 0; i < 8; i++){  //循环遍历八列LED
    drawBar(i, (vImag[i*3+2]+vImag[i*3+3]+vImag[i*3+4])/3/200, &flag); //选取频谱中取平均后的8个值,传递时间标志到绘制函数
  }
  FastLED.show(); //显示灯条
  
  if((millis()-t) > dt){ //读取时间，判断是否达到掉落时长
      flag = 1;  //达到则标记为1
      t = millis(); //更新时间
  }
}
