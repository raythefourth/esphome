#pragma once  // verified good

#include "esphome/core/component.h"              // verified good
#include "esphome/components/climate/climate.h"  // verified good
#include "esphome/core/hal.h"                    // verified good

namespace esphome {   // verified good
namespace hoseline {  // verified good

// Constants from protocol
const uint32_t BIT_PERIOD_MICROS = 3400;  // 3.4 milliseconds
const uint32_t BIT_HALF_PERIOD_MICROS = 1700;
const uint32_t OFF_TIMEOUT_MICROS = 2500000;  // 2.5 seconds
const uint8_t FRAME_TOTAL_BITS = 48;
const uint32_t IDLE_BITS_REQUIRED = 50;

// Converts bit number to array/bitset index (Bit 1 is at bitset[0])
inline uint8_t bit_to_idx(uint8_t bit_num) {
  if (bit_num == 0)
    return 0;
  return bit_num - 1;
}

struct HoselineHVACStore {  // (reference remote_receiver.h)
  static void edge_detector(HoselineHVACStore *arg);

  /// Stores the time (in micros) that the leading/falling edge happened at
  ///  * An even index means a falling edge appeared at the time stored at the index
  ///  * An uneven index means a rising edge appeared at the time stored at the index
  volatile uint32_t *buffer{nullptr};
  volatile uint32_t buffer_write_at;
  uint32_t buffer_read_at{0};
  bool overflow{false};
  uint32_t buffer_size{1000};
  uint32_t filter_us{30};
  volatile uint32_t last_time;  // for debug
  ISRInternalGPIOPin rx_pin;
};

class Hoseline : public Component, public climate::Climate {  // verified good
 public:
  // void set_rx_pin(GPIOPin *rx_pin);
  // void set_tx_pin(GPIOPin *tx_pin);
  void set_tx_pin(InternalGPIOPin *tx_pin) { this->tx_pin_ = tx_pin; }        // reference uart_component.h
  void set_rx_pin(InternalGPIOPin *rx_pin) { this->rx_pin_ = rx_pin; }        // reference uart_component.h
  float get_setup_priority() const override { return setup_priority::DATA; }  // reference remote_receiver.h
  // void setup() override;                                                              // reference remote_receiver.h
  void setup(InternalGPIOPin *tx_pin, InternalGPIOPin *rx_pin);  // verified good (reference uart_component_esp8266.h)
  void dump_config() override;                                   // verified good (reference gpio_binary_sensor.h)
  void loop() override;                                          // verified good (reference gpio_binary_sensor.h)
  climate::ClimateTraits traits() override;                      // verified good (climate_ir.h)
  void control(const climate::ClimateCall &call) override;       // verified good (climate_ir.h)

 protected:                                // protected fields represent the state of an object
  InternalGPIOPin *gpio_tx_pin_{nullptr};  // verified good (reference uart_component_esp8266.h)
  ISRInternalGPIOPin tx_pin_;              // verified good (reference uart_component_esp8266.h)
  InternalGPIOPin *gpio_rx_pin_{nullptr};  // verified good (reference uart_component_esp8266.h)
  ISRInternalGPIOPin rx_pin_;              // verified good (reference uart_component_esp8266.h)

  HoselineHVACStore store_;

  uint32_t buffer_size_{1000};
  uint32_t filter_us_{30};   // ignore short changes
  uint32_t idle_us_{10000};  // quiet for 10 bits?
  uint32_t now_{0};
  uint32_t prev_time_{0};
  uint32_t delay_time_{3400};
  bool is_high_{false};

  // esp_timer_handle_t idle_timeout_{nullptr};    // transmission ended
  // esp_timer_handle_t tx_timer_{nullptr}; // begin transmitting
  // esp_timer_handle_t master_timeout_timer_{nullptr}; // HVAC has gone offline

};  // close class definition

}  // namespace hoseline
}  // namespace esphome
