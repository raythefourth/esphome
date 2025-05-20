#include "edge_counter.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace edge_counter {

static const char *const TAG = "edge_counter";

void EdgeCounterSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Edge Counter...");
  this->rx_pin_->setup();
  this->store_.rx_pin = this->rx_pin_->to_isr();

  this->rx_pin_->attach_interrupt(EdgeCounterSensorStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);

  // this->tx_pin_->set_flags(gpio::FLAG_OUTPUT);
  // this->tx_pin_->setup();
  // this->tx_pin_->digital_write(this->tx_pin_current_state_);
}

float EdgeCounterSensor::get_setup_priority() const { return setup_priority::DATA; }

void EdgeCounterSensor::dump_config() {
  LOG_SENSOR("", "Edge Counter Sensor", this);
  LOG_PIN("      RX Pin: ", this->rx_pin_);
  LOG_PIN("      TX Pin: ", this->tx_pin_);
}

void EdgeCounterSensor::loop() {
  // if count has increased, print it.
  uint32_t current_count = this->store_.edge_count;  // copy current count as it might change.
  uint32_t last_count = this->last_edge_count_;
  uint32_t difference = current_count - last_count;
  if (difference > 0) {
    ESP_LOGD(TAG, "Edge Count: %u  Increment: %u", current_count, difference);
    this->last_edge_count_ = current_count;
  }
}

void IRAM_ATTR EdgeCounterSensorStore::gpio_intr(EdgeCounterSensorStore *arg) {
  const bool new_level = arg->rx_pin.digital_read();
  if (new_level == arg->last_level)
    return;
  arg->edge_count = arg->edge_count + 1;
}

}  // namespace edge_counter
}  // namespace esphome
