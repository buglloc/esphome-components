from esphome import automation, core
import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_COUNT,
    CONF_DURATION,
    CONF_ID,
    CONF_ON_ERROR,
    CONF_PRESET,
    CONF_TIMEOUT,
)
from esphome.types import ConfigType

CODEOWNERS = ["@buglloc"]

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor"]

CONF_ON_MS = "on_ms"
CONF_OFF_MS = "off_ms"
CONF_PORTIONS = "portions"
CONF_SEND_TIMEOUT = "send_timeout"

CONF_ON_COMPLETE = "on_complete"
CONF_WAIT_FOR_COMPLETE = "wait_for_complete"

pkt_p530_ns = cg.esphome_ns.namespace("pkt_p530")
P530Component = pkt_p530_ns.class_("P530Component", cg.Component, uart.UARTDevice)

ErrorCode = pkt_p530_ns.enum("ErrorCode", is_class=True)

LedUpperAction = pkt_p530_ns.class_("LedUpperAction", automation.Action)
LedLowerAction = pkt_p530_ns.class_("LedLowerAction", automation.Action)
BeepAction = pkt_p530_ns.class_("BeepAction", automation.Action)

InitAction = pkt_p530_ns.class_("InitAction", automation.Action)

DoorOpenAction = pkt_p530_ns.class_("DoorOpenAction", automation.Action)
DoorCloseAction = pkt_p530_ns.class_("DoorCloseAction", automation.Action)
DispenseAction = pkt_p530_ns.class_("DispenseAction", automation.Action)

IsReadyCondition = pkt_p530_ns.class_("IsReadyCondition", automation.Condition)
HasFoodCondition = pkt_p530_ns.class_("HasFoodCondition", automation.Condition)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(P530Component),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config: ConfigType):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)


# ============== Actions ==============

BASE_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(P530Component),
        cv.Optional(
            CONF_SEND_TIMEOUT, default="1s"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_ON_COMPLETE): automation.validate_action_list,
        cv.Optional(CONF_ON_ERROR): automation.validate_action_list,
        cv.Optional(CONF_WAIT_FOR_COMPLETE, default=True): cv.boolean,
    }
)


async def base_action_code(
    config: ConfigType,
    action_id: core.ID,
    template_arg: cg.TemplateArguments,
    args: list[tuple],
) -> cg.Pvariable:
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])

    cg.add(var.set_send_timeout(config[CONF_SEND_TIMEOUT]))
    cg.add(var.set_wait_for_complete(config[CONF_WAIT_FOR_COMPLETE]))
    # cg.add(var.set_continue_on_error(CONF_ON_ERROR in config))

    if on_complete_config := config.get(CONF_ON_COMPLETE):
        actions = await automation.build_action_list(
            on_complete_config, template_arg, args
        )
        cg.add(var.add_on_complete(actions))

    if on_error_config := config.get(CONF_ON_ERROR):
        actions = await automation.build_action_list(
            on_error_config, template_arg, args
        )
        cg.add(var.add_on_error(actions))

    return var


# ============== LED Control Actions ==============

LED_PRESETS = {
    "custom": {
        CONF_ON_MS: 0,
        CONF_OFF_MS: 0,
        CONF_COUNT: 0,
    },
    "steady_on": {
        CONF_ON_MS: 65535,
        CONF_OFF_MS: 0,
        CONF_COUNT: 65535,
    },
    "steady_off": {
        CONF_ON_MS: 0,
        CONF_OFF_MS: 65535,
        CONF_COUNT: 65535,
    },
    "blink": {
        CONF_ON_MS: 1000,
        CONF_OFF_MS: 1000,
        CONF_COUNT: 65535,
    },
    "blink_fast": {
        CONF_ON_MS: 500,
        CONF_OFF_MS: 500,
        CONF_COUNT: 65535,
    },
}

BEEP_PRESETS = {
    "custom": {
        CONF_ON_MS: 0,
        CONF_OFF_MS: 0,
        CONF_COUNT: 0,
    },
    "single": {
        CONF_ON_MS: 200,
        CONF_OFF_MS: 0,
        CONF_COUNT: 1,
    },
    "short": {
        CONF_ON_MS: 200,
        CONF_OFF_MS: 200,
        CONF_COUNT: 3,
    },
    "long": {
        CONF_ON_MS: 500,
        CONF_OFF_MS: 500,
        CONF_COUNT: 10,
    },
}

BASE_LED_CTL_SCHEMA = BASE_ACTION_SCHEMA.extend(
    {
        cv.Optional(CONF_ON_MS): cv.int_range(min=0, max=65535),
        cv.Optional(CONF_OFF_MS): cv.int_range(min=0, max=65535),
        cv.Optional(CONF_COUNT): cv.int_range(min=0, max=65535),
    }
)

LED_CTL_SCHEMA = cv.maybe_simple_value(
    BASE_LED_CTL_SCHEMA.extend(
        {
            cv.Optional(CONF_PRESET, default="custom"): cv.enum(LED_PRESETS),
        }
    ),
    key=CONF_PRESET,
)

BEEP_CTL_SCHEMA = cv.maybe_simple_value(
    BASE_LED_CTL_SCHEMA.extend(
        {
            cv.Optional(CONF_PRESET, default="custom"): cv.enum(BEEP_PRESETS),
        }
    ),
    key=CONF_PRESET,
)


async def led_preset_to_code(config, var, source):
    preset = dict(source[config[CONF_PRESET]])
    for conf_name in [CONF_ON_MS, CONF_OFF_MS, CONF_COUNT]:
        if conf_name in config:
            preset[conf_name] = config[conf_name]

    cg.add(var.set_on_ms(preset[CONF_ON_MS]))
    cg.add(var.set_off_ms(preset[CONF_OFF_MS]))
    cg.add(var.set_count(preset[CONF_COUNT]))
    return var


@automation.register_action("pkt_p530.led_upper", LedUpperAction, LED_CTL_SCHEMA)
@automation.register_action("pkt_p530.led_lower", LedLowerAction, LED_CTL_SCHEMA)
async def pkt_p530_led_ctl_to_code(config, action_id, template_arg, args):
    var = await base_action_code(config, action_id, template_arg, args)
    return await led_preset_to_code(config, var, LED_PRESETS)


@automation.register_action("pkt_p530.beep", BeepAction, BEEP_CTL_SCHEMA)
async def pkt_p530_beep_ctl_to_code(config, action_id, template_arg, args):
    var = await base_action_code(config, action_id, template_arg, args)
    return await led_preset_to_code(config, var, BEEP_PRESETS)


# ================ Feed actions ==============

DOOR_ACTION_SCHEMA = BASE_ACTION_SCHEMA.extend(
    {
        cv.Optional(CONF_DURATION, default=0x1E): cv.int_range(min=0, max=255),
        cv.Optional(CONF_TIMEOUT, default="10s"): cv.positive_time_period_milliseconds,
    }
)


@automation.register_action("pkt_p530.door_open", DoorOpenAction, DOOR_ACTION_SCHEMA)
@automation.register_action("pkt_p530.door_close", DoorCloseAction, DOOR_ACTION_SCHEMA)
async def pkt_p530_open_door_to_code(config, action_id, template_arg, args):
    var = await base_action_code(config, action_id, template_arg, args)

    cg.add(var.set_duration(config[CONF_DURATION]))
    cg.add(var.set_timeout(config[CONF_TIMEOUT]))
    return var


DISPENSE_ACTION_SCHEMA = cv.maybe_simple_value(
    BASE_ACTION_SCHEMA.extend(
        {
            cv.Optional(CONF_PORTIONS, default=0x01): cv.templatable(
                cv.int_range(min=0, max=255)
            ),
        }
    ),
    key=CONF_PORTIONS,
)


@automation.register_action("pkt_p530.dispense", DispenseAction, DISPENSE_ACTION_SCHEMA)
async def pkt_p530_dispense_to_code(config, action_id, template_arg, args):
    var = await base_action_code(config, action_id, template_arg, args)

    template_ = await cg.templatable(config[CONF_PORTIONS], args, cg.uint8)
    cg.add(var.set_portions(template_))
    return var


# ============== Misc Actions ==============


@automation.register_action("pkt_p530.init", InitAction, BASE_ACTION_SCHEMA)
async def pkt_p530_init_action_to_code(config, action_id, template_arg, args):
    return await base_action_code(config, action_id, template_arg, args)


# ============== Conditions ==============

CONDITION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(P530Component),
    }
)


@automation.register_condition("pkt_p530.is_ready", IsReadyCondition, CONDITION_SCHEMA)
async def pkt_p530_is_ready_to_code(config, condition_id, template_arg, args):
    var = cg.new_Pvariable(condition_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_condition("pkt_p530.has_food", HasFoodCondition, CONDITION_SCHEMA)
async def pkt_p530_has_food_to_code(config, condition_id, template_arg, args):
    var = cg.new_Pvariable(condition_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
