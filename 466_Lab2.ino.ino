#include <PDM.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

// Adjust these values based on actual readings in your room
const int AUDIO_THRESHOLD = 50;      // mic > threshold = NOISY
const int DARK_THRESHOLD = 30;       // clear < threshold = DARK
const float MOTION_THRESHOLD = 0.15; // motion > threshold = MOVING
const int NEAR_THRESHOLD = 100;      // prox < threshold = NEAR (APDS9960 usually returns 0 for close, 255 for far)

short sampleBuffer[256];
volatile int samplesRead = 0;
int currentMicLevel = 0;


int clearChannel = 0;
float motionLevel = 0.0;
int proximityLevel = 255; 

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  while (!Serial); 

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM microphone.");
    while (1);
  }
}

void loop() {
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    currentMicLevel = sum / samplesRead;
    samplesRead = 0;
  }

  int r = 0, g = 0, b = 0;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, clearChannel);
  }

  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    float magnitude = sqrt((x * x) + (y * y) + (z * z));
    motionLevel = abs(magnitude - 1.0); 
  }

  if (APDS.proximityAvailable()) {
    proximityLevel = APDS.readProximity();
  }

  int soundFlag = (currentMicLevel > AUDIO_THRESHOLD) ? 1 : 0;
  int darkFlag = (clearChannel < DARK_THRESHOLD) ? 1 : 0;
  int movingFlag = (motionLevel > MOTION_THRESHOLD) ? 1 : 0;
  int nearFlag = (proximityLevel < NEAR_THRESHOLD) ? 1 : 0; 

  String finalLabel = "UNKNOWN";

  if (!soundFlag && !darkFlag && !movingFlag && !nearFlag) {
    finalLabel = "QUIET_BRIGHT_STEADY_FAR";
  } 
  else if (soundFlag && !darkFlag && !movingFlag && !nearFlag) {
    finalLabel = "NOISY_BRIGHT_STEADY_FAR";
  } 
  else if (!soundFlag && darkFlag && !movingFlag && nearFlag) {
    finalLabel = "QUIET_DARK_STEADY_NEAR";
  } 
  else if (soundFlag && !darkFlag && movingFlag && nearFlag) {
    finalLabel = "NOISY_BRIGHT_MOVING_NEAR";
  }

  Serial.print("raw,mic="); Serial.print(currentMicLevel);
  Serial.print(",clear="); Serial.print(clearChannel);
  Serial.print(",motion="); Serial.print(motionLevel, 3);
  Serial.print(",prox="); Serial.println(proximityLevel);

  Serial.print("flags,sound="); Serial.print(soundFlag);
  Serial.print(",dark="); Serial.print(darkFlag);
  Serial.print(",moving="); Serial.print(movingFlag);
  Serial.print(",near="); Serial.println(nearFlag);

  Serial.print("state,"); Serial.println(finalLabel);
  
  delay(1000); 
}