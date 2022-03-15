#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>

#include "nmea.hpp"
#include "coord_utils.hpp"

extern "C" void app_main()
{
    {
      uart_config_t uart_config = {
          .baud_rate = 115200,
          .data_bits = UART_DATA_8_BITS,
          .parity = UART_PARITY_DISABLE,
          .stop_bits = UART_STOP_BITS_1,
          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      };
      ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
      ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
      ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 256, 0, NULL, 0));
    }

    {
      uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      };
      ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
      ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_27, GPIO_NUM_14, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
      ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 256, 256, 0, NULL, 0));
    }


    /*char buf[512];
    while(1) {
      const int read = uart_read_bytes(UART_NUM_1, buf, sizeof(buf), 0);
      if(read > 0) {
        uart_write_bytes(UART_NUM_0, buf, read);
      } else {
        vTaskDelay(1);
      }
    }*/

    NmeaParser parser(UART_NUM_1);

    float lastLat = 0, lastLon = 0;

    while(1) {
      auto msg = parser.readMessage();
      //printf("%s", msg.raw);

      switch(msg.type) {
      case NmeaMessageType::GGA: {
        auto& g = msg.gga;
        printf("                    HDOP: %f m altitude: %f separation: %f %f\n", g.HDOP, g.altitude_meters, g.geoid_separation,
          g.altitude_meters - g.geoid_separation);
        /*printf("                                                Date: %d:%d:%2.3f at %f %f alt %f, fix %d satelites %d\n",
          g.time.hours, g.time.minutes, g.time.seconds,
          g.latitude, g.longitude, g.altitude_meters,
          g.fix_indicator, g.used_satellites);
        
        auto dist = coords::distanceMeters(g.latitude, g.longitude, 49.1944522, 16.5995939);
        auto distf = coords::distanceMetersFast(g.latitude, g.longitude, 49.1944522, 16.5995939);
        auto bearing = coords::bearingInitial(g.latitude, g.longitude, 49.1944522, 16.5995939) + rand()%10;
        printf("                                                    Distance to spilberk: %f / %f km, bearing: %f°\n", dist/1000, distf/1000, bearing);

        dist = coords::distanceMeters(g.latitude, g.longitude, 49.4875122, 16.6599706);
        distf = coords::distanceMetersFast(g.latitude, g.longitude, 49.4875122, 16.6599706);
        bearing = coords::bearingInitial(g.latitude, g.longitude, 49.4875122, 16.6599706) + rand()%10;
        printf("                                                    Distance to Boskovice: %f / %f km, bearing: %f°\n", dist / 1000, distf / 1000, bearing);
        

        dist = coords::distanceMeters(g.latitude, g.longitude, -44.6190189, 167.8687603);
        distf = coords::distanceMetersFast(g.latitude, g.longitude, -44.6190189, 167.8687603);
        bearing = coords::bearingInitial(g.latitude, g.longitude, -44.6190189, 167.8687603) + rand()%10;
        printf("                                                    Distance to Milford Sound: %f / %f km, bearing: %f°\n", dist / 1000, distf / 1000, bearing);
        */
        break;
      }
      case NmeaMessageType::RMC: {
        auto& r = msg.rmc;
        /*printf("                                                    Date: 20%d-%02d-%02d %2d:%02d:%2.3f Speed: %f knots (%f km/h) heading: %f\n",
                r.date.year, r.date.month, r.date.day, r.time.hours, r.time.minutes, r.time.seconds,
                r.speed_over_ground, r.speed_over_ground * 1.852f, r.course_over_ground);*/
        
        auto distFromLast = coords::distanceMeters(lastLat, lastLon, r.latitude, r.longitude);
        auto bearingFromLast = coords::bearingInitial(lastLat, lastLon, r.latitude, r.longitude);

        printf("Distance from last: %fm, heading: %f (%f), speed: %f km/h\n",
          distFromLast, bearingFromLast, r.course_over_ground, r.speed_over_ground * 1.852f);

        lastLat = r.latitude;
        lastLon = r.longitude;
        break;
      }
      default: break;
      }
    }
}
