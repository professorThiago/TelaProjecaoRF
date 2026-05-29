/**
 * @file TelaProjecaoRF.h
 * @brief Biblioteca para controle de telas de projeção motorizadas via RF 433 MHz.
 *
 * @details
 * Muitas telas de projeção motorizadas genéricas utilizam um protocolo
 * RF proprietário em 433 MHz com a seguinte estrutura de quadro:
 *
 * @code
 * Byte 0   Bytes 1–5     Byte 6    Byte 7
 * 0xA3     Endereço(5)   Comando   Checksum
 * ^^^^^^   ^^^^^^^^^^^   ^^^^^^^   ^^^^^^^^
 * Cabeçalho fixo         CIMA/     soma(bytes 1-6) & 0xFF
 *                        PARAR/
 *                        BAIXO/
 *                        LEARN
 * @endcode
 *
 * A biblioteca oferece dois modos de operação:
 *  - **Recepção (sniffer):** captura o endereço único do controle original.
 *  - **Transmissão:** envia comandos para a tela usando o endereço capturado.
 *
 * @par Ligação de hardware
 * @code
 *   ESP32 GPIO TX  →  DATA do módulo transmissor RF 433 MHz
 *   ESP32 GPIO RX  →  DATA do módulo receptor RF 433 MHz
 *   3.3 V / 5 V    →  VCC dos módulos (verifique a tensão do seu módulo)
 *   GND            →  GND dos módulos
 * @endcode
 *
 * @note  `update()` **deve** ser chamado a cada iteração do `loop()`.
 *        Sem isso a detecção de fim de quadro por silêncio não funciona.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 1.1.0
 * @date    2026
 * @license MIT
 *
 * @par Licença MIT
 * Copyright (c) 2026 professorThiago\n
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:\n
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef TELA_PROJECAO_RF_H
#define TELA_PROJECAO_RF_H

#include <Arduino.h>
#include "driver/gpio.h"

/**
 * @brief Controla telas de projeção motorizadas via RF 433 MHz.
 *
 * @par Uso básico
 * @code
 * #include <TelaProjecaoRF.h>
 *
 * TelaProjecaoRF tela(7, 6);   // TX no GPIO 7, RX no GPIO 6
 *
 * const uint8_t MINHA_TELA[5] = {0x77, 0x18, 0x16, 0x01, 0x00};
 *
 * void setup() {
 *     Serial.begin(115200);
 *     tela.begin(&Serial);
 * }
 *
 * void loop() {
 *     tela.update();   // sempre no loop()
 *     tela.enviarCima(MINHA_TELA);
 *     delay(3000);
 *     tela.enviarParar(MINHA_TELA);
 *     delay(3000);
 * }
 * @endcode
 */
class TelaProjecaoRF
{
public:
    // -----------------------------------------------------------------------
    // Constantes públicas
    // -----------------------------------------------------------------------

    /** @brief Número de bytes do endereço do controle remoto. */
    static constexpr uint8_t TAMANHO_ENDERECO = 5;

    /** @brief Número total de bytes em um quadro RF. */
    static constexpr uint8_t TAMANHO_QUADRO = 8;

    /**
     * @defgroup Comandos Códigos de comando RF
     * Byte de comando transmitido no Byte 6 do quadro.
     * @{
     */
    static constexpr uint8_t COMANDO_LEARN = 0x53; ///< Pareamento / programação da tela
    static constexpr uint8_t COMANDO_CIMA  = 0x0B; ///< Sobe a tela
    static constexpr uint8_t COMANDO_PARAR = 0x23; ///< Para o movimento
    static constexpr uint8_t COMANDO_BAIXO = 0x43; ///< Desce a tela
    /** @} */

    // -----------------------------------------------------------------------
    // Tipos públicos
    // -----------------------------------------------------------------------

    /**
     * @brief Representa um quadro RF decodificado ou montado para envio.
     */
    struct QuadroRF
    {
        uint8_t valido;                 ///< 1 se o quadro é válido, 0 caso contrário
        uint8_t bytes[TAMANHO_QUADRO];  ///< Bytes brutos do quadro (cabeçalho + endereço + comando + checksum)
    };

    // -----------------------------------------------------------------------
    // Construtor e inicialização
    // -----------------------------------------------------------------------

    /**
     * @brief Constrói um objeto TelaProjecaoRF.
     * @param pinoTransmissor  GPIO conectado ao DATA do módulo TX.
     * @param pinoReceptor     GPIO conectado ao DATA do módulo RX.
     */
    TelaProjecaoRF(uint8_t pinoTransmissor, uint8_t pinoReceptor);

    /**
     * @brief Inicializa a biblioteca, configura os pinos e ativa a interrupção.
     * @param debug  Ponteiro para um `Stream` (ex: `&Serial`) para imprimir logs.
     *               Passe `nullptr` para desabilitar (padrão).
     *
     * @par Exemplo
     * @code
     * tela.begin(&Serial);   // com log
     * tela.begin();          // sem log
     * @endcode
     */
    void begin(Stream* debug = nullptr);

    /**
     * @brief Processa eventos pendentes. **Deve ser chamado a cada `loop()`.**
     *
     * Responsável por:
     * - Detectar fim de quadro por silêncio (timeout de 18 ms sem pulsos).
     * - Decodificar e validar quadros capturados pela ISR.
     */
    void update();

    /**
     * @brief Inverte o nível lógico do sinal de transmissão.
     * @param inverter  `true` para inverter (padrão de fábrica), `false` para nível normal.
     *
     * @note Alguns módulos transmissores operam com lógica invertida.
     *       Se a tela não responder, tente alternar este valor.
     */
    void setInverterSinal(bool inverter);

    // -----------------------------------------------------------------------
    // Recepção — captura do endereço do controle original
    // -----------------------------------------------------------------------

    /**
     * @brief Habilita o modo de captura de endereço.
     *
     * Aponte o controle original para o receptor RF e pressione qualquer botão.
     * Quando o endereço for capturado, `enderecoCapturadoDisponivel()` retornará `true`.
     *
     * @return `true` sempre (reservado para expansão futura).
     *
     * @par Exemplo
     * @code
     * tela.iniciarLeituraEndereco();
     *
     * while (!tela.enderecoCapturadoDisponivel()) {
     *     tela.update();
     * }
     * @endcode
     */
    bool iniciarLeituraEndereco();

    /**
     * @brief Verifica se um endereço foi capturado e está disponível para leitura.
     * @return `true` se há um endereço válido pronto para ser lido.
     */
    bool enderecoCapturadoDisponivel() const;

    /**
     * @brief Copia o endereço capturado para um buffer do usuário.
     * @param enderecoDestino  Array de `TAMANHO_ENDERECO` bytes que receberá o endereço.
     * @param limparDepois     Se `true` (padrão), marca o endereço como consumido.
     * @return `true` se havia um endereço disponível e foi copiado com sucesso.
     *
     * @par Exemplo
     * @code
     * uint8_t addr[TelaProjecaoRF::TAMANHO_ENDERECO];
     * if (tela.obterEnderecoCapturado(addr)) {
     *     tela.imprimirEndereco(addr);
     * }
     * @endcode
     */
    bool obterEnderecoCapturado(uint8_t enderecoDestino[TAMANHO_ENDERECO], bool limparDepois = true);

    // -----------------------------------------------------------------------
    // Transmissão — envio de comandos para a tela
    // -----------------------------------------------------------------------

    /**
     * @brief Envia o comando LEARN (pareamento) para a tela.
     * @param endereco  Array de `TAMANHO_ENDERECO` bytes com o endereço alvo.
     * @return `true` se o quadro foi transmitido; `false` se o endereço for inválido.
     */
    bool enviarLearn(const uint8_t endereco[TAMANHO_ENDERECO]);

    /**
     * @brief Envia o comando CIMA (sobe a tela).
     * @param endereco  Array de `TAMANHO_ENDERECO` bytes com o endereço alvo.
     * @return `true` se o quadro foi transmitido; `false` se o endereço for inválido.
     */
    bool enviarCima(const uint8_t endereco[TAMANHO_ENDERECO]);

    /**
     * @brief Envia o comando PARAR (para o movimento da tela).
     * @param endereco  Array de `TAMANHO_ENDERECO` bytes com o endereço alvo.
     * @return `true` se o quadro foi transmitido; `false` se o endereço for inválido.
     */
    bool enviarParar(const uint8_t endereco[TAMANHO_ENDERECO]);

    /**
     * @brief Envia o comando BAIXO (desce a tela).
     * @param endereco  Array de `TAMANHO_ENDERECO` bytes com o endereço alvo.
     * @return `true` se o quadro foi transmitido; `false` se o endereço for inválido.
     */
    bool enviarBaixo(const uint8_t endereco[TAMANHO_ENDERECO]);

    // -----------------------------------------------------------------------
    // Utilitários
    // -----------------------------------------------------------------------

    /**
     * @brief Imprime o endereço no formato hexadecimal via `debug` ou `Serial`.
     *
     * Saída exemplo: `77 18 16 01 00`
     *
     * @param endereco  Array de `TAMANHO_ENDERECO` bytes a imprimir.
     */
    void imprimirEndereco(const uint8_t endereco[TAMANHO_ENDERECO]) const;

private:
    // -----------------------------------------------------------------------
    // Constantes do protocolo RF (obtidas por engenharia reversa)
    // -----------------------------------------------------------------------

    static constexpr uint8_t  BYTE_CABECALHO          = 0xA3;  ///< Primeiro byte de todo quadro válido
    static constexpr uint16_t TEMPO_SYNC_US            = 5140;  ///< Duração do pulso de sincronismo (µs)
    static constexpr uint16_t TEMPO_PREAMBULO_US       = 610;   ///< Duração do preâmbulo após sync (µs)
    static constexpr uint16_t TEMPO_CURTO_US           = 286;   ///< Duração de pulso curto — bit 0 (µs)
    static constexpr uint16_t TEMPO_LONGO_US           = 615;   ///< Duração de pulso longo — bit 1 (µs)
    static constexpr uint16_t LIMITE_CURTO_LONGO_US    = 450;   ///< Threshold para classificar curto/longo (µs)
    static constexpr uint16_t MAX_PULSOS               = 220;   ///< Tamanho máximo do buffer de captura

    static constexpr uint32_t PULSO_MINIMO_VALIDO      = 80;    ///< Pulso menor que isso é ruído (µs)
    static constexpr uint32_t PULSO_MAXIMO_VALIDO      = 2500;  ///< Pulso maior que isso é inválido (µs)
    static constexpr uint32_t SYNC_MINIMO              = 3500;  ///< Mínimo para ser considerado sync (µs)
    static constexpr uint32_t SYNC_MAXIMO              = 9000;  ///< Máximo para ser considerado sync (µs)
    static constexpr uint32_t TEMPO_SILENCIO_FIM_QUADRO= 18000; ///< Silêncio que indica fim do quadro (µs)
    static constexpr uint8_t  REPETICOES_ENVIO         = 50;    ///< Número de repetições por transmissão

    // -----------------------------------------------------------------------
    // Tipos internos
    // -----------------------------------------------------------------------

    /** @brief Buffer temporário preenchido pela ISR durante a captura. */
    struct QuadroCapturado
    {
        uint16_t quantidade;          ///< Número de pulsos armazenados
        uint32_t duracoes[MAX_PULSOS]; ///< Duração de cada pulso em µs
    };

    // -----------------------------------------------------------------------
    // Membros privados
    // -----------------------------------------------------------------------

    uint8_t  _pinoTransmissor;
    uint8_t  _pinoReceptor;
    bool     _inverterSinalEnvio = true;
    Stream*  _debug = nullptr;

    uint8_t          _ultimoEnderecoCapturado[TAMANHO_ENDERECO];
    volatile bool    _temEnderecoCapturado = false;

    // Variáveis acessadas pela ISR — devem ser volatile
    volatile uint32_t _duracoesCaptura[MAX_PULSOS];
    volatile uint16_t _quantidadeCapturada = 0;
    volatile uint32_t _ultimoTempoMicros   = 0;
    volatile bool     _capturaHabilitada   = false;
    volatile bool     _capturandoQuadro    = false;
    volatile bool     _quadroPronto        = false;

    /** @brief Ponteiro estático para a instância ativa, usado pela ISR. */
    static TelaProjecaoRF* _instanciaAtiva;

    // -----------------------------------------------------------------------
    // ISR e detecção de quadro
    // -----------------------------------------------------------------------

    /** @brief Entry point estático da ISR (requerido pelo attachInterrupt). */
    static void IRAM_ATTR tratarInterrupcaoEstatica();

    /** @brief Lógica real da ISR — mede duração dos pulsos e detecta sync. */
    void IRAM_ATTR tratarInterrupcao();

    /** @brief Chamado por update(): encerra captura se não chegaram pulsos por TEMPO_SILENCIO_FIM_QUADRO. */
    void verificarFimPorSilencio();

    /** @brief Chamado por update(): decodifica o quadro quando _quadroPronto == true. */
    void verificarQuadroRecebido();

    // -----------------------------------------------------------------------
    // Processamento de quadros
    // -----------------------------------------------------------------------

    /**
     * @brief Converte os pulsos brutos capturados em bytes do protocolo RF.
     * @param quadro    Buffer de pulsos preenchido pela ISR.
     * @param quadroRF  Saída: estrutura com os bytes decodificados.
     * @return `true` se a decodificação foi bem-sucedida.
     */
    bool decodificarQuadroCapturado(const QuadroCapturado& quadro, QuadroRF& quadroRF);

    /**
     * @brief Valida cabeçalho, comando e checksum de um quadro decodificado.
     * @return `true` se o quadro é válido.
     */
    bool validarQuadro(const QuadroRF& quadro) const;

    /** @brief Calcula o checksum: soma(endereço + comando) & 0xFF. */
    uint8_t calcularChecksum(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando) const;

    /** @brief Calcula o checksum a partir de um QuadroRF já montado. */
    uint8_t calcularChecksumQuadro(const QuadroRF& quadro) const;

    /** @brief Extrai os 5 bytes de endereço dos bytes 1–5 de um QuadroRF. */
    void extrairEndereco(const QuadroRF& quadro, uint8_t endereco[TAMANHO_ENDERECO]) const;

    // -----------------------------------------------------------------------
    // Transmissão
    // -----------------------------------------------------------------------

    /**
     * @brief Monta e envia um quadro RF para o endereço e comando fornecidos.
     * @param endereco  Endereço de 5 bytes.
     * @param comando   Um dos COMANDO_* definidos nesta classe.
     * @return `true` se o envio foi realizado.
     */
    bool enviarComandoEndereco(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando);

    /**
     * @brief Monta o QuadroRF (8 bytes) a partir do endereço e comando.
     * @return `true` sempre (reservado para validação futura).
     */
    bool montarQuadro(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando, QuadroRF& quadro) const;

    /** @brief Transmite os 8 bytes do quadro com as repetições configuradas. */
    void transmitirQuadroBytes(const uint8_t bytes[TAMANHO_QUADRO]);

    /** @brief Codifica e transmite um único bit (par curto/longo ou longo/curto). */
    void transmitirBit(bool bit);

    /**
     * @brief Coloca o pino TX no nível `nivel` por `tempoUs` microssegundos.
     *        Aplica inversão se `_inverterSinalEnvio == true`.
     */
    void escreverTX(bool nivel, uint32_t tempoUs);

    // -----------------------------------------------------------------------
    // Helpers de validação
    // -----------------------------------------------------------------------

    /** @brief Retorna `true` se o endereço contém ao menos um byte não-zero. */
    bool enderecoValido(const uint8_t endereco[TAMANHO_ENDERECO]) const;

    /** @brief Retorna `true` se o comando é um dos quatro COMANDO_* conhecidos. */
    bool comandoConhecido(uint8_t comando) const;

    /** @brief Retorna o nome em texto do comando (para log). */
    const char* nomeComando(uint8_t comando) const;

    // -----------------------------------------------------------------------
    // Debug
    // -----------------------------------------------------------------------
    void debugPrint(const String& texto) const;
    void debugPrintln(const String& texto) const;
};

#endif // TELA_PROJECAO_RF_H
