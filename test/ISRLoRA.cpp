#include <RadioLib.h>

// --- PINOS ---
static constexpr uint8_t LORA_SCK    = 9;
static constexpr uint8_t LORA_MISO   = 11;
static constexpr uint8_t LORA_MOSI   = 10;
static constexpr uint8_t LORA_CS     = 8;
static constexpr uint8_t LORA_BUSY   = 13;
static constexpr uint8_t LORA_DIO1   = 14;
static constexpr uint8_t LORA_RESET  = 12;

// --- SPI + SX1262 ---
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

// --- Variáveis globais ---
volatile bool packetReceived = false;
volatile bool packetSent = false;

uint64_t cpuTime_us() {
  return micros();
}

// --- CALLBACKS ---
void onTxDone() {
  packetSent = true;
}

void onRxDone() {
  packetReceived = true;
}

// --- FUNÇÕES DE ENVIO E RECEBIMENTO ---
void sendPacket(const String* msg) {
  packetSent = false;
  // int state = radio.startTransmit(msg->c_str());
  int state = radio.transmit(msg->c_str());
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Erro ao transmitir: "));
    Serial.println(state);
  }
}

String receivePacket() {
  packetReceived = false;
  String str;
  int state = radio.readData(str);
  if (state == RADIOLIB_ERR_NONE) {
    return str;
  }
  return "";
}

// --- CONFIGURAÇÃO DO LORA ---
void initLoRa() {
  Serial.println(F("Iniciando LoRa..."));
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Falha: "));
    Serial.println(state);
    while (true);
  }
  radio.setDio1Action(onRxDone);
  radio.setPacketSentAction(onTxDone);
  radio.setFrequency(915.0);
  radio.setSpreadingFactor(12);
  radio.setBandwidth(125.0);
  // radio.setCodingRate(5);
  // radio.setOutputPower(14);
  // radio.startReceive();
  Serial.println(F("LoRa pronto."));
}

// ====================================================
//              DISPOSITIVO MÓVEL
// ====================================================
// Esse envia o primeiro ECHO e faz os cálculos.
void device1_loop() {
  static uint64_t t1, t3;
  static bool waitingResponse = false;

  if (!waitingResponse) {
    String msg = "ECHO," + String(cpuTime_us());
    sendPacket(&msg);
    t1 = cpuTime_us();
    waitingResponse = true;
    Serial.println(F("ECHO enviado"));
  }

  if (packetReceived) {
    String resp = receivePacket();
    t3 = cpuTime_us();
    Serial.println("Recebido: " + resp);
    waitingResponse = false;

    // Parse da resposta: "RESP,t2,tp,toa"
    if (resp.startsWith("RESP")) {
      uint64_t t2, tp, toa;
      sscanf(resp.c_str(), "RESP,%llu,%llu,%llu", &t2, &tp, &toa);

      uint64_t rtt = t3 - t1;
      uint64_t tProp = (rtt - tp - toa) / 2;

      Serial.println("---- RESULTADOS ----");
      Serial.printf("RTT: %llu us\n", rtt);
      Serial.printf("ToA: %llu us\n", toa);
      Serial.printf("Proc: %llu us\n", tp);
      Serial.printf("Tprop: %llu us\n", tProp);

      // Envia pacote de sincronização
      String syncMsg = "SYNC," + String(t1) + "," + String(t2) + "," + String(t3) + "," + String(tp) + "," + String(toa) + "," + String(tProp);
      sendPacket(&syncMsg);
      Serial.println(F("SYNC enviado."));
    }
    radio.startReceive();
  }
}

// ====================================================
//              DISPOSITIVO ÂNCORA
// ====================================================
void device2_loop() {
  if (packetReceived) {
    String msg = receivePacket();
    uint64_t t2 = cpuTime_us();

    if (msg.startsWith("ECHO")) {
      Serial.println("ECHO recebido");
      uint64_t tpStart = cpuTime_us();
      //elay(10); // simula pequeno tempo de processamento
      uint64_t tp = cpuTime_us() - tpStart;

      String response = "RESP," + String(t2) + "," + String(tp);
      uint64_t toa_us = (uint64_t)(radio.getTimeOnAir(response.length()) * 1000.0);
      String response_temp = response + "," + String(toa_us);
      toa_us = (uint64_t)(radio.getTimeOnAir(response_temp.length()) * 1000.0);
      response = "RESP," + String(t2) + "," + String(tp) + "," + String(toa_us);
      sendPacket(&response);
      Serial.println("RESP enviado");
    } 
    else if (msg.startsWith("SYNC")) {
      Serial.println("SYNC recebido");
      // Aqui você poderia ajustar o relógio do microcontrolador
      // para sincronizar com o do móvel.
    }

    radio.startReceive();
  }
}

// ====================================================
//                ARDUINO SETUP/LOOP
// ====================================================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  initLoRa();

  // Troque aqui para escolher o dispositivo:
  // 1 -> móvel
  // 2 -> âncora
  Serial.println(F("Modo: Dispositivo 1 (móvel)"));
}

void loop() {
  // Descomente o modo desejado:
  device1_loop();
   //device2_loop();
}
