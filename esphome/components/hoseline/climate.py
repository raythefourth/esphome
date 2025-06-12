from esphome import pins
import esphome.codegen as cg
from esphome.components import climate
import esphome.config_validation as cv
from esphome.const import CONF_RX_PIN, CONF_TX_PIN

hoseline_ns = cg.esphome_ns.namespace("hoseline")
Hoseline = hoseline_ns.class_("Hoseline", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(Hoseline)
    .extend(
        {
            cv.Required(CONF_RX_PIN): cv.All(pins.internal_gpio_input_pin_schema),
            cv.Required(CONF_TX_PIN): cv.All(pins.internal_gpio_output_pin_schema),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))

    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(tx_pin))
