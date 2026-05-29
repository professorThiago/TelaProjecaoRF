/**
 * @file 02_ControlarTela.ino
 * @brief TelaProjecaoRF — Exemplo 2: Controla a tela via Monitor Serial.
 *
 * @details
 * Envia os comandos CIMA, PARAR e BAIXO para a tela de projeção usando
 * o endereço previamente capturado com o exemplo 01_LerEndereco.
 *
 * Abra o Monitor Serial (115200 baud) e envie:
 *   'c' → CIMA  (sobe a tela)
 *   'p' → PARAR (para o movimento)
 *   'b' → BAIXO (desce a tela)
 *   'l' → LEARN (envia o comando de pareamento)
 *
 * @par Ligação de hardware
 * @code
 *   Módulo TX 433 MHz DATA  →  GPIO 7
 *   Módulo RX 433 MHz DATA  →  GPIO 6  (necessário para a biblioteca)
 *   VCC                     →  3.3 V ou 5 V (conforme o módulo)
 *   GND                     →  GND
 * @endcode
 *
 * @note Se a tela não responder, tente alternar `setInverterSinal(false)`.
 *       Alguns módulos TX operam com lógica invertida.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include "TelaProjecaoRF.h"

// ── Pinos ────────────────────────────────────────────────────
const uint8_t PINO_TX = 7;
const uint8_t PINO_RX = 6;

// ── Endereço da tela ─────────────────────────────────────────
// Substitua pelos bytes obtidos no exemplo 01_LerEndereco
const uint8_t ENDERECO_DA_MINHA_TELA[TelaProjecaoRF::TAMANHO_ENDERECO] = {
    0x77, 0x18, 0x16, 0x01, 0x00
};

TelaProjecaoRF telaRF(PINO_TX, PINO_RX);

// =============================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    telaRF.begin(&Serial);
    telaRF.setInverterSinal(true); // Altere para false se a tela não responder

    Serial.println("TelaProjecaoRF — Controle via Serial");
    Serial.println("Comandos: c=CIMA  p=PARAR  b=BAIXO  l=LEARN\n");
}

// =============================================================
void loop() {
    telaRF.update();

    if (Serial.available() > 0) {
        char c = Serial.read();

        switch (c) {
            case 'c': case 'C':
                Serial.println(">> Subindo tela...");
                telaRF.enviarCima(ENDERECO_DA_MINHA_TELA);
                break;

            case 'p': case 'P':
                Serial.println(">> Parando tela...");
                telaRF.enviarParar(ENDERECO_DA_MINHA_TELA);
                break;

            case 'b': case 'B':
                Serial.println(">> Descendo tela...");
                telaRF.enviarBaixo(ENDERECO_DA_MINHA_TELA);
                break;

            case 'l': case 'L':
                Serial.println(">> Enviando LEARN...");
                telaRF.enviarLearn(ENDERECO_DA_MINHA_TELA);
                break;

            case '\n': case '\r': case ' ':
                break; // ignora whitespace

            default:
                Serial.println("Comando desconhecido. Use: c, p, b ou l");
                break;
        }
    }
}
