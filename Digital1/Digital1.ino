/**
 * Primeiro teste de comunicação com o sensor de
 * Digitais FPM10
 * 
 * Daniel Quadros, out/20
 */

// Algumas constantes
const uint16_t START = 0xEF01;
const byte CMD_READSYSPARAM = 0x0F;

// Estrutura do cabeçalho dos pacotes
typedef struct {
  byte startHi;
  byte startLo;
  byte addr3;
  byte addr2;
  byte addr1;
  byte addr0;
  byte tipo;
  byte tamHi;
  byte tamLo;
} HEADER;
HEADER header;

// Estrutura dos comandos
typedef struct {
  byte cmd;
} READSYSPARAM;

// Estrutura da resposta
typedef struct {
  HEADER header;
  byte codigo;
  union {
    byte dados[256];
    struct {
      byte statusreg[2];
      byte verify[2];
      byte libsize[2];
      byte seclevel[2];
      byte addr[4];
      byte datasize[2];
      byte baud[2];
    } sysparam;
  } d;
} RESPOSTA;
RESPOSTA resp;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial1.begin(9600*6);
  while (!Serial1)
    ;
  delay (500);
  Serial.println("Pronto");
  READSYSPARAM cmd;
  cmd.cmd = CMD_READSYSPARAM;
  enviaCmd ((byte *) &cmd, sizeof(cmd));
  Serial.println("Resposta:");
  if (recebeResp()) {
    Serial.println("Sucesso");
    Serial.print("Memoria para ");
    uint16_t mem = (resp.d.sysparam.libsize[0] << 8) + resp.d.sysparam.libsize[1];
    Serial.print(mem);
    Serial.println(" digitais");
  } else {
    Serial.println("Falhou");
  }
}

void loop() {
}

// Envia um comando para o sensor
void enviaCmd (byte *cmd, int tam) {
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
  Serial1.write((byte *) &header, sizeof(header));

  // calcula o checksum
  uint16_t chksum = header.tipo + header.tamHi + header.tamLo;
  for (int i = 0; i < tam; i++) {
    chksum += cmd[i];
  }

  // envia o comando
  Serial1.write(cmd, tam);
  
  // envia o checksum
  Serial1.write(chksum >> 8);
  Serial1.write(chksum & 0xFF);
}

// Recebe uma resposta do sensor
bool recebeResp() {
  enum { START1, START2, ADDR, TAG, LEN1, LEN2, DADOS, CHECK1, CHECK2 } estado = START1;
  uint16_t checksum;
  byte *p = (byte *) &resp;
  int tam;
  int i;
  bool ok = false;
  while (true) {
    int c = Serial1.read();
    if (c == -1) {
      continue;
    }
    Serial.print (c, HEX);
    Serial.print (" ");
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
        *p++ = c;
        if (++i == tam) {
          estado = CHECK1;
        }
        break;
      case CHECK1:
        ok = c == (checksum >> 8);
        estado = CHECK2;
        break;
      case CHECK2:
        Serial.println ();
        return ok && (c == (checksum & 0xFF));
    }
  }
}
