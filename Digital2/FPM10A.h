class FPM10A {
  public:

    static const byte RESP_OK = 0;        // Sucesso
    static const byte RESP_ERRCOM = 1;    // Erro na comunicaçãp
    static const byte RESP_NOFINGER = 2;  // Dedo ausente
    static const byte RESP_IMGFAIL = 3;   // Não conseguiu capturar imagem
    static const byte RESP_BADIMG = 6;    // Imagem ruim
    static const byte RESP_FEWFP = 7;     // Não achou features suficientes
    static const byte RESP_TOODIF = 10;   // Imagens muito diferentes
    static const byte RESP_NOIMG = 21;    // Não tem imagem

    typedef struct {
      byte statusreg[2];
      byte verify[2];
      byte libsize[2];
      byte seclevel[2];
      byte addr[4];
      byte datasize[2];
      byte baud[2];
    } SYSPARAM;
  
    FPM10A ();
    FPM10A (HardwareSerial *serial);
    bool begin ();
    byte ultimaResposta();
    bool leSysParam (SYSPARAM *param);
    int leTemplateCount ();
    bool capturaDigital ();
    bool geraFeature (byte numBuf);
    bool geraModelo ();
    bool salvaDigital (byte numBuf, int posicao);
    bool limpaDigitais ();
    bool apagaDigital (int posicao);
    
  private:
    // Ponteiro para a Serial usada na comunicação
    HardwareSerial *pSerial;
  
    // Algumas constantes
    static const uint16_t START = 0xEF01;
    static const byte CMD_GENIMG = 0x01;
    static const byte CMD_IMG2TZ = 0x02;
    static const byte CMD_REGMODEL = 0x05;
    static const byte CMD_STORE = 0x06;
    static const byte CMD_DELETE = 0x0C;
    static const byte CMD_EMPTY = 0x0D;
    static const byte CMD_READSYSPARAM = 0x0F;
    static const byte CMD_TEMPLATECOUNT = 0x1D;
    
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
    } CMDSEMPARAM;
    typedef struct {
      byte cmd;
      byte bufno;
    } CMDIMG2TZ;
    typedef struct {
      byte cmd;
      byte bufno;
      byte posicao[2];
    } CMDSTORE;
    typedef struct {
      byte cmd;
      byte posicao[2];
      byte qtde[2];
    } CMDDELETE;
    
    // Estrutura da resposta
    typedef struct {
      HEADER header;
      byte codigo;
      union {
        byte dados[256];
        byte templcount[2];
        SYSPARAM sysparam;
      } d;
    } RESPOSTA;
    RESPOSTA resp;

    void enviaCmd (byte *cmd, int tam);
    bool recebeResp ();
};
