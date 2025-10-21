#ifndef CMS_LORA_H
#define CMS_LORA_H

#include <RadioLib.h>
#include <SPI.h>
//#include "timer.h"

class CMSLoRa {
  public:
  bool isTransmitting = false;
  /**
  * @brief Aloca dinamicamente na memóra um módulo de rádio
  *   
 */
    CMSLoRa();

  /**
  * @brief Inicializa o módulo LoRa
  * 
 */
    void begin();

     /**
  * @brief Envia um pacote de dados
  * @param data pacote de dados a ser enviado
  * @param size tamanho do pacote de dados
  * 
 */
    void sendData(unsigned char* data, size_t size);

  /**
  * @brief Recebe um pacote de dados
  * @param packetBuffer variavel onde será armazenado o pacote recebido
  * @param bufferSize tamanho do buffer que irá armazenar o pacote
  * @return retorna TRUE se o dado foi recebido sem erro, tem um tamanho maior que zero e se o CRC estiver correto.
  *         Retorna FALSE caso contrário
  * 
 */
    bool receiveData(unsigned char* packetBuffer, size_t bufferSize, unsigned long timeOut = 10000);
/**
  * @brief Para o recebimento de dados
  *   
 */

    void StopReceive();
  /**
  * @brief Define o Spreading Factor da transmissão
  *   
 */
    void SpreadingFactor(int SF);
  /**
  * @brief Obtém a potência do sinal 
  *   
 */
    float getSignal();
    /**
  * @brief Obtém a taxa de envio da dados
  *   
 */
    float getDataRate();
  /**
  * @brief Obtém o tempo de ocupação do módulo para envio de dados
  *   
 */
    unsigned long timeOnAir(int size);
  /**
  * @brief define a banda de operação 
  *   
 */
    void setBandwidth(float bandwidth = 31.25);

  private:
    static constexpr uint8_t LORA_SCK    = 9;
    static constexpr uint8_t LORA_MISO   = 11;
    static constexpr uint8_t LORA_MOSI   = 10;
    static constexpr uint8_t LORA_CS     = 8;
    static constexpr uint8_t LORA_BUSY   = 13;
    static constexpr uint8_t LORA_DIO1   = 14;
    static constexpr uint8_t LORA_RESET  = 12;

    // static constexpr uint8_t LORA_SCK    = 10;
    // static constexpr uint8_t LORA_MISO   = 6;
    // static constexpr uint8_t LORA_MOSI   = 7;
    // static constexpr uint8_t LORA_CS     = 8;
    // static constexpr uint8_t LORA_BUSY   = 4;
    // static constexpr uint8_t LORA_DIO1   = 3;
    // static constexpr uint8_t LORA_RESET  = 5;


      /**
  * @brief Função a ser passada como parâmetro para o callback de envio
  *   
 */
    static void onSendDone(void);
    /**
  * @brief Função a ser passada como parâmetro para o callback de dado enviado
  *   
 */
    static void onReceiveDone(void);

    static void onOperationDone(void);
    SPIClass spiLoRa = SPIClass(HSPI);
    Module* module;
    SX1262 radio;

    //CMSTimer timerSend, timerReceiver;
    int transmissionState = RADIOLIB_ERR_NONE;
    
    static volatile bool sendDone;
    static volatile bool receiveDone;
    static volatile bool operationDone;
    bool receiving = false; // flag para indicar se está recebendo dados
    unsigned long millisSent;
    unsigned long millisReceiver;
    unsigned long timeSend = 0; // time out antes de enviar ou receber algum dado
};

#endif
