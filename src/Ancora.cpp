#include "cmslora.h"
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"
#include "esp_timer.h"

CMSLoRa lora;

#include <Arduino.h>
#include <string.h> // para memcpy

#define PAYLOAD_SIZE 20

uint8_t packetBuffer[PAYLOAD_SIZE];
uint8_t receivedBuffer[PAYLOAD_SIZE];
int seq = 0;
size_t packetSize = 0;
uint64_t t_1 = 0;
uint64_t t_2 = 0;

// #define WiFiSSID "Galaxy M31C458"
// #define WiFiPassWord "ibse1870"
#define WiFiSSID "M34 de Marcos"
#define WiFiPassWord "marcos123"
const char *ntpServer1 = "time.nist.gov";
const char *ntpServer2 = "time.nist.gov";
RTC_DATA_ATTR bool initTimer = false;
bool initSNTP = false;
RTC_DATA_ATTR time_t base_unix_time = 0;

// Use variáveis globais para TWR para maior clareza
static uint64_t T0_recv = 0; // T0 no relógio do móvel
static uint64_t T1_send = 0; // T1 no relógio do fixo
static uint64_t T2_recv = 0; // T2 no relógio do móvel
static uint64_t T3_send = 0; // T3 no relógio do fixo

static uint32_t rcvdSeq = 0;
static uint64_t rcvdTime = 0;
struct Packet {
    uint32_t seq;
    uint64_t t_device;
    uint8_t payload[PAYLOAD_SIZE];
};

// --- Função que cria o pacote a partir de seq e payload ---
size_t buildPacket(uint8_t* buffer, size_t bufferSize, uint32_t seq, uint64_t t_device, uint8_t* data = nullptr, size_t dataSize = 0) {
    if(bufferSize < PAYLOAD_SIZE) return 0; // buffer pequeno

    Packet pkt;
    pkt.seq = seq;
    pkt.t_device = t_device;
    memset(pkt.payload, 0, PAYLOAD_SIZE);

    if(data != nullptr && dataSize > 0) {
        size_t copySize = (dataSize > PAYLOAD_SIZE) ? PAYLOAD_SIZE : dataSize;
        memcpy(pkt.payload, data, copySize);
    }

    memcpy(buffer, &pkt, PAYLOAD_SIZE);
    return PAYLOAD_SIZE; // tamanho real do pacote
}

// --- Função que decodifica um pacote recebido ---
bool decodePacket(uint8_t* buffer, size_t bufferSize, uint32_t &seq, uint64_t &t_device, uint8_t* payloadOut = nullptr, size_t payloadSize = PAYLOAD_SIZE) {
    if(bufferSize < PAYLOAD_SIZE) return false;

    Packet pkt;
    memcpy(&pkt, buffer, PAYLOAD_SIZE);

    seq = pkt.seq;
    t_device = pkt.t_device;

    if(payloadOut != nullptr) {
        size_t copySize = (payloadSize > PAYLOAD_SIZE) ? PAYLOAD_SIZE : payloadSize;
        memcpy(payloadOut, pkt.payload, copySize);
    }

    return true;
}

void  getUnixTime(void *arg) {
  /*time_t now = time(nullptr);
    if (now < base_unix_time) {
        return base_unix_time;
    }
    return now;*/
    struct timeval now;
    char strftime_buf[64];
    struct tm timeinfo;
    configTime(0, 0, ntpServer1, ntpServer2);

    setenv("TZ", "<-03>3", 1); // Define o fuso horário para BRT (Brasília)
    tzset();
    time(&now.tv_sec);
    // getLocalTime(&now);
    // Set timezone to Brazil
    localtime_r(&now.tv_sec, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    Serial.printf("\n  The current date/time in São Paulo is: %s\n  ", strftime_buf);
    base_unix_time = now.tv_sec; // Atualiza o tempo base;
    Serial.printf("Unix time: %ld\n", base_unix_time);
}

void notify(struct timeval* t) {
  Serial.println("Synchronized");
}

void wait4SNTP() {
    int validation;
    unsigned long start = millis();
    do {
        validation = sntp_get_sync_status();
        if (millis() - start > 20000) { // 20 seconds timeout
            Serial.println("SNTP sync timeout!");
            validation = 0;
            break;
        }
    } while (validation != SNTP_SYNC_STATUS_COMPLETED);

    if (validation == SNTP_SYNC_STATUS_COMPLETED) {
        Serial.println("SNTP sync completed!");
        // return true;
    }else{
        Serial.println("Could not sync with SNTP");
        // return false;
    }
}

void syncSNTPtime(void *arg) {
    esp_netif_init();
    if(sntp_enabled()){
        esp_sntp_stop();
    }
    // configTime(0, 0, ntpServer1, ntpServer2); // Set NTP servers
    esp_sntp_setservername(0, ntpServer1);
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH); // Use smooth sync mode for better accuracy
    sntp_set_sync_interval(1);// Set sync interval to 1 hour
    sntp_set_time_sync_notification_cb(notify); // Optional: callback on sync
    esp_sntp_servermode_dhcp(1); // Enable DHCP for NTP server address
    esp_sntp_init();
    wait4SNTP();
}


void setup() {
  Serial.begin(115200);
  lora.begin();
  lora.SpreadingFactor(12);
}

void loop()
{
  getUnixTime(nullptr);
  if(!initSNTP && WiFi.status() != WL_CONNECTED){
    syncSNTPtime(nullptr);
    initSNTP = true;
    if(!initTimer){
      do{

        getUnixTime(nullptr);

      }while ((base_unix_time % 10) != 0);
      initTimer = true;
      esp_deep_sleep_start();
    }
  }

  if(lora.receiveData(receivedBuffer, PAYLOAD_SIZE, 5000))
  {
    uint64_t t_recv = micros(); // momento de recepção do pacote

    if(decodePacket(receivedBuffer, sizeof(receivedBuffer), rcvdSeq, rcvdTime))
    {
      uint64_t t_decode = micros() - t_recv; //tempo de processamento (atual - recebimento)
      if(rcvdSeq == 0)
      {
        
        T0_recv = t_recv - rcvdTime; // tempo de recebimento - tempo no ar = tempo de envio
        T1_send = (micros() + 1581056); // momento atual + tempo no ar =
        Serial.println("Enviando pacote 1");
        Serial.println("Tempo de processamento: " + String(t_decode) + " us");
        Serial.println("Time on air: " + String(lora.timeOnAir(PAYLOAD_SIZE)) + " us");
        Serial.println("Time de envio: " + String(T0_recv) + " us");
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, T1_send);
        lora.sendData(packetBuffer, packetSize);
      }
      if(rcvdSeq == 2)
      {
        Serial.println("Tempo do disp móvel: " + String(rcvdTime));
        
        // T2_recv = t_recv + rcvdTime;
        // Serial.println("Enviando pacote 3");
        // T3_send = micros() + lora.timeOnAir(PAYLOAD_SIZE);
        // packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, T3_send);
        // lora.sendData(packetBuffer, packetSize);
      }
    }
}
}