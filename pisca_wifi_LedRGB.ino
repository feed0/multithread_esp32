/*
 * Um led RGB brilha em várias cores, conforme o valor enviado para três de seus contatos.
 * O quarto contato é o terra (GND)
 *
 * Programa abaixo foi criado para solicitar "instruções" para um servidor
 * em relação ao comportamento de um led RGB. Os pinos do led estarão conectados aos
 * GPIOs 23, 22 e 21 do ESP32. O 21 está no pino R, o 22 no pino G e o 23 no pino B.
 * 
 * O servidor determinará qual o comportamento que o ESP32 deve ter em relação às
 * cores que devem piscar a cada 0,5 segundo. O servidor enviará um número entre 1 e 10.
 * 
 * 1. Vermelho (100) e branco (111)
 * 2. Verde (010) e branco
 * 3. Azul (001) e branco
 * 4. Cyan (011) e branco
 * 5. Vermelho e amarelo (110)
 * 6. Vermelho e azul
 * 7. Amarelo e azul
 * 8. Verde e vermelho
 * 9. Azul e verde
 *10. Roxo (101) e amarelo
 * 
 * O servidor respondera apenas a requisições GET no endereço
 * http://ff.h20.com.br/so/p01.php
 * ... passando um parâmetro q=Gnn, onde nn é o número do grupo.
 * 
 * Portanto, a requisição será da forma
 * http://ff.h20.com.br/so/p01.php?q=Gnn
 * 
 * A resposta do servidor será sempre um inteiro entre 1 e 10
 * 
 * Mais requisitos:
 * O ESP32 deve enviar uma requisição e permanecer no comportamento
 * solicitado pelo servidor (num entre 1 e 8) por 5 segundos, quando
 * deve então refazer a solicitação e ficar nesse ciclo indefinidamente,
 * sempre reproduzindo nos leds o comportamento indicado.
 */


// coloque seu grupo ...
// AQUI         ---
String GRUPO = "G20";
//              ---
#define RR 21
#define GG 22
#define BB 23

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid     = "AAAAAAAA";
const char* password = "08120812";
String serverName    = "http://h20.com.br/so/";

// CHANGES 5 -------------------------------------------------------------------------------------------------
int comando = 1; // Inicializa com algum valor 

// CHANGES 7 -------------------------------------------------------------------------------------------------
#include "freertos/semphr.h"
SemaphoreHandle_t xSemaphore = xSemaphoreCreateBinary();

#define CICLO_LED      5000 // ms
#define CICLO_REQUEST 10000 // ms
 
void setup() {
  // para debug, se quiser...
  Serial.begin(115200);  // velocidade em 115200
  while (!Serial);  // Waiting for Serial Monitor
  Serial.println("\nComando de LED!");

  // inicializa os pinos usados como SAIDA
  pinMode(RR, OUTPUT);
  pinMode(GG, OUTPUT);
  pinMode(BB, OUTPUT);

  // inicializa WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("Conectou o WiFi");

  // Pisca os leds alternadamente para indicar que conectou no wifi
  for (int i = 0; i < 3; i++) {
    vermelho();
    delay(500);
    verde();
    delay(500);
    azul();
    delay(500);
  }

  // CHANGES 3 -------------------------------------------------------------------------------------------------
  // Task para controle dos LEDs
  xTaskCreatePinnedToCore(
    controleLed,   // Função da task
    "ControleLed", // Nome da task
    10000,         // Tamanho da stack
    NULL,          // Parâmetro
    1,             // Prioridade ??
    NULL,          // Task handle
    0              // Core a ser usado
  );

  // CHANGES 4 -------------------------------------------------------------------------------------------------
  // Task para fazer requisição HTTP
  xTaskCreatePinnedToCore(
    requisicaoHttp,   // Função da task
    "RequisicaoHTTP", // Nome da task
    10000,            // Tamanho da stack
    NULL,             // Parâmetro
    1,                // Prioridade ??
    NULL,             // Task handle
    1                 // Core a ser usado
  );
}

void loop() {

  // CHANGES 6
  // int comando;
  // // pede comando ao servidor
  // comando = getServer(GRUPO);
  // Serial.print("Comando recebido: ");
  // Serial.println(comando);

  // // comanda os leds de acordo com o parâmetro recebido
  // setLed(comando);
}

// CHANGES 1 -------------------------------------------------------------------------------------------------
void controleLed(void *pvParameters) {
  while (true) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY); // Aguarde até que o comando seja atualizado
    setLed(comando);
    xSemaphoreGive(xSemaphore);                // Libere o semáforo para a task HTTP
    
    // DELAY ??
    // vTaskDelay(pdMS_TO_TICKS(CICLO_LED));
  }
}

// CHANGES 2 -------------------------------------------------------------------------------------------------
void requisicaoHttp(void *pvParameters) {
  while (true) {
    comando = getServer(GRUPO); // Requisicao
    xSemaphoreGive(xSemaphore); // Libere o semáforo para a task LEDs
    
    // DELAY ??
    // vTaskDelay(pdMS_TO_TICKS(CICLO_REQUEST));
  }
}

int getServer(String G) {
  // apaga os leds
  digitalWrite(RR, LOW);
  digitalWrite(GG, LOW);
  digitalWrite(BB, LOW);
  // prepara a requisição...
  String serverPath = serverName + "p01.php?q=" + G;
  Serial.print("ServerPath: ");
  Serial.println(serverPath);
  // ... e faz requisição ao servidor
  HTTPClient http;
  http.setTimeout(10000);  // aumentando timeout por causa da "sacanagem" no servidor...
  http.begin(serverPath.c_str());
  // variavel para receber o status da resposta do servidor (200 para ok, 404 para page fault etc)
  int httpResponseCode = http.GET();
  // variavel que receberá a resposta
  String payload = "";
  Serial.print("Response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {  // se responseCode = 200 (ok!)
    payload = http.getString();
  }
  int com = payload.toInt();
  Serial.print("Server enviou ... ");
  Serial.print(payload);
  Serial.print(", que ficou sendo ... ");
  Serial.println(com);

  if ((com < 1) || (com > 10)) {
    setLed(0);
  }
  return com;
}

// Padroes estabelecidos pelo professor
void setLed(int c) {
  if (c == 0) {  // deu pau! Pisca rápido e para sempre...
    while (1) {
      vermelho();
      delay(100);
      branco();
      delay(100);
    }
  } else if (c == 1) {
    for (int i = 0; i < 8; i++) {
      vermelho();
      delay(500);
      branco();
      delay(500);
    }
  } else if (c == 2) {
    for (int i = 0; i < 8; i++) {
      verde();
      delay(500);
      branco();
      delay(500);
    }
  } else if (c == 3) {
    for (int i = 0; i < 8; i++) {
      azul();
      delay(500);
      branco();
      delay(500);
    }
  } else if (c == 4) {
    for (int i = 0; i < 8; i++) {
      cyan();
      delay(500);
      branco();
      delay(500);
    }
  } else if (c == 5) {
    for (int i = 0; i < 8; i++) {
      vermelho();
      delay(500);
      amarelo();
      delay(500);
    }
  } else if (c == 6) {
    for (int i = 0; i < 8; i++) {
      vermelho();
      delay(500);
      azul();
      delay(500);
    }
  } else if (c == 7) {
    for (int i = 0; i < 8; i++) {
      amarelo();
      delay(500);
      azul();
      delay(500);
    }
  } else if (c == 8) {
    for (int i = 0; i < 8; i++) {
      verde();
      delay(500);
      vermelho();
      delay(500);
    }
  } else if (c == 9) {
    for (int i = 0; i < 8; i++) {
      azul();
      delay(500);
      verde();
      delay(500);
    }
  } else {
    for (int i = 0; i < 8; i++) {
      roxo();
      delay(500);
      amarelo();
      delay(500);
    }
  }
  return;
}

// Cores prontas
void branco() {
  digitalWrite(RR, HIGH);
  digitalWrite(GG, HIGH);
  digitalWrite(BB, HIGH);
}
void vermelho() {
  digitalWrite(RR, HIGH);
  digitalWrite(GG, LOW);
  digitalWrite(BB, LOW);
}
void verde() {
  digitalWrite(RR, LOW);
  digitalWrite(GG, HIGH);
  digitalWrite(BB, LOW);
}
void azul() {
  digitalWrite(RR, LOW);
  digitalWrite(GG, LOW);
  digitalWrite(BB, HIGH);
}
void amarelo() {
  digitalWrite(RR, HIGH);
  digitalWrite(GG, HIGH);
  digitalWrite(BB, LOW);
}
void cyan() {
  digitalWrite(RR, LOW);
  digitalWrite(GG, HIGH);
  digitalWrite(BB, HIGH);
}
void roxo() {
  digitalWrite(RR, HIGH);
  digitalWrite(GG, LOW);
  digitalWrite(BB, HIGH);
}
