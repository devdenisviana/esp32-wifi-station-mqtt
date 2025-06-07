# ESP32 Wi-Fi + MQTT com FreeRTOS

Este projeto demonstra como conectar um ESP32 a uma rede Wi-Fi e a um broker MQTT usando FreeRTOS. O dispositivo se conecta ao Wi-Fi, estabelece conexão com o broker público **HiveMQ**, e publica mensagens periodicamente em um tópico MQTT.

## 📦 Funcionalidades

- Conexão com rede Wi-Fi (modo station)
- Conexão com broker MQTT (`mqtt://broker.hivemq.com:1883`)
- Publicação periódica de mensagens no tópico `asgard/`
- Implementação baseada em FreeRTOS (nativo no ESP-IDF)
- Log via UART para depuração

## 🧰 Requisitos

- Placa ESP32 DevKit
- Ambiente de desenvolvimento [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) instalado
- Acesso à internet
- Broker MQTT (neste exemplo, o público HiveMQ)

## ⚙️ Configuração de Wi-Fi

No código, a rede Wi-Fi é definida pelas macros:

```c
#define EXAMPLE_ESP_WIFI_SSID      "redeteste"
#define EXAMPLE_ESP_WIFI_PASS      "teste@#2571"
Altere esses valores para o nome e senha da sua rede local.

🚀 Funcionamento
O ESP32 inicializa o NVS (necessário para o Wi-Fi)

Conecta-se ao Wi-Fi como station

Após obter IP, conecta-se ao broker MQTT

Publica mensagens no tópico asgard/ a cada 10 segundos

🗨️ Mensagens publicadas
O conteúdo das mensagens é:

Ola do ESP32! Contagem: X
Onde X é um contador incremental enviado a cada 10 segundos.

📂 Estrutura do Projeto
app_main() – Função principal, inicializa NVS, Wi-Fi e MQTT

wifi_init_sta() – Inicializa e gerencia a conexão Wi-Fi

mqtt_app_start() – Inicia o cliente MQTT

mqtt_event_handler() – Trata eventos do MQTT como conexão, publicação, erro, etc.

📌 Observações
O broker HiveMQ usado é público e sem autenticação. Para aplicações reais, recomenda-se um broker privado com TLS.

O tópico MQTT "asgard/" pode ser alterado conforme a necessidade do projeto.

O código é compatível com o SDK ESP-IDF e utiliza a biblioteca oficial esp-mqtt.



📄 Licença
Este projeto é fornecido como exemplo educacional e está livre para uso pessoal e acadêmico.

Desenvolvido com ❤️ utilizando ESP-IDF


