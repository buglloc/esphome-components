import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import i2c
from esphome.const import (
  CONF_ID,
)

CODEOWNERS = ["@buglloc"]
DEPENDENCIES = ["i2c"]

sy6970_ns = cg.esphome_ns.namespace("sy6970")
SY6970 = sy6970_ns.class_(
    "SY6970",
    cg.Component,
    i2c.I2CDevice,
)

CONF_STATE_LED_ENABLE = "state_led_enable"
CONF_ILIM_PIN_ENABLE = "ilim_pin_enable"
CONF_BATFET_ENABLE = "batfet_enable"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SY6970),
        cv.Optional(CONF_STATE_LED_ENABLE, default=True): cv.boolean,
        cv.Optional(CONF_ILIM_PIN_ENABLE, default=True): cv.boolean,
        cv.Optional(CONF_BATFET_ENABLE, default=True): cv.boolean,
    }
).extend(
    cv.COMPONENT_SCHEMA
).extend(
    i2c.i2c_device_schema(0x6A)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await i2c.register_i2c_device(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_state_led_enabled(config.get(CONF_STATE_LED_ENABLE)))
    cg.add(var.set_ilim_pin_enable(config.get(CONF_ILIM_PIN_ENABLE)))
    cg.add(var.set_batfet_enabled(config.get(CONF_BATFET_ENABLE)))