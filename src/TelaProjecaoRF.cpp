/**
 * @file TelaProjecaoRF.cpp
 * @brief Implementação da biblioteca TelaProjecaoRF.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 1.1.0
 * @license MIT
 */

#include "TelaProjecaoRF.h"

// Ponteiro estático para a instância ativa — necessário para a ISR
TelaProjecaoRF* TelaProjecaoRF::_instanciaAtiva = nullptr;

// =============================================================================
// Construtor e inicialização
// =============================================================================

TelaProjecaoRF::TelaProjecaoRF(uint8_t pinoTransmissor, uint8_t pinoReceptor)
{
    _pinoTransmissor = pinoTransmissor;
    _pinoReceptor    = pinoReceptor;
}

void TelaProjecaoRF::begin(Stream* debug)
{
    _debug = debug;

    pinMode(_pinoReceptor, INPUT);
    pinMode(_pinoTransmissor, OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(_pinoTransmissor), 0);

    memset(_ultimoEnderecoCapturado, 0, TAMANHO_ENDERECO);
    _temEnderecoCapturado = false;
    _ultimoTempoMicros    = micros();
    _instanciaAtiva       = this;

    attachInterrupt(
        digitalPinToInterrupt(_pinoReceptor),
        TelaProjecaoRF::tratarInterrupcaoEstatica,
        CHANGE
    );

    debugPrintln("[TelaProjecaoRF] Biblioteca iniciada.");
}

// =============================================================================
// Loop público
// =============================================================================

void TelaProjecaoRF::update()
{
    verificarFimPorSilencio();
    verificarQuadroRecebido();
}

// =============================================================================
// Configuração
// =============================================================================

void TelaProjecaoRF::setInverterSinal(bool inverter)
{
    _inverterSinalEnvio = inverter;
}

// =============================================================================
// Recepção — captura do endereço original
// =============================================================================

bool TelaProjecaoRF::iniciarLeituraEndereco()
{
    noInterrupts();
    _quantidadeCapturada  = 0;
    _quadroPronto         = false;
    _capturandoQuadro     = false;
    _capturaHabilitada    = true;
    _temEnderecoCapturado = false;
    _ultimoTempoMicros    = micros();
    interrupts();

    debugPrintln("[TelaProjecaoRF] Aguardando sinal RF para captura de endereco...");
    return true;
}

bool TelaProjecaoRF::enderecoCapturadoDisponivel() const
{
    return _temEnderecoCapturado;
}

bool TelaProjecaoRF::obterEnderecoCapturado(uint8_t enderecoDestino[TAMANHO_ENDERECO], bool limparDepois)
{
    if (!_temEnderecoCapturado) return false;

    memcpy(enderecoDestino, _ultimoEnderecoCapturado, TAMANHO_ENDERECO);

    if (limparDepois) _temEnderecoCapturado = false;
    return true;
}

// =============================================================================
// Transmissão — envio de comandos
// =============================================================================

bool TelaProjecaoRF::enviarLearn(const uint8_t endereco[TAMANHO_ENDERECO])
{
    return enviarComandoEndereco(endereco, COMANDO_LEARN);
}

bool TelaProjecaoRF::enviarCima(const uint8_t endereco[TAMANHO_ENDERECO])
{
    return enviarComandoEndereco(endereco, COMANDO_CIMA);
}

bool TelaProjecaoRF::enviarParar(const uint8_t endereco[TAMANHO_ENDERECO])
{
    return enviarComandoEndereco(endereco, COMANDO_PARAR);
}

bool TelaProjecaoRF::enviarBaixo(const uint8_t endereco[TAMANHO_ENDERECO])
{
    return enviarComandoEndereco(endereco, COMANDO_BAIXO);
}

bool TelaProjecaoRF::enviarComandoEndereco(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando)
{
    if (!enderecoValido(endereco)) {
        debugPrintln("[TelaProjecaoRF] Erro: endereco invalido (todos zeros).");
        return false;
    }
    if (!comandoConhecido(comando)) {
        debugPrintln("[TelaProjecaoRF] Erro: comando desconhecido.");
        return false;
    }

    QuadroRF quadro;
    if (!montarQuadro(endereco, comando, quadro)) return false;

    debugPrint("[TelaProjecaoRF] Enviando: ");
    debugPrintln(nomeComando(comando));

    transmitirQuadroBytes(quadro.bytes);
    return true;
}

// =============================================================================
// Montagem de quadro
// =============================================================================

bool TelaProjecaoRF::montarQuadro(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando, QuadroRF& quadro) const
{
    quadro.valido    = 1;
    quadro.bytes[0]  = BYTE_CABECALHO;
    for (uint8_t i = 0; i < TAMANHO_ENDERECO; i++) {
        quadro.bytes[i + 1] = endereco[i];
    }
    quadro.bytes[6] = comando;
    quadro.bytes[7] = calcularChecksum(endereco, comando);
    return true;
}

// =============================================================================
// ISR — interrupção de hardware
// =============================================================================

void IRAM_ATTR TelaProjecaoRF::tratarInterrupcaoEstatica()
{
    if (_instanciaAtiva != nullptr) _instanciaAtiva->tratarInterrupcao();
}

void IRAM_ATTR TelaProjecaoRF::tratarInterrupcao()
{
    if (!_capturaHabilitada) return;

    uint32_t tempoAtual = micros();
    uint32_t duracao    = tempoAtual - _ultimoTempoMicros;
    _ultimoTempoMicros  = tempoAtual;

    if (_quadroPronto) return;

    bool ehSync = (duracao >= SYNC_MINIMO && duracao <= SYNC_MAXIMO);

    if (ehSync) {
        // Um sync no meio de uma captura em andamento sinaliza fim do quadro
        if (_capturandoQuadro && _quantidadeCapturada > 20) {
            _quadroPronto     = true;
            _capturandoQuadro = false;
            return;
        }
        // Início de novo quadro
        _capturandoQuadro    = true;
        _quantidadeCapturada = 0;
        _duracoesCaptura[_quantidadeCapturada++] = duracao;
        return;
    }

    if (!_capturandoQuadro) return;

    // Descarta pulsos fora dos limites válidos (ruído)
    if (duracao < PULSO_MINIMO_VALIDO || duracao > PULSO_MAXIMO_VALIDO) {
        _capturandoQuadro    = false;
        _quantidadeCapturada = 0;
        return;
    }

    if (_quantidadeCapturada < MAX_PULSOS) {
        _duracoesCaptura[_quantidadeCapturada++] = duracao;
    } else {
        // Buffer cheio — considera quadro completo
        _quadroPronto     = true;
        _capturandoQuadro = false;
    }
}

// =============================================================================
// Processamento de quadros (chamado por update())
// =============================================================================

void TelaProjecaoRF::verificarFimPorSilencio()
{
    noInterrupts();
    bool deveFinalizar = _capturaHabilitada &&
                         _capturandoQuadro  &&
                         !_quadroPronto     &&
                         _quantidadeCapturada > 20 &&
                         (micros() - _ultimoTempoMicros > TEMPO_SILENCIO_FIM_QUADRO);
    if (deveFinalizar) {
        _quadroPronto     = true;
        _capturandoQuadro = false;
    }
    interrupts();
}

void TelaProjecaoRF::verificarQuadroRecebido()
{
    if (!_quadroPronto) return;

    // Copia o buffer volátil para área local antes de processar
    QuadroCapturado quadroLocal;
    noInterrupts();
    uint16_t qtd = (_quantidadeCapturada > MAX_PULSOS) ? MAX_PULSOS : _quantidadeCapturada;
    quadroLocal.quantidade = qtd;
    for (uint16_t i = 0; i < qtd; i++) quadroLocal.duracoes[i] = _duracoesCaptura[i];

    _quantidadeCapturada = 0;
    _quadroPronto        = false;
    _capturandoQuadro    = false;
    _capturaHabilitada   = false;
    interrupts();

    QuadroRF quadroRF;
    if (!decodificarQuadroCapturado(quadroLocal, quadroRF)) return;
    if (!validarQuadro(quadroRF)) return;

    extrairEndereco(quadroRF, _ultimoEnderecoCapturado);
    _temEnderecoCapturado = true;

    debugPrint("[TelaProjecaoRF] Endereco capturado: ");
    imprimirEndereco(_ultimoEnderecoCapturado);
    debugPrintln("");
}

bool TelaProjecaoRF::decodificarQuadroCapturado(const QuadroCapturado& quadro, QuadroRF& quadroRF)
{
    quadroRF.valido = 0;
    memset(quadroRF.bytes, 0, TAMANHO_QUADRO);

    // Quadro mínimo: sync(1) + preâmbulo(1) + 64 bits × 2 pulsos = 130 pulsos
    if (quadro.quantidade < 130) return false;
    if (quadro.duracoes[0] < SYNC_MINIMO || quadro.duracoes[0] > SYNC_MAXIMO) return false;

    // Decodifica 8 bytes (64 bits) a partir do pulso de índice 2 (após sync e preâmbulo)
    uint16_t indicePulso = 2;
    for (uint8_t bitGlobal = 0; bitGlobal < 64; bitGlobal++) {
        uint32_t p1 = quadro.duracoes[indicePulso];
        uint32_t p2 = quadro.duracoes[indicePulso + 1];

        bool p1Curto = (p1 <= LIMITE_CURTO_LONGO_US);
        bool p2Longo = (p2 >  LIMITE_CURTO_LONGO_US);
        bool p1Longo = (p1 >  LIMITE_CURTO_LONGO_US);
        bool p2Curto = (p2 <= LIMITE_CURTO_LONGO_US);

        bool bit;
        if      (p1Curto && p2Longo) bit = false; // curto+longo = 0
        else if (p1Longo && p2Curto) bit = true;  // longo+curto = 1
        else return false;                         // padrão inválido

        uint8_t indiceByte = bitGlobal / 8;
        quadroRF.bytes[indiceByte] <<= 1;
        if (bit) quadroRF.bytes[indiceByte] |= 0x01;

        indicePulso += 2;
    }

    quadroRF.valido = 1;
    return true;
}

bool TelaProjecaoRF::validarQuadro(const QuadroRF& quadro) const
{
    if (quadro.valido != 1)                  return false;
    if (quadro.bytes[0] != BYTE_CABECALHO)   return false;
    if (!comandoConhecido(quadro.bytes[6]))  return false;
    if (calcularChecksumQuadro(quadro) != quadro.bytes[7]) return false;
    return true;
}

// =============================================================================
// Cálculo de checksum
// =============================================================================

uint8_t TelaProjecaoRF::calcularChecksum(const uint8_t endereco[TAMANHO_ENDERECO], uint8_t comando) const
{
    uint16_t soma = comando;
    for (uint8_t i = 0; i < TAMANHO_ENDERECO; i++) soma += endereco[i];
    return static_cast<uint8_t>(soma & 0xFF);
}

uint8_t TelaProjecaoRF::calcularChecksumQuadro(const QuadroRF& quadro) const
{
    uint16_t soma = 0;
    for (uint8_t i = 1; i <= 6; i++) soma += quadro.bytes[i];
    return static_cast<uint8_t>(soma & 0xFF);
}

void TelaProjecaoRF::extrairEndereco(const QuadroRF& quadro, uint8_t endereco[TAMANHO_ENDERECO]) const
{
    for (uint8_t i = 0; i < TAMANHO_ENDERECO; i++) endereco[i] = quadro.bytes[i + 1];
}

// =============================================================================
// Transmissão — geração do sinal RF
// =============================================================================

void TelaProjecaoRF::transmitirQuadroBytes(const uint8_t bytes[TAMANHO_QUADRO])
{
    // Desabilita recepção durante a transmissão para evitar auto-recepção
    _capturaHabilitada = false;
    detachInterrupt(digitalPinToInterrupt(_pinoReceptor));

    delay(20);
    gpio_set_level(static_cast<gpio_num_t>(_pinoTransmissor), 0);
    delayMicroseconds(10000);

    for (uint8_t repeticao = 0; repeticao < REPETICOES_ENVIO; repeticao++) {
        escreverTX(false, TEMPO_SYNC_US);       // pulso de sincronismo
        escreverTX(true,  TEMPO_PREAMBULO_US);  // preâmbulo

        for (uint8_t indiceByte = 0; indiceByte < TAMANHO_QUADRO; indiceByte++) {
            for (int8_t bit = 7; bit >= 0; bit--) {
                transmitirBit((bytes[indiceByte] >> bit) & 0x01);
            }
        }
        escreverTX(false, 8000); // espaço entre repetições
    }

    gpio_set_level(static_cast<gpio_num_t>(_pinoTransmissor), 0);
    delay(20);

    // Reativa a recepção
    _ultimoTempoMicros = micros();
    attachInterrupt(digitalPinToInterrupt(_pinoReceptor), TelaProjecaoRF::tratarInterrupcaoEstatica, CHANGE);
}

void TelaProjecaoRF::transmitirBit(bool bit)
{
    if (bit) {
        escreverTX(false, TEMPO_LONGO_US);  // longo + curto = 1
        escreverTX(true,  TEMPO_CURTO_US);
    } else {
        escreverTX(false, TEMPO_CURTO_US);  // curto + longo = 0
        escreverTX(true,  TEMPO_LONGO_US);
    }
}

void TelaProjecaoRF::escreverTX(bool nivel, uint32_t tempoUs)
{
    if (_inverterSinalEnvio) nivel = !nivel;
    gpio_set_level(static_cast<gpio_num_t>(_pinoTransmissor), nivel ? 1 : 0);
    delayMicroseconds(tempoUs);
}

// =============================================================================
// Helpers de validação
// =============================================================================

bool TelaProjecaoRF::enderecoValido(const uint8_t endereco[TAMANHO_ENDERECO]) const
{
    if (!endereco) return false;
    for (uint8_t i = 0; i < TAMANHO_ENDERECO; i++) {
        if (endereco[i] != 0) return true;
    }
    return false;
}

bool TelaProjecaoRF::comandoConhecido(uint8_t comando) const
{
    return comando == COMANDO_LEARN ||
           comando == COMANDO_CIMA  ||
           comando == COMANDO_PARAR ||
           comando == COMANDO_BAIXO;
}

const char* TelaProjecaoRF::nomeComando(uint8_t comando) const
{
    switch (comando) {
        case COMANDO_LEARN: return "LEARN";
        case COMANDO_CIMA:  return "CIMA";
        case COMANDO_PARAR: return "PARAR";
        case COMANDO_BAIXO: return "BAIXO";
        default:            return "DESCONHECIDO";
    }
}

// =============================================================================
// Utilitários
// =============================================================================

void TelaProjecaoRF::imprimirEndereco(const uint8_t endereco[TAMANHO_ENDERECO]) const
{
    if (!_debug) return;
    for (uint8_t i = 0; i < TAMANHO_ENDERECO; i++) {
        if (endereco[i] < 0x10) _debug->print("0");
        _debug->print(endereco[i], HEX);
        if (i + 1 < TAMANHO_ENDERECO) _debug->print(" ");
    }
}

void TelaProjecaoRF::debugPrint(const String& texto) const   { if (_debug) _debug->print(texto);   }
void TelaProjecaoRF::debugPrintln(const String& texto) const { if (_debug) _debug->println(texto); }
