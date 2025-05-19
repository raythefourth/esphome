#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace my_first_component {

/// Store data in a class that doesn't use multiple-inheritance (vtables in flash)
struct MyFirstComponentStore {
  volatile uint32_t last_interrupt{0};
  volatile uint32_t on_time{0};
  volatile bool last_level{false};
  ISRInternalGPIOPin pin;

  static void gpio_intr(MyFirstComponentStore *arg);
};

class MyFirstComponent : public sensor::Sensor, public PollingComponent {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }

  void setup() override;
  float get_setup_priority() const override;
  void dump_config() override;
  void update() override;

 protected:
  InternalGPIOPin *pin_;

  MyFirstComponentStore store_{};
  uint32_t last_update_{0};
};

}  // namespace my_first_component
}  // namespace esphome
