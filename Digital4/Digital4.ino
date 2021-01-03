/**
 * Captura e carga de digitais com o sensor de digitais FPM10
 * Este programa foi feito para conversar com uma aplicação
 * específica no PC
 * 
 * Daniel Quadros, dez/20-jan/21
 */

#include "FPM10A.h"

FPM10A fpm;
int maxDigitais;

// Vamos usar apenas uma posição da memória
// Execício para o leitor: receber a posição do micro
const int posicao = 42;

// Códigos enviados ao PC
// Para evitar confusões não usa caracteres 0-9 e A-F
const char TX_IMAGEM = 'I';
const char TX_FEATURE = 'T';
const char TX_MATCH = 'M';
const char TX_NOMATCH = 'N';
const char TX_PRONTO = 'P';

// Iniciação
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  while (!Serial)
    ;
  fpm.begin();

  // Le a configuração para testar a conexão
  FPM10A::SYSPARAM config;
  if (!fpm.leSysParam(&config)) {
    while (true) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
    }
  }
  maxDigitais = (config.libsize[0] << 8) + config.libsize[1];
}

// Laço principal
void loop() {
  switch (leComando()) {
    case 'C':   // captura digital
      capturaDigital();
      if (fpm.salvaDigital(1, posicao)) {
        piscaOK();
      }
      break;
    case 'S':   // carrega digital na memória do módulo
      carregaDigital();
      if (fpm.salvaDigital(1, posicao)) {
        piscaOK();
      }
      break;
    case 'V':   // verifica digital
      verificaDigital();
      break;
  }
}

// Aguarda um comando do micro
int leComando() {
  // Insiste até ter uma opção válida
  while (true) {
    int c = -1;
    while (c == -1) {
      c = toupper(Serial.read());
    }
    if ((c == 'C') || (c == 'S') || (c == 'V')) {
      return c;
    }
  }
}

// Rotina de "callback" para enviar ao PC (em hexa)
// os dados recebidos do módulo
void txDados(byte *buffer, int tam) {
  for (int i = 0; i < tam; i++) {
    Serial.print (buffer[i] >> 4, HEX);
    Serial.print (buffer[i] & 0xF, HEX);
  }
}

// Faz o captura de uma digital
void capturaDigital() {
  while (true) {
    capturaFeature(1, false);
    capturaFeature(2, true);
    if (fpm.geraModelo()) {
      Serial.write(TX_FEATURE);  // indica inicio de feature
      if (!fpm.leFeature(1, &txDados)) {
        Serial.println("Erro");
      }
      return; // Sucesso
    }
  }
}

// Converte dígito hexadecimal no seu valor
byte nibble(byte d) {
  if ((d >= '0') && (d <= '9')) {
    return d - '0';
  } else if ((d >= 'A') && (d <= 'F')) {
    return d - 'A' + 10;
  } else if ((d >= 'a') && (d <= 'f')) {
    return d - 'a' + 10;
  }
  return 0;
}

// Rotina de "callback" para receber do PC (em hexa)
// os dados a enviar ao módulo
// Retorna o número de bytes no buffer, com sinal trocado se
// for o último pacote
int rxDados(byte *buffer) {
  Serial.write(TX_PRONTO);  // indica que pode receber um pacote
  int pos = 0;
  while (true) {
    int a, b;
    do {
      a = Serial.read();
    } while (a == -1);
    if ((a == 'Z') || (a == 'z')) {
      // Fim do pacote
      return (a == 'z')? pos : -pos;
    }
    do {
      b = Serial.read();
    } while (b == -1);
    if (pos < 128) {
      buffer[pos++] = (nibble(a) << 4) + nibble(b);
    }
  }
}

// Carrega a digital enviada pelo micro
void carregaDigital() {
  fpm.gravaFeature (1, rxDados);
}

// Verifica uma digital
void verificaDigital() {
  capturaFeature(1, false);
  if (!fpm.recuperaDigital(2, posicao)) {
    return;
  }
  if (fpm.comparaDigital()) {
    Serial.write(TX_MATCH);  // indica digital reconhecida
    piscaOK();
  } else {
    Serial.write(TX_NOMATCH);  // indica digital não reconhecida
    piscaErro();
  }
}

// Faz a captura de imagem e geração da feature
void capturaFeature(byte numBuf, bool enviaImg) {
  while (true) {
    // Captura a imagem
    digitalWrite(LED_BUILTIN, LOW);
    do {
      delay(50);
    } while (!fpm.capturaDigital());
    digitalWrite(LED_BUILTIN, HIGH);
    if (enviaImg) {
      Serial.write(TX_IMAGEM);  // indica inicio de imagem
      fpm.leImagem(&txDados);
    }

    // Tenta gerar as features
    bool ok = fpm.geraFeature(numBuf);

    // Pede para retirar o dedo
    do {
      delay(2000);
    } while (fpm.capturaDigital() || (fpm.ultimaResposta() != FPM10A::RESP_NOFINGER));

    // Verifica se precisa tentar novamente
    if (ok) {
      return;
    }
  }
}

// Pisca o LED para indicar um erro
void piscaErro() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
}

// Pisca o LED para indicar sucesso
void piscaOK() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
}
