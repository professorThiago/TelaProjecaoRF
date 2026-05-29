/**
 * @file 01_LerEndereco.ino
 * @brief TelaProjecaoRF — Exemplo 1: Captura o endereço do controle original.
 *
 * @details
 * Coloca a biblioteca em modo de escuta RF. Ao pressionar qualquer botão
 * no controle remoto original da tela, o endereço único é capturado,
 * validado e impresso no Monitor Serial.
 *
 * Esse endereço deve ser copiado e usado nos demais exemplos para enviar
 * comandos diretamente para a tela.
 *
 * @par Ligação de hardware
 * @code
 *   Módulo RX 433 MHz DATA  →  GPIO 6
 *   Módulo TX 433 MHz DATA  →  GPIO 7  (não usado neste exemplo)
 *   VCC                     →  3.3 V ou 5 V (conforme o módulo)
 *   GND                     →  GND
 * @endcode
 *
 * @par Saída esperada no Serial Monitor (115200 baud)
 * @code
 *   [TelaProjecaoRF] Biblioteca iniciada.
 *   Aguardando sinal do controle original...
 *   [TelaProjecaoRF] Aguardando sinal RF para captura de endereco...
 *
 *   >>> Endereco capturado com sucesso! <<<
 *   Endereco: 77 18 16 01 00
 *
 *   Copie a linha abaixo para o seu codigo:
 *   const uint8_t MINHA_TELA[5] = {0x77, 0x18, 0x16, 0x01, 0x00};
 *
 *   Aguardando proximo sinal...
 * @endcode
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include "TelaProjecaoRF.h"

// ── Pinos ────────────────────────────────────────────────────
const uint8_t PINO_TX = 7;
const uint8_t PINO_RX = 6;

TelaProjecaoRF telaRF(PINO_TX, PINO_RX);

// =============================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    telaRF.begin(&Serial);

    Serial.println("Aguardando sinal do controle original...");
    Serial.println("Pressione qualquer botao no controle da tela.\n");

    telaRF.iniciarLeituraEndereco();
}

// =============================================================
void loop() {
    // update() deve ser chamado sempre para detectar fim de quadro
    telaRF.update();

    if (telaRF.enderecoCapturadoDisponivel()) {
        uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO];

        if (telaRF.obterEnderecoCapturado(endereco)) {
            Serial.println("\n>>> Endereco capturado com sucesso! <<<");
            Serial.print("Endereco: ");
            telaRF.imprimirEndereco(endereco);
            Serial.println();

            // Imprime a linha pronta para copiar ao código
            Serial.print("const uint8_t MINHA_TELA[5] = {");
            for (uint8_t i = 0; i < TelaProjecaoRF::TAMANHO_ENDERECO; i++) {
                Serial.print("0x");
                if (endereco[i] < 0x10) Serial.print("0");
                Serial.print(endereco[i], HEX);
                if (i + 1 < TelaProjecaoRF::TAMANHO_ENDERECO) Serial.print(", ");
            }
            Serial.println("};");

            Serial.println("\nAguardando proximo sinal...\n");

            // Reinicia a leitura para capturar outro controle, se necessário
            telaRF.iniciarLeituraEndereco();
        }
    }
}
