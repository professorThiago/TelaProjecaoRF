#include <Arduino.h>
#include "TelaProjecaoRF.h"

// Pinos conectados aos modulos RF
const uint8_t PINO_TX = 7;
const uint8_t PINO_RX = 6;

TelaProjecaoRF telaRF(PINO_TX, PINO_RX);

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    // Inicializa a biblioteca passando a Serial para imprimir os logs
    telaRF.begin(&Serial);
    
    Serial.println("Pronto. Aperte qualquer botao no controle original.");
    telaRF.iniciarLeituraEndereco();
}

void loop() {
    // Essencial chamar o update dentro do loop
    telaRF.update();

    if (telaRF.enderecoCapturadoDisponivel()) {
        uint8_t meuEndereco[TelaProjecaoRF::TAMANHO_ENDERECO];
        
        if (telaRF.obterEnderecoCapturado(meuEndereco)) {
            Serial.print("Endereco Clonado com Sucesso: ");
            telaRF.imprimirEndereco(meuEndereco);
            Serial.println();
            
            // Reinicia a leitura caso queira clonar outro controle
            telaRF.iniciarLeituraEndereco();
        }
    }
}
