#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

// 1. INCLUIR A BIBLIOTECA MQTT
#include "mqtt_client.h"

/* ===================================================
 * CONFIGURAÇÕES DE WI-FI 
 * ===================================================*/
#define EXAMPLE_ESP_WIFI_SSID      "redeteste"
#define EXAMPLE_ESP_WIFI_PASS      "teste@#2571"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const char *TAG = "WIFI_E_MQTT";
static int s_retry_num = 0;

// Declaração do handle do cliente MQTT para ser usado globalmente
esp_mqtt_client_handle_t client;


/* ===================================================
 * LÓGICA DO MQTT
 * ===================================================*/

// Função para tratar os eventos do MQTT (conexão, desconexão, dados recebidos, etc.)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client; // Não é necessário, pois temos o client global
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED: Conectado ao broker MQTT!");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED: Desconectado do broker MQTT!");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR, error_code=%d", event->error_handle->error_type);
        break;
    default:
        ESP_LOGI(TAG, "Outro evento do MQTT: event_id:%ld", event_id);
        break;
    }
}

// Função para iniciar o cliente MQTT
static void mqtt_app_start(void) {
    // Configurações do cliente MQTT
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.hivemq.com:1883", // URI do broker público HiveMQ (porta padrão 1883)
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    // Registra a função de callback para os eventos do MQTT
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // Inicia o cliente MQTT
    esp_mqtt_client_start(client);
}


/* ===================================================
 * LÓGICA DO WI-FI 
 * ===================================================*/
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Tentando reconectar ao Wi-Fi...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Falha ao conectar no Wi-Fi");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP Obtido:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finalizado.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado ao AP SSID:%s", EXAMPLE_ESP_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Falha ao conectar ao SSID:%s", EXAMPLE_ESP_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "EVENTO INESPERADO");
    }
    
}


/* ===================================================
 * FUNÇÃO PRINCIPAL (app_main)
 * ===================================================*/
void app_main(void) {
    // Inicializa o NVS (necessário para o Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Iniciando modo Wi-Fi Station");
    wifi_init_sta();

    ESP_LOGI(TAG, "Iniciando aplicação MQTT...");
    mqtt_app_start();

    // Loop principal para publicar mensagens periodicamente
    int msg_id;
    char msg_buffer[50];
    int count = 0;

    while (1) {
        // Prepara a mensagem a ser enviada
        sprintf(msg_buffer, "Ola do ESP32! Contagem: %d", count++);
        
        // Publica a mensagem
        msg_id = esp_mqtt_client_publish(
            client,             // Handle do cliente
            "asgard/",          // Tópico (o mesmo da sua imagem)
            msg_buffer,         // Mensagem
            0,                  // Tamanho da mensagem (0 para calcular automaticamente)
            1,                  // QoS (Qualidade de Serviço)
            0                   // Reter mensagem (0 = não)
        );
        ESP_LOGI(TAG, "Mensagem publicada, msg_id=%d", msg_id);

        // Aguarda 10 segundos antes de enviar a próxima mensagem
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}