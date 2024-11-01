#include "I2S.h"

void I2S_Init(i2s_mode_t MODE, int SAMPLE_RATE, i2s_bits_per_sample_t BPS) {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | MODE),
    .sample_rate = (size_t)SAMPLE_RATE,
    .bits_per_sample = BPS,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,//只获取单声道 左声道
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };
  // i2s_pin_config_t pin_config;
  // pin_config.bck_io_num = PIN_I2S_BCLK;
  // pin_config.ws_io_num = PIN_I2S_LRC;
  // pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  // pin_config.data_in_num = PIN_I2S_DIN;
  // if (MODE == I2S_MODE_RX) {
  //   pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  //   pin_config.data_in_num = PIN_I2S_DIN;
  // }
  // else if (MODE == I2S_MODE_TX) {
  //   pin_config.data_out_num = PIN_I2S_DOUT;
  //   pin_config.data_in_num = I2S_PIN_NO_CHANGE;
  // }
  i2s_pin_config_t pin_config = {
    .bck_io_num = PIN_I2S_BCLK,
    .ws_io_num = PIN_I2S_LRC,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PIN_I2S_DIN
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  //最终设置: 16k, 16位，单声道
   //1.0.3 rc1 以后的版本不要调用下句，否则I2S不可用
  //i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, BPS, I2S_CHANNEL_MONO);
}

int I2S_Read(void *dest, int numData) {
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, dest, (size_t)numData, &bytes_read, portMAX_DELAY);
  return int(bytes_read);
}

void I2S_Write(char* data, int numData) {
  size_t bytes_write = 0;
  i2s_write(I2S_NUM_0, (const char *)data, numData, &bytes_write, portMAX_DELAY);
}

void I2S_uninstall()
{
  i2s_driver_uninstall(I2S_NUM_0);
}