#include "heat_word.h"

#include "esphome/components/light/esp_hsv_color.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <cstring>

namespace esphome::heat_word {

static const char *const TAG = "heat_word";

static bool parse_minutes_to_seconds(StringRef state, uint32_t &out_seconds) {
  const auto minutes = parse_number<uint32_t>(state.c_str());
  if (!minutes.has_value())
    return false;
  out_seconds = *minutes * 60;
  return true;
}

void HeatWordComponent::setup() {
  if (this->print_status_entity_ != nullptr) {
    this->subscribe_homeassistant_state(&HeatWordComponent::on_ha_print_status_, this->print_status_entity_);
  }

  if (this->remain_time_entity_ != nullptr) {
    this->subscribe_homeassistant_state(&HeatWordComponent::on_ha_remain_time_, this->remain_time_entity_);
  }

  if (this->elapsed_time_entity_ != nullptr) {
    this->subscribe_homeassistant_state(&HeatWordComponent::on_ha_elapsed_time_, this->elapsed_time_entity_);
  }

  if (this->total_time_entity_ != nullptr) {
    this->subscribe_homeassistant_state(&HeatWordComponent::on_ha_total_time_, this->total_time_entity_);
  }

  this->redraw();
}

void HeatWordComponent::update() { this->redraw(); }

void HeatWordComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Heat Word:");
  ESP_LOGCONFIG(TAG, "  PrintState mappings: %u", static_cast<unsigned>(this->state_mappings_.size()));

  if (this->light_state_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Display light: %s", this->light_state_->get_name().c_str());
  }

  if (this->print_status_entity_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  homeassistant print_status entity: %s", this->print_status_entity_);
  }

  if (this->remain_time_entity_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  homeassistant remain_time entity: %s", this->remain_time_entity_);
  }

  if (this->elapsed_time_entity_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  homeassistant elapsed_time entity: %s", this->elapsed_time_entity_);
  }

  if (this->total_time_entity_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  homeassistant total_time entity: %s", this->total_time_entity_);
  }
}

Color HeatWordComponent::word_color(WordKind kind) const {
  if (kind >= WordKind::MAX_KIND)
    kind = WordKind::MISC;

  return this->words_color_[static_cast<size_t>(kind)];
}

void HeatWordComponent::on_print_state(PrintState state) {
  this->state_ = state;
  this->redraw();
}

void HeatWordComponent::on_remaining_seconds(uint32_t sec) {
  this->remaining_seconds_ = sec;
  this->redraw();
}

void HeatWordComponent::on_total_seconds(uint32_t sec) {
  this->total_seconds_ = sec;
  this->redraw();
}

void HeatWordComponent::redraw() {
  if (this->light_state_ == nullptr) {
    ESP_LOGE(TAG, "Cannot render: display light state is not set");
    return;
  }

  auto *strip = static_cast<light::AddressableLight *>(this->light_state_->get_output());
  if (strip == nullptr) {
    ESP_LOGE(TAG, "Cannot render: light output is not addressable light");
    return;
  }

  const int32_t strip_size = strip->size();
  if (strip_size < STRIP_SIZE) {
    ESP_LOGE(TAG, "Cannot render: LED strip has %d pixels, need at least %d", strip_size, STRIP_SIZE);
    return;
  }

  if (strip->is_effect_active()) {
    return;
  }

  const uint32_t mask = build_phrase(this->state_, this->remaining_seconds_, this->total_seconds_);
  if (mask == this->last_phrase_mask_) {
    return;
  }

  this->last_phrase_mask_ = mask;
  render_phrase(mask, this, strip);
  strip->schedule_show();
}

void HeatWordComponent::add_print_status(const char *ha_status, PrintState state) {
  this->state_mappings_.push_back({ha_status, state});
}

void HeatWordComponent::on_ha_print_status_(StringRef status) {
  for (const auto &m : this->state_mappings_) {
    if (status == m.ha_status) {
      ESP_LOGI(TAG, "print_status changed: '%.*s' -> state %d", static_cast<int>(status.size()), status.c_str(),
               static_cast<int>(m.state));
      this->on_print_state(m.state);
      return;
    }
  }

  ESP_LOGI(TAG, "print_status changed: '%.*s' -> state IDLE (no mapping)", static_cast<int>(status.size()),
           status.c_str());
  this->on_print_state(PrintState::IDLE);
}

void HeatWordComponent::on_ha_remain_time_(StringRef state) {
  uint32_t sec = 0;
  if (!parse_minutes_to_seconds(state, sec)) {
    ESP_LOGW(TAG, "Cannot parse remain_time (entity: %s, state: '%.*s'). ", this->remain_time_entity_,
             static_cast<int>(state.size()), state.c_str());
    return;
  }

  ESP_LOGD(TAG, "remain_time changed: %u sec", sec);
  this->remaining_seconds_ = sec;
  if (this->total_time_entity_ == nullptr)
    this->total_seconds_ = this->elapsed_seconds_ + this->remaining_seconds_;
  this->redraw();
}

void HeatWordComponent::on_ha_elapsed_time_(StringRef state) {
  uint32_t sec = 0;
  if (!parse_minutes_to_seconds(state, sec)) {
    ESP_LOGW(TAG, "Cannot parse elapsed_time (entity: %s, state: '%.*s'). ", this->remain_time_entity_,
             static_cast<int>(state.size()), state.c_str());
    return;
  }

  ESP_LOGD(TAG, "elapsed_time changed: %u sec", sec);
  this->elapsed_seconds_ = sec;
  if (this->total_time_entity_ == nullptr)
    this->total_seconds_ = this->elapsed_seconds_ + this->remaining_seconds_;
  this->redraw();
}

void HeatWordComponent::on_ha_total_time_(StringRef state) {
  uint32_t sec = 0;
  if (!parse_minutes_to_seconds(state, sec)) {
    ESP_LOGW(TAG, "Cannot parse total_time (entity: %s, state: '%.*s'). ", this->remain_time_entity_,
             static_cast<int>(state.size()), state.c_str());
    return;
  }

  ESP_LOGD(TAG, "total_time changed: %u sec", sec);
  this->on_total_seconds(sec);
}

}  // namespace esphome::heat_word
