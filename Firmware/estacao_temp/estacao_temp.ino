/*
  ============================================================================
  ESTACAO METEOROLOGICA E RELOGIO COM ESP8266 (UNO+WIFI R3) E DISPLAY NEXTION
  ============================================================================    
  ==== ARDUINO DAS COISAS - @arduinodascoisas =====
  =====================================================
  Autor: Tiago Pereira
  E-mail: arduinodascoisas@outlook.com
  Instagram: @arduinodascoisas
  YouTube: @arduinodascoisas
  =====================================================
*/

#include <ESP8266WiFi.h>       // Biblioteca para conectar o ESP8266 a rede Wi-Fi
#include <ESP8266HTTPClient.h> // Biblioteca para fazer requisicoes HTTP (GET) na internet
#include <WiFiClient.h>        // Biblioteca base de cliente TCP/IP para o Wi-Fi
#include <ArduinoJson.h>       // Biblioteca para ler e extrair dados do texto JSON da API
#include <time.h>              // Biblioteca padrao de manipulacao de data e hora

// --- CONFIGURACOES DA REDE WI-FI E DA API ---
const char* ssid = "NOME_DO_SEU_WIFI";                   // Nome da sua rede Wi-Fi (2.4 GHz)
const char* password = "SENHA_DO_SEU_WIFI";              // Senha da sua rede Wi-Fi
String apiKey = "digite-aqui-a-sua-chave-OpenAPI";       // Sua chave de acesso a API OpenWeatherMap crie seu perfil em: https://openweathermap.org/

// --- CONFIGURACAO DAS CIDADES ---
// IDs oficiais: Salvador (3450554), Sao Paulo (3448439), Brasilia (3469058), Curitiba (3464975)
String idsCidades[] = {"3450554", "3448439", "3469058", "3464975"}; 
// Nomes que serao exibidos no campo tCidade da tela Nextion
String nomesExibicao[] = {"Salvador", "Sao Paulo", "Brasilia", "Curitiba"}; 
int indiceCidade = 0;                                    // Controla qual cidade esta selecionada (0 = Salvador)
int totalCidades = 4;                                    // Quantidade total de cidades cadastradas

// --- VARIAVEIS DE CONTROLE DE TEMPO E ATUALIZACAO ---
unsigned long epochBase = 0;                             // Armazena o timestamp (segundos) recebido da internet
unsigned long millisBase = 0;                            // Marca o momento (millis) em que o horario foi sincronizado
unsigned long ultimaAtualizacaoClima = 0;                // Marca o momento da ultima consulta de clima na API
const unsigned long intervaloClima = 900000;             // Intervalo de atualizacao do clima (15 min = 900.000 ms)

// ============================================================================
// FUNCAO SETUP: EXECUTADA UMA UNICA VEZ AO LIGAR OU REINICIAR A PLACA
// ============================================================================
void setup() {
  Serial.begin(9600);                                    // Inicia comunicacao Serial a 9600 bps (padrao Nextion)
  
  WiFi.mode(WIFI_STA);                                   // Configura o ESP8266 no modo Estacao (cliente Wi-Fi)
  WiFi.begin(ssid, password);                            // Inicia a conexao com o roteador Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {                // Aguarda em loop ate que o Wi-Fi esteja conectado
    delay(500);                                          // Espera meio segundo entre cada verificacao
  }

  delay(1000);                                           // Aguarda 1 segundo para estabilizar o DNS e a rede
  atualizarClima();                                      // Faz a primeira busca de clima e sincroniza data e hora
}

// ============================================================================
// FUNCAO LOOP: EXECUTADA CONTINUAMENTE ENQUANTO A PLACA ESTIVER LIGADA
// ============================================================================
void loop() {
  // 1. Verifica se o usuario tocou no botao "Selecionar cidades" na tela Nextion
  while (Serial.available() > 0) {                       // Enquanto houver bytes chegando na porta Serial
    byte b = Serial.read();                              // Le o primeiro byte disponivel
    if (b == 0x65 || b == 'C') {                         // Verifica se e evento de toque (0x65) ou caractere 'C'
      delay(30);                                         // Aguarda 30 ms para garantir que todo o pacote chegou
      while (Serial.available() > 0) Serial.read();      // Limpa os bytes restantes do pacote
      
      indiceCidade = (indiceCidade + 1) % totalCidades;  // Avanca para a proxima cidade (volta ao 0 apos a ultima)
      enviarTexto("tCidade", "Buscando...");             // Mostra aviso imediato na tela enquanto baixa os dados
      atualizarClima();                                  // Consulta e exibe o clima da nova cidade selecionada
      break;                                             // Sai do loop de leitura Serial
    }
  }

  // 2. Atualiza o relogio na tela a cada 1 segundo (1000 ms)
  static unsigned long ultimoTick = 0;                   // Guarda o instante da ultima atualizacao dos segundos
  if (millis() - ultimoTick >= 1000) {                   // Se ja passou 1 segundo desde a ultima atualizacao
    ultimoTick = millis();                               // Atualiza o marcador de tempo
    atualizarRelogioTela();                              // Recalcula e envia hora, segundos, data e fase da lua
  }

  // 3. Atualiza a previsao do tempo automaticamente a cada 15 minutos
  if (millis() - ultimaAtualizacaoClima > intervaloClima) { // Se ja passaram 15 minutos desde a ultima consulta
    ultimaAtualizacaoClima = millis();                   // Atualiza o marcador de tempo
    atualizarClima();                                    // Busca novos dados meteorologicos e ressincroniza hora
  }
}
// ============================================================================
// FUNCAO PARA CALCULAR E EXIBIR DATA, HORA, SEGUNDOS E FASE DA LUA
// ============================================================================
void atualizarRelogioTela() {
  if (epochBase == 0) return;                            // Se o relogio ainda nao foi sincronizado, sai da funcao

  // Calcula horario atual somando os segundos passados e subtraindo 3h (10800s) do fuso horario
  time_t agora = epochBase + ((millis() - millisBase) / 1000) - 10800; 
  struct tm *ptm = gmtime(&agora);                       // Converte o timestamp em estrutura de data e hora

  // Listas com os nomes dos dias da semana e meses abreviados
  String diasSemana[] = {"Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado"};
  String meses[] = {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun", "Jul", "Ago", "Set", "Out", "Nov", "Dez"};

  int h = ptm->tm_hour;                                  // Extrai a hora atual (0 a 23)
  int m = ptm->tm_min;                                   // Extrai os minutos atuais (0 a 59)
  int s = ptm->tm_sec;                                   // Extrai os segundos atuais (0 a 59)
  
  // Monta o texto da hora no formato HH:MM:SS adicionando zero a esquerda quando menor que 10
  String horaStr = (h < 10 ? "0" : "") + String(h) + ":" + 
                   (m < 10 ? "0" : "") + String(m) + ":" + 
                   (s < 10 ? "0" : "") + String(s);

  // Envia o dia da semana + hora com segundos para o campo tDiaHora (Ex: "Segunda 00:07:15")
  enviarTexto("tDiaHora", diasSemana[ptm->tm_wday] + " " + horaStr);

  // Envia o dia, mes e ano completo para o campo tData (Ex: "16 Nov 2026")
  int ano = ptm->tm_year + 1900;                         // A estrutura tm conta anos a partir de 1900
  enviarTexto("tData", String(ptm->tm_mday) + " " + meses[ptm->tm_mon] + " " + String(ano));

  // Atualiza os dias do rodape e a fase da lua apenas uma vez por minuto (quando segundo == 0)
  static bool primeiraVez = true;                        // Garante que rode imediatamente na primeira execucao
  if (s == 0 || primeiraVez) {                           // Executa na virada do minuto ou ao ligar
    primeiraVez = false;                                 // Desmarca a flag de primeira execucao
    int dia1Idx = (ptm->tm_wday + 1) % 7;                // Calcula o indice do dia de amanha
    int dia2Idx = (ptm->tm_wday + 2) % 7;                // Calcula o indice do dia depois de amanha
    enviarTexto("tDia1", diasSemana[dia1Idx]);           // Atualiza o nome do dia 1 no rodape
    enviarTexto("tDia2", diasSemana[dia2Idx]);           // Atualiza o nome do dia 2 no rodape

    calcularFaseLua(ano, ptm->tm_mon + 1, ptm->tm_mday); // Calcula e envia o texto e o icone da fase da lua
  }
}

// ============================================================================
// FUNCAO PARA EXTRAIR DATA E HORA DO CABECALHO HTTP "Date" DA OPENWEATHERMAP
// ============================================================================
void sincronizarHoraHeader(String dateHeader) {
  if (dateHeader.length() < 25) return;                  // Ignora se o cabecalho vier incompleto

  // Extrai cada parte da string padrao RFC (Ex: "Mon, 16 Nov 2026 03:07:00 GMT")
  int dia = dateHeader.substring(5, 7).toInt();          // Extrai o dia do mes
  String mesStr = dateHeader.substring(8, 11);           // Extrai a sigla do mes em ingles
  int ano = dateHeader.substring(12, 16).toInt();        // Extrai o ano com 4 digitos
  int hora = dateHeader.substring(17, 19).toInt();       // Extrai a hora UTC
  int min = dateHeader.substring(20, 22).toInt();        // Extrai os minutos
  int seg = dateHeader.substring(23, 25).toInt();        // Extrai os segundos

  // Converte a sigla do mes em ingles para numero (0 a 11)
  String mesesEn[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int mes = 0;                                           // Variavel que guardara o indice do mes
  for (int i = 0; i < 12; i++) {                         // Percorre os 12 meses
    if (mesStr == mesesEn[i]) { mes = i; break; }        // Quando encontrar o mes correspondente, salva o indice
  }

  // Preenche a estrutura de tempo para converter em timestamp Unix (segundos)
  struct tm t;                                           // Cria estrutura de data/hora
  t.tm_year = ano - 1900;                                // Ajusta ano para padrao C (anos desde 1900)
  t.tm_mon = mes;                                        // Define o mes (0-11)
  t.tm_mday = dia;                                       // Define o dia do mes (1-31)
  t.tm_hour = hora;                                      // Define a hora
  t.tm_min = min;                                        // Define os minutos
  t.tm_sec = seg;                                        // Define os segundos
  t.tm_isdst = 0;                                        // Sem horario de verao

  epochBase = mktime(&t);                                // Converte a estrutura em segundos totais (timestamp)
  millisBase = millis();                                 // Registra o cronometro interno exato desse momento
  atualizarRelogioTela();                                // Atualiza a tela imediatamente com o novo horario
}

// ============================================================================
// FUNCAO PARA BUSCAR DADOS DO CLIMA ATUAL E PREVISAO DE 2 DIAS NA API
// ============================================================================
void atualizarClima() {
  if (WiFi.status() != WL_CONNECTED) return;             // So tenta buscar se o Wi-Fi estiver conectado

  WiFiClient client;                                     // Cria cliente de rede
  HTTPClient http;                                       // Cria objeto cliente HTTP
  http.setTimeout(10000);                                // Define tempo limite de espera de 10 segundos

  const char *headerKeys[] = {"Date"};                   // Define que queremos capturar o cabecalho "Date"
  http.collectHeaders(headerKeys, 1);                    // Ativa a captura do cabecalho HTTP

  // Monta o endereco da API pedindo 17 blocos de 3 horas (cobre hoje + 48 horas)
  String uri = "/data/2.5/forecast?id=" + idsCidades[indiceCidade] + "&units=metric&lang=pt&cnt=17&appid=" + apiKey;
  
  if (http.begin(client, "api.openweathermap.org", 80, uri)) { // Inicia conexao com o servidor OpenWeatherMap
    int httpCode = http.GET();                           // Envia a requisicao GET e guarda o codigo (200 = OK)

    if (httpCode == 200) {                               // Se a API respondeu com sucesso
      if (http.hasHeader("Date")) {                      // Se o servidor enviou o cabecalho de data/hora
        sincronizarHoraHeader(http.header("Date"));      // Sincroniza o relogio interno com a hora oficial
      }

      JsonDocument doc;                                  // Cria o documento JSON na memoria
      DeserializationError error = deserializeJson(doc, http.getStream()); // Le o fluxo JSON da resposta

      if (!error) {                                      // Se a leitura do JSON ocorreu sem erros
        JsonArray list = doc["list"];                    // Acessa a lista de previsoes retornada pela API
        int idx0 = 0;                                    // Indice 0 representa o clima atual
        int idx8 = 8;                                    // Indice 8 representa +24 horas (amanha)
        int idx16 = 16;                                  // Indice 16 representa +48 horas (depois de amanha)

        // --- LEITURA E ENVIO DO CLIMA ATUAL ---
        JsonObject atual = list[idx0];                   // Pega o primeiro bloco de previsao (agora)
        int temp = round((float)atual["main"]["temp"]);  // Le a temperatura atual e arredonda para inteiro
        int umid = atual["main"]["humidity"];            // Le a umidade relativa do ar (%)
        float ventoMs = atual["wind"]["speed"];          // Le a velocidade do vento em m/s
        int ventoDeg = atual["wind"]["deg"];             // Le a direcao do vento em graus (0 a 360)
        const char* descPtr = atual["weather"][idx0]["description"] | ""; // Le a descricao do ceu
        const char* iconePtr = atual["weather"][idx0]["icon"] | "01d";    // Le o codigo do icone do clima
        String desc = String(descPtr);                   // Converte a descricao para String
        String icone = String(iconePtr);                 // Converte o codigo do icone para String

        if (desc.length() > 0) {                         // Se houver texto na descricao
          desc.setCharAt(0, toupper(desc.charAt(0)));    // Transforma a primeira letra em maiuscula
        }

        enviarTexto("tCidade", nomesExibicao[indiceCidade]); // Atualiza o nome da cidade no topo esquerdo
        enviarTexto("tTemp", String(temp) + "\xB0" + "C");   // Atualiza a temperatura grande (0xB0 = simbolo grau)
        enviarTexto("tCondicao", desc);                      // Atualiza a descrição do clima atual
        enviarTexto("tUmid", "Umid: " + String(umid) + "%"); // Atualiza a umidade atual
        
        int ventoKmh = round(ventoMs * 3.6);             // Converte velocidade do vento de m/s para km/h
        enviarTexto("tVento", "Vento: " + direcaoVento(ventoDeg) + " " + String(ventoKmh) + "km/h");
        enviarComando("pClimaHoje.pic=" + String(mapearIconeClima(icone))); // Troca a imagem do clima atual

        // --- PREVISAO DIA 1 (AMANHA: VARRE OS BLOCOS 1 A 8 PARA MINIMA E MAXIMA) ---
        if (list.size() > 8) {                           // Garante que existem dados suficientes na lista
          float min1 = 100, max1 = -100;                 // Inicia variaveis de comparacao com valores extremos
          for (int i = 1; i <= 8; i++) {                 // Percorre os 8 intervalos de 3h das proximas 24h
            float t = list[i]["main"]["temp"];           // Le a temperatura prevista em cada intervalo
            if (t < min1) min1 = t;                      // Atualiza min1 se encontrar temperatura menor
            if (t > max1) max1 = t;                      // Atualiza max1 se encontrar temperatura maior
          }
          int umid1 = list[idx8]["main"]["humidity"];    // Pega a umidade prevista para amanha
          const char* ico1Ptr = list[idx8]["weather"][idx0]["icon"] | "01d"; // Pega o icone previsto
          String ico1 = String(ico1Ptr);                 // Converte para String

          enviarTexto("tMinMax1", String((int)round(min1)) + "\xB0" + "~" + String((int)round(max1)) + "\xB0" + "C");
          enviarTexto("tUmid1", "Umid:" + String(umid1) + "%");
          enviarComando("pDia1.pic=" + String(mapearIconeClima(ico1)));
        }

        // --- PREVISAO DIA 2 (DEPOIS DE AMANHA: VARRE OS BLOCOS 9 A 16 PARA MINIMA E MAXIMA) ---
        if (list.size() > 16) {                          // Garante que existem dados ate o indice 16
          float min2 = 100, max2 = -100;                 // Inicia variaveis de comparacao com valores extremos
          for (int i = 9; i <= 16; i++) {                // Percorre os 8 intervalos de 3h entre 24h e 48h
            float t = list[i]["main"]["temp"];           // Le a temperatura prevista em cada intervalo
            if (t < min2) min2 = t;                      // Atualiza a menor temperatura encontrada
            if (t > max2) max2 = t;                      // Atualiza a maior temperatura encontrada
          }
          int umid2 = list[idx16]["main"]["humidity"];   // Pega a umidade prevista para depois de amanha
          const char* ico2Ptr = list[idx16]["weather"][idx0]["icon"] | "01d"; // Pega o icone previsto
          String ico2 = String(ico2Ptr);                 // Converte para String

          enviarTexto("tMinMax2", String((int)round(min2)) + "\xB0" + "~" + String((int)round(max2)) + "\xB0" + "C");
          enviarTexto("tUmid2", "Umid:" + String(umid2) + "%");
          enviarComando("pDia2.pic=" + String(mapearIconeClima(ico2)));
        }
      }
    } else {
      enviarTexto("tCidade", "Erro: " + String(httpCode)); // Se a API retornar erro HTTP, exibe no topo
    }
    http.end();                                          // Encerra a conexao HTTP para liberar memoria
  }
}
// ============================================================================
// FUNCAO PARA CONVERTER GRAUS (0-360) EM DIRECAO CARDEAL (N, NE, L, SE...)
// ============================================================================
String direcaoVento(int deg) {
  String dirs[] = {"N", "NE", "L", "SE", "S", "SO", "O", "NO"}; // Siglas das 8 direcoes
  int idx = round(((deg % 360) / 45.0));                 // Divide os 360 graus em 8 fatias de 45 graus
  return dirs[idx % 8];                                  // Retorna a sigla correspondente
}

// ============================================================================
// FUNCAO PARA MAPEAR O CODIGO DA API PARA O ID DA IMAGEM NO NEXTION EDITOR
// ============================================================================
int mapearIconeClima(String code) {
  if (code.startsWith("01")) return 1;                   // 01d/01n -> ID 1 (Sol limpo)
  if (code.startsWith("02")) return 2;                   // 02d/02n -> ID 2 (Sol com poucas nuvens)
  if (code.startsWith("03") || code.startsWith("04")) return 3; // 03/04 -> ID 3 (Nublado)
  if (code.startsWith("09") || code.startsWith("10")) return 4; // 09/10 -> ID 4 (Chuva)
  if (code.startsWith("11")) return 5;                   // 11d/11n -> ID 5 (Tempestade)
  return 1;                                              // Padrao caso venha outro codigo -> ID 1
}

// ============================================================================
// FUNCAO PARA CALCULAR A FASE DA LUA A PARTIR DA DATA ATUAL
// ============================================================================
void calcularFaseLua(int ano, int mes, int dia) {
  if (mes < 3) { ano--; mes += 12; }                     // Ajuste de calendario juliano para jan e fev
  ++mes;                                                 // Incrementa o mes para a formula astronomica
  double c = 365.25 * ano;                               // Calcula dias acumulados pelos anos
  double e = 30.6 * mes;                                 // Calcula dias acumulados pelos meses
  double jd = c + e + dia - 694039.09;                   // Dias passados desde uma lua nova de referencia
  jd /= 29.5305882;                                      // Divide pela duracao exata de um ciclo lunar (29,53 dias)
  int b = (int)jd;                                       // Isola a parte inteira (ciclos completos)
  jd -= b;                                               // Mantem apenas a fracao do ciclo atual (0.0 a 0.99)
  b = round(jd * 8);                                     // Converte a fracao em um indice de 0 a 8
  if (b >= 8) b = 0;                                     // Se arredondar para 8, volta para 0 (Lua Nova)

  // Nomes das 8 fases lunares
  String fases[] = {"Lua Nova", "Lua Crescente", "Quarto Crescente", "Gibosa Crescente", "Lua Cheia", "Gibosa Minguante", "Quarto Minguante", "Lua Minguante"};
  enviarTexto("tLua", fases[b]);                         // Envia o nome da fase atual para o campo tLua
  enviarComando("pLua.pic=" + String(10 + b));           // Atualiza a imagem da lua (IDs 10 a 17 no Nextion)
}

// ============================================================================
// FUNCOES DE COMUNICACAO COM O PROTOCOLO SERIAL DO DISPLAY NEXTION
// ============================================================================
void enviarTexto(String comp, String val) {
  Serial.print(comp + ".txt=\"" + val + "\"");           // Envia comando no formato: componente.txt="valor"
  Serial.write(0xff); Serial.write(0xff); Serial.write(0xff); // Envia os 3 bytes obrigatorios de fim
}

void enviarComando(String cmd) {
  Serial.print(cmd);                                     // Envia comando direto (ex: pClimaHoje.pic=3)
  Serial.write(0xff); Serial.write(0xff); Serial.write(0xff); // Envia os 3 bytes obrigatorios de fim
}