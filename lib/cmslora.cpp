#include "cmslora.h"
#include "lorapacket.h"
LoRaPacket pack;
ReceivedData rd;
volatile bool CMSLoRa::sendDone = false;
volatile bool CMSLoRa::receiveDone = false;
//Callback chama automaticamente quando o envio é concluido
void CMSLoRa::onSendDone(void)
{
  sendDone = true;
}
//Callback chama automaticamente quando o recebimento é concluido
void CMSLoRa::onReceiveDone(void)
{
  receiveDone = true;
}
// coloca o rádio em modo StandBy, interrompendo o recebimento
void CMSLoRa::StopReceive()
{
  radio.standby();
}

void CMSLoRa::SpreadingFactor(int SF)
{
  radio.setSpreadingFactor(SF);
}

float CMSLoRa::getSignal(){
  return radio.getRSSI();
}

float CMSLoRa::getDataRate()
{
  return radio.getDataRate();
}
// retorna o tempo que o rádio ficará ocupado enviando o pacote 
unsigned long CMSLoRa::timeOnAir(int size)
{
return radio.getTimeOnAir(size);
}
void CMSLoRa::setBandwidth(float bandwidth)
{
  radio.setBandwidth(bandwidth);
  
}

CMSLoRa::CMSLoRa()
  : module(new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, spiLoRa)),
    radio(module){}
// inicializa o LoRa, os pinos do spi são para o heltec wifi lora v3
void CMSLoRa::begin() {
  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(915.0, 125, 7);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[SX1262] Inicializado com sucesso!");
  } else {
    Serial.print(F("[SX1262] Erro ao inicializar: "));
    Serial.println(state);
  }
  radio.setCRC(true); // habilita a verificação de integridade
  radio.setPacketSentAction(onSendDone);  //define o callback de envio
  radio.setPacketReceivedAction(onReceiveDone); // define o callback de recepção
  radio.setWhitening(false);
}

void CMSLoRa::sendData(unsigned char* data, size_t size) {
  //if(receiving) {return;}
  sendDone = false;
  int transmissionState = radio.transmit(data, size); //essa função é do tipo bloqueante, ou seja, a execução fica nessa linha até
  // que o pacote seja enviado, definido pelo time on air.
    if(transmissionState != RADIOLIB_ERR_NONE)
    {
      Serial.print(F("[cmsLoRa] Erro ao iniciar envio: "));
      Serial.println(transmissionState);
      return;
    }
    
}
bool CMSLoRa::receiveData(unsigned char* packetBuffer, size_t bufferSize, unsigned long timeOut){
    receiveDone = false;
    radio.startReceive(); // inicia o recebimento no modo não bloqueante
    unsigned long startTime = millis(); // guarda o tempo em que o recebimento começou
    while (!receiveDone) { // fica nesse loop enquanto o dado não for recebido ou se der o timeout, a implementação sem o while não funcionou
        if (millis() - startTime > timeOut) {
            Serial.println("[cmsLora] Timeout ao receber dados");
            return false;
        }
      yield(); // permite que o sistema execute tarefas internas durante o loop e não dê erro de watchdog
    }
    if(radio.getPacketLength() > bufferSize){
      Serial.println("[cmsLoRa] pacote muito grande");
        return false;
      }
    int state = radio.readData(packetBuffer, bufferSize); // passa os dados recebido para o packetbuffer

    if(state == RADIOLIB_ERR_NONE){ //verifica se o código de erro é do tipo "nenhum erro"
      return true;
    }
    else{
      Serial.print("[cmsLoRa] Erro ao receber dados:");
      Serial.println(state);
      return false;
    }
}
