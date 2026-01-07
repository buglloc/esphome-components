import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DOOR,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import P530Component

CONF_DOOR_OPENED = "door_opened"
CONF_DOOR_OPEN_ISSUE = "door_open_issue"
CONF_DOOR_CLOSE_ISSUE = "door_close_issue"
CONF_FOOD_LOW_ISSUE = "food_low_issue"

DEPENDENCIES = ["pkt_p530"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(P530Component),
        cv.Optional(CONF_DOOR_OPENED): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_DOOR,
        ),
        cv.Optional(CONF_DOOR_OPEN_ISSUE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_DOOR_CLOSE_ISSUE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_FOOD_LOW_ISSUE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])

    if cfg := config.get(CONF_DOOR_OPENED):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(parent.set_door_opened_sensor(sens))

    if cfg := config.get(CONF_DOOR_OPEN_ISSUE):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(parent.set_door_open_issue_sensor(sens))

    if cfg := config.get(CONF_DOOR_CLOSE_ISSUE):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(parent.set_door_close_issue_sensor(sens))

    if cfg := config.get(CONF_FOOD_LOW_ISSUE):
        sens = await binary_sensor.new_binary_sensor(cfg)
        cg.add(parent.set_food_low_issue_sensor(sens))
