/**
 * @file 03_SalvarControles.ino
 * @brief TelaProjecaoRF — Exemplo 3: Gerencia até 3 controles salvos em memória flash.
 *
 * @details
 * Usa a biblioteca `Preferences` (NVS do ESP32) para salvar e recuperar
 * até 3 endereços RF em memória não-volátil. Os endereços persistem mesmo
 * após desligar o ESP32.
 *
 * @par Comandos disponíveis no Monitor Serial (115200 baud)
 * | Comando | Ação |
 * |---------|------|
 * | `l1` `l2` `l3` | Entra em modo de escuta e salva o endereço capturado |
 * | `c1` `c2` `c3` | Envia CIMA para a tela correspondente |
 * | `p1` `p2` `p3` | Envia PARAR para a tela correspondente |
 * | `b1` `b2` `b3` | Envia BAIXO para a tela correspondente |
 * | `listar`        | Mostra todos os endereços salvos |
 * | `apagar`        | Apaga toda a memória |
 *
 * @par Ligação de hardware
 * @code
 *   Módulo TX 433 MHz DATA  →  GPIO 7
 *   Módulo RX 433 MHz DATA  →  GPIO 6
 *   VCC                     →  3.3 V ou 5 V (conforme o módulo)
 *   GND                     →  GND
 * @endcode
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include <Preferences.h>
#include "TelaProjecaoRF.h"

// ── Pinos ────────────────────────────────────────────────────
const uint8_t PINO_TX = 7;
const uint8_t PINO_RX = 6;

// ── Constantes ───────────────────────────────────────────────
const uint8_t  MAX_CONTROLES       = 3;
const char*    NAMESPACE_NVS       = "telas_rf";

TelaProjecaoRF telaRF(PINO_TX, PINO_RX);
Preferences    preferences;

int controleParaAprender = -1;  // -1 = não está aguardando leitura

// =============================================================
// Helpers de persistência (NVS)
// =============================================================

/** Gera a chave NVS para o controle N (ex: "ctrl1"). */
String obterChave(int controle) {
    return "ctrl" + String(controle);
}

void salvarControle(int controle, const uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO]) {
    String chave = obterChave(controle);
    preferences.putBytes(chave.c_str(), endereco, TelaProjecaoRF::TAMANHO_ENDERECO);

    Serial.print("Controle ");
    Serial.print(controle);
    Serial.print(" salvo! Endereco: ");
    telaRF.imprimirEndereco(endereco);
    Serial.println();
}

bool carregarControle(int controle, uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO]) {
    String chave = obterChave(controle);
    if (preferences.getBytesLength(chave.c_str()) != TelaProjecaoRF::TAMANHO_ENDERECO) return false;
    preferences.getBytes(chave.c_str(), endereco, TelaProjecaoRF::TAMANHO_ENDERECO);
    return true;
}

void listarControles() {
    Serial.println("\n--- Controles Salvos ---");
    for (int i = 1; i <= MAX_CONTROLES; i++) {
        uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO];
        Serial.print("  Controle ");
        Serial.print(i);
        Serial.print(": ");
        if (carregarControle(i, endereco)) {
            telaRF.imprimirEndereco(endereco);
            Serial.println();
        } else {
            Serial.println("[Vazio — use l" + String(i) + " para aprender]");
        }
    }
    Serial.println("------------------------\n");
}

void imprimirMenu() {
    Serial.println("\n========== GERENCIADOR DE CONTROLES ==========");
    Serial.println("  l1/l2/l3  -> Aprender e salvar controle 1, 2 ou 3");
    Serial.println("  c1/c2/c3  -> Enviar CIMA para a tela");
    Serial.println("  p1/p2/p3  -> Enviar PARAR para a tela");
    Serial.println("  b1/b2/b3  -> Enviar BAIXO para a tela");
    Serial.println("  listar    -> Mostrar enderecos salvos");
    Serial.println("  apagar    -> Limpar toda a memoria");
    Serial.println("==============================================\n");
}

// =============================================================
// Processamento de comandos Serial
// =============================================================

void processarSerial() {
    if (!Serial.available()) return;

    String entrada = Serial.readStringUntil('\n');
    entrada.trim();
    entrada.toLowerCase();

    if (entrada == "listar") {
        listarControles();
        return;
    }

    if (entrada == "apagar") {
        preferences.clear();
        Serial.println("Memoria apagada!");
        listarControles();
        return;
    }

    if (entrada.length() != 2) {
        Serial.println("Comando invalido. Digite 'listar' para ver as opcoes.");
        return;
    }

    char    acao     = entrada[0];
    int     controle = entrada[1] - '0';

    if (controle < 1 || controle > MAX_CONTROLES) {
        Serial.printf("Erro: use controles de 1 a %d.\n", MAX_CONTROLES);
        return;
    }

    if (acao == 'l') {
        controleParaAprender = controle;
        Serial.printf("Aguardando sinal RF para salvar no Controle %d...\n", controle);
        Serial.println("Pressione qualquer botao no controle original.");
        telaRF.iniciarLeituraEndereco();
        return;
    }

    // Ações de transmissão: c, p, b
    uint8_t enderecoCarregado[TelaProjecaoRF::TAMANHO_ENDERECO];
    if (!carregarControle(controle, enderecoCarregado)) {
        Serial.printf("Erro: Controle %d nao configurado. Use l%d primeiro.\n", controle, controle);
        return;
    }

    switch (acao) {
        case 'c':
            Serial.printf("Subindo tela %d...\n", controle);
            telaRF.enviarCima(enderecoCarregado);
            break;
        case 'p':
            Serial.printf("Parando tela %d...\n", controle);
            telaRF.enviarParar(enderecoCarregado);
            break;
        case 'b':
            Serial.printf("Descendo tela %d...\n", controle);
            telaRF.enviarBaixo(enderecoCarregado);
            break;
        default:
            Serial.println("Acao desconhecida. Use c, p ou b.");
            break;
    }
}

// =============================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    telaRF.begin(&Serial);
    telaRF.setInverterSinal(true); // Altere para false se a tela não responder

    preferences.begin(NAMESPACE_NVS, false);

    imprimirMenu();
    listarControles();
}

// =============================================================
void loop() {
    // 1. Atualiza a biblioteca RF
    telaRF.update();

    // 2. Lê os comandos do Monitor Serial
    processarSerial();

    // 3. Verifica se capturou um endereço no modo "Aprender"
    if (controleParaAprender != -1 && telaRF.enderecoCapturadoDisponivel()) {
        uint8_t novoEndereco[TelaProjecaoRF::TAMANHO_ENDERECO];
        if (telaRF.obterEnderecoCapturado(novoEndereco)) {
            salvarControle(controleParaAprender, novoEndereco);
            controleParaAprender = -1;
            imprimirMenu();
        }
    }
}
