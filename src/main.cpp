#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <arduinoFFT.h>
#include "driver/i2s.h"
#include <U8g2_for_Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 25
#define I2C_SCL 21
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

#define TEXT_AREA_WIDTH 32
#define VISUALIZER_START_X TEXT_AREA_WIDTH
#define VISUALIZER_WIDTH (SCREEN_WIDTH - TEXT_AREA_WIDTH)
#define SCROLL_SPEED 2 

#define I2S_MIC_CLK  GPIO_NUM_33
#define I2S_MIC_DATA GPIO_NUM_23
#define I2S_PORT     I2S_NUM_0

#define SAMPLES 512
#define SAMPLING_FREQUENCY 16000
double vReal[SAMPLES];
double vImag[SAMPLES];
int16_t samples_read[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

#define NUM_BANDS 16
#define NOISE_FLOOR 900
#define MAX_MAGNITUDE 35000
#define WAVEFORM_SENSITIVITY 8000

const char* japanesePhrases[] = {
  "オーディオスペクトラム",     // Espectro de audio
  "音楽は力",                  // La música es poder
  "M5Stack Atom",             // M5Stack Atom
  "こんにちは世界",             // Hola, mundo
  "音の波形",                  // Forma de onda del sonido
  "マイクロフォン入力",         // Entrada de micrófono
  "お前はイケてないよ、兄弟",     // You are not a vibe, bro
  "俺は純粋なパフォーマンスだ"    // I am pure performance
  "ケツを食べる"
};
const int numPhrases = sizeof(japanesePhrases) / sizeof(japanesePhrases[0]);
int currentPhraseIndex = -1;
const char* currentScrollText;
int text_width = 0;
int scroll_x_pos = 0;

void selectNextRandomPhrase() {
  int newIndex = random(numPhrases);
  while (newIndex == currentPhraseIndex) {
    newIndex = random(numPhrases);
  }
  
  currentPhraseIndex = newIndex;
  String tempText = String("") + japanesePhrases[currentPhraseIndex];
  currentScrollText = tempText.c_str();

  text_width = u8g2_for_adafruit_gfx.getUTF8Width(currentScrollText);
  text_width -= 10; // Añadir un pequeño margen para evitar corte abrupto
  scroll_x_pos = TEXT_AREA_WIDTH;
}

void setupI2S() {
   i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate = SAMPLING_FREQUENCY,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_PIN_NO_CHANGE,
      .ws_io_num = I2S_MIC_CLK,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_MIC_DATA
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_set_clk(I2S_PORT, SAMPLING_FREQUENCY, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error al iniciar pantalla OLED"));
    while (1);
  }
  
  u8g2_for_adafruit_gfx.begin(display); 
  u8g2_for_adafruit_gfx.setFont(u8g2_font_wqy12_t_gb2312);
  randomSeed(analogRead(32)); 

  currentPhraseIndex = random(numPhrases);
  currentScrollText = japanesePhrases[currentPhraseIndex];
  scroll_x_pos = 0;
  text_width = u8g2_for_adafruit_gfx.getUTF8Width(currentScrollText)-10;

  display.clearDisplay();
  
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(scroll_x_pos, 30); 
  u8g2_for_adafruit_gfx.print(currentScrollText);

  display.fillRect(VISUALIZER_START_X, 0, VISUALIZER_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);

  display.drawRoundRect(VISUALIZER_START_X, 0, VISUALIZER_WIDTH, SCREEN_HEIGHT, 8, SSD1306_WHITE);
  
  display.display();

  setupI2S();
}

void loop() {
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, &samples_read, sizeof(samples_read), &bytes_read, portMAX_DELAY);
  
  if (bytes_read > 0) {
    for (int i = 0; i < SAMPLES; i++) {
      vReal[i] = (double)samples_read[i];
      vImag[i] = 0.0;
    }
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();

    display.clearDisplay();

    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(scroll_x_pos, 30); 
    u8g2_for_adafruit_gfx.print(currentScrollText);
    display.fillRect(VISUALIZER_START_X, 0, VISUALIZER_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);

    scroll_x_pos -= SCROLL_SPEED;
    if (scroll_x_pos < -(text_width + 5)) {
      selectNextRandomPhrase();
    }

    display.drawRoundRect(VISUALIZER_START_X, 0, VISUALIZER_WIDTH, SCREEN_HEIGHT, 8, SSD1306_WHITE);
    int bar_width = VISUALIZER_WIDTH / NUM_BANDS;
    for (int i = 1; i < NUM_BANDS; i++) {
      int start_bin = i * (SAMPLES / 2 / NUM_BANDS);
      int end_bin = (i + 1) * (SAMPLES / 2 / NUM_BANDS);
      double band_magnitude = 0;
      for (int j = start_bin; j < end_bin; j++) {
        if (vReal[j] > band_magnitude) band_magnitude = vReal[j];
      }
      if (band_magnitude < NOISE_FLOOR) band_magnitude = 0;
      int bar_height = map(band_magnitude, NOISE_FLOOR, MAX_MAGNITUDE, 0, SCREEN_HEIGHT - 2);
      bar_height = constrain(bar_height, 0, SCREEN_HEIGHT - 2);
      int start_x = VISUALIZER_START_X + ((i - 1) * bar_width);
      for (int dx = 0; dx < bar_width - 1; dx++) {
        for (int dy = 0; dy < bar_height; dy++) {
          int current_x = start_x + dx;
          int current_y = SCREEN_HEIGHT - 2 - dy;
          if ((current_x + current_y) % 2 == 0) {
            display.drawPixel(current_x, current_y, SSD1306_WHITE);
          }
        }
      }
    }
    int prev_plot_x = 0;
    int prev_y = SCREEN_HEIGHT / 2;
    prev_y -= 5;
    for (int plot_x = 25; plot_x < VISUALIZER_WIDTH; plot_x++) {
      int sample_index = map(plot_x, 0, VISUALIZER_WIDTH, 0, SAMPLES);
      int y = map(samples_read[sample_index], -WAVEFORM_SENSITIVITY, WAVEFORM_SENSITIVITY, 1, SCREEN_HEIGHT - 2);
      y = constrain(y, 1, SCREEN_HEIGHT - 2);
      //y -= 8; // Ajuste para centrar la forma de onda
      display.drawLine(VISUALIZER_START_X + prev_plot_x, prev_y, VISUALIZER_START_X + plot_x, y, SSD1306_WHITE);
      prev_plot_x = plot_x;
      prev_y = y;
    }
    
    display.display();
    delay(20);
  }
}
