from esphome import pins
import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_RX_PIN, CONF_TX_PIN

edge_counter_ns = cg.esphome_ns.namespace("edge_counter")
EdgeCounterSensor = edge_counter_ns.class_(
    "EdgeCounterSensor", sensor.Sensor, cg.Component
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        EdgeCounterSensor,
    )
    .extend(
        {
            cv.Required(CONF_RX_PIN): cv.All(pins.internal_gpio_input_pin_schema),
            cv.Required(CONF_TX_PIN): cv.All(pins.internal_gpio_output_pin_schema),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))

    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(tx_pin))
