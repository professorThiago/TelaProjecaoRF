#include <Arduino.h>
#include "TelaProjecaoRF.h"

const uint8_t PINO_TX = 7;
const uint8_t PINO_RX = 6;

TelaProjecaoRF telaRF(PINO_TX, PINO_RX);

// Insira aqui o endereco capturado no exemplo anterior
const uint8_t ENDERECO_DA_MINHA_TELA[5] = {0x77, 0x18, 0x16, 0x01, 0x00};

void setup() {
    Serial.begin(9600);
    telaRF.begin(&Serial);
    telaRF.setInverterSinal(true); // Ajuste conforme seu hardware

    Serial.println("Envie 'c' para CIMA, 'p' para PARAR, 'b' para BAIXO.");
}

void loop() {
    telaRF.update();

    if (Serial.available() > 0) {
        char c = Serial.read();

        if (c == 'c') {
            Serial.println("Subindo tela...");
            telaRF.enviarCima(ENDERECO_DA_MINHA_TELA);
        } 
        else if (c == 'p') {
            Serial.println("Parando tela...");
            telaRF.enviarParar(ENDERECO_DA_MINHA_TELA);
        } 
        else if (c == 'b') {
            Serial.println("Descendo tela...");
            telaRF.enviarBaixo(ENDERECO_DA_MINHA_TELA);
        }
    }
}
