#ifndef LORAPACKET_H
#define LORAPACKET_H

#include <Arduino.h>
#include <string.h>
#include "AES.h"
//identificadores de codificação e decoficação dos pacotes
#define IDFirstConnection 0x1
#define IDToken 0x2
#define IDData 0x3
#define IDIns 0x4
#define IDDiscovery 0x5
// identificadores de instruções
#define IDConfirmation 0x1
#define NoMatchPassword 0x2
#define ConnectionSucessfully 0x3
#define Desconnect 0x4

#define SizePacket 64

struct ReceivedData {
  unsigned char ID = 0xFF;
  unsigned char ByteID = 0xFF;
  String ssid = "";
  String senha = "";
  String token = "";  
  float data = 0.0;
  unsigned char ins = 0xFF;
};
  /** 
  * @brief Monta o pacote, codifica e decodifica o pacote de dados
  * 
 */
class LoRaPacket {
private:
  unsigned char packet[SizePacket] ={0};
  AES128 aes;
  unsigned char aes_key[16] ={0};
  unsigned char cryptedpacket[SizePacket] ={0};
  unsigned char aux[16] ={0};
  unsigned char crypt_[16] ={0};
  char temp[SizePacket];
  /**
   @brief verifica se há risco de overflow iminente
  */
  bool verifyOverflow(int index);
public:
  ReceivedData RD; // cria a estrutura de dados 
  /**
  * @brief Retorna o tamanho dos dados passados, em bytes.
  * 
 */
  int PacketSize(String ssid = "", String senha = "", String token = "", int dado_length = 0);

  /**
  * @brief Cria um pacote para envio de dados para a conexão na rede LoRa
  * @param returnVariable Variável onde o pacote será salvo
  * 
 */
  void ConnectionPacket(unsigned char ID, String ssid, String senha, unsigned char* returnVariable = nullptr);

  /**
  * @brief Cria um pacote para envio do token
  * @param returnVariable Variável onde o pacote será salvo
  * 
 */
  void TokenPacket(unsigned char ID, String token, unsigned char* returnVariable = nullptr);

   /**
  * @brief Cria um pacote para envio de dado
  * @param returnVariable Variável onde o pacote será salvo
  * 
 */
  void DataPacket(unsigned char ID, String token, float data, unsigned char* returnVariable = nullptr);

   /**
  * @brief Cria um pacote para envio de instruções
  * @param returnVariable Variável onde o pacote será salvo
  * 
 */
  void InsPacket(unsigned char ID, int data_length ,unsigned char data, unsigned char* returnVariable = nullptr);

  void DiscoveryPacket(unsigned char ID, String SSID, unsigned char* returnVariable = nullptr);

   /**
  * @brief Transforma dados binários em string
  * @param packet pacote onde está o dado
  * @param length tamanho do dado, em bytes
  * @param index posição do dado no vetor packet
  * 
 */
  String toString(unsigned char* packet, int length, int index);

   /**
  * @brief Decodifica os dados do pacote e salva numa struct, conforme o tipo de pacote
  * @param pckt Pacote a ser decodificado 
  * @param token chave de descritografia (só é necessário passá-lo para receber pacotes criptografados)
  * 
  * 
 */
  ReceivedData packetReceiver(unsigned char* pckt, String token = "");

     /**
  * @brief Criptografa o pacote de dados usando o token como chave de criptografia
 */
  void crypt(String token);

       /**
  * @brief Descriptografa um pacote de dados usando o token como chave de descriptografia
 */
  void decrypt(unsigned char* packet, String token);

};

#endif
