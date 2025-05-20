#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace edge_counter {

struct EdgeCounterSensorStore {
  volatile uint32_t edge_count{0};
  volatile uint32_t last_time{0};
  volatile int32_t duration_buffer[100];  // 400 kB, signed!
  volatile bool is_high{false};
  volatile bool last_level{false};
  ISRInternalGPIOPin rx_pin;

  static void gpio_intr(EdgeCounterSensorStore *arg);
};

class EdgeCounterSensor : public sensor::Sensor, public Component {
 public:
  void set_rx_pin(InternalGPIOPin *rx_pin) { this->rx_pin_ = rx_pin; }
  void set_tx_pin(InternalGPIOPin *tx_pin) { this->tx_pin_ = tx_pin; }

  void setup() override;
  float get_setup_priority() const override;
  void dump_config() override;
  void loop() override;

 protected:
  InternalGPIOPin *rx_pin_;
  InternalGPIOPin *tx_pin_;

  EdgeCounterSensorStore store_{};
  uint32_t last_edge_count_{0};
};

}  // namespace edge_counter
}  // namespace esphome
