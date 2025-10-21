#include "cmslora2.h"

CMSLoRa2 lora;

#include <Arduino.h>
#include <string.h> // para memcpy

#define PAYLOAD_SIZE 20

uint8_t packetBuffer[sizeof(Packet)];
uint8_t receivedBuffer[sizeof(Packet)];
int seq = 0;
size_t packetSize = 0;
unint64_t t_1 = 0;
uint64_t t_2 = 0;

struct Packet {
    uint32_t seq;
    uint64_t t_device;
    uint8_t payload[PAYLOAD_SIZE];
};

// --- Função que cria o pacote a partir de seq e payload ---
size_t buildPacket(uint8_t* buffer, size_t bufferSize, uint32_t seq, uint64_t t_device, uint8_t* data = nullptr, size_t dataSize = 0) {
    if(bufferSize < sizeof(Packet)) return 0; // buffer pequeno

    Packet pkt;
    pkt.seq = seq;
    pkt.t_device = t_device;
    memset(pkt.payload, 0, PAYLOAD_SIZE);

    if(data != nullptr && dataSize > 0) {
        size_t copySize = (dataSize > PAYLOAD_SIZE) ? PAYLOAD_SIZE : dataSize;
        memcpy(pkt.payload, data, copySize);
    }

    memcpy(buffer, &pkt, sizeof(Packet));
    return sizeof(Packet); // tamanho real do pacote
}

// --- Função que decodifica um pacote recebido ---
bool decodePacket(uint8_t* buffer, size_t bufferSize, uint32_t &seq, uint64_t &t_device, uint8_t* payloadOut = nullptr, size_t payloadSize = PAYLOAD_SIZE) {
    if(bufferSize < sizeof(Packet)) return false;

    Packet pkt;
    memcpy(&pkt, buffer, sizeof(Packet));

    seq = pkt.seq;
    t_device = pkt.t_device;

    if(payloadOut != nullptr) {
        size_t copySize = (payloadSize > PAYLOAD_SIZE) ? PAYLOAD_SIZE : payloadSize;
        memcpy(payloadOut, pkt.payload, copySize);
    }

    return true;
}


void setup() {
  lora.begin();
  lora.SpreadingFactor(12);

}
void loop()
{
  if(lora.receiveData(receivedBuffer, PAYLOAD_SIZE, 5000))
  {
    uint32_t rcvdSeq;
    uint64_t rcvdTime;
    uint64_t nextTime = micros();
    if(decodePacket(receivedBuffer, sizeof(receivedBuffer), rcvdSeq, rcvdTime))
    {
      if(rcvdSeq == 1)
      {
        t_1 = micros();
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, nextTime);
        lora.send(packetBuffer, packetSize);
      }
      if(rcvdSeq == 3)
      {
        t_2 = micros();
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, nextTime);
        lora.send(packetBuffer, packetSize);
      }
        if(rcvdSeq == 4)
        {
            Serial.println("RTT: " + String(t_2 - t_1) + " us");
        }
    }
}
}