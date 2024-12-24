#include "pinky_winky.h"

#include <cinttypes>

#include "esphome/core/log.h"
#include "mbedtls/sha1.h"

#ifdef USE_ESP32

namespace esphome {
namespace pinky_winky {

#define IDX_MFG_VER       (0)
#define IDX_MFG_BATT_LVL  (IDX_MFG_VER + 1)
#define IDX_MFG_BTN_STATE (IDX_MFG_BATT_LVL + 1)
#define IDX_MFG_TS        (IDX_MFG_BTN_STATE + 1)
#define IDX_MFG_SIGN      (IDX_MFG_TS + 4)
#define MFG_SIGN_LEN      (10)
#define MFG_LEN           (IDX_MFG_SIGN + MFG_SIGN_LEN)

namespace {
  const char *const TAG = "pinky_winky.parser";
  constexpr uint32_t kCompanyID = 0x035D;
  constexpr uint8_t kMaxBattLvl = 100;
  constexpr uint8_t kSupportedMfgVersion = 0x37;

  bool check_pinky_sign(const std::vector<uint8_t> &mfg_data, const std::string &password) {
    uint8_t hash[20] = {};
    bool ok = false;
    mbedtls_sha1_context hashCtx;
    mbedtls_sha1_init(&hashCtx);
    int err = mbedtls_sha1_starts(&hashCtx);
    if (err) {
      ESP_LOGE(TAG, "check_pinky_sign: start sha1: %d", err);
      mbedtls_sha1_free(&hashCtx);
      goto cleanup;
    }

    err = mbedtls_sha1_update(&hashCtx, &mfg_data[0], IDX_MFG_SIGN);
    if (err) {
      ESP_LOGE(TAG, "check_pinky_sign: update hash: %d", err);
      goto cleanup;
    }

    err = mbedtls_sha1_update(&hashCtx, (const uint8_t*)password.c_str(), password.size());
    if (err) {
      ESP_LOGE(TAG, "check_pinky_sign: update hash: %d", err);
      goto cleanup;
    }

    err = mbedtls_sha1_finish(&hashCtx, hash);
    if (err) {
      ESP_LOGE(TAG, "check_pinky_sign: calculate hash (err %d)", err);
      goto cleanup;
    }

    ok = memcmp(&mfg_data[IDX_MFG_SIGN], hash, MFG_SIGN_LEN) == 0;

cleanup:
    mbedtls_sha1_free(&hashCtx);
    return ok;
  }
}

void PinkyParser::setup() {
  this->company_uuid_ = esp32_ble::ESPBTUUID::from_uint32(kCompanyID);
}

void PinkyParser::set_secret(const std::string &secret) {
  this->secret_ = secret;
}

bool PinkyParser::have_secret() {
  return !this->secret_.empty();
}

bool PinkyParser::is_pinky_uuid(const esp32_ble::ESPBTUUID uuid) {
  return uuid == this->company_uuid_;
}

optional<PinkyState> PinkyParser::parse_state(const std::vector<uint8_t> &mfg_data, const uint32_t last_ts) {
  if (mfg_data.size() < 4) {
    ESP_LOGW(TAG, "parse_state: invalid mfg data size: %zu", mfg_data.size());
    return {};
  }

  if (mfg_data[IDX_MFG_VER] != kSupportedMfgVersion) {
    ESP_LOGW(TAG, "parse_state: unexpected version: %d (expected) != %d (actual)", kSupportedMfgVersion, mfg_data[IDX_MFG_VER]);
    return {};
  }

  if (mfg_data.size() != MFG_LEN) {
    ESP_LOGW(TAG, "parse_state: invalid mfg v%d data size: %zu", kSupportedMfgVersion, mfg_data.size());
    return {};
  }

  uint32_t ts = encode_uint32(
    mfg_data[IDX_MFG_TS],
    mfg_data[IDX_MFG_TS + 1],
    mfg_data[IDX_MFG_TS + 2],
    mfg_data[IDX_MFG_TS + 3]
  );

  if (ts < last_ts) {
    ESP_LOGW(TAG, "parse_state: skip too old state: %" PRIu32 " (remote) < %" PRIu32 " (our)", ts, last_ts);
    return {};
  }

  if (!check_pinky_sign(mfg_data, this->secret_)) {
    ESP_LOGE(TAG, "parse_state: invalid sign");
    return {};
  }

  return PinkyState{
    .battery_level = mfg_data[IDX_MFG_BATT_LVL] < kMaxBattLvl ? mfg_data[IDX_MFG_BATT_LVL] : kMaxBattLvl,
    .pressed = mfg_data[IDX_MFG_BTN_STATE] == 1,
    .ts = ts
  };
}

}  // namespace pinky_winky
}  // namespace esphome

#endif
