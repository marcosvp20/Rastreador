#include "lorapacket.h"


int LoRaPacket::PacketSize(String ssid, String senha, String token, int dado_length) {
    int ssid_length = ssid.length();
    int senha_length = senha.length();
    int token_length = token.length();
    return 1 + 1 + sizeof(ssid_length) + ssid_length + sizeof(senha_length) + senha_length + sizeof(token_length) + token_length + dado_length;
}
void LoRaPacket::crypt(String token)
{
token.getBytes(aes_key, 16); // transforma o token (chave de critografia) de string para byte
// a chave de criptografia pode ter, no máximo, 16 bytes

 aes.setKey(aes_key, 16); // define o token como chave de criptografia
 int index = 2; //começa do dois pois o ID e o byteID não serão criptografados
 memset(aux, 0, 16); // zera o vetor auxiliar
 for(int i = 0; i < (SizePacket/16); i++) //
 {
    for(int j = 0; j < 16; j++)
    {
        if(index < SizePacket){
        aux[j] = packet[index++];
        // divide o pacote em pacotes de 16 bytes (limite da biblioteca para criptografia)
        // como as duas primeiras posições do pacote serão mantidas, este if serve para evitar segment fault
        }
    }
    aes.encryptBlock(crypt_, aux); //Faz a criptografia 
    memcpy(&cryptedpacket[i*16], &crypt_, 16); // copia os dados para o vetor de dados criptografados 
    // repete o processo (SizePacket/16) vezes
 } 
 index = 2;
 for(int i = 0; i < SizePacket-2; i++)
 {
    packet[i+2] = cryptedpacket[i];
    // sobreescreve o pacote de dados não criptografados com os dados criptografados
 }
}

void LoRaPacket::decrypt(unsigned char* Dpacket, String token)
{
token.getBytes(aes_key, 16); // transforma o token (chave de critografia) de string para byte
// a chave de criptografia pode ter, no máximo, 16 bytes
aes.setKey(aes_key, 16); // define o token como chave de criptografia
memset(aux, 0, 16); // zera o vetor auxiliar

int index = 2; //começa do dois pois o ID e o byteID não estão criptografados
for(int i = 0; i < (SizePacket/16); i++)
 {
    for(int j = 0; j < 16; j++)
    {
        if(index < SizePacket){
        aux[j] = Dpacket[index++];
        // divide o pacote em pacotes com 16 bytes (limite da biblioteca para descriptografia)
        }
    }
    aes.decryptBlock(crypt_, aux);
    memcpy(&cryptedpacket[i*16], &crypt_, 16);
 }
 index = 2;
 for(int i = 0; i < SizePacket-2; i++)
 {
    Dpacket[index++] = cryptedpacket[i]; // sobreescreve o pacote de dados criptografados com os dados descriptografados
 }

}

void LoRaPacket::ConnectionPacket(unsigned char ID, String ssid, String senha, unsigned char* returnVariable) {
    int ssid_length = ssid.length();
    int senha_length = senha.length();
    unsigned char ByteID = IDFirstConnection;
    int packet_length = 1 + 1 + sizeof(ssid_length) + ssid_length + sizeof(senha_length) + senha_length; // calcula o tamanho do pacote
    
    if(packet_length > SizePacket){
        Serial.println("[lorapacket] Pacote muito grande!");
        return;
    }
    
    int index = 0;
    packet[index++] = ID; // ID ocupa a primeira posição
    packet[index++] = ByteID; // ByteID ocupa a segunda posição

    memcpy(&packet[index], &ssid_length, sizeof(ssid_length)); // copia o tamanho do SSID para o pacote
    index += sizeof(ssid_length); // itera o indexador
    memcpy(&packet[index], ssid.c_str(), ssid_length); // copia o SSID para o pacote
    index += ssid_length;

    memcpy(&packet[index], &senha_length, sizeof(senha_length)); // copia o tamanho da senha para o pacote
    index += sizeof(senha_length);
    memcpy(&packet[index], senha.c_str(), senha_length); //copia a senha para o pacote
    index += senha_length;

    for (int i = 0; i < packet_length; i++) {
        returnVariable[i] = packet[i];
        // Copia o pacote criado para o pacote passado para a função
    }
}

void LoRaPacket::TokenPacket(unsigned char ID, String token, unsigned char* returnVariable) {
    int token_length = token.length();
    unsigned char ByteID = IDToken;
    int packet_length = 1 + 1 + sizeof(token_length) + token_length; // calcula o tamanho do pacote

    if(packet_length > SizePacket){
        Serial.println("[lorapacket] Pacote muito grande!");
        return;
    }

    int index = 0;
    packet[index++] = ID;
    packet[index++] = ByteID;

    memcpy(&packet[index], &token_length, sizeof(token_length)); // copia o tamanho do token para o pacote
    index += sizeof(token_length);
    memcpy(&packet[index], token.c_str(), token_length); // copia o token para o pacote
    index += token_length;

    for (int i = 0; i < packet_length; i++) {
        returnVariable[i] = packet[i];
        // copia os pacote criado para o vetor passado pela função
    }
}

void LoRaPacket::DataPacket(unsigned char ID, String token, float data, unsigned char* returnVariable) {
    int token_length = token.length();
    int data_length = sizeof(data);
    unsigned char ByteID = IDData;
    int packet_length = 1 + 1 + sizeof(token_length) + token_length + sizeof(data_length) + data_length; // calcula o tamanho do pacote
    if(packet_length > SizePacket){
        Serial.println("[lorapacket] Pacote muito grande!");
        return;
    }

    int index = 0;
    packet[index++] = ID;
    packet[index++] = ByteID;

    memcpy(&packet[index], &token_length, sizeof(token_length)); // copia o tamanho do token para o pacote
    index += sizeof(token_length);
    memcpy(&packet[index], token.c_str(), token_length); // copia o token para o pacote
    index += token_length;

    memcpy(&packet[index], &data_length, sizeof(data_length)); // copia o tamanho do dado para o pacote
    index += sizeof(data_length);
    memcpy(&packet[index], &data, data_length); // copia o dado para o pacote
    index += data_length;
    crypt(token); // criptografa o pacote usando o token como chave

    for (int i = 0; i < SizePacket; i++) {
        returnVariable[i] = packet[i];
        // copia o pacote para o vetor passado pela função
    }
 }

void LoRaPacket::InsPacket(unsigned char ID, int data_length ,unsigned char data, unsigned char* returnVariable)
{
    unsigned char ByteID = IDIns;
    int packet_length = 1 + 1 + sizeof(data_length) + data_length;
    int index = 0;

    if(packet_length > SizePacket){
        Serial.println("[lorapacket] Pacote muito grande!");
        return;
    }

    packet[index++] = ID;
    packet[index++] = ByteID;

    memcpy(&packet[index], &data_length, sizeof(data_length));
    index += sizeof(data_length);
    packet[index++] = data;

    for (int i = 0; i < packet_length; i++) {
        returnVariable[i] = packet[i];
    }
}

void LoRaPacket::DiscoveryPacket(unsigned char ID, String ssid, unsigned char* returnVariable)
{
    int ssid_length = ssid.length();
    int packet_length = 1 + 1 + sizeof(ssid_length)+ ssid_length;
    int index = 0;

    if(packet_length > SizePacket){
        Serial.println("[lorapacket] Pacote muito grande!");
        return;
    }

    packet[index++] = ID;
    packet[index++] = IDDiscovery;

    memcpy(&packet[index], &ssid_length, sizeof(ssid_length));
    index += sizeof(ssid_length);
    memcpy(&packet[index], ssid.c_str(), ssid_length); // copia o SSID para o pacote
    index += ssid_length;

    for (int i = 0; i < packet_length; i++) {
        returnVariable[i] = packet[i];
    }
}

String LoRaPacket::toString(unsigned char* packet, int length, int index) { // transforma um array de char em string
    memcpy(&temp[0], &packet[index], length);
    temp[length] = '\0';
    return String(temp);
}
// o overflow pode acontecer em casos em que o token de descriptografia for diferente do token de criptografia
bool LoRaPacket::verifyOverflow(int index) {
    if (index >= SizePacket) {
        return false; // Retorna falso se o índice ultrapassar o tamanho do pacote
    }
    else if(index < 0) {
        Serial.println("[lorapacket] Índice negativo detectado!");
        return false; // Retorna falso se o índice for negativo
    }
    return true; // Retorna verdadeiro se não houver overflow
}

ReceivedData LoRaPacket::packetReceiver(unsigned char* pckt, String token) {
    
    ReceivedData RD; // cria a estrutura de dados 

    int index = 0;
    RD.ID = pckt[index++];
    RD.ByteID = pckt[index++];
    // ID e ByteID não são criptografados pois eles são necessários para saber qual será o tipo de dado a ser processado

    if (RD.ByteID == IDFirstConnection) { // se o pacote for referente a primeira conexão na rede
        int ssid_length = 0;
        int senha_length = 0;
        // como a estrutura do pacote é conhecida, sabemos quais informações estamos buscando primeiro
        memcpy(&ssid_length, &pckt[index], 4); // obtém o tamanho do SSID, como é conhecido que esse número é um inteiro, ele ocupa 4 posições no pacote
        index += 4; // itera para a posição do próximo dado
        RD.ssid = toString(pckt, ssid_length, index); // obtém o SSID e o transforma em String
        index += ssid_length;

        memcpy(&senha_length, &pckt[index], 4); // obtém o tamanho da senha
        index += 4;
        RD.senha = toString(pckt, senha_length, index); // obtém a senha e a transforma em string
        index += senha_length;
    }

    else if (RD.ByteID == IDToken) { // pacote do tipo token
        int token_length = 0;
        memcpy(&token_length, &pckt[index], 4); // obtém o tamanho do token
        index += 4;
        RD.token = toString(pckt, token_length, index); // obtém o token e o transforma em string
        index += token_length;
    }

    else if (RD.ByteID == IDData) { // se for pacote de dados
    
        decrypt(pckt, token); // descriptografa o pacote

        int token_length = 0;
        int data_length = 0;

        memcpy(&token_length, &pckt[index], 4); // obtém o tamanho do token
        index += 4;

        if(!verifyOverflow(index + token_length)) {
            Serial.println("[lorapacket] Overflow detectado");
            return RD; // Retorna RD vazio se houver overflow
        }
        RD.token = toString(pckt, token_length, index); //obtém o token
        index += token_length;
        if(!verifyOverflow(index)) {
            Serial.println("[lorapacket] Overflow detectado");
            return RD; // Retorna RD vazio se houver overflow
        }
        memcpy(&data_length, &pckt[index], 4); // obtém o tamanho do dado
        index += 4;
        if(!verifyOverflow(index)) {
            Serial.println("[lorapacket] Overflow detectado");
            return RD; // Retorna RD vazio se houver overflow
        }
        memcpy(&RD.data, &pckt[index], data_length); // obtém o dado
        Serial.print("[lorapacket] Dado recebido: ");
        Serial.println(RD.data);
    }

    else if(RD.ByteID == IDIns){ // pacote de instruções
        int data_length = 0;
        memcpy(&data_length, &pckt[index], 4); // obtém o tamanho da intrução
        index += 4;
        
        RD.ins = pckt[index]; // obtém a instrução
    }
    else if(RD.ByteID == IDDiscovery){
        int ssid_length = 0;
        // como a estrutura do pacote é conhecida, sabemos quais informações estamos buscando primeiro
        memcpy(&ssid_length, &pckt[index], 4); // obtém o tamanho do SSID, como é conhecido que esse número é um inteiro, ele ocupa 4 posições no pacote
        index += 4; // itera para a posição do próximo dado
        RD.ssid = toString(pckt, ssid_length, index); // obtém o SSID e o transforma em String
    }
    else
    {
        Serial.println("ID desconhecido");
    }
    return RD;
}