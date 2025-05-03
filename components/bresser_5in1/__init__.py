import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi, sensor
from esphome import pins
from esphome.const import *
from esphome.core import CORE

DEPENDENCIES = ["spi"]

CONF_CS_PIN = "cs_pin"
CONF_GD0_PIN = "gd0_pin"
CONF_GD2_PIN = "gd2_pin"

bresser_5in1_ns = cg.esphome_ns.namespace("bresser_5in1")
Bresser_5in1 = bresser_5in1_ns.class_("Bresser_5in1", cg.Component, spi.SPIDevice)

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(Bresser_5in1),
        cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_GD0_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_GD2_PIN): pins.gpio_output_pin_schema,
        cv.Optional("temperature"): sensor.sensor_schema(
            unit_of_measurement="°C",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("humidity"): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("wind_speed"): sensor.sensor_schema(
            unit_of_measurement="m/s",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("wind_gust"): sensor.sensor_schema(
            unit_of_measurement="m/s",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("wind_direction"): sensor.sensor_schema(
            unit_of_measurement="°",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_WIND_DIRECTION_DEGREES,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("rain"): sensor.sensor_schema(
            unit_of_measurement="mm",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("battery"): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    cg.add_library("RadioLib", None)

    if CORE.using_arduino:
        cg.add_library("SPI", None)
        cg.add_platformio_option("lib_ldf_mode", "deep+")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    gd0 = await cg.gpio_pin_expression(config[CONF_GD0_PIN])
    gd2 = await cg.gpio_pin_expression(config[CONF_GD2_PIN])

    cg.add(var.set_cs_pin(cs))
    cg.add(var.set_gd0_pin(gd0))
    cg.add(var.set_gd2_pin(gd2))

    if "temperature" in config:
        sens = await sensor.new_sensor(config["temperature"])
        cg.add(var.set_temperature_sensor(sens))
    if "humidity" in config:
        sens = await sensor.new_sensor(config["humidity"])
        cg.add(var.set_humidity_sensor(sens))
    if "wind_speed" in config:
        sens = await sensor.new_sensor(config["wind_speed"])
        cg.add(var.set_wind_speed_sensor(sens))
    if "wind_gust" in config:
        sens = await sensor.new_sensor(config["wind_gust"])
        cg.add(var.set_wind_gust_sensor(sens))
    if "wind_direction" in config:
        sens = await sensor.new_sensor(config["wind_direction"])
        cg.add(var.set_wind_direction_sensor(sens))
    if "rain" in config:
        sens = await sensor.new_sensor(config["rain"])
        cg.add(var.set_rain_sensor(sens))
    if "battery" in config:
        sens = await sensor.new_sensor(config["battery"])
        cg.add(var.set_battery_sensor(sens))
    