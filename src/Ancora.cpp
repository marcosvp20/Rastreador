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

}
void loop()
{
  // Use variáveis globais para TWR para maior clareza
    static uint64_t T0_recv = 0; // T0 no relógio do móvel
    static uint64_t T1_send = 0; // T1 no relógio do fixo
    static uint64_t T2_recv = 0; // T2 no relógio do móvel
    static uint64_t T3_send = 0; // T3 no relógio do fixo

    static uint32_t rcvdSeq = 0;
    static uint64_t rcvdTime = 0;

  if(lora.receiveData(receivedBuffer, PAYLOAD_SIZE, 5000))
  {
    uint64_t t_recv = micros(); // tempo de recepção do pacote
    if(decodePacket(receivedBuffer, sizeof(receivedBuffer), rcvdSeq, rcvdTime))
    {

      if(rcvdSeq == 0)
      {
        T0_recv = t_recv + rcvdTime;
        Serial.println("Enviando pacote 1");
        T1_send = micros() + lora.getTimeOnAir(PAYLOAD_SIZE); 
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, T1_send);
        lora.sendData(packetBuffer, packetSize);
      }
      if(rcvdSeq == 2)
      {
        T2_recv = t_recv + rcvdTime;
        Serial.println("Enviando pacote 3");
        T3_send = micros() + lora.getTimeOnAir(PAYLOAD_SIZE);
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, T3_send);
        lora.sendData(packetBuffer, packetSize);
      }
    }
}
}