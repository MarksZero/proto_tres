#ifndef FUNCIONES_NODO_H
#define FUNCIONES_NODO_H
#include "slip.h"
#include <stdio.h>
#include "serial.h"
#include "string.h"

#define LEN 1500
#define BYTE unsigned char

#define HELP "-h"

struct frame_ipv4{
    BYTE flag_fragmento=0; //4bits  0 no fragmentado, 1 fragmentado, 2 ultimo fragmento
    int offset_fragmento=0; //12bits   si esta fragmentado
    int len_datos=0; //16bits 2bytes largo total del paquete
    BYTE TTL=0; //8bits 1byte
    BYTE identificacion=0; //8bits 1byte    si esta fragmentado
    int s_verificacion=0; //16bits 2bytes
    BYTE ip_origen[4]={0}; //32bits 4bytes
    BYTE ip_destino[4]={0}; //32 bits 4bytes
    BYTE DATA[LEN]={0}; //  memcpy(void*(org),void*(dst),len)
};

struct nodo{
    BYTE ip_origen[3]={0};
    BYTE ip_destino[3]={0};
    int TTL=0;
    nodo* siguiente = nullptr;
};



void menu();

void mensaje(char *nombreIP);

int fcs_IPV4(const BYTE* buff);
int largo_data(const frame_ipv4 *frame);
void empaquetar_IPV4(BYTE *buff, frame_ipv4* trama);
void desempaquetar_IPV4(BYTE *buff, frame_ipv4* recuperado);
void doom();
void agregarNodo(nodo** cabeza, BYTE ip_origen[3], BYTE ip_destino[3], int TTL);
nodo* buscarNodo(nodo* cabeza, BYTE ip_destino[3]);
void eliminarNodo(nodo** cabeza, BYTE ip_destino[3]);
void borrar_lista(nodo** cabeza);

#endif //FUNCIONES_NODO_H