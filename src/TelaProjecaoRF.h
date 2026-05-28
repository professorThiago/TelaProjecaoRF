#ifndef TELA_PROJECAO_RF_H
#define TELA_PROJECAO_RF_H

#include <Arduino.h>
#include "driver/gpio.h"

class TelaProjecaoRF
{
public:
    static constexpr uint8_t TAMANHO_ENDERECO = 5;
    static constexpr uint8_t TAMANHO_QUADRO = 8;

    static constexpr uint8_t COMANDO_LEARN = 0x53;
    static constexpr uint8_t COMANDO_CIMA = 0x0B;
    static constexpr uint8_t COMANDO_PARAR = 0x23;
    static constexpr uint8_t COMANDO_BAIXO = 0x43;

    struct QuadroRF
    {
        uint8_t valido;
        uint8_t bytes[TAMANHO_QUADRO];
    };

    TelaProjecaoRF(uint8_t pinoTransmissor, uint8_t pinoReceptor);

    void begin(Stream* debug = nullptr);
    void update(); // Deve ser chamado no loop()
    void setInverterSinal(bool inverter);

    // --- Métodos de Recepção (Sniffer) ---
    bool iniciarLeituraEndereco();
    bool enderecoCapturadoDisponivel() const;
    bool obterEnderecoCapturado(uint8_t enderecoDestino[TAMANHO_ENDERECO], bool limparDepois = true);

    // --- Métodos de Transmissão ---
    bool enviarLearn(const uint8_t endereco[TAMANHO_ENDERECO]);
    bool enviarCima(const uint8_t endereco[TAMANHO_ENDERECO]);
    bool enviarParar(const uint8_t endereco[TAMANHO_ENDERECO]);
    bool enviarBaixo(const uint8_t endereco[TAMANHO_ENDERECO]);
    
    // Utilitário
    void imprimirEndereco(const uint8_t endereco[TAMANHO_ENDERECO]) const;

private:
    static constexpr uint8_t BYTE_CABECALHO = 0xA3;
    static constexpr uint16_t TEMPO_SYNC_US = 5140;
    static constexpr uint16_t TEMPO_PREAMBULO_US = 610;
    static constexpr uint16_t TEMPO_CURTO_US = 286;
    static constexpr uint16_t TEMPO_LONGO_US = 615;
    static constexpr uint16_t LIMITE_CURTO_LONGO_US = 450;
    static constexpr uint16_t MAX_PULSOS = 220;

    static constexpr uint32_t PULSO_MINIMO_VALIDO = 80;
    static constexpr uint32_t PULSO_MAXIMO_VALIDO = 2500;
    static constexpr uint32_t SYNC_MINIMO = 3500;
    static constexpr uint32_t SYNC_MAXIMO = 9000;
    static constexpr uint32_t TEMPO_SILENCIO_FIM_QUADRO = 18000;
    static constexpr uint8_t REPETICOES_ENVIO = 50;

    struct QuadroCapturado
    {
        uint16_t quantidade;
        uint32_t duracoes[MAX_PULSOS];
    };

    uint8_t _pinoTransmissor;
    uint8_t _pinoReceptor;
    bool _inverterSinalEnvio = true;
    Stream* _debug = nullptr;

    uint8_t _ultimoEnderecoCapturado[TAMANHO_ENDERECO];
    volatile bool _temEnderecoCapturado = false;

    volatile uint32_t _duracoesCaptura[MAX_PULSOS];
    volatile uint16_t _quantidadeCapturada = 0;
    volatile uint32_t _ultimoTempoMicros = 0;
    volatile bool _capturaHabilitada = false;
    volatile bool _capturandoQuadro = false;
    volatile bool _quadroPronto = false;

    static TelaProjecaoRF* _instanciaAtiva;

    static void IRAM_ATTR tratarInterrupcaoEstatica();
    void IRAM_ATTR tratarInterrupcao();

    void verificarFimPorSilencio();
    void verificarQuadroRecebido();

    bool decodificarQuadroCapturado(const QuadroCapturado& quadro, QuadroRF& quadroRF);
    bool validarQuadro(const QuadroRF& quadro) const;
    uint8_t calcularChecksum(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando) const;
    uint8_t calcularChecksumQuadro(const QuadroRF& quadro) const;
    void extrairEndereco(const QuadroRF& quadro, uint8_t endereco[TAMANHO_ENDERECO]) const;

    bool enviarComandoEndereco(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando);
    bool montarQuadro(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando, QuadroRF& quadro) const;
    void transmitirQuadroBytes(const uint8_t bytes[TAMANHO_QUADRO]);
    void transmitirBit(bool bit);
    void escreverTX(bool nivel, uint32_t tempoUs);

    bool enderecoValido(const uint8_t endereco[TAMANHO_ENDERECO]) const;
    bool comandoConhecido(uint8_t comando) const;
    const char* nomeComando(uint8_t comando) const;

    void debugPrint(const String& texto) const;
    void debugPrintln(const String& texto) const;
};

#endif
