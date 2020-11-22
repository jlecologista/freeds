/*
  Support_functions.ino - FreeDs support functions
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32
  
  Copyright (C) 2020 Pablo Zerón (https://github.com/pablozg/freeds)

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

void getSensorData(void)
{
  if (config.flags.wifi)
  {
    switch (config.wversion)
    {
      case SOLAX_V2: // Solax v2
        readESP01();
        break;
      case SOLAX_V2_LOCAL: // Solax v2 local mode
      case SOLAX_V1: // Solax v1
      case WIBEEE: // Wibee
      case SHELLY_EM: // Shelly EM
      case FRONIUS_API: // Fronius API
      case SLAVE_MODE: // Master FreeDS
        runAsyncClient();
        break;
      case DDS238_METER: // DDS2382
      case DDSU666_METER: // DDSU666
      case SDM_METER: // SDM120/220
      case SMA_BOY: // SMA
      case SMA_ISLAND: // SMA
      case VICTRON: // Victron
      case FRONIUS_MODBUS: // Fronius Modbus
      case HUAWEI_MODBUS: // Huawei
      case SOLAREDGE: // SolarEdge
      case MUSTSOLAR: // MustSolar
        readModbus();
        break;
    }
  }
}

void setGetDataTime(void)
{
  switch (config.wversion) {
    case SOLAX_V2:
      config.getDataTime = 250;
      break;
    case SOLAX_V2_LOCAL:
    case SOLAX_V1:
    case WIBEEE:
    case SHELLY_EM:
    case FRONIUS_API:
      if (config.getDataTime < 1000) config.getDataTime = 1000;
      break;
    case SLAVE_MODE:
      if (config.getDataTime < 500) config.getDataTime = 500;
      break;
    case FRONIUS_MODBUS:
      if (config.getDataTime < 250) config.getDataTime = 250;
      break;
    case SMA_BOY:
    case SMA_ISLAND:
    case VICTRON:
    case HUAWEI_MODBUS:
    case SOLAREDGE:
      if (config.getDataTime < 1000) config.getDataTime = 1000;
      break;
  }
  Tickers.updatePeriod(4, config.getDataTime);
}

String midString(String *str, String start, String finish){
  int locStart = str->indexOf(start);
  if (locStart == -1) return "";
  locStart += start.length();
  int locFinish = str->indexOf(finish, locStart);
  if (locFinish == -1) return "";
  return str->substring(locStart, locFinish);
}

char *dtostrfd(double number, unsigned char prec, char *s)
{
  if ((isnan(number)) || (isinf(number)))
  { // Fix for JSON output (https://stackoverflow.com/questions/1423081/json-left-out-infinity-and-nan-json-status-in-ecmascript)
    strcpy(s, "null");
    return s;
  } else {
    return dtostrf(number, 1, prec, s);
  }
}

int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

void buildWifiArray(void)
{
  WiFi.scanNetworks();
  for (int i = 0; i < 15; ++i) {
    if(WiFi.SSID(i) == "") { break; }
    scanNetworks[i] = WiFi.SSID(i);
    rssiNetworks[i] = (int8_t)WiFi.RSSI(i);
    INFOV("SSID %i - %s (%d%%, %d dBm)\n", i, scanNetworks[i].c_str(), WifiGetRssiAsQuality(rssiNetworks[i]), rssiNetworks[i]);
  }
}

const char *get_filename_ext(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if(!dot || dot == filename) return "";
  return dot + 1;
}

void changeScreen(void)
{
  if (digitalRead(0) == LOW)
  {

    if (ButtonState == false && (millis() - lastDebounceTime) > debounceDelay)
    {
      ButtonState = true;
      lastDebounceTime = millis();
    }

    // Cambio de modo de trabajo
    if (((millis() - lastDebounceTime) > 2000) && ButtonLongPress == false)
    {
      ButtonLongPress = true;
      workingMode++;
      if (workingMode > 2)
      {
        workingMode = 0;
      }
      switch (workingMode)
      {
      case 0: // AUTO
        config.flags.pwmEnabled = true;
        config.flags.pwmMan = false;
        break;

      case 1: // MANUAL
        config.flags.pwmEnabled = true;
        config.flags.pwmMan = true;
        break;

      case 2: // OFF
        config.flags.pwmEnabled = false;
        config.flags.pwmMan = false;
        break;
      }
      saveEEPROM();
    }

    // // Apagar y encender la pantalla
    // if (((millis() - lastDebounceTime) > 5000) && ButtonLongPress == false)
    // {
    //   ButtonLongPress = true;
    //   turnOffOled();
    // }

    if ((millis() - lastDebounceTime) > 10000)
    {
      defaultValues();
      restartFunction();
    }
  }
  else
  {
    if (ButtonState == true)
    {
      if (ButtonLongPress == true)
      {
        ButtonLongPress = false;
        ButtonState = false;
      }
      else
      {
        ButtonState = false;
        timers.OledAutoOff = millis();
        if (config.flags.oledAutoOff && !config.flags.oledPower)
        {
          config.flags.oledPower = true;
          turnOffOled();
        }
        else
        {
          screen++;
          if (screen > MAX_SCREENS)
          {
            screen = 0;
          }
        }
      }
    }
  }
}

void turnOffOled(void)
{
  display.clear();
  config.flags.oledPower ? display.displayOn() : display.displayOff();
}

void restartFunction(void)
{
  
  if (!Flags.firstInit)
  {
    down_pwm(false);
  }

  saveEEPROM();

  uint8_t tcont = 4;
  while (tcont-- > 0)
  {
    INFOV(PSTR("RESTARTING IN %i SEC !!!!\n"), tcont);
    delay(1000);
  }
  ESP.restart();
}

void saveEEPROM(void)
{
  EEPROM.put(0, config);
  EEPROM.commit();
  INFOV("DATA SAVED!!!!\n");
}

void updateUptime()
{
  //************************ Time function from https://hackaday.io/project/7008-fly-wars-a-hackers-solution-to-world-hunger/log/25043-updated-uptime-counter ****************// 
  //************************ Uptime Code - Makes a count of the total up time since last start ****************// 
  
  //** Making Note of an expected rollover *****//
  if (millis() >= 3000000000)
  {
    uptime.HighMillis = 1;
  }
  //** Making note of actual rollover **//
  if (millis() <= 100000 && uptime.HighMillis == 1)
  {
    uptime.Rollover++;
    uptime.HighMillis = 0;
  }

  long secsUp = millis() / 1000;
  uptime.Second = secsUp % 60;
  uptime.Minute = (secsUp / 60) % 60;
  uptime.Hour = (secsUp / (60 * 60)) % 24;
  uptime.Day = (uptime.Rollover * 50) + (secsUp / (60 * 60 * 24)); //First portion takes care of a rollover [around 50 days]
};

//******************* Prints the uptime to serial window **********************//
const char *printUptime()
{
  //char tmp[80];
 
  if (Flags.ntpTime) {
    sprintf(response, "Fecha: %02d/%02d/%04d Hora: %02d:%02d:%02d<br>Uptime: %li días %02d:%02d:%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, uptime.Day, uptime.Hour, uptime.Minute, uptime.Second);
  } else {
    sprintf(response, "Uptime: %li días %02d:%02d:%02d", uptime.Day, uptime.Hour, uptime.Minute, uptime.Second);
  }
  return response;
};

String printUptimeOled()
{
  char tmp[33];
  sprintf(tmp, "UPTIME: %li días %02d:%02d:%02d", uptime.Day, uptime.Hour, uptime.Minute, uptime.Second);
  return tmp;
};

String printDateOled()
{
  char tmp[33];
  sprintf(tmp, "Fecha: %02d/%02d/%04d Hora: %02d:%02d:%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return tmp;
};

void updateLocalTime(void)
{
  if(!getLocalTime(&timeinfo)){
    if (config.flags.debug1) { INFOV("Failed to obtain time\n"); }
    Flags.ntpTime = false;
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //INFOV("Fecha: %02d/%02d/%04d Hora: %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  if (timeinfo.tm_year >= 120) { Flags.ntpTime = true; }
}

uint16_t getHour(uint16_t n) 
{ 
  return n / 100;
} 

uint16_t getMin(uint16_t n) 
{ 
  return (n % 100);
}

void checkTimer(void)
{
  if (config.flags.timerEnabled && Flags.ntpTime) {
    boolean changeToManual = false;
  
    if (getHour(config.timerStart) <= getHour(config.timerStop)) {
      if (((timeinfo.tm_hour == getHour(config.timerStart) && timeinfo.tm_min >= getMin(config.timerStart)) || timeinfo.tm_hour > getHour(config.timerStart)) &&
          ((timeinfo.tm_hour == getHour(config.timerStop) && timeinfo.tm_min < getMin(config.timerStop)) || timeinfo.tm_hour < getHour(config.timerStop)) )
      {
        changeToManual = true;
      } else {
        changeToManual = false;
      }
    }

    if (getHour(config.timerStart) > getHour(config.timerStop)){
        if (((timeinfo.tm_hour == getHour(config.timerStart) && timeinfo.tm_min >= getMin(config.timerStart)) || timeinfo.tm_hour > getHour(config.timerStart)) &&
              timeinfo.tm_hour <= 23)
        {
          changeToManual = true;
        } else if ((timeinfo.tm_hour == getHour(config.timerStop) && timeinfo.tm_min < getMin(config.timerStop)) || timeinfo.tm_hour < getHour(config.timerStop)) {
          changeToManual = true;
        } else if ((timeinfo.tm_hour == getHour(config.timerStop) && timeinfo.tm_min > getMin(config.timerStop)) || timeinfo.tm_hour > getHour(config.timerStop))  {
          changeToManual = false;
        }
    }
        
    if (changeToManual) {
      if (!Flags.timerSet) { 
        INFOV("Timer started\n");
        Flags.timerSet = true;
        config.flags.pwmMan = true;
        if (config.modoTemperatura != 3) { Flags.pwmIsWorking = true; }
      }
    } else {
      if (Flags.timerSet) {
        INFOV("Timer stopped\n");
        Flags.timerSet = false;
        config.flags.pwmMan = false;
        if (config.modoTemperatura != 3) { Flags.pwmIsWorking = true; }
      }
    }
  }
}

void calcWattsToday()
{
  // To avoid bad data, we set a maximum of 20000W as right value
  if (inverter.wgrid < -20000 || inverter.wgrid > 20000) return;

  float timeCalcWattsToday = (float(millis() - timers.KwToday)/1000.0);
  float KwIncrement = (inverter.wgrid * (timeCalcWattsToday/60/60/1000)); // Calculate kilowatt hours used

  if (KwIncrement < -1 || KwIncrement > 1) {
    INFOV("Millis: %lu, time float: %.03f\n", millis() - timers.KwToday, timeCalcWattsToday);
    INFOV("Wgrid: %.03f, KwIncrement: %.10f, ChangeGridSign: %s\n", inverter.wgrid, KwIncrement, config.flags.changeGridSign ? "ON" : "OFF");
  }
  
  if (config.flags.changeGridSign ? KwIncrement > 0 : KwIncrement < 0) {
    if (!config.flags.changeGridSign) { KwIncrement *= -1; }
    config.KwToday += KwIncrement;
    config.KwTotal += KwIncrement;
  } else {
    if (config.flags.changeGridSign) { KwIncrement *= -1; }
    config.KwExportToday += KwIncrement;
    config.KwExportTotal += KwIncrement;
  }

  if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) // If midnight
  {
    config.KwYesterday = config.KwToday;
    config.KwExportYesterday = config.KwExportToday;
    config.KwToday = 0;
    config.KwExportToday = 0;
    saveEEPROM();
  }
  timers.KwToday = millis();
}

void defineWebMonitorFields(uint8_t version)
{
  switch (version)
  {
    case SOLAX_V2: // Solax v2
      webMonitorFields.data = 0x0177E000;
      break;
    case SOLAX_V2_LOCAL: // Solax v2 local mode
      webMonitorFields.data = 0x0177E000;
      break;
    case SOLAX_V1: // Solax v1
      webMonitorFields.data = 0x0177E000;
      break;
    case MQTT_BROKER: // Mqtt
      webMonitorFields.data = 0x0177E000;
      break;
    case ICC_SOLAR: // Icc Solar
      webMonitorFields.data = 0x0F77E000;
      break;
    case WIBEEE: // Wibee
      webMonitorFields.data = 0x005801E6;
      break;
    case SHELLY_EM: // Shelly EM
      webMonitorFields.data = 0x00580322;
      break;
    case FRONIUS_API: // Fronius API
      webMonitorFields.data = 0x00700000;
      break;
    case DDS238_METER: // DDS2382
      webMonitorFields.data = 0x004003EF;
      break;
    case DDSU666_METER: // DDSU666
      webMonitorFields.data = 0x004003EE;
      break;
    case SDM_METER: // SDM120/220
      webMonitorFields.data = 0x00401FFF;
      break;
    case SMA_BOY: // SMA
      webMonitorFields.data = 0x0077E000;
      break;
    case SMA_ISLAND: // SMA
      webMonitorFields.data = 0x07000006;
      break;
    case VICTRON: // Victron
      webMonitorFields.data = 0x06500006;
      break;
    case FRONIUS_MODBUS: // Fronius Modbus
      webMonitorFields.data = 0x0057E006;
      break;
    case HUAWEI_MODBUS: // Huawei
      webMonitorFields.data = 0x0377E000;
      break;
    case SOLAREDGE: // SolarEdge
      webMonitorFields.data = 0x0152A000;
      break;
    default:
      webMonitorFields.data = 0x0177E000;
      break;
  }
}

void verbose_print_reset_reason(int cpu)
{
  const char * reason;

  switch ((int)rtc_get_reset_reason(cpu))
  {
    case 1:
      reason = {"Vbat power on reset"};
      break;
    case 3:
      reason = {"Software reset digital core"};
      break;
    case 4:
      reason = {"Legacy watch dog reset digital core"};
      break;
    case 5:
      reason = {"Deep Sleep reset digital core"};
      break;
    case 6:
      reason = {"Reset by SLC module, reset digital core"};
      break;
    case 7:
      reason = {"Timer Group0 Watch dog reset digital core"};
      break;
    case 8:
      reason = {"Timer Group1 Watch dog reset digital core"};
      break;
    case 9:
      reason = {"RTC Watch dog Reset digital core"};
      break;
    case 10:
      reason = {"Instrusion tested to reset CPU"};
      break;
    case 11:
      reason = {"Time Group reset CPU"};
      break;
    case 12:
      reason = {"Software reset CPU"};
      break;
    case 13:
      reason = {"RTC Watch dog Reset CPU"};
      break;
    case 14:
      reason = {"for APP CPU, reseted by PRO CPU"};
      break;
    case 15:
      reason = {"Reset when the vdd voltage is not stable"};
      break;
    case 16:
      reason = {"RTC Watch dog reset digital core and rtc module"};
      break;
    default:
      reason = {"NO_MEAN"};
  }
  
  INFOV("CPU%i reset reason: %s\n", cpu, reason);
}

/// BASIC LOGGING

void addLog(char *data)
{
  if (logcount > (LOGGINGSIZE - 1)) { logcount = 0; }
  
  if (Flags.ntpTime) {
    sprintf(loggingMessage[logcount], "%02d:%02d:%02d - %s\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, data);
  } else {
    sprintf(loggingMessage[logcount], "%02d:%02d:%02d - %s\n", uptime.Hour, uptime.Minute, uptime.Second, data);
  }
  logcount++;

  if (Flags.weblogConnected) sendWeblogStreamTest();
}

void sendWeblogStreamTest(void)
{
  // Print old messages
  if (strcmp(loggingMessage[logcount], "") != 0 && logcount > 1) {
    for (int counter = logcount; counter < LOGGINGSIZE; counter++)
    {
      webLogs.send(loggingMessage[counter], "weblog");
      memset(loggingMessage[counter], 0, 1024);
    }
  }

  // Print new messages
  for (int counter = 0; counter < logcount; counter++) {
    webLogs.send(loggingMessage[counter], "weblog");
    memset(loggingMessage[counter], 0, 1024);
  }

  logcount = 0;
}

// int INFOV(const char * __restrict format, ...)
// {
// 	va_list arg;
// 	int rcode = 0;
//   int len = strlen(format) + 150; // 50 chars to acomodate the variables values
//   char *buffer{ new char[len]{} };
    
//   if (buffer) {

//     va_start(arg, format);
//     rcode = vsprintf(buffer, format, arg);
//     va_end(arg);

//     if (config.flags.serial) Serial.print(buffer);
//     if (config.flags.weblog) addLog((String)buffer);

//     delete[] buffer;
//   }

// 	return rcode;
// }

int INFOV(const char * __restrict format, ...)
{
	va_list arg;
	int rcode = 0;
  char buffer[1024];
    
  va_start(arg, format);
  rcode = vsprintf(buffer, format, arg);
  va_end(arg);

  if (config.flags.serial) Serial.print(buffer);
  if (config.flags.weblog) addLog(buffer);

	return rcode;
}

float getFragmentation() {
  //return 100 - getLargestAvailableBlock() * 100.0 / getTotalAvailableMemory();
  return 100 - ESP.getMaxAllocHeap() * 100.0 / ESP.getFreeHeap();

}

bool readLanguages() {

  Serial.printf("Actual language: %s\n", config.language);
  
  File langFile = SPIFFS.open("/lang-" + String(config.language) + ".json", "r");
  if (!langFile) {
    Serial.println("Failed to open language file");
    return false;
  }

  size_t size = langFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Asigne un buffer para almacenar el contenido del archivo.
  std::unique_ptr<char[]> buf(new char[size]);

  // No usamos String aquí porque la biblioteca ArduinoJson requiere la entrada
  // buffer para ser mutable Si no usas ArduinoJson, también puedes
  // use langFile.readString en su lugar.
  langFile.readBytes(buf.get(), size);

  DynamicJsonDocument jsonBuffer(1024);

  DeserializationError error = deserializeJson(jsonBuffer, buf.get());
  
  if (error) {
    Serial.println("Error al analizar el archivo de lenguajes");
    return false;
  }

  strcpy(lang._GRID_, jsonBuffer[config.language][0]);
  strcpy(lang._SOLAR_, jsonBuffer[config.language][1]);
  strcpy(lang._BATTERY_, jsonBuffer[config.language][2]);
  strcpy(lang._INVERTERINFO_, jsonBuffer[config.language][3]);
  strcpy(lang._METERINFO_, jsonBuffer[config.language][4]);
  strcpy(lang._OLEDPOWER_, jsonBuffer[config.language][5]);
  strcpy(lang._VOLTAGE_, jsonBuffer[config.language][6]);
  strcpy(lang._CURRENT_, jsonBuffer[config.language][7]);
  strcpy(lang._IMPORT_, jsonBuffer[config.language][8]);
  strcpy(lang._EXPORT_, jsonBuffer[config.language][9]);
  strcpy(lang._OLEDTODAY_, jsonBuffer[config.language][10]);
  strcpy(lang._START_, jsonBuffer[config.language][11]);
  strcpy(lang._CONNECTING_, jsonBuffer[config.language][12]);
  strcpy(lang._RELAY_, jsonBuffer[config.language][13]);
  strcpy(lang._CONNECTSSID_, jsonBuffer[config.language][14]);
  strcpy(lang._CONFIGPAGE_, jsonBuffer[config.language][15]);
  strcpy(lang._PRGRESTORE_, jsonBuffer[config.language][16]);
  strcpy(lang._WAIT_, jsonBuffer[config.language][17]);
  strcpy(lang._UPDATING_, jsonBuffer[config.language][18]);
  strcpy(lang._LOSTWIFI_, jsonBuffer[config.language][19]);
  strcpy(lang._TEMPERATURES_, jsonBuffer[config.language][20]);
  strcpy(lang._INVERTERTEMP_, jsonBuffer[config.language][21]);
  strcpy(lang._TERMOTEMP_, jsonBuffer[config.language][22]);
  strcpy(lang._TRIACTEMP_, jsonBuffer[config.language][23]);
  strcpy(lang._DERIVADOR_, jsonBuffer[config.language][24]);
  strcpy(lang._COMPILATION_, jsonBuffer[config.language][25]);

  return true;
}

void checkEEPROM(void) {
  
  byte actualVersion = config.eeinit;
    
  // Paso de versión 0x0A - 0x10 a 0x11
  if(config.eeinit >= 0x0A && config.eeinit <= 0x10)
  {
    defaultValues();
    config.eeinit = 0x11;
  }

  if(config.eeinit == 0x11)
  {
    config.attachedLoadWatts = 2000;
    config.eeinit = 0x12;
  }

  if(config.eeinit == 0x12)
  {
    config.flags.changeGridSign = false;
    config.flags.debug3 = false;
    config.eeinit = 0x13;
  }

  if(config.eeinit == 0x13)
  {
    config.maxPwmLowCost = 1073; // Max 1232
    config.eeinit = 0x14;
  }

  if(config.eeinit == 0x14)
  {
    config.KwToday = 0;
    config.KwExportToday = 0;
    config.KwYesterday = 0;
    config.KwExportYesterday = 0;
    config.KwTotal = 0;
    config.KwExportTotal = 0;
    config.flags.flipScreen = true;
    config.flags.offGrid = false;
    config.soc = 100;
    config.battWatts = -200;
    config.eeinit = 0x15;
  }

  if(config.eeinit == 0x15)
  {
    config.flags.showEnergyMeter = true;
    config.maxWattsTariff = 3450;
    strcpy(config.tzConfig, "CET-1CEST,M3.5.0,M10.5.0/3");
    strcpy(config.language, "es");
    config.eeinit = 0x16;
    INFOV("EEPROM Settings upgraded from versión %x to %x", actualVersion, config.eeinit);
    saveEEPROM();
  }
}