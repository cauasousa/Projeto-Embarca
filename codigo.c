#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

#define LED_COUNT 25 // Matriz 5x5 = 25 LEDs
#define LED_PIN 7    // Pino de controle dos LEDs

// pinos para o display oled, para comunicação I2C
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// conexao wi-fi
#define WIFI_SSID "NOME_DA_REDE"
#define WIFI_PASS "SENHA"

// Definições para o Sensor
const int SENSOR_PIN = 28;   // Pino analógico conectado ao sensor
const int ADC_CHANNEL_2 = 2; // Canal ADC para o sensor
const float Rref = 1000.0;   // Resistência de referência (1kΩ)
const float Vin = 3.33;      // Tensão de alimentação (5V do Arduino)

float Vout = 0.0f, vout_anterior = 0.0f;
float resistencia_da_solucao = 0.0f, resistencia_da_solucao_anterior = 0.0f;
float condutividade = 0.0f, condutividade_anterior = 0.0f;

// vai exibir no oled
static char text[3][50];

// variavel para controlar o sinal da matriz de led
int aumentou_valor_lido = 1;

// Estrutura para representar cores RGB
typedef struct
{
  uint8_t G, R, B;
} npLED_t;

// Buffer de LEDs
npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

// Mapeia a matriz 5x5 para um array linear (ordem em "Zig-Zag")
const uint8_t led_map[5][5] = {
    {0, 1, 2, 3, 4},
    {9, 8, 7, 6, 5},
    {10, 11, 12, 13, 14},
    {19, 18, 17, 16, 15},
    {20, 21, 22, 23, 24}};

// Definição dos números de 0 a 14 na matriz 5x5 (0 = Alerta com seta pra cima com cor Verde aceso, 1 = Alerta com seta para baixo com cor Vermelho aceso)
const uint8_t numbers[2][5] = {
    {
        0b10001,
        0b10010,
        0b10100,
        0b11000,
        0b11111,
    }, // subiu - verde

    {
        0b11111,
        0b00011,
        0b00101,
        0b01001,
        0b10001,
    }, // desceu - vermelho

};

// meu html para cadastrar os dados no firebase

#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"                                               \
                      "<!DOCTYPE html><html lang=\"pt-br\">"                                                             \
                      "<head>"                                                                                           \
                      "<meta charset=\"UTF-8\" />"                                                                       \
                      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"                     \
                      "<title>Cadastro no Firebase</title>"                                                              \
                      "<script src=\"https://www.gstatic.com/firebasejs/10.9.0/firebase-app-compat.js\"></script>"       \
                      "<script src=\"https://www.gstatic.com/firebasejs/10.9.0/firebase-firestore-compat.js\"></script>" \
                      "</head>"                                                                                          \
                      "<body>"                                                                                           \
                      "<h2>Cadastro de Soluções</h2>"                                                                    \
                      "<form id=\"solutionForm\">"                                                                       \
                      "  Nome da Solução: <input type=\"text\" id=\"solutionName\" required><br>"                        \
                      "  Tensão: <input type=\"number\" id=\"voltage\" step=\"0.01\" required><br>"                      \
                      "  Resistência da Solução: <input type=\"number\" id=\"resistance\" step=\"0.01\" required><br>"   \
                      "  Condutividade: <input type=\"number\" id=\"conductivity\" step=\"0.01\" required><br>"          \
                      "  <button type=\"submit\">Enviar</button>"                                                        \
                      "</form>"                                                                                          \
                      "<p id=\"status\"></p>"                                                                            \
                      "<h2>Soluções Cadastradas</h2>"                                                                    \
                      "<ul id=\"solutionList\"></ul>"                                                                    \
                      "</body>"                                                                                          \
                      "<script>"                                                                                         \
                      "const firebaseConfig = {"                                                                         \
                      "  apiKey: \"COLOQUE_AQUI_SUA_CHAVE\","                                                            \
                      "  authDomain: \"COLOQUE_AQUI_SUA_CHAVE\","                                                        \
                      "  projectId: \"COLOQUE_AQUI_SUA_CHAVE\","                                                         \
                      "  storageBucket: \"COLOQUE_AQUI_SUA_CHAVE\","                                                     \
                      "  messagingSenderId: \"COLOQUE_AQUI_SUA_CHAVE\","                                                 \
                      "  appId: \"COLOQUE_AQUI_SUA_CHAVE\""                                                              \
                      "};"                                                                                               \
                      "firebase.initializeApp(firebaseConfig);"                                                          \
                      "const db = firebase.firestore();"                                                                 \
                      "document.getElementById(\"solutionForm\").onsubmit = function(event) {"                           \
                      "  event.preventDefault();"                                                                        \
                      "  let solutionName = document.getElementById(\"solutionName\").value;"                            \
                      "  let voltage = document.getElementById(\"voltage\").value;"                                      \
                      "  let resistance = document.getElementById(\"resistance\").value;"                                \
                      "  let conductivity = document.getElementById(\"conductivity\").value;"                            \
                      "  db.collection(\"solutions\").add({"                                                             \
                      "    solutionName: solutionName,"                                                                  \
                      "    voltage: parseFloat(voltage),"                                                                \
                      "    resistance: parseFloat(resistance),"                                                          \
                      "    conductivity: parseFloat(conductivity)"                                                       \
                      "  }).then(() => {"                                                                                \
                      "    document.getElementById(\"status\").innerText = \"Dados enviados!\";"                         \
                      "    carregarSolucoes();"                                                                          \
                      "  }).catch((error) => {"                                                                          \
                      "    document.getElementById(\"status\").innerText = \"Erro ao enviar.\";"                         \
                      "  });"                                                                                            \
                      "};"                                                                                               \
                      "function carregarSolucoes() {"                                                                    \
                      "  let lista = document.getElementById(\"solutionList\");"                                         \
                      "  lista.innerHTML = \"Carregando...\";"                                                           \
                      "  db.collection(\"solutions\").get().then((querySnapshot) => {"                                   \
                      "    lista.innerHTML = \"\";"                                                                      \
                      "    querySnapshot.forEach((doc) => {"                                                             \
                      "      let solution = doc.data();"                                                                 \
                      "      let item = document.createElement(\"li\");"                                                 \
                      "      item.textContent = `${solution.solutionName} - Tensão: ${solution.voltage}V, "              \
                      "Resistência: ${solution.resistance}Ω, Condutividade: ${solution.conductivity} S/m`;"              \
                      "      lista.appendChild(item);"                                                                   \
                      "    });"                                                                                          \
                      "  }).catch((error) => {"                                                                          \
                      "    lista.innerHTML = \"Erro ao carregar soluções.\";"                                            \
                      "  });"                                                                                            \
                      "}"                                                                                                \
                      "carregarSolucoes();"                                                                              \
                      "</script></html>\r\n"

// função para configurar a entrada anlógica
static void setup_adc(void);
// Função para ler os sensores e atualizar as variáveis
void ler_vout_resistencia_condutividade()
{

  vout_anterior = Vout;
  resistencia_da_solucao_anterior = resistencia_da_solucao;
  condutividade_anterior = condutividade;

  adc_select_input(ADC_CHANNEL_2);
  sleep_us(2); // estabilizar

  // 1. Leitura do valor analógico (0-4095)
  uint16_t sensorValue = adc_read();

  // 2. Conversão do valor analógico para tensão (0-5V)
  Vout = sensorValue * (Vin / (1 << 12));

  // 3. Cálculo da resistência da solução (Lei de Ohm)
  resistencia_da_solucao = (Rref * Vout) / (Vin - Vout);

  // 4. Cálculo da condutividade elétrica (σ = 1 / R)
  condutividade = (1.0 / resistencia_da_solucao) * 100000; // Convertendo para µS/cm

  printf("\nLitura:%d, T:%f,  R:%f, C:%f ", sensorValue, Vout, resistencia_da_solucao, condutividade);

  if (condutividade > condutividade_anterior)
  {
    aumentou_valor_lido = 0;
  }
  else
  {
    aumentou_valor_lido = 1;
  }
}

// =-=-=-=-= Funções da Matriz de Leds =-=--=-=
// Inicializa a máquina PIO
void npInit(uint pin);
// Define a cor de um LED específico
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b);
// Limpa a matriz de LEDs
void npClear();
// Escreve no LED
void npWrite();
// Exibe um número na matriz de LEDs
void display_number(int num);

// =-=-=-=-= Funções do Display OLED =-=-=-=-
void configuracao_display_oled();
void exibicao_do_texto();
void substituir_ponto_por_F(char *str);
static void formatar_saida_oled();

// =-=-=-=-= Funções HTTP =-=-=-=-
// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
// Função de setup do servidor TCP
static void start_http_server(void);

// configurar o wifi
static void setup_wifi(void);

int main()
{
  stdio_init_all();
  sleep_ms(5000); // estabilizar para inicializar

  // config a entrada edc
  setup_adc();

  // config do display oled
  configuracao_display_oled();
  // config da matriz de led
  npInit(LED_PIN);
  npClear();
  npWrite();

  // config do wifi
  setup_wifi();
  // Inicia o servidor HTTP
  start_http_server();

  while (1)
  {
    cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo

    ler_vout_resistencia_condutividade();
    formatar_saida_oled(text);
    exibicao_do_texto();
    display_number(aumentou_valor_lido);
    sleep_ms(2000); // Troca de número a cada 2 segundo
  }

  cyw43_arch_deinit(); // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
  return 0;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=
// ==-=-=-=-= FUNÇÕES PARA CONFIGURAR O ADC -=-=-==-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--==-=-=-==
// função para configurar a entrada anlógica
static void setup_adc()
{

  // Inicializa o ADC
  // Seleciona o canal ADC0 (GPIO28)
  adc_init();
  adc_gpio_init(SENSOR_PIN);

  adc_select_input(ADC_CHANNEL_2);

  sleep_us(2); // estabilizar
  printf("Entrada ADC configurado ...\n");
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=
// ==-=-=-=-= FUNÇÕES DO MEU SERVIDOR LOCAL -=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (p == NULL)
  {
    // Cliente fechou a conexão
    tcp_close(tpcb);
    return ERR_OK;
  }

  // Processa a requisição HTTP
  char *request = (char *)p->payload;

  // Envia a resposta HTTP
  tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);

  tcp_output(tpcb);

  // Libera o buffer recebido
  pbuf_free(p);

  return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  tcp_recv(newpcb, http_callback); // Associa o callback HTTP
  return ERR_OK;
}

static void start_http_server(void)
{
  struct tcp_pcb *pcb = tcp_new();
  if (!pcb)
  {
    printf("Erro ao criar PCB\n");
    return;
  }

  // Liga o servidor na porta 80
  if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
  {
    printf("Erro ao ligar o servidor na porta 80\n");
    return;
  }

  pcb = tcp_listen(pcb);                // Coloca o PCB em modo de escuta
  tcp_accept(pcb, connection_callback); // Associa o callback de conexão

  printf("Servidor HTTP rodando na porta 80...\n");
}

static void setup_wifi(void)
{
  // Inicializa o Wi-Fi
  if (cyw43_arch_init())
  {
    printf("Erro ao inicializar o Wi-Fi\n");
    return;
  }

  cyw43_arch_enable_sta_mode();
  printf("Conectando ao Wi-Fi...\n");

  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
  {
    printf("Falha ao conectar ao Wi-Fi\n");
    return;
  }
  else
  {
    printf("Conectado.\n");
    // Read the ip address in a human readable way
    uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
  }

  printf("Wi-Fi conectado!\n");
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=
// ==-=-=-=-= FUNÇÕES DO DIPLAY OLED -=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=
void configuracao_display_oled()
{

  // Inicialização do i2c
  uint real_baudrate = i2c_init(i2c1, ssd1306_i2c_clock * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  // Processo de inicialização completo do OLED SSD1306
  ssd1306_init();

  // limpa
  struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
  };
  calculate_render_area_buffer_length(&frame_area);
  uint8_t ssd[ssd1306_buffer_length];
  memset(ssd, 0, ssd1306_buffer_length);
  render_on_display(ssd, &frame_area);

  printf("I2C Baudrate configurado: %d\n", real_baudrate);
}

void exibicao_do_texto()
{

  // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
  struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
  };

  calculate_render_area_buffer_length(&frame_area);

  // zera o display inteiro
  uint8_t ssd[ssd1306_buffer_length];
  memset(ssd, 0, ssd1306_buffer_length);
  render_on_display(ssd, &frame_area);

  // Exibindo o meu conteúdo
  int y = 0;
  for (uint i = 0; i < count_of(text); i++)
  {
    ssd1306_draw_string(ssd, 10, y, text[i]);
    y += 20;
  }

  render_on_display(ssd, &frame_area);
}

void substituir_ponto_por_F(char *str)
{
  char *ptr = strchr(str, '.'); // Encontra o ponto decimal
  if (ptr)
  {
    *ptr = 'F'; // Substitui por 'F'
  }
}

static void formatar_saida_oled()
{

  snprintf(text[0], sizeof(text[0]), "T: %.2f V", Vout);
  snprintf(text[1], sizeof(text[1]), "R: %.2f o", resistencia_da_solucao);
  snprintf(text[2], sizeof(text[2]), "C: %.2f u", condutividade);

  // Substituir ponto decimal por 'F'
  substituir_ponto_por_F(text[0]);
  substituir_ponto_por_F(text[1]);
  substituir_ponto_por_F(text[2]);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=
// ==-=-=-=- FUNÇÕES DA MATRIZ DE LED =-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=

// Inicializa a máquina PIO
void npInit(uint pin)
{
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0)
  {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true);
  }
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
}

// Define a cor de um LED específico
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b)
{
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

// Limpa a matriz de LEDs
void npClear()
{
  for (uint i = 0; i < LED_COUNT; i++)
    npSetLED(i, 0, 0, 0);
}

// Escreve no LED
void npWrite()
{
  for (uint i = 0; i < LED_COUNT; i++)
  {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100);
}

// Exibe um número na matriz de LEDs
void display_number(int num)
{
  npClear();
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 5; col++)
    {
      uint8_t bit = (numbers[num][row] >> (4 - col)) & 1;
      if (bit)
      {
        if (num == 0)
        {
          npSetLED(led_map[row][col], 0, 90, 0); // verde para os LEDs acesos
        }
        else if (num == 1)
        {
          npSetLED(led_map[row][col], 90, 0, 0); // vermelho para os LEDs acesos
        }
      }
    }
  }
  npWrite();
}
