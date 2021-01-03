import tkinter
import serial
import threading

from tkinter import Canvas
from tkinter import Label
from tkinter import Button
from tkinter import Entry
from tkinter import StringVar

from PIL import Image
from PIL import ImageTk

####################################################################
# Gera imagem no formato bitmap
####################################################################
WIDTH = 256
HEIGHT = 288
READ_LEN = int(WIDTH * HEIGHT / 2)
DEPTH = 8
HEADER_SZ = 54

# Cria o cabeçalho do arquivo bmp
# Copiado de https://github.com/brianrho/FPM/blob/master/extras/GetImage/getImage.py
def assembleHeader(width, height, depth, cTable=False):
    header = bytearray(HEADER_SZ)
    header[0:2] = b'BM'   # assinatura
    byte_width = int((depth*width + 31) / 32) * 4
    if cTable:
        header[2:6] = ((byte_width * height) + (2**depth)*4 + HEADER_SZ).to_bytes(4, byteorder='little')  #file size
    else:
        header[2:6] = ((byte_width * height) + HEADER_SZ).to_bytes(4, byteorder='little')  #file size
    header[6:10] = (0).to_bytes(4, byteorder='little')
    if cTable:
        header[10:14] = ((2**depth) * 4 + HEADER_SZ).to_bytes(4, byteorder='little') #offset
    else:
        header[10:14] = (HEADER_SZ).to_bytes(4, byteorder='little') #offset

    header[14:18] = (40).to_bytes(4, byteorder='little')    #file header size
    header[18:22] = width.to_bytes(4, byteorder='little') #width
    header[22:26] = (-height).to_bytes(4, byteorder='little', signed=True) #height
    header[26:28] = (1).to_bytes(2, byteorder='little') #no of planes
    header[28:30] = depth.to_bytes(2, byteorder='little') #depth
    header[30:34] = (0).to_bytes(4, byteorder='little')
    header[34:38] = (byte_width * height).to_bytes(4, byteorder='little') #image size
    header[38:42] = (1).to_bytes(4, byteorder='little') #resolution
    header[42:46] = (1).to_bytes(4, byteorder='little')
    header[46:50] = (0).to_bytes(4, byteorder='little')
    header[50:54] = (0).to_bytes(4, byteorder='little')
    return header

# Grava o arquivo BMP com a imagem da digital
def gravaBmp(nome, dados):
    # cria o arquivo
    out = open(nome+'.bmp', 'wb')
    # grava o cabeçalho
    out.write(assembleHeader(WIDTH, HEIGHT, DEPTH, True))
    # grava a paleta de cores
    for i in range(256):
        out.write(i.to_bytes(1,byteorder='little') * 4)
    # grava os pontos
    for i in range(READ_LEN):
        # cada byte recebido tem 2 pontos
        out.write((dados[i] & 0xf0).to_bytes(1, byteorder='little'))
        out.write(((dados[i] & 0x0f) << 4).to_bytes(1, byteorder='little'))
    # fecha o arquivo
    out.close()  # close file


####################################################################
# Tratamento da captura de digital
####################################################################
def captura():
    mensagem.set('Capturando digital')
    ser.write(b'C')
    
####################################################################
# Tratamento da conferencia de digital
####################################################################
def confere():
    mensagem.set('Conferindo digital')
    ser.write(b'V')
    
####################################################################
# Tratamento da carga de digital
####################################################################
def carga():
    global dadosDigital
    global iPkt
    global nPkt
    mensagem.set('Carregando digital')
    arq = nome.get()
    if (arq is None) or (arq == ''):
        arq = 'teste'
    try:
        with open(arq+'.fpm', 'rb') as f:
            dadosDigital = f.read()
            nPkt = (len(dadosDigital)+127) // 128
            iPkt = 0
            ser.write(b'S')
    except IOError:
        mensagem.set('Erro ao ler o arquivo')
    
    
####################################################################
# Monta a tela
####################################################################
top = tkinter.Tk()
top.title('Sensor Digitais')

global imgDigital
canvDigital = Canvas(top, bg = "white", height = HEIGHT, width = WIDTH)
canvDigital.grid(column=0, row=0, columnspan=3)
imgDigital = canvDigital.create_image(WIDTH/2, HEIGHT/2, anchor="center")

lblMensagem = Label(top, text="Mensagem", padx=5, pady=5)
mensagem = StringVar()
lblMensagem['textvariable'] = mensagem
mensagem.set('mensagem')
lblMensagem.grid(column=0, row=1, columnspan=3)

lblNome = Label(top, text="Nome:")
lblNome.grid(column=0, row=2, sticky="E")
nome = Entry(top, width=24)
nome.grid(column=1, row=2, columnspan=2, padx=5, pady=5, sticky="W")

btnCaptura = Button(top, text="Captura", command=captura)
btnCaptura.grid(column=0, row=3, pady=5)
btnConfere = Button(top, text="Confere", command=confere)
btnConfere.grid(column=1, row=3)
btnCarrega= Button(top, text="Carrega", command=carga)
btnCarrega.grid(column=2, row=3)

####################################################################
# Tratamento dos bytes recebidos pela serial
####################################################################
TMODEL = 1536

def hex2byte(x):
    if (x >= 0x30) and (x <= 0x39):
        return x - 0x30
    elif (x >= 0x41) and (x <= 0x46):
        return x - 0x41 + 10
    elif (x >= 0x61) and (x <= 0x66):
        return x - 0x61 + 10
    else:
        return 0

def read_from_port(ser):
    global running
    global dadosDigital
    global iPkt
    global nPkt
    while running:
        x = ser.read()
        if x == b'I':
            mensagem.set('Recebendo imagem')
            dados = bytearray(READ_LEN)
            for i in range(READ_LEN):
                a = ord(ser.read())
                b = ord(ser.read())
                dados[i] = hex2byte(a)*16 + hex2byte(b)
            arq = nome.get()
            if (arq is None) or (arq == ''):
                arq = 'teste'
            gravaBmp(arq, dados)
            img = Image.open(arq+".bmp")
            img = img.resize((WIDTH,HEIGHT), Image.ANTIALIAS)
            img = ImageTk.PhotoImage(img)
            canvDigital.itemconfig(imgDigital, image = img)
        elif x == b'T':
            mensagem.set('Recebendo template')
            dados = bytearray(TMODEL)
            for i in range(TMODEL):
                a = ord(ser.read())
                b = ord(ser.read())
                dados[i] = hex2byte(a)*16 + hex2byte(b)
            arq = nome.get()
            if (arq is None) or (arq == ''):
                arq = 'teste'
            out = open(arq+'.fpm', 'wb')
            out.write(dados)
            out.close()
            mensagem.set('Captura encerrada')
        elif x == b'M':
            mensagem.set('Digital reconhecida')
        elif x == b'N':
            mensagem.set('Digital não reconhecida')
        elif x == b'P':
            # solicitou próximo pacote de dados do template
            # cada pacote contem até 128 bytes codificados em hexadecimal
            # o final de cada pacote é marcado por 'z' (se tem mais) ou 
            # 'Z' (se é o último)
            pos = 128*iPkt
            tam = len(dadosDigital) - pos
            if tam > 128:
                tam = 128
            ser.write(bytes(dadosDigital[pos:pos+tam].hex(),'ascii'))
            iPkt = iPkt + 1
            if iPkt == nPkt:
                ser.write(b'Z')
                mensagem.set('Fim da carga da digital')
            else:
                ser.write(b'z')

####################################################################
# Loop principal
####################################################################

ser = serial.Serial('COM3', 115200)
thread = threading.Thread(target=read_from_port, args=(ser,))
running = True
thread.start()

top.mainloop()
running = False
ser.close()

