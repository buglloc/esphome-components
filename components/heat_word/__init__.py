from esphome import automation
import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_BRIGHTNESS, CONF_ID, CONF_LIGHT, CONF_OUTPUT_ID

DEPENDENCIES = ["light", "api"]
AUTO_LOAD = ["light"]

heat_word_ns = cg.esphome_ns.namespace("heat_word")

HeatWordComponent = heat_word_ns.class_("HeatWordComponent", cg.PollingComponent)
HeatWordSetBrightnessAction = heat_word_ns.class_(
    "HeatWordSetBrightnessAction", automation.Action
)
PrintState = heat_word_ns.enum("PrintState", is_class=True)
ColorPalette = heat_word_ns.enum("ColorPalette", is_class=True)
HeatWordLightDisplay = heat_word_ns.class_(
    "HeatWordLightDisplay", light.AddressableLight
)

CONF_BACKEND = "backend"
CONF_COLORS = "colors"
CONF_MISC = "misc"
CONF_ERROR = "error"
CONF_HEAT = "heat"
CONF_DONE = "done"
CONF_HOUR = "hour"
CONF_MINUTE = "minute"
CONF_PAUSE = "pause"
CONF_OFF = "off"
CONF_IDLE = "idle"

CONF_REMAINING_TIME = "remaining_time"
CONF_TOTAL_TIME = "total_time"
CONF_DONE_TIMEOUT = "done_timeout"

CONF_HUE = "hue"
CONF_SATURATION = "saturation"

CONF_HOMEASSISTANT = "homeassistant"
CONF_ENTITY_PREFIX = "entity_prefix"
CONF_PRINT_STATUS_ENTITY = "print_status_entity"
CONF_REMAIN_TIME_ENTITY = "remain_time_entity"
CONF_ELAPSED_TIME_ENTITY = "elapsed_time_entity"
CONF_PRINT_STATUSES = "print_statuses"

# Default Anycubic Kobra S1 entity suffixes
_ENTITY_SUFFIX_PRINT_STATUS = "_print_status"
_ENTITY_SUFFIX_REMAIN_TIME = "_print_remaining_time"
_ENTITY_SUFFIX_ELAPSED_TIME = "_print_time"

DEFAULT_PRINT_STATUS_MAPPINGS = {
    "unavailable": "unavailable",
    "unknown": "idle",
    "preheating": "heating",
    "auto_leveling": "heating",
    "printing": "printing",
    "paused": "paused",
    "resuming": "resuming",
    "failed": "error",
    "finished": "done",
}

COLORS_ENUM = {
    CONF_DONE: ColorPalette.DONE,
    CONF_ERROR: ColorPalette.ERROR,
    CONF_HEAT: ColorPalette.HEAT,
    CONF_HOUR: ColorPalette.HOUR,
    CONF_IDLE: ColorPalette.IDLE,
    CONF_MINUTE: ColorPalette.MINUTE,
    CONF_MISC: ColorPalette.MISC,
    CONF_OFF: ColorPalette.OFF,
    CONF_PAUSE: ColorPalette.PAUSE,
}

PRINT_STATE_ENUM = {
    "unavailable": PrintState.UNAVAILABLE,
    "idle": PrintState.IDLE,
    "heating": PrintState.HEATING,
    "printing": PrintState.PRINTING,
    "paused": PrintState.PAUSED,
    "resuming": PrintState.RESUMING,
    "error": PrintState.ERROR,
    "done": PrintState.DONE,
}

HSV_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_HUE, default=0.0): cv.float_range(min=0.0, max=360.0),
        cv.Optional(CONF_SATURATION, default=1.0): cv.percentage,
        cv.Optional(CONF_BRIGHTNESS, default=1.0): cv.percentage,
    }
)

COLORS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_DONE): HSV_SCHEMA,
        cv.Optional(CONF_ERROR): HSV_SCHEMA,
        cv.Optional(CONF_HEAT): HSV_SCHEMA,
        cv.Optional(CONF_HOUR): HSV_SCHEMA,
        cv.Optional(CONF_IDLE): HSV_SCHEMA,
        cv.Optional(CONF_MINUTE): HSV_SCHEMA,
        cv.Optional(CONF_MISC): HSV_SCHEMA,
        cv.Optional(CONF_OFF): HSV_SCHEMA,
        cv.Optional(CONF_PAUSE): HSV_SCHEMA,
    }
)

COLOR_DEFAULTS = {
    # Statuses
    CONF_DONE: {
        CONF_HUE: 140.0,
        CONF_SATURATION: 0.60,
        CONF_BRIGHTNESS: 0.80,
    },
    CONF_ERROR: {
        CONF_HUE: 5.0,
        CONF_SATURATION: 0.85,
        CONF_BRIGHTNESS: 0.85,
    },
    CONF_HEAT: {
        CONF_HUE: 48.0,
        CONF_SATURATION: 1.0,
        CONF_BRIGHTNESS: 0.8,
    },
    # Time
    CONF_HOUR: {
        CONF_HUE: 45.0,
        CONF_SATURATION: 0.50,
        CONF_BRIGHTNESS: 0.88,
    },
    CONF_MINUTE: {
        CONF_HUE: 300.0,
        CONF_SATURATION: 0.60,
        CONF_BRIGHTNESS: 0.88,
    },
    # Special
    CONF_MISC: {
        CONF_HUE: 220.0,
        CONF_SATURATION: 0.15,
        CONF_BRIGHTNESS: 0.60,
    },
    CONF_IDLE: {
        CONF_HUE: 240.0,
        CONF_SATURATION: 0.10,
        CONF_BRIGHTNESS: 0.35,
    },
    CONF_OFF: {CONF_HUE: 0.0, CONF_SATURATION: 0.0, CONF_BRIGHTNESS: 0.0},
    CONF_PAUSE: {
        CONF_HUE: 240.0,
        CONF_SATURATION: 0.10,
        CONF_BRIGHTNESS: 0.35,
    },
}


def _degrees_to_hue8(deg: float) -> int:
    d = float(deg) % 360.0
    if d < 0:
        d += 360.0
    return int(round(d * 255.0 / 360.0)) & 0xFF


def _hsv8_from_fragment(fragment: dict, defaults: dict) -> tuple[int, int, int]:
    merged = {**defaults, **fragment}
    h8 = _degrees_to_hue8(merged[CONF_HUE])
    s8 = int(round(float(merged[CONF_SATURATION]) * 255))
    v8 = int(round(float(merged[CONF_BRIGHTNESS]) * 255))
    return h8, s8, v8


def _apply_entity_prefix(config):
    """Derive entity IDs from entity_prefix when explicit IDs are not set."""
    prefix = config.get(CONF_ENTITY_PREFIX)
    if prefix is None:
        return config
    config = dict(config)
    if CONF_PRINT_STATUS_ENTITY not in config:
        config[CONF_PRINT_STATUS_ENTITY] = prefix + _ENTITY_SUFFIX_PRINT_STATUS
    if CONF_REMAIN_TIME_ENTITY not in config:
        config[CONF_REMAIN_TIME_ENTITY] = prefix + _ENTITY_SUFFIX_REMAIN_TIME
    if CONF_ELAPSED_TIME_ENTITY not in config:
        config[CONF_ELAPSED_TIME_ENTITY] = prefix + _ENTITY_SUFFIX_ELAPSED_TIME
    return config


HOMEASSISTANT_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_ENTITY_PREFIX): cv.string,
            cv.Optional(CONF_PRINT_STATUS_ENTITY): cv.string,
            cv.Optional(CONF_REMAIN_TIME_ENTITY): cv.string,
            cv.Optional(CONF_ELAPSED_TIME_ENTITY): cv.string,
            cv.Optional(
                CONF_PRINT_STATUSES, default=DEFAULT_PRINT_STATUS_MAPPINGS
            ): cv.Schema(
                {
                    cv.string: cv.one_of(*PRINT_STATE_ENUM.keys(), lower=True),
                }
            ),
        }
    ),
    _apply_entity_prefix,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HeatWordComponent),
            cv.Required(CONF_BACKEND): cv.use_id(light.AddressableLightState),
            cv.Optional(CONF_COLORS, default={}): COLORS_SCHEMA,
            cv.Optional(
                CONF_LIGHT, default={"name": "Printer Clock"}
            ): light.light_schema(
                HeatWordLightDisplay,
                light.LightType.ADDRESSABLE,
                default_restore_mode="RESTORE_DEFAULT_ON",
            ),
            cv.Optional(
                CONF_DONE_TIMEOUT, default="5min"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_HOMEASSISTANT): HOMEASSISTANT_SCHEMA,
        }
    ).extend(cv.polling_component_schema("1s")),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    light_conf = config[CONF_LIGHT]

    disp = cg.new_Pvariable(light_conf[CONF_OUTPUT_ID])
    backend = await cg.get_variable(config[CONF_BACKEND])
    cg.add(disp.set_backend_state(backend))

    await light.register_light(disp, light_conf)
    await cg.register_component(disp, light_conf)

    matrix_state = await cg.get_variable(light_conf[CONF_ID])
    cg.add(var.set_light_state(matrix_state))

    colors_cfg = config[CONF_COLORS]
    for key, kind in COLORS_ENUM.items():
        h8, s8, v8 = _hsv8_from_fragment(colors_cfg.get(key, {}), COLOR_DEFAULTS[key])
        cg.add(var.set_color_hsv(kind, h8, s8, v8))

    cg.add(var.set_done_timeout_ms(config[CONF_DONE_TIMEOUT]))

    ha_cfg = config.get(CONF_HOMEASSISTANT, {})
    state_mappings = ha_cfg.get(CONF_PRINT_STATUSES, [])
    cg.add_define("HEAT_WORD_MAX_PRINT_STATES", max(len(state_mappings), 1))

    if ha_cfg:
        if CONF_PRINT_STATUS_ENTITY in ha_cfg:
            cg.add(var.set_print_status_entity(ha_cfg[CONF_PRINT_STATUS_ENTITY]))
        if CONF_REMAIN_TIME_ENTITY in ha_cfg:
            cg.add(var.set_remain_time_entity(ha_cfg[CONF_REMAIN_TIME_ENTITY]))
        if CONF_ELAPSED_TIME_ENTITY in ha_cfg:
            cg.add(var.set_elapsed_time_entity(ha_cfg[CONF_ELAPSED_TIME_ENTITY]))
        for ha_status, esp_state in state_mappings.items():
            cg.add(var.add_print_status(ha_status, PRINT_STATE_ENUM[esp_state.lower()]))

    await cg.register_component(var, config)


@automation.register_action(
    "heat_word.set_brightness",
    HeatWordSetBrightnessAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(HeatWordComponent),
            cv.Required(CONF_BRIGHTNESS): cv.templatable(cv.percentage),
        }
    ),
    synchronous=True,
)
async def heat_word_set_brightness_to_code(config, action_id, template_arg, args):
    component = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, component)
    template_ = await cg.templatable(config[CONF_BRIGHTNESS], args, float)
    cg.add(var.set_brightness(template_))
    return var
