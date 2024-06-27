#ifndef SLIP_H
#define SLIP_H
#define BYTE unsigned char
#include "serial.h"
#include "stdio.h"
int writeSlip(BYTE* origen,BYTE * destino, int n);
int writeSlip(BYTE* destino,int n,FILE * vPort); // Envía un buffer de tamaño n
int readSlip(BYTE* destino,int n,FILE * vPort);// Intenta hacer una lectura retorna 0 si no hay datos
#endif