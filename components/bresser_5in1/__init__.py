import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi, sensor
from esphome import pins
from esphome.const import *
from esphome.core import CORE

CODEOWNERS = ["@Pi57"]
DEPENDENCIES = ["spi"]
MULTI_CONF = False

bresser_5in1_ns = cg.esphome_ns.namespace("bresser_5in1")
Bresser5in1Component = bresser_5in1_ns.class_("Bresser5in1Component", cg.Component, spi.SPIDevice)

CONF_BRESSER_5IN1_ID = "conf_bresser_5in1_id"

CONF_CS_PIN = "cs_pin"
CONF_GD0_PIN = "gd0_pin"
CONF_GD2_PIN = "gd2_pin"

CONF_BATTERY = "battery"
CONF_RAIN = "rain"
CONF_WIND_DIRECTION = "wind_direction"
CONF_WIND_GUST = "wind_gust"


Bresser5in1Component_SCHEMA = cv.Schema(
  {
    cv.GenerateID(CONF_BRESSER_5IN1_ID): cv.use_id(Bresser5in1Component),
  }
)

CONFIG_SCHEMA = (
  cv.Schema(
    {
      cv.GenerateID(): cv.declare_id(Bresser5in1Component),
      cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
      cv.Required(CONF_GD0_PIN): pins.gpio_output_pin_schema,
      cv.Required(CONF_GD2_PIN): pins.gpio_output_pin_schema,
      cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
        unit_of_measurement="°C",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
      cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
        unit_of_measurement="%",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_HUMIDITY,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
      cv.Optional(CONF_WIND_SPEED): sensor.sensor_schema(
        unit_of_measurement="m/s",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_WIND_SPEED,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
      cv.Optional(CONF_WIND_GUST): sensor.sensor_schema(
        unit_of_measurement="m/s",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_WIND_SPEED,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
      cv.Optional(CONF_WIND_DIRECTION): sensor.sensor_schema(
        unit_of_measurement="°",
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_WIND_DIRECTION,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
      cv.Optional(CONF_RAIN): sensor.sensor_schema(
        unit_of_measurement="mm",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRECIPITATION,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
      cv.Optional(CONF_BATTERY): sensor.sensor_schema(
        unit_of_measurement="%",
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
      ),
    }
  )
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
  cg.add(var.set_cs_pin(cs))

  gd0 = await cg.gpio_pin_expression(config[CONF_GD0_PIN])
  cg.add(var.set_gd0_pin(gd0))

  gd2 = await cg.gpio_pin_expression(config[CONF_GD2_PIN])
  cg.add(var.set_gd2_pin(gd2))

  if CONF_TEMPERATURE in config:
    sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
    cg.add(var.set_temperature_sensor(sens))

  if CONF_HUMIDITY in config:
    sens = await sensor.new_sensor(config[CONF_HUMIDITY])
    cg.add(var.set_humidity_sensor(sens))

  if CONF_WIND_SPEED in config:
    sens = await sensor.new_sensor(config[CONF_WIND_SPEED])
    cg.add(var.set_wind_speed_sensor(sens))

  if CONF_WIND_GUST in config:
    sens = await sensor.new_sensor(config[CONF_WIND_GUST])
    cg.add(var.set_wind_gust_sensor(sens))

  if CONF_WIND_DIRECTION in config:
    sens = await sensor.new_sensor(config[CONF_WIND_DIRECTION])
    cg.add(var.set_wind_direction_sensor(sens))

  if CONF_RAIN in config:
    sens = await sensor.new_sensor(config[CONF_RAIN])
    cg.add(var.set_rain_sensor(sens))

  if CONF_BATTERY in config:
    sens = await sensor.new_sensor(config[CONF_BATTERY])
    cg.add(var.set_battery_sensor(sens))
    