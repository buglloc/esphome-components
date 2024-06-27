import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import esp32_ble_tracker
from esphome.components import time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
    CONF_MAC_ADDRESS
)


CODEOWNERS = ["@buglloc"]
DEPENDENCIES = ["esp32_ble_tracker"]
MULTI_CONF = True

CONF_SECRET = "secret"
CONF_MAX_TS_DRIFT = "max_ts_drift"

pinky_winky_ns = cg.esphome_ns.namespace("pinky_winky")
PinkyWinky = pinky_winky_ns.class_(
    "PinkyWinky", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PinkyWinky),
            cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Required(CONF_SECRET): cv.templatable(
                cv.All(cv.string, cv.Length(min=8, max=32))
            ),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Optional(CONF_MAX_TS_DRIFT, default="60"): cv.All(
              cv.positive_int,
            ),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    time_ = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_))
    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_secret(config[CONF_SECRET]))
    cg.add(var.set_max_ts_drift(config[CONF_MAX_TS_DRIFT]))


PinkyWinkyResetAction = pinky_winky_ns.class_(
    "PinkyWinkyResetAction", automation.Action
)

ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(PinkyWinky),
    }
)

@automation.register_action(
    "pinky_winky.reset", PinkyWinkyResetAction, ACTION_SCHEMA
)
async def pinky_winky_reset_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
