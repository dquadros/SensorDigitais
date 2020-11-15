/**
 * Cadastramento e Conferência de digitais com o sensor de digitais FPM10
 * 
 * Daniel Quadros, nov/20
 */

#include "FPM10A.h"

FPM10A fpm;
int maxDigitais;

// Iniciação
void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  fpm.begin();
  Serial.println("Pronto");

  // Descobre o número máximo de digitais que o módulo suporta
  FPM10A::SYSPARAM config;
  if (!fpm.leSysParam(&config)) {
    Serial.println ("Erro ao ler configuracao");
    while (true) {
      delay(1000);
    }
  }
  maxDigitais = (config.libsize[0] << 8) + config.libsize[1];
}

// Laço principal
void loop() {
  int posicao;

  // Informa quantas digitais estão armazenadas no sensor
  Serial.println();
  Serial.print(fpm.leTemplateCount());
  Serial.println(" digitais armazenadas");
  Serial.println();

  // Pergunta o que quer fazer
  Serial.println("(1) Apaga todas as digitais, (2) Apaga uma digital, (3) Cadastra digital");
  Serial.println("(4) Compara digital, (5) Procura digital");
  switch (leOpcao()) {
    case '1':
      Serial.println("Apaga todas as digitais");
      if (!fpm.limpaDigitais()) {
        Serial.println ("Erro ao limpar digitais");
      }
      break;    
    case '2':
      Serial.println("Apaga uma digital");
      posicao = lePosicao();
      if (!fpm.apagaDigital(posicao)) {
        Serial.println ("Erro ao apagar a digital");
      }
      break;    
    case '3':
      Serial.println("Cadastra digital");
      cadastraDigital();
      posicao = lePosicao();
      if (fpm.salvaDigital(1, posicao)) {
        Serial.println ("Digital Salva");
      }
      break;    
    case '4':
      Serial.println("Compara digital");
      posicao = lePosicao();
      capturaFeature(1);
      if (!fpm.recuperaDigital(2, posicao)) {
        Serial.println ("Erro ao recuperar digital");
        break;
      }
      if (fpm.comparaDigital()) {
        Serial.println ("Digitais iguais");
      } else {
        Serial.println ("Digitais diferentes");
      }
      break;    
    case '5':
      Serial.println("Procura digital");
      capturaFeature(1);
      posicao = fpm.procuraDigital(1, 0, maxDigitais);
      if (posicao == -1) {
        Serial.println ("Digital nao encontrada");
      } else {
        Serial.print ("Identificou digital ");
        Serial.println (posicao);
      }
      break;    
  }
}

// Lê uma opção do menu
int leOpcao() {
  // Limpa a fila
  while (Serial.read() != -1) {
    delay(100);
  }

  // Insiste até ter uma opção válida
  while (true) {
    Serial.print ("Opcao? ");
    int c = -1;
    while (c == -1) {
      c = Serial.read();    
    }
    if ((c > '0') && (c < '6')) {
      Serial.println();
      return c;
    }
  }
}

// Lê uma posição (0 a maxDigitais-1)
int lePosicao() {
  int posicao;
  int c;

  while (Serial.read() != -1) {
    delay(100);
  }
  
  while (true) {
    posicao = 0;
    Serial.print ("Posicao? ");
    while (true) {
      c = Serial.read();
      if (c == 0x0D) {
        break;  // enter
      }
      if ((c >= '0') && (c <= '9') && (posicao < 100)) {
        Serial.write(c);
        posicao = (posicao*10 + c - '0');
      }
    }
    Serial.println();
    if (posicao < maxDigitais) {
      return posicao;
    }
    Serial.println ("Posicao invalida");    
  }
}

// Faz o cadastro de uma digital
void cadastraDigital() {
  while (true) {
    capturaFeature(1);
    capturaFeature(2);
    if (fpm.geraModelo()) {
      Serial.println ("Sucesso!");
      return; // Sucesso
    }
    Serial.println ("Imagens não combinam, vamos tentar de novo");
    Serial.println ("(Você usou o mesmo dedo as duas vezes?)");
  }
}

// Faz a captura de imagem e geração da feature
void capturaFeature(byte numBuf) {
  while (true) {
    // Captura a imagem
    Serial.println("Coloque o dedo");
    do {
      delay(50);
    } while (!fpm.capturaDigital());
    Serial.println("OK");

    // Tenta gerar as features
    bool ok = fpm.geraFeature(numBuf);

    // Pede para retirar o dedo
    Serial.println("Retire o dedo");
    do {
      delay(2000);
    } while (fpm.capturaDigital() || (fpm.ultimaResposta() != FPM10A::RESP_NOFINGER));

    // Verifica se precisa tentar novamente
    if (ok) {
      return;
    }
    Serial.println("Imagem ruim, vamos tentar novamente");
  }
}
