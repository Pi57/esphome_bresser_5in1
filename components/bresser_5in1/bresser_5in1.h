// esphome-bresser-5in1.h
#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"
#include "RadioLib.h"

namespace esphome {
namespace bresser_5in1 {

static const uint8_t MAX_PACKET_LENGTH = 27;
static const float METERS_SEC_TO_KMPH = 3.6;

enum DecodeStatus 
{ 
  DECODE_OK, 
  DECODE_PARITY_ERR, 
  DECODE_CHECKSUM_ERR 
};

struct WeatherData 
{
  uint8_t sensor_id;
  float temp_celsius;
  int humidity;
  float wind_direction_degre;
  float wind_gust_meter_sec;
  float wind_avg_meter_sec;
  float rain_mm;
  bool battery_ok;
};

class Bresser5in1Component : public Component 
{
  public:
    void setup() override;
    void loop() override;

    void set_cs_pin(InternalGPIOPin *pin) { 
      this->cs_pin_ = pin; 
    }
    void set_gd0_pin(InternalGPIOPin *pin) { 
      this->gd0_pin_ = pin; 
    }
    void set_gd2_pin(InternalGPIOPin *pin) { 
      this->gd2_pin_ = pin; 
    }

    void set_temperature_sensor(sensor::Sensor *sensor) { 
      this->temperature_sensor_ = sensor; 
    }
    void set_humidity_sensor(sensor::Sensor *sensor) { 
      this->humidity_sensor_ = sensor; 
    }
    void set_wind_speed_sensor(sensor::Sensor *sensor) { 
      this->wind_speed_sensor_ = sensor; 
    }
    void set_wind_gust_sensor(sensor::Sensor *sensor) { 
      this->wind_gust_sensor_ = sensor; 
    }
    void set_wind_direction_sensor(sensor::Sensor *sensor) { 
      this->wind_direction_sensor_ = sensor; 
    }
    void set_rain_sensor(sensor::Sensor *sensor) { 
      this->rain_sensor_ = sensor; 
    }
    void set_battery_sensor(sensor::Sensor *sensor) { 
      this->battery_sensor_ = sensor; 
    }

    void set_sensor_id(number::Number *sensor_id) {
      this->sensor_id_ = sensor_id;
    }

  protected:
    static void setFlag();

    number::Number *sensor_id_{nullptr};

    InternalGPIOPin *cs_pin_{nullptr};
    InternalGPIOPin *gd0_pin_{nullptr};
    InternalGPIOPin *gd2_pin_{nullptr};

    uint8_t verify_checksum(uint8_t *msg, uint8_t msgSize);
    int8_t verify_parity(uint8_t *msg, uint8_t msgSize);

    uint8_t get_sensor_id(uint8_t *msg);
    float get_temperature(uint8_t *msg);
    int get_humidity(uint8_t *msg);
    float get_wind_direction(uint8_t *msg);
    float get_wind_gust(uint8_t *msg);
    float get_wind_average(uint8_t *msg);
    float get_rain(uint8_t *msg);
    bool get_battery(uint8_t *msg);

    DecodeStatus decode_bresser_5in1(uint8_t *msg, uint8_t msgSize, WeatherData *pOut);

    sensor::Sensor *temperature_sensor_{nullptr};
    sensor::Sensor *humidity_sensor_{nullptr};
    sensor::Sensor *wind_speed_sensor_{nullptr};
    sensor::Sensor *wind_gust_sensor_{nullptr};
    sensor::Sensor *wind_direction_sensor_{nullptr};
    sensor::Sensor *rain_sensor_{nullptr};
    sensor::Sensor *battery_sensor_{nullptr};
};

}  // namespace bresser_5in1
}  // namespace esphome