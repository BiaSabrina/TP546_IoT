
/*******************************************************
 * Lixeira IoT -> Blynk Cloud (Web Dashboard + App)
 * ESP32 + HC-SR04 + 3 LEDs físicos
 * Virtuais:
 *   V1 = LED Verde (dashboard)
 *   V2 = LED Amarelo (dashboard)
 *   V3 = LED Vermelho (dashboard)
 *   V4 = Distância (cm)    [Label/Value Display] (opcional)
 *   V5 = Nível (%)         [Gauge/Chart]
 *   V6 = Alerta (0/1)      [LED/Label]

 *******************************************************/

#define BLYNK_TEMPLATE_ID "TMPL2uExoG7ff"
#define BLYNK_TEMPLATE_NAME "Lixeira IoT"
#define BLYNK_AUTH_TOKEN "-qBxcSXa4hloTsDCzFBxs4qoStgWvreh"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ===== Wi-Fi (Wokwi/real) =====
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ===== Pinos FÍSICOS =====
#define PIN_TRIG 5
#define PIN_ECHO 18
#define LED_RED  19
#define LED_YEL  21
#define LED_GRE  22

// ===== Config =====
const float ALTURA_TOTAL_CM = 30.0;   // altura interna da lixeira
const uint32_t SAMPLE_MS = 3000;      // período de envio (ms)
const float ALERTA_PERCENT = 80.0;    // limiar alerta

// ===== GPS (simulado) =====
const float GPS_LAT = -22.254;
const float GPS_LON = -45.703;

BlynkTimer timer;

// -------- Leitura do HC-SR04 (cm) --------
float readDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000UL); // timeout 30ms (~5m)
  if (dur == 0) return ALTURA_TOTAL_CM;        // sem eco => vazio
  return dur * 0.0343f / 2.0f;                 // cm
}

// -------- LEDs físicos --------
void setLedsFisicos(float nivel) {
  bool red = (nivel >= ALERTA_PERCENT);
  bool yel = (nivel >= 40.0 && nivel < ALERTA_PERCENT);
  bool gre = (nivel < 40.0);
  digitalWrite(LED_RED, red);
  digitalWrite(LED_YEL, yel);
  digitalWrite(LED_GRE, gre);
}

// -------- Envio p/ Blynk + lógica dos LEDs virtuais --------
void sendToBlynk() {
  float dist = readDistanceCM();
  float nivel = (ALTURA_TOTAL_CM - dist) / ALTURA_TOTAL_CM * 100.0f;
  if (nivel < 0)   nivel = 0;
  if (nivel > 100) nivel = 100;

  int alerta = (nivel >= ALERTA_PERCENT) ? 1 : 0;

  // LEDs físicos
  setLedsFisicos(nivel);

  // Widgets do dashboard
  Blynk.virtualWrite(V5, nivel);     // Gauge
  Blynk.virtualWrite(V6, alerta);    // LED/Label de alerta
  Blynk.virtualWrite(V4, dist);      // Distância (opcional)
  Blynk.virtualWrite(V7, GPS_LAT);   // Map
  Blynk.virtualWrite(V8, GPS_LON);   // Map

  // LEDs virtuais (um de cada vez)
  if (nivel < 40.0) {
    Blynk.virtualWrite(V1, 1); // verde ON
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 0);
  } else if (nivel < 80.0) {
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, 1); // amarelo ON
    Blynk.virtualWrite(V3, 0);
  } else {
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 1); // vermelho ON
  }

  Serial.printf("Dist=%.2f cm | Nivel=%.2f%% | Alerta=%d\n", dist, nivel, alerta);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YEL, OUTPUT);
  pinMode(LED_GRE, OUTPUT);

  WiFi.mode(WIFI_STA);
  // Conecta Wi-Fi + Blynk (forma simples)
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

  // Envia leituras periodicamente
  timer.setInterval(SAMPLE_MS, sendToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
}
