import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, ICON_COUNTER, STATE_CLASS_MEASUREMENT

from . import P530Component

CONF_DISPENSED_PORTIONS = "dispensed_portions"
CONF_PORTIONS = "portions"


DEPENDENCIES = ["pkt_p530"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(P530Component),
        cv.Optional(CONF_DISPENSED_PORTIONS): sensor.sensor_schema(
            unit_of_measurement=CONF_PORTIONS,
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])

    if cfg := config.get(CONF_DISPENSED_PORTIONS):
        sens = await sensor.new_sensor(cfg)
        cg.add(parent.set_dispensed_portions_sensor(sens))
