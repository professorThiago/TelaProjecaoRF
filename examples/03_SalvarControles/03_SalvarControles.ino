#include <Arduino.h>
#include <Preferences.h>
#include "TelaProjecaoRF.h"

// Pinos conectados aos módulos RF
const uint8_t PINO_TX = 7;
const uint8_t PINO_RX = 6;

TelaProjecaoRF telaRF(PINO_TX, PINO_RX);
Preferences preferences;

int controleParaAprender = -1; // -1 significa que não está aguardando leitura

void imprimirMenu() {
    Serial.println("\n========== GERENCIADOR DE CONTROLES ==========");
    Serial.println("Digite o comando e aperte ENTER:");
    Serial.println("  l1, l2, l3 -> Ler e salvar controle 1, 2 ou 3");
    Serial.println("  c1, c2, c3 -> Enviar comando CIMA");
    Serial.println("  p1, p2, p3 -> Enviar comando PARAR");
    Serial.println("  b1, b2, b3 -> Enviar comando BAIXO");
    Serial.println("  listar     -> Mostrar os enderecos salvos");
    Serial.println("  apagar     -> Limpar toda a memoria");
    Serial.println("==============================================");
}

// Função auxiliar para gerar a chave de salvamento (ex: "ctrl1", "ctrl2")
String obterChave(int controle) {
    return "ctrl" + String(controle);
}

void salvarControle(int controle, const uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO]) {
    String chave = obterChave(controle);
    preferences.putBytes(chave.c_str(), endereco, TelaProjecaoRF::TAMANHO_ENDERECO);
    
    Serial.print("Controle ");
    Serial.print(controle);
    Serial.print(" salvo na memoria com sucesso! Endereco: ");
    telaRF.imprimirEndereco(endereco);
    Serial.println();
}

bool carregarControle(int controle, uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO]) {
    String chave = obterChave(controle);
    
    // Verifica se a chave existe e tem o tamanho correto (5 bytes)
    if (preferences.getBytesLength(chave.c_str()) == TelaProjecaoRF::TAMANHO_ENDERECO) {
        preferences.getBytes(chave.c_str(), endereco, TelaProjecaoRF::TAMANHO_ENDERECO);
        return true;
    }
    return false;
}

void listarControles() {
    Serial.println("\n--- Controles Salvos ---");
    for (int i = 1; i <= 3; i++) {
        uint8_t endereco[TelaProjecaoRF::TAMANHO_ENDERECO];
        Serial.print("Controle ");
        Serial.print(i);
        Serial.print(": ");
        
        if (carregarControle(i, endereco)) {
            telaRF.imprimirEndereco(endereco);
            Serial.println();
        } else {
            Serial.println("[Vazio]");
        }
    }
    Serial.println("------------------------");
}

void setup() {
    Serial.begin(9600);
    delay(1000);

    telaRF.begin(&Serial);
    telaRF.setInverterSinal(true); // Ajuste para false se o TX não responder

    // Inicia a memória com o "namespace" telas_rf no modo leitura/escrita (false)
    preferences.begin("telas_rf", false);

    imprimirMenu();
}

void processarSerial() {
    if (Serial.available() > 0) {
        String entrada = Serial.readStringUntil('\n');
        entrada.trim();
        entrada.toLowerCase();

        if (entrada == "listar") {
            listarControles();
        } 
        else if (entrada == "apagar") {
            preferences.clear();
            Serial.println("Memoria completamente apagada!");
            listarControles();
        }
        // Verifica comandos no formato Letra + Numero (ex: c1, l2, p3)
        else if (entrada.length() == 2) {
            char acao = entrada[0];
            int controle = entrada[1] - '0';

            if (controle < 1 || controle > 3) {
                Serial.println("Erro: Use apenas controles 1, 2 ou 3.");
                return;
            }

            if (acao == 'l') { // Aprender (Learn)
                controleParaAprender = controle;
                Serial.print("Aguardando sinal para salvar no Controle ");
                Serial.println(controle);
                telaRF.iniciarLeituraEndereco();
            } 
            else { // Ações de Transmissão (Cima, Parar, Baixo)
                uint8_t enderecoCarregado[TelaProjecaoRF::TAMANHO_ENDERECO];
                
                if (!carregarControle(controle, enderecoCarregado)) {
                    Serial.println("Erro: Controle nao esta configurado. Use l1, l2 ou l3 primeiro.");
                    return;
                }

                if (acao == 'c') {
                    Serial.print("Subindo tela "); Serial.println(controle);
                    telaRF.enviarCima(enderecoCarregado);
                } 
                else if (acao == 'p') {
                    Serial.print("Parando tela "); Serial.println(controle);
                    telaRF.enviarParar(enderecoCarregado);
                } 
                else if (acao == 'b') {
                    Serial.print("Descendo tela "); Serial.println(controle);
                    telaRF.enviarBaixo(enderecoCarregado);
                } 
                else {
                    Serial.println("Comando desconhecido.");
                }
            }
        }
    }
}

void loop() {
    // 1. Atualiza a biblioteca RF
    telaRF.update();

    // 2. Lê os comandos do Monitor Serial
    processarSerial();

    // 3. Verifica se capturou um endereço enquanto estava em modo "Aprender"
    if (controleParaAprender != -1 && telaRF.enderecoCapturadoDisponivel()) {
        uint8_t novoEndereco[TelaProjecaoRF::TAMANHO_ENDERECO];
        
        if (telaRF.obterEnderecoCapturado(novoEndereco)) {
            salvarControle(controleParaAprender, novoEndereco);
            controleParaAprender = -1; // Sai do modo de aprendizagem
            imprimirMenu();
        }
    }
}
