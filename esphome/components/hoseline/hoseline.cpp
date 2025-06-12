#include "hoseline.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace hoseline {

static const char *const TAG = "hoseline.climate";

void Hoseline::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Hoseline HVAC...");
  this->rx_pin_->setup();
  this->store_.rx_pin = this->rx_pin_->to_isr();
  this->tx_pin_->setup();
  this->tx_pin_->digital_write(false);

  /* TEST WAKEUP
  delayMicroseconds(10000);
  this->tx_pin_->digital_write(true);  // begin bit 1
  delayMicroseconds(3400);
  this->tx_pin_->digital_write(false);  // end of bit 1, start of 2
  */

  // simplify our code here a bit with some shortcuts
  auto &s = this->store_;
  s.filter_us = this->filter_us_;
  s.rx_pin = this->rx_pin_;
  s.buffer_size = this->buffer_size_;

  // create our data bucket
  s.buffer = new uint32_t[s.buffer_size];
  void *buf = (void *) s.buffer;  // something about getting a pointer to memory location for this buffer
  memset(buf, 0, s.buffer_size * sizeof(uint32_t));  // then clearing that memory location

  // Configure where to start reading/writing based on line HIGH or LOW.
  if (this->gpio_rx_pin_->digital_read()) {
    s.buffer_write_at = s.buffer_read_at = 1;
  } else {
    s.buffer_write_at = s.buffer_read_at = 0;
  }

  // start the ISR
  this->rx_pin_->attach_interrupt(HoselineHVACStore::edge_detector, &this->store_, gpio::INTERRUPT_ANY_EDGE);
}

void Hoseline::dump_config() {
  LOG_CLIMATE("", "Hoseline HVAC Climate", this);
  LOG_PIN("  RX Pin: ", this->rx_pin_);
  LOG_PIN("  TX Pin: ", this->tx_pin_);
}

/* --- Interrupt Service Routines (ISR) --- */
void IRAM_ATTR HOT HoselineHVACStore::edge_detector(HoselineHVACStore *arg) {
  const uint32_t now = micros();
  arg->last_time = now;
  const uint32_t next_write_at = (arg->buffer_write_at + 1) % arg->buffer_size;  //
  const bool level = arg->rx_pin.digital_read();
  if (level != next_write_at % 2)
    return;

  // If next_write_at is buffer_read, we have hit an overflow
  if (next_write_at == arg->buffer_read_at)
    return;

  const uint32_t last_change = arg->buffer[arg->buffer_write_at];
  const uint32_t time_since_change = now - last_change;
  if (time_since_change <= arg->filter_us)
    return;  // ignore short dips and blips

  arg->buffer[arg->buffer_write_at = next_write_at] = now;
}

void Hoseline::loop() {
  const uint32_t now = micros();
  auto &s = this->store_;  // make some shortcuts

  ESP_LOGD(TAG, "%u read_at %u    write_at %u    last_time %u", now, s.buffer_read_at, s.buffer_write_at, s.last_time);

  // is_high_ = this->gpio_rx_pin_->digital_read();
  // ESP_LOGD(TAG, "%u     %s", now, is_high_ ? "HIGH" : "LOW");

  /*** We need to determine if it's time to read from the pulse buffer ***/
  // copy write at to local variables, as it's volatile
  const uint32_t write_at = s.buffer_write_at;
  // calculate the number of unread pulses in the buffer
  // % s.buffer_size is used to wraparound the ring buffer.
  const uint32_t unread_num = (s.buffer_size + write_at - s.buffer_read_at) % s.buffer_size;
  // read signals in pairs (one HIGH and one LOW)
  // must be 2 or more to proceed
  if (unread_num <= 1)
    return;  // do nothing, keep waiting
  //////const uint32_t now = micros();
  // wait for end of transmission before reading the pulses (and processing them)
  // idle_us_ specifies the signal timeout
  if (now - s.buffer[write_at] < this->idle_us_)
    return;  // do nothing, keep waiting

  ESP_LOGD(TAG, "read_at=%u write_at=%u unread=%u now=%u end=%u", s.buffer_read_at, write_at, unread_num, now,
           s.buffer[write_at]);

  /*** It's time to read the pulses and do something with the information ***/
  // skip first value, it's from the previous idle level (and buffer[0] is always LOW)
  // IMPORTANT: Remember, the buffer contains DURATIONS which may be multiple bits
  s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;  // skip first
  uint32_t prev = s.buffer_read_at;                           // remember the first pulse we want to read.
  s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;  // increment read_at

  // this next part is specific to sharing data with the remote_decoder
  /*
  const uint32_t reserve_size = 1 + (s.buffer_size + write_at - s.buffer_read_at) % s.buffer_size;
  this->temp_.clear();
  this->temp_.reserve(reserve_size);
  // Determine the multiplier, which will give the pulse a "sign"
  int32_t multiplier = s.buffer_read_at % 2 == 0 ? 1 : -1; // it is a "signed" integer
  */

  // altering the "sign detection" logic above.
  bool is_high = s.buffer_read_at % 2 == 0 ? true : false;  // this could be backwards

  // iterate through remaining unread_num of pulses
  for (uint32_t i = 0; prev != write_at; i++) {  // increment i until prev == write_at (also incrementing prev)
    int32_t delta =
        s.buffer[s.buffer_read_at] - s.buffer[prev];  // calculate the duration. We already incremented read_at
    if (uint32_t(delta) >= this->idle_us_) {          // not sure why this is needed
      // already found a space longer than idle. There must have been two pulses
      break;
    }

    // print the data from this buffer position we are reading
    ESP_LOGD(TAG, "  i=%u buffer[%u]=%u - buffer[%u]=%u -> %d -> %s", i, s.buffer_read_at, s.buffer[s.buffer_read_at],
             prev, s.buffer[prev], delta, is_high ? "HIGH" : "LOW");

    // this is specific to remote receiver
    // this->temp_.push_back(multiplier * delta);

    // increment things for the next iteration of the FOR loop
    prev = s.buffer_read_at;                                    // increment prev
    s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;  // increment read_at
    // multiplier *= -1; // invert multiplier
    is_high = !is_high;  // invert signal level
  }                      // end for loop

  // increment read_at up to write_at
  s.buffer_read_at = (s.buffer_size + s.buffer_read_at - 1) % s.buffer_size;

  // add a final idle pulse
  // this->temp_.push_back(this->idle_us_ * multiplier);
  // tell remote_base to do something with the data
  // this->call_listeners_dumpers_();

  /** TO DO **/
  // first, test this code.
  // create a second buffer to push bits into.
  // some quick math to determine how many bits in a pulse.
  // reset the bit_buffer before we...
  // push the bits into the bit_buffer (not to be confused with the pulse buffer)
  // call "process bit_buffer" (only happens when the line goes idle)
  // this should process the RX and set up the next TX.
  // figure out how to start our TX timer when we hear the first pulse.
  // probably incorporate idle_timeout into edge_detector_isr
  // if idle_timeout is TRUE and we detect a pulse, then start tx_timer and reset idle_timeout
}

climate::ClimateTraits Hoseline::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_action(true);
  traits.set_supported_modes(
      {climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_COOL, climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_AUTO});
  traits.set_supported_fan_modes(
      {climate::CLIMATE_FAN_QUIET, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH});
  // traits.set_visual_min_temperature(50.0f);
  // traits.set_visual_max_temperature(80.0f);
  // traits.set_visual_temperature_step(1.0f);
  return traits;
}

void Hoseline::control(const climate::ClimateCall &call) {
  /*
  bool mode_changed_from_off = false;

  if (call.get_mode().has_value()) {
    climate::ClimateMode requested_mode = *call.get_mode();
    if (this->mode == climate::CLIMATE_MODE_OFF &&
        (requested_mode == climate::CLIMATE_MODE_COOL ||
         requested_mode == climate::CLIMATE_MODE_HEAT ||
         requested_mode == climate::CLIMATE_MODE_AUTO)) {
      mode_changed_from_off = true;
    }
    this->mode = requested_mode;
    mode_change_pending_ = true;
    ESP_LOGD(TAG, "Control: Mode set to %s", climate::climate_mode_to_string(this->mode));
  }

  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
    setpoint_change_pending_ = true;
    ESP_LOGD(TAG, "Control: Target temp set to %.1f°F", this->target_temperature);
  }

  if (call.get_fan_mode().has_value()) {
    this->fan_mode = *call.get_fan_mode();
    fan_mode_change_pending_ = true;
    ESP_LOGD(TAG, "Control: Fan mode set to %s",
  climate::climate_fan_mode_to_string(this->fan_mode.value_or(climate::CLIMATE_FAN_QUIET)));
  }

  if (mode_changed_from_off) {
    ESP_LOGI(TAG, "Mode changed from OFF. Requesting Master wake-up.");
    this->initiate_master_wakeup_ = true;
  }

  // Always ensure pending flags are set if any aspect of desired state differs from last known master state
  // This ensures that even if only setpoint changes while already ON, it gets sent.
  if (this->mode != this->master_current_mode_) mode_change_pending_ = true;
  if (this->target_temperature != this->master_current_setpoint_f_) setpoint_change_pending_ = true;
  if (this->fan_mode != this->master_current_fan_mode_) fan_mode_change_pending_ = true;


  this->publish_state(); // Publish optimistic state
  */
}

}  // namespace hoseline
}  // namespace esphome
