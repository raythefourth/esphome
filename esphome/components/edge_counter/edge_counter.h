#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP_IDF
#include "driver/gpio.h"
#endif

namespace esphome {
namespace edge_counter {

class EdgeCounterSensor : public sensor::Sensor, public PollingComponent {
 public:
  // Constructor now takes two pins
  EdgeCounterSensor(InternalGPIOPin *rx_pin, InternalGPIOPin *tx_pin);

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;

  static void IRAM_ATTR gpio_intr_handler(void *arg);

 protected:
  InternalGPIOPin *rx_pin_;  // Renamed from pin_
  InternalGPIOPin *tx_pin_;  // New output pin

  volatile uint32_t counter_{0};
  volatile bool new_value_ready_{false};
  uint32_t last_published_value_{0};
  bool tx_pin_current_state_{false};  // To store the current state of the tx_pin (false = LOW, true = HIGH)
};

}  // namespace edge_counter
}  // namespace esphome
