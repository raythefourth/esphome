#include "edge_counter.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace edge_counter {

static const char *const TAG = "edge_counter";

void IRAM_ATTR EdgeCounterSensor::gpio_intr_handler(void *arg) {
  EdgeCounterSensor *sensor_instance = static_cast<EdgeCounterSensor *>(arg);
  sensor_instance->counter_++;
  sensor_instance->new_value_ready_ = true;
}

// Updated constructor to accept both pins
EdgeCounterSensor::EdgeCounterSensor(InternalGPIOPin *rx_pin, InternalGPIOPin *tx_pin)
    : rx_pin_(rx_pin), tx_pin_(tx_pin) {
  // tx_pin_current_state_ is initialized to false by default in the header
}

float EdgeCounterSensor::get_setup_priority() const { return setup_priority::IO; }

void EdgeCounterSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Edge Counter...");

  // Setup RX (input) pin for edge detection
  ESP_LOGCONFIG(TAG, "  RX Pin: GPIO%d (Input for edge detection)", this->rx_pin_->get_pin());
  this->rx_pin_->set_flags(gpio::FLAG_INPUT);  // Ensure it's an input
  // Add pull-up if needed for your rx_pin signal, e.g., if connecting a button to ground
  // this->rx_pin_->set_flags(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  this->rx_pin_->setup();
  this->rx_pin_->attach_interrupt(EdgeCounterSensor::gpio_intr_handler, this, gpio::INTERRUPT_ANY_EDGE);

  // Setup TX (output) pin for toggling
  ESP_LOGCONFIG(TAG, "  TX Pin: GPIO%d (Output for toggling)", this->tx_pin_->get_pin());
  this->tx_pin_->set_flags(gpio::FLAG_OUTPUT);
  this->tx_pin_->setup();
  this->tx_pin_->digital_write(this->tx_pin_current_state_);  // Set initial state (LOW)

  ESP_LOGCONFIG(TAG, "Edge Counter setup complete.");
}

void EdgeCounterSensor::dump_config() {
  LOG_SENSOR("  ", "Edge Counter Sensor", this);
  ESP_LOGCONFIG(TAG, "    RX (Input) Pin:");
  LOG_PIN("      Pin: ", this->rx_pin_);
  ESP_LOGCONFIG(TAG, "    TX (Output) Pin:");
  LOG_PIN("      Pin: ", this->tx_pin_);
  ESP_LOGCONFIG(TAG, "    Initial counter value: %u", this->counter_);
  ESP_LOGCONFIG(TAG, "    Initial TX pin state: %s", this->tx_pin_current_state_ ? "HIGH" : "LOW");
}

void EdgeCounterSensor::loop() {
  if (this->new_value_ready_) {
    uint32_t current_count = this->counter_;

    // Publish sensor state
    if (this->last_published_value_ != current_count || !this->has_state()) {
      // this->publish_state(current_count);
      this->last_published_value_ = current_count;
      ESP_LOGD(TAG, "Published Edge Count: %u", current_count);
    }

    // Toggle the TX pin state
    this->tx_pin_current_state_ = !this->tx_pin_current_state_;
    // this->tx_pin_->digital_write(this->tx_pin_current_state_);
    ESP_LOGD(TAG, "Toggled TX Pin (GPIO%d) to: %s", this->tx_pin_->get_pin(),
             this->tx_pin_current_state_ ? "HIGH" : "LOW");

    this->new_value_ready_ = false;  // Reset the flag
  }
}

void EdgeCounterSensor::update() {
  // This method is called by PollingComponent based on update_interval.
  // For this component, primary actions happen in loop() driven by interrupts.
  // You could add logic here for less frequent, guaranteed updates if desired,
  // or simply leave it to primarily rely on the interrupt-driven loop() updates.
  // ESP_LOGV(TAG, "Update() called. Current count: %u", this->counter_);
}

}  // namespace edge_counter
}  // namespace esphome
