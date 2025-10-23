#include "cmslora.h"

CMSLoRa lora;

#include <Arduino.h>
#include <string.h> // para memcpy

#define PAYLOAD_SIZE 20

uint8_t packetBuffer[PAYLOAD_SIZE];
uint8_t receivedBuffer[PAYLOAD_SIZE];
int seq = 0;
size_t packetSize = 0;

  // Use variáveis globais para TWR para maior clareza
   uint64_t T0_send = 0; // T0 no relógio do móvel
   uint64_t T1_recv = 0; // T1 no relógio do fixo
   uint64_t T2_send = 0; // T2 no relógio do móvel
   uint64_t T3_recv = 0; // T3 no relógio do fixo
   uint64_t T_offset = 0; // T3 no relógio do fixo
   uint64_t T_process = 0; // T3 no relógio do fixo

   uint32_t rcvdSeq = 0;
   uint64_t rcvdTime = 0;
   uint64_t RTT = 0;

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

  
  if((!rcvdSeq || seq == 0))
  {
  seq = 0;
  Serial.println("Enviando pacote 0");
  packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), seq, 0);
  T0_send = micros();
  lora.sendData(packetBuffer, packetSize);
  seq=1;
  }
  if(lora.receiveData(receivedBuffer, PAYLOAD_SIZE, 5000))
  {
    uint64_t t_recv = micros(); // tempo de recepção do pacote
    if(decodePacket(receivedBuffer, sizeof(receivedBuffer), rcvdSeq, rcvdTime))
    {
      if(rcvdSeq == 1)
      {
        uint64_t T_processTEMP = micros();
        RTT = (micros() - T0_send)/2;
        Serial.println("RTT estimado: " + String(RTT) + " us");
        Serial.println("Distancia estimada = " + String(((RTT * 10e-6) * 299792458)) + " m");
        
        Serial.println("Enviando pacote 3");

        T_process = micros() - T_processTEMP;
        Serial.println("Tempo de processamento: " + String(T_process) + " us");
        T2_send = T_process + micros() + lora.timeOnAir(PAYLOAD_SIZE);
        Serial.println("Tempo do dispositivo móvel: " + String(T2_send) + " us");
        packetSize = buildPacket(packetBuffer, sizeof(packetBuffer), ++rcvdSeq, T2_send);
        lora.sendData(packetBuffer, packetSize);
      }
      // if(rcvdSeq == 3)
      // {
      //   uint64_t T3_recv = t_recv + rcvdTime;
      //   Serial.println("Medição Completa!");
      //           Serial.println("T0 (Móvel Envio): " + String(T0_send) + " us");
      //           Serial.println("T1 (Âncora Envio): " + String(T1_recv) + " us");
      //           Serial.println("T2 (Móvel Envio): " + String(T2_send) + " us");
      //           Serial.println("T3 (Âncora Envio): " + String(T3_recv) + " us");
                
      //           Serial.println("RTT (us) - Cálculo Simplificado (Ainda impreciso sem os 6 tempos): ");
      //           // Seu cálculo de RTT precisa de T1_recv e T3_recv da Âncora no pacote.
      //           // A implementação de TWR-S (Two-Way Ranging Symmetric) exigiria mais dados no pacote.
                
      //           // Para o seu RTT atual (4 pacotes com ToA compensado):
      //           Serial.println("RTT parcial: " + String(T1_recv - T0_send) + " us");
      //           Serial.println("RTT parcial: " + String(T3_recv - T2_send) + " us");
      //           Serial.println("RTT total : " + String((T3_recv - T2_send)-(T1_recv - T0_send)) + " us");
      //           Serial.println("RTT total : " + String((T3_recv - T0_send)-(T2_send - T1_recv)) + " us");

      //   seq = 0;
      //   rcvdSeq = 0;
      // }

    }
}
}