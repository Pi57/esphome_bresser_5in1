#include "esphome/core/log.h"
#include "bresser_5in1.h"
#include "RadioLib.h"

namespace esphome {
namespace bresser_5in1 {

static const char *const TAG = "bresser_5in1";

// Offsets dans le message décodé (après suppression du dernier octet de sync)
static const uint8_t OFFSET_CHECKSUM = 13;
static const uint8_t OFFSET_SENSOR_ID = 14;
static const uint8_t OFFSET_WIND_GUST_LO = 16;
static const uint8_t OFFSET_WIND_GUST_HI = 17;
static const uint8_t OFFSET_WIND_DIR = 17;
static const uint8_t OFFSET_WIND_AVG_LO = 18;
static const uint8_t OFFSET_WIND_AVG_HI = 19;
static const uint8_t OFFSET_TEMP_LO = 20;
static const uint8_t OFFSET_TEMP_HI = 21;
static const uint8_t OFFSET_HUMIDITY = 22;
static const uint8_t OFFSET_RAIN_LO = 23;
static const uint8_t OFFSET_RAIN_HI = 24;
static const uint8_t OFFSET_TEMP_SIGN_BATTERY = 25;
static const uint8_t CHECKSUM_START = 14;

volatile bool Bresser5in1Component::received_flag_ = false;

uint8_t Bresser5in1Component::verify_checksum(const uint8_t *msg, uint8_t msgSize) const
{
  // Verify checksum (number of bits set in bytes 14-25)
  uint8_t bitsSet = 0;
  for (uint8_t p = CHECKSUM_START; p < msgSize; p++)
  {
    uint8_t currentByte = msg[p];
    while (currentByte)
    {
      bitsSet += (currentByte & 1);
      currentByte >>= 1;
    }
  }
  return bitsSet;
}

int8_t Bresser5in1Component::verify_parity(const uint8_t *msg, uint8_t msgSize) const
{
  // First 13 bytes need to match inverse of last 13 bytes
  for (unsigned col = 0; col < msgSize / 2; ++col)
  {
    if ((msg[col] ^ msg[col + 13]) != 0xFF)
    {
      return col;
    }
  }
  return -1;
}

uint8_t Bresser5in1Component::get_sensor_id(const uint8_t *msg) const
{ 
  return msg[OFFSET_SENSOR_ID]; 
}

float Bresser5in1Component::get_temperature(const uint8_t *msg) const
{
  int temp_raw = (msg[OFFSET_TEMP_LO] & 0x0F) + ((msg[OFFSET_TEMP_LO] & 0xF0) >> 4) * 10 +
                 (msg[OFFSET_TEMP_HI] & 0x0F) * 100;
  if (msg[OFFSET_TEMP_SIGN_BATTERY] & 0x0F)
  {
    temp_raw = -temp_raw;
  }

  return temp_raw * 0.1f;
}

int Bresser5in1Component::get_humidity(const uint8_t *msg) const
{ 
  return (msg[OFFSET_HUMIDITY] & 0x0F) + ((msg[OFFSET_HUMIDITY] & 0xF0) >> 4) * 10; 
}

float Bresser5in1Component::get_wind_direction(const uint8_t *msg) const
{ 
  return ((msg[OFFSET_WIND_DIR] & 0xF0) >> 4) * 22.5f; 
}

float Bresser5in1Component::get_wind_gust(const uint8_t *msg) const
{
  int gust_raw = ((msg[OFFSET_WIND_GUST_HI] & 0x0F) << 8) + msg[OFFSET_WIND_GUST_LO];

  return gust_raw * 0.1f;
}

float Bresser5in1Component::get_wind_average(const uint8_t *msg) const
{
  int wind_raw = (msg[OFFSET_WIND_AVG_LO] & 0x0F) + ((msg[OFFSET_WIND_AVG_LO] & 0xF0) >> 4) * 10 +
                 (msg[OFFSET_WIND_AVG_HI] & 0x0F) * 100;

  return wind_raw * 0.1f;
}

float Bresser5in1Component::get_rain(const uint8_t *msg) const
{
  int rain_raw = (msg[OFFSET_RAIN_LO] & 0x0F) + ((msg[OFFSET_RAIN_LO] & 0xF0) >> 4) * 10 +
                 (msg[OFFSET_RAIN_HI] & 0x0F) * 100;

  return rain_raw * 0.1f;
}

bool Bresser5in1Component::get_battery(const uint8_t *msg) const
{ 
  return (msg[OFFSET_TEMP_SIGN_BATTERY] & 0x80) ? false : true; 
}

DecodeStatus Bresser5in1Component::decode_bresser_5in1(const uint8_t *msg, uint8_t msgSize, WeatherData *pWeatherData) const
{
  int8_t parity = verify_parity(msg, msgSize);

  if (parity != -1)
  {
    ESP_LOGE(TAG, "%s: Parity wrong at %u", __func__, parity);
    return DECODE_PARITY_ERR;
  }

  uint8_t expectedBitsSet = msg[OFFSET_CHECKSUM];
  uint8_t bitsSet = verify_checksum(msg, msgSize);

  if (bitsSet != expectedBitsSet)
  {
    ESP_LOGE(TAG, "%s: Checksum wrong : actual [%02X] != expected [%02X]", __func__, bitsSet,
             expectedBitsSet);
    return DECODE_CHECKSUM_ERR;
  }

  pWeatherData->sensor_id = get_sensor_id(msg);
  pWeatherData->temp_celsius = get_temperature(msg);
  pWeatherData->humidity = get_humidity(msg);
  pWeatherData->wind_direction_degre = get_wind_direction(msg);
  pWeatherData->wind_gust_meter_sec = get_wind_gust(msg);
  pWeatherData->wind_avg_meter_sec = get_wind_average(msg);
  pWeatherData->rain_mm = get_rain(msg);
  pWeatherData->battery_ok = get_battery(msg);

  return DECODE_OK;
}

IRAM_ATTR
void Bresser5in1Component::setFlag(void)
{
  received_flag_ = true; 
}

void Bresser5in1Component::setup()
{
  uint32_t cs = cs_pin_->get_pin();
  uint32_t gd0 = gd0_pin_->get_pin();
  uint32_t gd2 = gd2_pin_->get_pin();

  radio_ = new CC1101(new Module(cs, gd0, RADIOLIB_NC, gd2));

  ESP_LOGI(TAG, "[CC1101] Initializing ... ");

  int state = radio_->begin(868.35, 8.22, 57.136417, 270.0, 10, 32);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error initialising: [%d]", state);
    this->mark_failed();
    return;
  }

  state = radio_->setCrcFiltering(false);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error disabling crc filtering: [%d]", state);
    this->mark_failed();
    return;
  }

  state = radio_->fixedPacketLengthMode(MAX_PACKET_LENGTH);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error setting fixed packet length: [%d]", state);
    this->mark_failed();
    return;
  }

  // Preamble: AA AA AA AA AA
  // Sync is: 2D D4
  // Preamble 40 bits but the CC1101 doesn't allow us to set that
  // so we use a preamble of 32 bits and then use the sync as AA 2D
  // which then uses the last byte of the preamble - we recieve the last sync byte
  // as the 1st byte of the payload.
  state = radio_->setSyncWord(0xAA, 0x2D, 0, false);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error setting sync words: [%d]", state);
    this->mark_failed();
    return;
  }

  radio_->setPacketReceivedAction(setFlag);

  state = radio_->startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error start receive: [%d]", state);
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "[CC1101] Setup complete - awaiting incoming messages...");
}

void Bresser5in1Component::loop()
{
  if (received_flag_ && sensor_id_ != nullptr)
  {
    int state = radio_->readData(recv_data_, MAX_PACKET_LENGTH);
    if (state == RADIOLIB_ERR_NONE)
    {
      // Verify last syncword is 1st byte of payload (saa above)
      if (recv_data_[0] == 0xD4)
      {
#ifdef _DEBUG_MODE_
        // print the data of the packet
        ESP_LOGD("bresser_5in1", "[CC1101] Data:\t\t");
        for (int i = 0; i < sizeof(recv_data_); i++)
        {
          ESP_LOGD("bresser_5in1", " %02X", recv_data_[i]);
        }

        ESP_LOGD("bresser_5in1", "[CC1101] R [0x%02X] RSSI: %f LQI: %d", recv_data_[0], radio_->getRSSI(), radio_->getLQI());
#endif
        // Decode the information - skip the last sync byte we use to check the data is OK
        WeatherData weatherData{};
        if (decode_bresser_5in1(&recv_data_[1], sizeof(recv_data_) - 1, &weatherData) == DECODE_OK)
        {
          if(weatherData.sensor_id == sensor_id_->state)
          {
            ESP_LOGI(TAG,
                   "Type: [Bresser-5in1] "
                   "Id: [%d] "
                   "Battery: [%s] "
                   "Temp: [%.1f °C] "
                   "Hum: [%d %%] "
                   "Wind Gust: [%.1f km/h] "
                   "Speed: [%.1f km/h] "
                   "Direction: [%.1f °] "
                   "Rain [%.1f mm]",
                   weatherData.sensor_id, weatherData.battery_ok ? "OK" : "Low", weatherData.temp_celsius,
                   weatherData.humidity, weatherData.wind_gust_meter_sec * METERS_SEC_TO_KMPH,
                   weatherData.wind_avg_meter_sec * METERS_SEC_TO_KMPH, weatherData.wind_direction_degre,
                   weatherData.rain_mm);

            if (temperature_sensor_ != nullptr)
            {
              temperature_sensor_->publish_state(weatherData.temp_celsius);
            }

            if (humidity_sensor_ != nullptr)
            {
              humidity_sensor_->publish_state(weatherData.humidity);
            }

            if (wind_speed_sensor_ != nullptr)
            {
              wind_speed_sensor_->publish_state(weatherData.wind_avg_meter_sec);
            }

            if (wind_gust_sensor_ != nullptr)
            {
              wind_gust_sensor_->publish_state(weatherData.wind_gust_meter_sec);
            }

            if (wind_direction_sensor_ != nullptr)
            {
              wind_direction_sensor_->publish_state(weatherData.wind_direction_degre);
            }

            if (rain_sensor_ != nullptr)
            {
              rain_sensor_->publish_state(weatherData.rain_mm);
            }

            if (battery_sensor_ != nullptr)
            {
              battery_sensor_->publish_state(weatherData.battery_ok);
            }
          }
          else
          {
            ESP_LOGI(TAG, "Type: [Bresser-5in1] Id: [%d]", weatherData.sensor_id);
          }
        }
      }
#ifdef _DEBUG_MODE_
      else
      {
        ESP_LOGD("bresser_5in1", "[CC1101] R [0x%02X] RSSI: %f LQI: %d", recv_data_[0], radio_->getRSSI(), radio_->getLQI());
      }
#endif
    }
#ifdef _DEBUG_MODE_
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
      ESP_LOGD("bresser_5in1", "T");
    }
#endif
    else
    {
      // some other error occurred
      ESP_LOGE(TAG, "[CC1101] Receive failed - failed, code %d", state);
    }

    received_flag_ = false;
    radio_->startReceive();
  }
}

}  // namespace bresser_5in1
}  // namespace esphome