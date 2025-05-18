import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_RX_PIN, CONF_TX_PIN
from esphome.pins import PIN_SCHEMA_FOR_INPUT, PIN_SCHEMA_FOR_OUTPUT

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
            cv.Required(
                CONF_RX_PIN
            ): PIN_SCHEMA_FOR_INPUT,  # Input pin for edge detection
            cv.Required(CONF_TX_PIN): PIN_SCHEMA_FOR_OUTPUT,  # Output pin for toggling
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))

    tx_pin = await cg.gpio_pin_expression(
        config[CONF_TX_PIN]
    )  # just like uart __init.py__
    cg.add(var.set_tx_pin(tx_pin))  # just like uart __init.py__
