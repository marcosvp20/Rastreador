#include "cmslora.h"

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


void setup() {
  Serial.begin(115200);
  lora.begin();
  lora.SpreadingFactor(12);
  t_1 = micros();
}
void loop()
{
  static uint32_t rcvdSeq = 0;
  static uint64_t rcvdTime = 0;
  uint64_t nextTime = micros();
  if((!rcvdSeq || seq == 0))
  {
  Serial.println("Enviando pacote 0");
  packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), seq, t_1);
  lora.sendData(packetBuffer, packetSize);
  seq++;
  }
  if(lora.receiveData(receivedBuffer, PAYLOAD_SIZE, 5000))
  {
    uint64_t t_pre = micros();
    if(decodePacket(receivedBuffer, sizeof(receivedBuffer), rcvdSeq, rcvdTime))
    {
      if(rcvdSeq == 1)
      {
        Serial.println("Enviando pacote 3");
        t_2 = micros() - t_pre + lora.getTimeOnAir(PAYLOAD_SIZE);
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, t_2);
        lora.sendData(packetBuffer, packetSize);
      }
      if(rcvdSeq == 3)
      {
        Serial.println("RTT: " + String(t_2 - t_1) + " us");
        seq = 0;
        rcvdSeq = 0;
      }

    }
}
}