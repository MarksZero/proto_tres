#include "slip.h"

#define END 0xC0 // 192
#define ESC 0xDB // 219
#define DC 0xDC  // 220
#define DD 0xDD  // 221

int writeSlip(BYTE* origen, int n,FILE * vPort){
	int descVport = fileno(vPort); // Se obtiene el descriptor de archivo del flujo
	BYTE buffer_aux[2];
	buffer_aux[0] = END;
	writePort(descVport,buffer_aux,1);
	int indx_dest = 1;
	for (int i = 0; i < n; i++)
	{
		if (origen[i] == END)
		{
			buffer_aux[0] = ESC;
			buffer_aux[1] = DC;
			writePort(descVport,buffer_aux,2);
			indx_dest += 2;
		}
		else if (origen[i]==ESC)
		{
			buffer_aux[0] = ESC;
			buffer_aux[1] = DD;
			writePort(descVport,buffer_aux,2);
			indx_dest += 2;
		}
		else
		{
			writePort(descVport,&origen[i],1);
			indx_dest ++;
		}
		
	}
	buffer_aux[0] = END;
	writePort(descVport,buffer_aux,1);
	fflush(vPort);
	return indx_dest;
}

int readSlip(BYTE* destino,int n,FILE * vPort){
	int descVport = fileno(vPort); // Se obtiene el descriptor de archivo del flujo
	int indx_dst = 0;
	BYTE aux_buff[1];
	int n_buff =0;
	do
	{
		// Se hace la lectura del puerto virtual
		n_buff = readPort(descVport,aux_buff,1,100);
		if (n_buff==0)// Si no hay datos se termina la comunicación slip
			return 0;
	} while (aux_buff[0]!=END);
	do
	{
		n_buff = readPort(descVport,aux_buff,1,100);
		if (n_buff==0)
			return 0;
		if (aux_buff[0] == ESC){
			n_buff = readPort(descVport,aux_buff,1,100);
			if (n_buff==0)
				return 0;
			if (aux_buff[0] == DC){
				destino[indx_dst++] = END;
			}
			else if (aux_buff[0] == DD)
			{
				destino[indx_dst++] = ESC;
			}
		}
		else
		{
			destino[indx_dst++] =aux_buff[0];
		}
		if (indx_dst == n){
			return indx_dst;
		}
	} while (aux_buff[0]!=END);
		return indx_dst;
}
