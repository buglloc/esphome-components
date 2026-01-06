import esphome.codegen as cg
from esphome.components import binary_sensor, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_GATE,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_COUNTER,
    STATE_CLASS_TOTAL_INCREASING,
)

from . import P530Component

DEPENDENCIES = ("pkt_p530",)

CONF_DOOR_OPENED = "door_opened"
CONF_FOOD_LOW = "food_low"
CONF_DOOR_ISSUE = "door_issue"
CONF_LAST_FEED = "last_feed_portions"
CONF_TOTAL = "total_portions"
CONF_PORTIONS = "portions"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(P530Component),
        cv.Optional(CONF_DOOR_OPENED): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_GATE,
        ),
        cv.Optional(CONF_FOOD_LOW): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_DOOR_ISSUE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_LAST_FEED): sensor.sensor_schema(
            unit_of_measurement=CONF_PORTIONS,
            icon=ICON_COUNTER,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_TOTAL): sensor.sensor_schema(
            unit_of_measurement=CONF_PORTIONS,
            icon=ICON_COUNTER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
)


async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])

    if cfg := config.get(CONF_DOOR_OPENED):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(var.set_door_opened_sensor(sens))

    if cfg := config.get(CONF_FOOD_LOW):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(var.set_food_low_sensor(sens))

    if cfg := config.get(CONF_DOOR_ISSUE):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(var.set_door_issue_sensor(sens))

    if cfg := config.get[CONF_LAST_FEED]:
        sens = await sensor.new_sensor(cfg)
        cg.add(var.last_feed_portions_sensor.set(sens))

    if cfg := config.get[CONF_TOTAL]:
        sens = await sensor.new_sensor(cfg)
        cg.add(var.total_portions_sensor.set(sens))
