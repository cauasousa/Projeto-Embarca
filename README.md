# DESENVOLVIMENTO DE UM SISTEMA DE MONITORAMENTO DA CONDUTIVIDADE ELÉTRICA (CE) DE SOLUÇÕES PARA OTIMIZAÇÃO E REDUÇÃO DE CUSTOS NA PRODUÇÃO DE VIDEIRAS   

---

# Projeto com Raspberry Pi Pico W e BitDogLab

Este projeto utiliza o **Raspberry Pi Pico W** em conjunto com a placa **BitDogLab**, um resistor de **1K** e uma tomada antiga para criar uma solução integrada que envolve leitura de dados, comunicação Wi-Fi e integração com o **Firebase**.

## Componentes e Bibliotecas Utilizadas

- **Raspberry Pi Pico W**: Utilizado como microcontrolador principal.
- **BitDogLab**: Placa utilizada para interface com componentes externos.
- **Resistor de 1K**: Utilizado para controle de corrente em circuitos específicos.
- **Tomada Antiga**: Adaptada para integração com o sistema.
- **Bibliotecas**:
  - `pico/stdlib.h`
  - `hardware/pio.h`
  - `hardware/clocks.h`
  - `hardware/i2c.h`
  - `hardware/adc.h`
  - `pico/cyw43_arch.h`
  - `lwip/tcp.h`
  - `ssd1306.h` (para controle de display OLED)
  - `ws2818b.pio.h` (gerada automaticamente durante a compilação)

## Configuração do Projeto

Para utilizar este código, é necessário configurar as seguintes informações no arquivo principal:

1. **Wi-Fi**:
   - Defina o nome da rede Wi-Fi e a senha no código:
     ```c
     #define WIFI_SSID "NOME_DA_REDE"
     #define WIFI_PASS "SENHA_DA_REDE"
     ```

2. **Firebase**:
   - Configure as credenciais do Firebase no seguinte trecho do código:
     ```c
     const firebaseConfig = {
       apiKey: "SUA_API_KEY",
       authDomain: "SEU_AUTH_DOMAIN",
       projectId: "SEU_PROJECT_ID",
       storageBucket: "SEU_STORAGE_BUCKET",
       messagingSenderId: "SEU_MESSAGING_SENDER_ID",
       appId: "SEU_APP_ID"
     };
     ```

3. **Estrutura do Banco de Dados**:
   - O Firebase está configurado para armazenar dados na seguinte estrutura:
     ```
     solutions/
       id/
         solutionName: "Nome da Solução",
         conductivity: Valor da Condutividade,
         voltage: Valor da Tensão,
         resistance: Valor da Resistência
     ```
   - Esses dados podem ser utilizados para criar aplicações web ou móveis que acessam e interpretam as informações armazenadas.

## Como Utilizar

1. Clone este repositório.
2. Configure as credenciais do Wi-Fi e do Firebase conforme descrito acima.
3. Compile e carregue o código no Raspberry Pi Pico W.
4. Conecte os componentes conforme necessário (BitDogLab, resistor de 1K, tomada antiga, etc.).
5. Monitore os dados no Firebase e utilize-os conforme sua necessidade.

## Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues ou pull requests para melhorias ou correções.
