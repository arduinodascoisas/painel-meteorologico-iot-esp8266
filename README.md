# 🌦️ Estação Meteorológica e Relógio com ESP8266 e Display Nextion

Projeto de estação meteorológica conectada via Wi-Fi utilizando uma placa **Arduino Uno + WiFi R3 (ATmega328P + ESP8266 integrado)** e um **display touch Nextion**. O sistema sincroniza data, hora com segundos e previsão do tempo em tempo real através da API da OpenWeatherMap.

## ✨ Funcionalidades

- **Relógio e Calendário em Tempo Real:** Exibe dia da semana, hora completa (`HH:MM:SS`) e data (`DD Mês AAAA`) sincronizados via cabeçalho HTTP.
- **Clima Atual:** Temperatura em graus Celsius (`°C`), descrição do céu, umidade relativa (`%`), velocidade (`km/h`) e direção cardeal do vento (`N, NE, L, SE, S, SO, O, NO`).
- **Previsão para 2 Dias:** Calcula automaticamente a temperatura mínima e máxima diária (`Mín°~Máx°C`), umidade e ícone do clima para os próximos dois dias.
- **Fase da Lua Automática:** Cálculo astronômico do ciclo lunar exibindo o nome da fase atual e o ícone correspondente.
- **Seleção de Cidades via Touch:** Botão interativo na tela para alternar entre **Salvador, São Paulo, Brasília e Curitiba**.

---

## 🛠️ Hardware Utilizado

- 1x Placa **Arduino Uno + WiFi R3** (ATmega328P + ESP8266 32Mb)
- 1x Display **Nextion** (HMI Touch Serial)
- 1x Fonte de alimentação externa **9V** (conectada ao Jack P4 da placa)
- Jumpers para conexão

---

## 🔌 Esquema de Ligação e Configuração da Placa

### 1. Configuração das Chaves (DIP Switches)
Na placa **Arduino Uno + WiFi R3**, ajuste as mini-chaves centrais conforme a etapa:

| Etapa | Chaves em `ON` | Chaves em `OFF` | Observação |
| :--- | :--- | :--- | :--- |
| **Gravar código no ESP8266** | `5, 6, 7` | `1, 2, 3, 4, 8` | Conecta a porta USB diretamente ao ESP8266 para upload. |
| **Operação Normal (ESP8266 + Nextion)** | *(Nenhuma)* | `1, 2, 3, 4, 5, 6, 7, 8` | Isola o chip ATmega328P para evitar conflito na linha RX do toque. |

### 2. Conexão Física com o Display Nextion
Para que o retorno do botão touch (**Selecionar cidades**) funcione sem interferência do ATmega328P, os fios de dados do Nextion são conectados **diretamente na barra de pinos dedicada ao ESP8266** no centro da placa (com todas as chaves DIP em `OFF`):

| Fio do Nextion | Onde ligar na Placa Uno + WiFi R3 |
| :--- | :--- |
| **Vermelho (+5V)** | Pino **`5V`** (barra de alimentação) |
| **Preto (GND)** | Pino **`GND`** (barra de alimentação) |
| **Azul (TX)** | Pino **`RXD0`** (barra central dedicada ao ESP8266) |
| **Amarelo (RX)** | Pino **`TXD0`** (barra central dedicada ao ESP8266) |

---

## 🖥️ Configuração da Interface (Nextion Editor)

- **Codificação das Fontes:** `iso-8859-1` (necessária para exibir o símbolo de grau `°` e acentos).
- **Componentes de Texto (`Text`):** `tCidade`, `tDiaHora`, `tData`, `tTemp`, `tCondicao`, `tVento`, `tUmid`, `tLua`, `tDia1`, `tMinMax1`, `tUmid1`, `tDia2`, `tMinMax2`, `tUmid2`.
- **Componentes de Imagem (`Picture`):** `pClimaHoje`, `pLua`, `pDia1`, `pDia2`.
- **Botão (`Button`):** `bCidades` com a opção **Send Component ID** ativada em *Touch Release Event*.
- **Ordem dos IDs de Imagem:**
  - `0`: Imagem de fundo
  - `1` a `5`: Ícones do clima (Sol, Sol com nuvens, Nublado, Chuva, Tempestade)
  - `6` a `9`: Imagens extras	
  - `10` a `17`: Ícones das 8 fases da lua

---

## 🚀 Como Instalar e Rodar

1. Instale o suporte às placas **ESP8266** na Arduino IDE (selecione **Generic ESP8266 Module**).
2. Instale a biblioteca **`ArduinoJson` (versão 7 ou superior)** pelo Gerenciador de Bibliotecas.
3. Abra o arquivo `.ino` na pasta `firmware/` e preencha suas credenciais:
   ```cpp
   const char* ssid = "NOME_DO_SEU_WIFI";
   const char* password = "SENHA_DO_SEU_WIFI";
   String apiKey = "SUA_CHAVE_OPENWEATHERMAP";
   ```
4. Ajuste as chaves DIP para `5, 6, 7` em `ON`, desconecte os fios do display e faça o upload do código.
5. Coloque todas as chaves DIP em `OFF`, conecte o display Nextion na barra central do ESP8266 (`TXD0`/`RXD0`) e alimente a placa.

---

## 📄 Licença

Este projeto está sob a licença MIT. Sinta-se à vontade para usar, modificar e compartilhar!
