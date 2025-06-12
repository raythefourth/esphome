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

  this->tx_pin_->setup();
  this->tx_pin_->digital_write(false);

  // test wakeup
  delayMicroseconds(10000);
  this->tx_pin_->digital_write(true);  // begin bit 1
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end of bit 1, start of 2
  /** Additional bits to send manually
  delayMicroseconds(98600);             // 29 bits = 98.6ms
  this->tx_pin_->digital_write(true);   // start bit 30 (data update)
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end bit 30, start of 31
  delayMicroseconds(6800);              // 2 bits
  this->tx_pin_->digital_write(true);   // start bit 33 (heat mode enable)
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end bit 33, start of 34
  delayMicroseconds(13600);             // 4 bits
  this->tx_pin_->digital_write(true);   // start bit 38 (fan low)
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end bit 38, start of 39
  delayMicroseconds(6800);              // 2 bits
  // heating set point 55 deg F = 0100 0000 = 0x40 hex
  this->tx_pin_->digital_write(true);  // start bit 40
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end bit 40, start of 41
  delayMicroseconds(20400);             // 6 bits
  this->tx_pin_->digital_write(true);   // start bit 47 (fan speed update)
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end bit 47
  **/
}

float EdgeCounterSensor::get_setup_priority() const { return setup_priority::DATA; }

void EdgeCounterSensor::dump_config() {
  LOG_SENSOR("", "Edge Counter Sensor", this);
  LOG_PIN("      RX Pin: ", this->rx_pin_);
  LOG_PIN("      TX Pin: ", this->tx_pin_);
}

void EdgeCounterSensor::loop() {
  // print durations from the buffer, but not backwards

  int32_t temp_duration_array[100];  // need this to reverse our buffer read out.
  uint8_t num_items{0};
  int32_t duration{0};
  while (this->store_.edge_count > 0) {
    duration = this->store_.duration_buffer[this->store_.edge_count - 1];
    this->store_.edge_count = this->store_.edge_count - 1;
    num_items = num_items + 1;
    temp_duration_array[num_items - 1] = duration;
  }
  while (num_items > 0) {
    ESP_LOGD(TAG, "Time since last edge (us): %i", temp_duration_array[num_items - 1]);
    num_items = num_items - 1;
  }

  /* previous edge counter output
  uint32_t current_count = this->store_.edge_count;  // copy current count as it might change.
  uint32_t last_count = this->last_edge_count_;
  uint32_t difference = current_count - last_count;
  if (difference > 0) {
    ESP_LOGD(TAG, "Edge Count: %u  Increment: %u", current_count, difference);
    this->last_edge_count_ = current_count;
  }
  */
}

void IRAM_ATTR EdgeCounterSensorStore::gpio_intr(EdgeCounterSensorStore *arg) {
  uint32_t now = micros();
  const bool new_level = arg->rx_pin.digital_read();  // this is after the edge, so level is opposite
  arg->edge_count = arg->edge_count + 1;
  arg->duration_buffer[arg->edge_count - 1] = (now - arg->last_time) * (arg->last_level == true ? 1 : -1);
  arg->last_time = now;
  arg->last_level = new_level;
}

}  // namespace edge_counter
}  // namespace esphome
