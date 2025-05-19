#include "my_first_component_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
duty namespace esphome {
  namespace my_first_component {

  static const char *const TAG = "my_first_component";

  void MyFirstComponent::setup() {
    ESP_LOGCONFIG(TAG, "Setting up My First Component Sensor '%s'...", this->get_name().c_str());
    this->pin_->setup();
    this->store_.pin = this->pin_->to_isr();
    this->store_.last_level = this->pin_->digital_read();
    this->store_.last_interrupt = micros();

    this->pin_->attach_interrupt(MyFirstComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
  }
  void MyFirstComponent::dump_config() {
    LOG_SENSOR("", "My First Component Sensor", this);
    LOG_PIN("  Pin: ", this->pin_);
    LOG_UPDATE_INTERVAL(this);
  }
  void MyFirstComponent::update() {
    const uint32_t now = micros();
    const uint32_t last_interrupt = this->store_.last_interrupt;  // Read the measurement taken by the interrupt
    uint32_t on_time = this->store_.on_time;

    this->store_.on_time = 0;  // Start new measurement, exactly aligned with the micros() reading
    this->store_.last_interrupt = now;

    if (this->last_update_ != 0) {
      const bool level = this->store_.last_level;

      if (level)
        on_time += now - last_interrupt;

      const float total_time = float(now - this->last_update_);

      const float value = (on_time * 100.0f) / total_time;
      ESP_LOGD(TAG, "'%s' Got duty cycle=%.1f%%", this->get_name().c_str(), value);
      this->publish_state(value);
    }
    this->last_update_ = now;
  }

  float MyFirstComponent::get_setup_priority() const { return setup_priority::DATA; }

  void IRAM_ATTR MyFirstComponent::gpio_intr(MyFirstComponentStore *arg) {
    const bool new_level = arg->pin.digital_read();
    if (new_level == arg->last_level)
      return;
    arg->last_level = new_level;
    const uint32_t now = micros();

    if (!new_level)
      arg->on_time += now - arg->last_interrupt;

    arg->last_interrupt = now;
  }

  }  // namespace my_first_component
}  // namespace esphome
