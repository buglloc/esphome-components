import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
)

from . import P530Component

CONF_LAST_FEED_PORTIONS = "last_feed_portions"
CONF_TOTAL_PORTIONS = "total_portions"
CONF_PORTIONS = "portions"


DEPENDENCIES = ["pkt_p530"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(P530Component),
        cv.Optional(CONF_LAST_FEED_PORTIONS): sensor.sensor_schema(
            unit_of_measurement=CONF_PORTIONS,
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TOTAL_PORTIONS): sensor.sensor_schema(
            unit_of_measurement=CONF_PORTIONS,
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])

    if last_feed_cfg := config.get(CONF_LAST_FEED_PORTIONS):
        sens = await sensor.new_sensor(last_feed_cfg)
        cg.add(parent.set_last_feed_portions_sensor(sens))

    if total_cfg := config.get(CONF_TOTAL_PORTIONS):
        sens = await sensor.new_sensor(total_cfg)
        cg.add(parent.set_total_portions_sensor(sens))
