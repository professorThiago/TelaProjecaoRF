# TelaProjecaoRF

Biblioteca para Arduino/ESP32 desenvolvida para controlar e clonar sinais de controle remoto de telas de projeção motorizadas via rádio frequência (RF 433 MHz).

Muitas telas de projeção genéricas utilizam um protocolo RF de 433 MHz específico (cabeçalho `0xA3`, 5 bytes de endereço, 1 byte de comando e 1 byte de checksum). Esta biblioteca permite tanto a **leitura (sniffing)** do endereço original do seu controle quanto a **transmissão** dos comandos para a tela.

## Recursos
- 📡 **Leitura de Endereço:** Capture o endereço único do seu controle remoto original.
- 🎬 **Controle Total:** Envie os comandos CIMA, PARAR, BAIXO e LEARN.
- ⚡ **Otimizado para ESP32:** Utiliza interrupções de hardware e temporização precisa.

## Instalação (PlatformIO)
Adicione o repositório ao seu arquivo `platformio.ini`:

```ini
lib_deps =
  [https://github.com/SEU_USUARIO/TelaProjecaoRF.git](https://github.com/SEU_USUARIO/TelaProjecaoRF.git)
  
  Hardware Necessário
Microcontrolador da família ESP32.

Módulo Transmissor RF 433 MHz (conectado ao pino TX).

Módulo Receptor RF 433 MHz (conectado ao pino RX).

Comandos Suportados
CIMA: Sobe a tela.

PARAR: Interrompe o movimento.

BAIXO: Desce a tela.

LEARN: Comando de pareamento (consulta o manual da sua tela).

Exemplos
Verifique a pasta examples/ para ver códigos completos de como ler o endereço do controle e como enviar comandos para a tela.

Licença
Este projeto está licenciado sob a Licença MIT - veja o arquivo LICENSE para detalhes.
# TelaProjecaoRF
