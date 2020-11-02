/**
 * Construtor usando a Serial1
 */
FPM10A::FPM10A() {
  pSerial = &Serial1;  
}

/**
 * Construtor para usar outras seriais
 */
FPM10A::FPM10A(HardwareSerial *serial) {
  pSerial = serial;
}

/**
 * Inicia a comunicação
 */
bool FPM10A::begin() {
  pSerial->begin(9600*6);
  delay (500);
  resp.codigo = RESP_OK;
}

/**
 * Retorna o código da última resposta recebida
 */
byte FPM10A::ultimaResposta() {
  return resp.codigo;
}

/**
 * Le parâmetros do sistema
 */
bool FPM10A::leSysParam (SYSPARAM *param) {
  CMDSEMPARAM cmd;
  cmd.cmd = CMD_READSYSPARAM;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  if (recebeResp() && (resp.codigo == RESP_OK)) {
    memcpy (param, &resp.d.sysparam, sizeof(SYSPARAM));
    return true;
  } else {
    return false;
  }
}

// Determina quantas digitais tem armazenadas no sensor
// Retorna -1 se algo der errado
int FPM10A::leTemplateCount() {
  CMDSEMPARAM cmd;
  cmd.cmd = CMD_TEMPLATECOUNT;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  if (recebeResp() && (resp.codigo == RESP_OK)) {
    return (resp.d.templcount[0] << 8)+ resp.d.templcount[1];
  } else {
    return -1;
  }
}

/**
 * Captura a imagem de uma digital
 */
bool FPM10A::capturaDigital () {
  CMDSEMPARAM cmd;
  cmd.cmd = CMD_GENIMG;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  return recebeResp() && (resp.codigo == RESP_OK);
}

/**
 * Converte a imagem atual em um feature template e 
 * coloca em CharBuffer 'numBuf' (1 ou 2)
 */
bool FPM10A::geraFeature (byte numBuf) {
  CMDIMG2TZ cmd;
  cmd.cmd = CMD_IMG2TZ;
  cmd.bufno = numBuf;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  return recebeResp() && (resp.codigo == RESP_OK);
}

/**
 * Gera um modelo a partir dos dois feature templates
 */
bool FPM10A::geraModelo () {
  CMDSEMPARAM cmd;
  cmd.cmd = CMD_REGMODEL;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  return recebeResp() && (resp.codigo == RESP_OK);
}

/**
 * Salva o modelo que está em CharBuffer 'numBuf' (1 ou 2) na posição
 * 'posicao' (0 a 149) da memória do módulo
 */
bool FPM10A::salvaDigital (byte numBuf, int posicao) {
  CMDSTORE cmd;
  cmd.cmd = CMD_STORE;
  cmd.bufno = numBuf;
  cmd.posicao[0] = posicao >> 8;
  cmd.posicao[1] = posicao & 0xFF;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  return recebeResp() && (resp.codigo == RESP_OK);
}

/**
 * Apaga todas as digitais registradas no modulo
 */
bool FPM10A::limpaDigitais () {
  CMDSEMPARAM cmd;
  cmd.cmd = CMD_EMPTY;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  return recebeResp() && (resp.codigo == RESP_OK);
}

/**
 * Apaga uma digital armazenada
 */
bool FPM10A::apagaDigital (int posicao) {
  CMDDELETE cmd;
  cmd.cmd = CMD_DELETE;
  cmd.posicao[0] = posicao >> 8;
  cmd.posicao[1] = posicao & 0xFF;
  cmd.qtde[0] = 0;
  cmd.qtde[1] = 1;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  return recebeResp() && (resp.codigo == RESP_OK);
}


// Envia um comando para o sensor
void FPM10A::enviaCmd (byte *cmd, int tam) {
  // monta o cabeçalho e envia
  header.startHi = START >> 8;
  header.startLo = START & 0xFF;
  header.addr3 = 0xFF;  // endereço default
  header.addr2 = 0xFF;
  header.addr1 = 0xFF;
  header.addr0 = 0xFF;
  header.tipo = 0x01; // comando
  header.tamHi = (tam+2) >> 8;
  header.tamLo = (tam+2) & 0xFF;
  pSerial->write((byte *) &header, sizeof(header));

  // calcula o checksum
  uint16_t chksum = header.tipo + header.tamHi + header.tamLo;
  for (int i = 0; i < tam; i++) {
    chksum += cmd[i];
  }

  // envia o comando
  pSerial->write(cmd, tam);
  
  // envia o checksum
  pSerial->write(chksum >> 8);
  pSerial->write(chksum & 0xFF);
}

// Recebe uma resposta do sensor
bool FPM10A::recebeResp() {
  enum { START1, START2, ADDR, TAG, LEN1, LEN2, DADOS, CHECK1, CHECK2 } estado = START1;
  uint16_t checksum;
  byte *p = (byte *) &resp;
  int tam;
  int i;
  bool ok = false;
  while (true) {
    int c = pSerial->read();
    if (c == -1) {
      continue;
    }
    //Serial.print (c, HEX);
    //Serial.print (" ");
    *p++ = c;
    switch (estado) {
      case START1:
        if (c == (START >> 8)) {
          estado = START2;
        } else {
          p = (byte *) &resp; // ignora o byte lido
        }
        break;
      case START2:
        if (c == (START & 0xFF)) {
          estado = ADDR;
          i = 0;
        } else {
          estado = START1;
          p = (byte *) &resp; // ignora os bytes lidos
        }
        break;
      case ADDR:
        if (++i == 4) {
          estado = TAG;
        }
        break;
      case TAG:
        checksum = c;
        estado = LEN1;
        break;
      case LEN1:
        checksum += c;
        tam = c << 8;
        estado = LEN2;
        break;
      case LEN2:
        checksum += c;
        tam += c - 2;  // desconta o checksum
        estado = DADOS;
        i = 0;
        break;
      case DADOS:
        checksum += c;
        if (++i == tam) {
          estado = CHECK1;
        }
        break;
      case CHECK1:
        ok = c == (checksum >> 8);
        estado = CHECK2;
        break;
      case CHECK2:
        //Serial.println ();
        return ok && (c == (checksum & 0xFF));
    }
  }
}
