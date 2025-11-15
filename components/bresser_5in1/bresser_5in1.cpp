#include "esphome/core/log.h"
#include "bresser_5in1.h"
#include "RadioLib.h"

namespace esphome {
namespace bresser_5in1 {

static const char *const TAG = "bresser_5in1";

CC1101 *radio;
volatile bool receivedFlag = false;

uint8_t Bresser5in1Component::verify_checksum(uint8_t *msg, uint8_t msgSize)
{
  // Verify checksum (number number bits set in bytes 14-25)
  uint8_t bitsSet = 0;
  for (uint8_t p = 14; p < msgSize; p++)
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

int8_t Bresser5in1Component::verify_parity(uint8_t *msg, uint8_t msgSize)
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

uint8_t Bresser5in1Component::get_sensor_id(uint8_t *msg)
{ 
  return msg[14]; 
}

float Bresser5in1Component::get_temperature(uint8_t *msg)
{
  int temp_raw = (msg[20] & 0x0F) + ((msg[20] & 0xF0) >> 4) * 10 + (msg[21] & 0x0F) * 100;
  if (msg[25] & 0x0F)
  {
    temp_raw = -temp_raw;
  }

  return temp_raw * 0.1f;
}

int Bresser5in1Component::get_humidity(uint8_t *msg)
{ 
  return (msg[22] & 0x0F) + ((msg[22] & 0xF0) >> 4) * 10; 
}

float Bresser5in1Component::get_wind_direction(uint8_t *msg)
{ 
  return ((msg[17] & 0xF0) >> 4) * 22.5f; 
}

float Bresser5in1Component::get_wind_gust(uint8_t *msg)
{
  int gust_raw = ((msg[17] & 0x0F) << 8) + msg[16];

  return gust_raw * 0.1f;
}

float Bresser5in1Component::get_wind_average(uint8_t *msg)
{
  int wind_raw = (msg[18] & 0x0F) + ((msg[18] & 0xF0) >> 4) * 10 + (msg[19] & 0x0F) * 100;

  return wind_raw * 0.1f;
}

float Bresser5in1Component::get_rain(uint8_t *msg)
{
  int rain_raw = (msg[23] & 0x0F) + ((msg[23] & 0xF0) >> 4) * 10 + (msg[24] & 0x0F) * 100;

  return rain_raw * 0.1f;
}

bool Bresser5in1Component::get_battery(uint8_t *msg)
{ 
  return (msg[25] & 0x80) ? false : true; 
}

DecodeStatus Bresser5in1Component::decode_bresser_5in1(uint8_t *msg, uint8_t msgSize, WeatherData *pWeatherData)
{
  int8_t parity = verify_parity(msg, msgSize);

  if (parity != -1)
  {
    ESP_LOGE(TAG, "%s: Parity wrong at %u\n", __func__, parity);
    return DECODE_PARITY_ERR;
  }

  uint8_t expectedBitsSet = msg[13];
  uint8_t bitsSet = verify_checksum(msg, msgSize);

  if (bitsSet != expectedBitsSet)
  {
    ESP_LOGE(TAG, "%s: Checksum wrong : actual [%02X] != expected [%02X]\n", __func__, bitsSet,
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

ICACHE_RAM_ATTR
void Bresser5in1Component::setFlag(void)
{
  receivedFlag = true; 
}

void Bresser5in1Component::setup()
{
  uint32_t cs = cs_pin_->get_pin();
  uint32_t gd0 = gd0_pin_->get_pin();
  uint32_t gd2 = gd2_pin_->get_pin();

  radio = new CC1101(new Module(cs, gd0, RADIOLIB_NC, gd2));

  ESP_LOGI(TAG, "[CC1101] Initializing ... ");

  int state = radio->begin(868.35, 8.22, 57.136417, 270.0, 10, 32);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error initialising: [%d]\n", state);
    while (true) {
    };
  }

  state = radio->setCrcFiltering(false);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error disabling crc filtering: [%d]\n", state);
    while (true) {
    };
  }

  state = radio->fixedPacketLengthMode(MAX_PACKET_LENGTH);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error setting fixed packet length: [%d]\n", state);
    while (true) {
    };
  }

  // Preamble: AA AA AA AA AA
  // Sync is: 2D D4
  // Preamble 40 bits but the CC1101 doesn't allow us to set that
  // so we use a preamble of 32 bits and then use the sync as AA 2D
  // which then uses the last byte of the preamble - we recieve the last sync byte
  // as the 1st byte of the payload.
  state = radio->setSyncWord(0xAA, 0x2D, 0, false);
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error setting sync words: [%d]\n", state);
    while (true) {
    };
  }

  radio->setPacketReceivedAction(setFlag);

  state = radio->startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    ESP_LOGE(TAG, "[CC1101] Error start receive: [%d]\n", state);
    while (true)
    {
    };
  }

  ESP_LOGI(TAG, "[CC1101] Setup complete - awaiting incoming messages...");
}

void Bresser5in1Component::loop()
{
  if (receivedFlag & sensor_id_ != nullptr)
  {
    uint8_t recvData[MAX_PACKET_LENGTH];

    int state = radio->readData(recvData, MAX_PACKET_LENGTH);
    if (state == RADIOLIB_ERR_NONE)
    {
      // Verify last syncword is 1st byte of payload (saa above)
      if (recvData[0] == 0xD4)
      {
#ifdef _DEBUG_MODE_
        // print the data of the packet
        ESP_LOGD("bresser_5in1", "[CC1101] Data:\t\t");
        for (int i = 0; i < sizeof(recvData); i++)
        {
          ESP_LOGD("bresser_5in1", " %02X", recvData[i]);
        }

        ESP_LOGD("bresser_5in1", "[CC1101] R [0x%02X] RSSI: %f LQI: %d\n", recvData[0], radio.getRSSI(), radio.getLQI());
#endif
        // Decode the information - skip the last sync byte we use to check the data is OK
        WeatherData weatherData = {0};
        if (decode_bresser_5in1(&recvData[1], sizeof(recvData) - 1, &weatherData) == DECODE_OK)
        {
          if(weatherData.sensor_id == sensor_id_->state)
          {
            ESP_LOGI(TAG,
                   "Type: [Bresser-5in1]\n"
                   "Id: [%d]\n"
                   "Battery: [%s]\n"
                   "Temp: [%.1f °C]\n"
                   "Hum: [%d %%]\n"
                   "Wind Gust: [%.1f km/h]\n"
                   "Speed: [%.1f km/h]\n"
                   "Direction: [%.1f °]\n"
                   "Rain [%.1f mm]\n",
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
            ESP_LOGI(TAG, "Type: [Bresser-5in1]\nId: [%d]\n", weatherData.sensor_id);
          }
        }
      }
#ifdef _DEBUG_MODE_
      else
      {
        ESP_LOGD("bresser_5in1", "[CC1101] R [0x%02X] RSSI: %f LQI: %d\n", recvData[0], radio.getRSSI(), radio.getLQI());
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
      ESP_LOGE(TAG, "[CC1101] Receive failed - failed, code %d\n", state);
    }

    receivedFlag = false;
    radio->startReceive();
  }
}

}  // namespace bresser_5in1
}  // namespace esphome