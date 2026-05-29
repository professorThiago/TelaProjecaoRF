# TelaProjecaoRF

Biblioteca Arduino/ESP32 para **controle e clonagem** de sinais RF 433 MHz de telas de projeção motorizadas genéricas.

> Desenvolvida e testada com um ESP32-S3 e uma tela motorizada de 433 MHz.  
> Autor: [professorThiago](https://github.com/professorThiago)

---

## Sobre o protocolo

Muitas telas de projeção motorizadas genéricas utilizam RF 433 MHz com um protocolo proprietário de 8 bytes:

```
Byte 0   Bytes 1–5       Byte 6    Byte 7
0xA3     Endereço (5 b)  Comando   Checksum
^^^^^^   ^^^^^^^^^^^^^   ^^^^^^^   ^^^^^^^^
fixo     único por tela  CIMA/     soma(bytes 1–6) & 0xFF
                         PARAR/
                         BAIXO/
                         LEARN
```

Cada tela responde **apenas ao seu endereço de 5 bytes**, programado de fábrica no controle remoto. Esta biblioteca permite capturar esse endereço e usá-lo para enviar comandos diretamente do ESP32.

---

## Recursos

- 📡 **Sniffer RF:** captura o endereço único do seu controle original pressionando qualquer botão.
- 🎬 **Controle completo:** envia CIMA, PARAR, BAIXO e LEARN.
- 💾 **Persistência opcional:** exemplo com `Preferences` (NVS) para salvar até 3 controles na flash.
- ⚡ **Otimizado para ESP32:** usa interrupção de hardware (`IRAM_ATTR`) e `gpio_set_level` para temporização precisa.
- 🔇 **Auto-mute durante TX:** desativa a recepção enquanto transmite para evitar auto-recepção.

---

## Instalação

### PlatformIO (recomendado)

```ini
lib_deps =
    https://github.com/professorThiago/TelaProjecaoRF
```

### Arduino IDE

1. Baixe este repositório como ZIP.
2. **Sketch → Incluir Biblioteca → Adicionar biblioteca .ZIP…**

---

## Hardware necessário

| Componente | Conexão |
|---|---|
| ESP32 (qualquer variante) | — |
| Módulo receptor RF 433 MHz (ex: XY-MK-5V, RXB6) | DATA → GPIO RX |
| Módulo transmissor RF 433 MHz (ex: FS1000A) | DATA → GPIO TX |

```
ESP32 GPIO TX  →  DATA do módulo transmissor RF 433 MHz
ESP32 GPIO RX  →  DATA do módulo receptor RF 433 MHz
3.3 V / 5 V    →  VCC dos módulos (verifique a tensão do seu modelo)
GND            →  GND dos módulos
```

> **Dica:** módulos de 5 V funcionam melhor para transmissão. O receptor pode operar com 3,3 V.

---

## Quick start

```cpp
#include <TelaProjecaoRF.h>

// Endereço capturado com o exemplo 01_LerEndereco
const uint8_t MINHA_TELA[5] = {0x77, 0x18, 0x16, 0x01, 0x00};

TelaProjecaoRF tela(7, 6);  // TX=GPIO7, RX=GPIO6

void setup() {
    Serial.begin(115200);
    tela.begin(&Serial);
}

void loop() {
    tela.update();              // sempre no loop()
    tela.enviarCima(MINHA_TELA);
    delay(5000);
    tela.enviarParar(MINHA_TELA);
    delay(5000);
}
```

---

## API

### Construtor

```cpp
TelaProjecaoRF tela(uint8_t pinoTX, uint8_t pinoRX);
```

### Inicialização

#### `void begin(Stream* debug = nullptr)`
Inicializa pinos e interrupção. Passe `&Serial` para habilitar logs.

#### `void update()`
**Deve ser chamado em todo `loop()`**. Detecta fim de quadro por silêncio e decodifica os dados capturados.

#### `void setInverterSinal(bool inverter)`
Inverte o nível lógico do TX. Padrão: `true`. Se a tela não responder, tente `false`.

---

### Recepção — capturar o endereço

#### `bool iniciarLeituraEndereco()`
Ativa o modo sniffer. Pressione qualquer botão no controle original para capturar.

#### `bool enderecoCapturadoDisponivel()`
Retorna `true` quando um endereço válido foi capturado e está pronto para leitura.

#### `bool obterEnderecoCapturado(uint8_t dest[5], bool limpar = true)`
Copia o endereço capturado para `dest`. Se `limpar = true`, marca como consumido.

```cpp
tela.iniciarLeituraEndereco();

while (!tela.enderecoCapturadoDisponivel()) {
    tela.update();
}

uint8_t addr[TelaProjecaoRF::TAMANHO_ENDERECO];
tela.obterEnderecoCapturado(addr);
tela.imprimirEndereco(addr);   // ex: "77 18 16 01 00"
```

---

### Transmissão — enviar comandos

Todos os métodos retornam `false` se o endereço for inválido (todos zeros).

| Método | Ação |
|--------|------|
| `enviarCima(addr)` | Sobe a tela |
| `enviarParar(addr)` | Para o movimento |
| `enviarBaixo(addr)` | Desce a tela |
| `enviarLearn(addr)` | Envia o comando de pareamento |

---

### Utilitário

#### `void imprimirEndereco(const uint8_t addr[5])`
Imprime o endereço em hexadecimal via `debug` (ex: `77 18 16 01 00`).

---

## Comandos suportados

| Constante | Byte | Ação |
|-----------|------|------|
| `COMANDO_CIMA` | `0x0B` | Sobe a tela |
| `COMANDO_PARAR` | `0x23` | Para o movimento |
| `COMANDO_BAIXO` | `0x43` | Desce a tela |
| `COMANDO_LEARN` | `0x53` | Pareamento / programação |

---

## Exemplos

| Exemplo | Descrição |
|---------|-----------|
| `01_LerEndereco` | Captura e imprime o endereço do controle original |
| `02_ControlarTela` | Controla a tela via comandos no Monitor Serial |
| `03_SalvarControles` | Gerencia até 3 controles salvos na flash (NVS) |

---

## Solução de problemas

| Sintoma | Possível causa | Solução |
|---------|---------------|---------|
| Tela não responde ao envio | Lógica do TX invertida | Alterne `setInverterSinal(false)` |
| Endereço não é capturado | Módulo RX fraco ou distância | Aproxime o controle; use RXB6 ou similar |
| Captura endereço errado | Ruído RF no ambiente | Verifique a alimentação dos módulos; use cabo curto no DATA |
| `update()` não chamado | Loop bloqueado | Garanta que `update()` seja chamado em todo ciclo do `loop()` |

---

## Licença

MIT © 2026 [professorThiago](https://github.com/professorThiago)
