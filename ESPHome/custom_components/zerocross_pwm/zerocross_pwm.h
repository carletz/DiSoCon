
#include "esphome.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

class ZeroCrossPWMController : public Component {
 public:
  ZeroCrossPWMController(int zc_pin, int out_pin, int* percentuale_ptr, bool* attivo_ptr)
      : zc_pin_(zc_pin), out_pin_(out_pin), percentuale_ptr_(percentuale_ptr), attivo_ptr_(attivo_ptr),
        timer_handle_(nullptr) {}

  void setup() override {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << zc_pin_);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    gpio_set_direction((gpio_num_t)out_pin_, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)out_pin_, 0);

    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)zc_pin_, &ZeroCrossPWMController::onZeroCrossStatic, this);

    esp_timer_create_args_t timer_args = {
        .callback = &ZeroCrossPWMController::firePulseStatic,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "pwm_pulse"
    };
    esp_timer_create(&timer_args, &timer_handle_);
  }

  static void IRAM_ATTR onZeroCrossStatic(void* arg) {
    static_cast<ZeroCrossPWMController*>(arg)->onZeroCross();
  }

  void IRAM_ATTR onZeroCross() {
    if (!(*attivo_ptr_)) return;

    int delay_us = map_value(*percentuale_ptr_, 0, 100, 9000, 0);
    esp_timer_stop(timer_handle_);
    esp_timer_start_once(timer_handle_, delay_us);
  }

  static void firePulseStatic(void* arg) {
    static_cast<ZeroCrossPWMController*>(arg)->firePulse();
  }

  void firePulse() {
    gpio_set_level((gpio_num_t)out_pin_, 1);
    esp_rom_delay_us(100);
    gpio_set_level((gpio_num_t)out_pin_, 0);
  }

 private:
  int zc_pin_;
  int out_pin_;
  int* percentuale_ptr_;
  bool* attivo_ptr_;
  esp_timer_handle_t timer_handle_;

  int map_value(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
};
