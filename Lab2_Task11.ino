#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

// Tuning Thresholds
const float TEMP_RISE_THRESH = 10.0;
const float HUMID_JUMP_THRESH = 10.0;
const float MAG_SHIFT_THRESH = 30.0;
const int LIGHT_CHANGE_THRESH = 100;

float baseTemp = 27, baseHumid = 36, baseMag = 50;
int baseClear = 0;
int cooldown = 0;
String currentEvent = "BASELINE_NORMAL";

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!HS300x.begin()) while (1);
  if (!IMU.begin()) while (1);
  if (!APDS.begin()) while (1);

  baseTemp = HS300x.readTemperature();
  baseHumid = HS300x.readHumidity();
  
  float x, y, z;
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(x, y, z);
    baseMag = sqrt((x * x) + (y * y) + (z * z));
  }
  
  int r, g, b, c;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
    baseClear = c;
  }
}

void loop() {
  float temperature = HS300x.readTemperature();
  float humidity = HS300x.readHumidity();

  float x, y, z;
  float mag = 0;
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(x, y, z);
    mag = sqrt((x * x) + (y * y) + (z * z));
  }

  int r = 0, g = 0, b = 0, c = 0;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }

  int humid_jump = (humidity - baseHumid > HUMID_JUMP_THRESH) ? 1 : 0;
  int temp_rise = (temperature - baseTemp > TEMP_RISE_THRESH) ? 1 : 0;
  int mag_shift = (abs(mag - baseMag) > MAG_SHIFT_THRESH) ? 1 : 0;
  int light_or_color_change = (abs(c - baseClear) > LIGHT_CHANGE_THRESH) ? 1 : 0;

  String nextEvent = "BASELINE_NORMAL";
  if (humid_jump || temp_rise) nextEvent = "BREATH_OR_WARM_AIR_EVENT";
  else if (mag_shift) nextEvent = "MAGNETIC_DISTURBANCE_EVENT";
  else if (light_or_color_change) nextEvent = "LIGHT_OR_COLOR_CHANGE_EVENT";

  if (nextEvent != "BASELINE_NORMAL") {
    currentEvent = nextEvent;
    cooldown = 12; // Holds the event state for roughly 3 seconds (12 loops * 250ms)
  } else {
    if (cooldown > 0) cooldown--;
    else currentEvent = "BASELINE_NORMAL";
  }

  Serial.print("raw,rh="); Serial.print(humidity);
  Serial.print(",temp="); Serial.print(temperature);
  Serial.print(",mag="); Serial.print(mag);
  Serial.print(",r="); Serial.print(r);
  Serial.print(",g="); Serial.print(g);
  Serial.print(",b="); Serial.print(b);
  Serial.print(",clear="); Serial.println(c);

  Serial.print("flags,humid_jump="); Serial.print(humid_jump);
  Serial.print(",temp_rise="); Serial.print(temp_rise);
  Serial.print(",mag_shift="); Serial.print(mag_shift);
  Serial.print(",light_or_color_change="); Serial.println(light_or_color_change);

  Serial.print("event,"); Serial.println(currentEvent);

  delay(250);
}