/*

   Copyright (c) 2018 Roman Hujer

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#define Product "WeatherHR"
#define Version "1.0"

#include <ESP8266WiFi.h>
#include <Wire.h>


#include "Config.h"

#ifdef SENSOR_TYPE_BME280

#include <BME280I2C.h>
BME280I2C bme;
#endif

#ifdef SENSOR_TYPE_DHT11
#include <DHT.h>
#define DHT_PIN D2

DHT myDHT(DHT_PIN, DHT11);

#endif

#ifdef SENSOR_TYPE_SMT160

#define SMT_PIN D2

#endif

#define SERIAL_BAUD 74880
#define USE_SERIAL Serial

void setup()
{
  USE_SERIAL.begin(SERIAL_BAUD);
  USE_SERIAL.println();
  Wire.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  USE_SERIAL.printf("Wait for WiFi.");
  for (uint8_t t = 10; t > 0; t--) {
    if ( WiFi.status() == WL_CONNECTED ) break;
    USE_SERIAL.print(".");
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    USE_SERIAL.printf("\nWIFI not connect. Now sleep retry next 60sec...");
    ESP.deepSleep(60e6);
  }

  USE_SERIAL.println("");
  USE_SERIAL.println("WiFi connected");
  USE_SERIAL.print("IP address: ");
  USE_SERIAL.println(WiFi.localIP());

}

void loop()
{
#ifdef SENSOR_TYPE_BME280
  USE_SERIAL.println("Sensor BME280");
  getBME280Data(&USE_SERIAL);
#endif

#ifdef SENSOR_TYPE_DHT11

  USE_SERIAL.println("Sensor DHT11");
  getDHT11Data(&USE_SERIAL);

#endif

#ifdef SENSOR_TYPE_SMT160

  USE_SERIAL.println("Sensor SMT160");
  getSMT160Data(&USE_SERIAL);

#endif

  /// WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED.
  USE_SERIAL.println("WiFi disconnet");
  WiFi.disconnect();
  delay(100);
  USE_SERIAL.println("Deep sleep.");
  ESP.deepSleep(SLEEP_SEC * 1000000);
  //delay(2000);

}

void send_cloud ( String url, Stream* myS )
{

  WiFiClient wifi_client;
  const int httpPort = 80;


  if (!wifi_client.connect(host, httpPort)) {
    myS->println("connection failed");
    return;
  }

  // This will send the request to the server
  wifi_client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (wifi_client.available() == 0) {
    if (millis() - timeout > 5000) {
      myS->println(">>> myS Timeout !");
      wifi_client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (wifi_client.available()) {
    String line = wifi_client.readStringUntil('\r');
    myS->print(line);
  }
  myS->println();
  myS->println("closing connection");

}


#ifdef SENSOR_TYPE_BME280
void getBME280Data ( Stream* myS)
{
  
  float temp = 0;
  float hum = 0;
  float pres = 0;
  float solar = 0;
  String  url, SensorID;

  if ( bme.begin()) {

    switch (bme.chipModel())
    {
      case BME280::ChipModel_BME280:
        SensorID = "BME280";
        Serial.println("Found BME280 sensor! Success.");
        break;
      case BME280::ChipModel_BMP280:
        SensorID = "BMP280";
        myS->println("Found BMP280 sensor! No Humidity available.");
        break;
      default:
        myS->println("Found UNKNOWN sensor! Error!");
    }
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);
    myS->print("Get sensord data.");
    myS->print(".");
    delay(500);
    bme.read(pres, temp, hum, tempUnit, presUnit);
    myS->println(".");

    solar = analogRead(A0) * (3.3 / 1023.0) * 10 * (47 + 10) / 100;

    pres = int(pres +  0.5) / 100.;
    temp = int(temp * 100 + 0.5) / 100. ;
    hum = int(hum  * 100 + 0.5) / 100.;
    solar = int(solar * 100 + 0.5) / 100.;

    myS->println("");
    myS->print("Temp: ");
    myS->print(temp);
    myS->println("°" + String(tempUnit == BME280::TempUnit_Celsius ? 'C' : 'F'));
    myS->print("Humidity: ");
    myS->print(hum );
    myS->println("%");
    myS->print("Pressure: ");
    myS->print(pres);
    myS->println(" hPa");
    myS->print("Solar Voltage: ");
    myS->print(solar);
    myS->println(" V");
    myS->println("");

    url = "/app.bin?ID=";
    url += SensorID;
    url += "&KEY=";
    url += sensor_key;
    url += "&T=";
    url +=  temp;
    url += "&H=";
    url += hum;
    url += "&P=";
    url += pres;
    url += "&V=";
    url += solar;
  }
  else
  {
    url = "/app.bin?KEY=";
    url += sensor_key;
    url += "&E=Sensor_error";
    myS->println("");
    myS->println("BMP error ");
    myS->println("");
  }
  myS->println(url);
  send_cloud ( url, myS );
}
#endif



#ifdef SENSOR_TYPE_DHT11
void getDHT11Data ( Stream* myS)
{

  char SensorID[] = "DHT11";

  float batery = 0;
  float temp = 0;
  float hum = 0;
  String  url;
  myDHT.begin();
  delay(500);
 
  hum = myDHT.readHumidity();
  temp = myDHT.readTemperature();

  batery = analogRead(A0) * (3.3 / 1023.0) * 10 * (47 + 10) / 100;
  batery = int(batery * 100 + 0.5) / 100.;

  myS->print("Temp: ");
  myS->print(temp);
  myS->println("°C");
  myS->print("Humidity: ");
  myS->print(hum);
  myS->println("%");
  myS->print("Batery: ");
  myS->println(batery);
  myS->println("");

  url = "/app.bin?ID=";
  url += SensorID;
  url += "&KEY=";
  url += sensor_key;
  url += "&T=";
  url +=  temp;
  url += "&H=";
  url += hum;
  url += "&P=0";
  url += "&V=";
  url += batery;
  myS->println(url);
  send_cloud ( url, myS );
}
#endif

#ifdef  SENSOR_TYPE_SMT160
void  getSMT160Data( Stream* myS)
{
  char SensorID[] = "SMT160";
  int SensorPin = SMT_PIN  ;       //Digital pin
  float batery = 0;
  double  DutyCycle = 0;                //DutyCycle that need to calculate
  unsigned int SquareWaveHighTime = 0;  //High time for the square wave
  unsigned int  SquareWaveLowTime = 0;   //Low time for the square wave
  float  temp = 0;               //Calculated temperature using dutycycle
  int sampleSize;
  String  url;

  myS->println("Zacatek mereni");

  SquareWaveHighTime = 0;
  SquareWaveLowTime = 0;
  sampleSize = 0;
  do
  {
    sampleSize++;
    SquareWaveHighTime = SquareWaveHighTime + pulseIn(SensorPin, HIGH);
    SquareWaveLowTime = SquareWaveLowTime +  pulseIn(SensorPin, LOW);

  }
  while (sampleSize < 200);

  DutyCycle = SquareWaveHighTime;          //Calculate Duty Cycle for the square wave
  DutyCycle /= (SquareWaveHighTime + SquareWaveLowTime);
  
  //Compute temperature reading according to D.C.
  // where D.C. = 0.320 + 0.00470 * T
  
  temp = (DutyCycle - 0.320) / 0.00470;

  batery = analogRead(A0) * (3.3 / 1023.0) * 10 * (47 + 10) / 100;
  batery = int(batery * 100 + 0.5) / 100.;

  myS->print(" Calculated Temperature (*C): ");
  myS->println(temp);

  url = "/app.bin?ID=";
  url += SensorID;
  url += "&KEY=";
  url += sensor_key;
  url += "&T=";
  url +=  temp;
  url += "&H=0";
  url += "&P=0";
  url += "&V=";
  url += batery;
  myS->println(url);
  send_cloud ( url, myS );
}

#endif
