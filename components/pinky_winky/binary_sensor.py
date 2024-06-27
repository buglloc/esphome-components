import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_INPUT,
)

from . import PinkyWinky

DEPENDENCIES = ["pinky_winky"]

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema()
    .extend(
        {
            cv.Required(CONF_INPUT): cv.use_id(PinkyWinky),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_INPUT])

    sens = await binary_sensor.new_binary_sensor(config)
    cg.add(parent.set_button(sens))
