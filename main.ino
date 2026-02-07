#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== PULSE SENSOR =====
#define PULSE_PIN A0
int signal;
int threshold = 550;
int BPM = 0;
int displayBPM = 0;
unsigned long lastBeatTime = 0;
bool beatDetected = false;

// ===== TIMING =====
unsigned long lastDisplayUpdate = 0;
const int DISPLAY_INTERVAL = 120;

// ===== GRAPH (LCD) =====
byte graph[16];

// Custom bar characters (0–7)
byte bars[8][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,31},
  {0,0,0,0,0,0,31,31},
  {0,0,0,0,0,31,31,31},
  {0,0,0,0,31,31,31,31},
  {0,0,0,31,31,31,31,31},
  {0,0,31,31,31,31,31,31},
  {0,31,31,31,31,31,31,31}
};

// ===== SERIAL PLOTTER VARIABLES =====
float plotY = 0;
float targetY = 800;
bool beatActive = false;
unsigned long beatStart = 0;
int beatDuration = 600;

bool fingerPresent = false;
unsigned long lastFingerTime = 0;
float baseline = 300;

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  for (int i = 0; i < 8; i++) {
    lcd.createChar(i, bars[i]);
  }

  lcd.setCursor(0, 0);
  lcd.print("Pulse Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing");
  delay(2000);
  lcd.clear();

  pinMode(LED_BUILTIN, OUTPUT);
  randomSeed(analogRead(A1));
}

void loop() {
  unsigned long now = millis();
  signal = analogRead(PULSE_PIN);

  fingerPresent = (signal > 450);

  if (fingerPresent) {
    lastFingerTime = now;
  } else {
    if (now - lastFingerTime > 2000) {
      displayBPM = 0;
      BPM = 0;
      lastBeatTime = 0;
    }
  }

  // ---------- BEAT DETECTION ----------
  if (signal > threshold && !beatDetected && fingerPresent) {
    beatDetected = true;
    digitalWrite(LED_BUILTIN, HIGH);

    beatActive = true;
    beatStart = now;

    if (lastBeatTime > 0) {
      unsigned long interval = now - lastBeatTime;
      if (interval > 300 && interval < 2000) {
        BPM = 60000 / interval;
        displayBPM = (displayBPM * 3 + BPM) / 4;
        displayBPM = constrain(displayBPM, 40, 200);
      }
    }
    lastBeatTime = now;
  }

  if (signal < threshold) {
    beatDetected = false;
    digitalWrite(LED_BUILTIN, LOW);
  }

  // ---------- LCD GRAPH UPDATE ----------
  int height = map(signal, 400, 700, 0, 7);
  height = constrain(height, 0, 7);

  for (int i = 0; i < 15; i++) {
    graph[i] = graph[i + 1];
  }

  if (beatDetected) {
    graph[15] = 7;
  } else {
    graph[15] = height;
  }

  // ---------- LCD UPDATE ----------
  if (now - lastDisplayUpdate > DISPLAY_INTERVAL) {
    lcd.setCursor(0, 0);
    lcd.print("BPM: ");
    if (fingerPresent && displayBPM > 0) {
      lcd.print(displayBPM);
      lcd.print("   ");
    } else {
      lcd.print("0   ");
    }

    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++) {
      lcd.write(byte(graph[i]));
    }

    lastDisplayUpdate = now;
  }

  // ---------- SERIAL PLOTTER (REALISTIC ECG WAVEFORM) ----------
  if (fingerPresent && beatActive) {
    float t = (float)(now - beatStart) / (float)beatDuration;
    
    if (t <= 1.0) {
      if (t < 0.08) {
        float pt = t / 0.08;
        float pWave = sin(pt * 3.14159);
        plotY = baseline + 60.0 * pWave;
      } else if (t < 0.12) {
        plotY = baseline;
      } else if (t < 0.14) {
        float qt = (t - 0.12) / 0.02;
        plotY = baseline - 40.0 * sin(qt * 3.14159);
      } else if (t < 0.18) {
        float rt = (t - 0.14) / 0.04;
        plotY = baseline + (float)targetY * sin(rt * 3.14159);
      } else if (t < 0.22) {
        float st = (t - 0.18) / 0.04;
        plotY = baseline - 120.0 * sin(st * 3.14159);
      } else if (t < 0.30) {
        float stt = (t - 0.22) / 0.08;
        plotY = baseline + 20.0 * (1.0 - stt);
      } else if (t < 0.50) {
        float tt = (t - 0.30) / 0.20;
        plotY = baseline + 150.0 * sin(tt * 3.14159);
      } else if (t < 0.55) {
        float ut = (t - 0.50) / 0.05;
        plotY = baseline + 30.0 * sin(ut * 3.14159);
      } else {
        plotY = baseline;
      }
    } else {
      beatActive = false;
      plotY = baseline;
    }
  } else if (!fingerPresent) {
    plotY = baseline;
  } else {
    plotY = baseline;
  }

  if (plotY < 0) plotY = 0;

  // Send to Serial Plotter
  Serial.println(plotY);

  delay(20);
}