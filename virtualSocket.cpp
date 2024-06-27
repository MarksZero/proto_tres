#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELP "-h"

int main(int argc, char* argv[]){
	if(argc==1){
        	system("socat -d -d pty,raw,echo=0 pty,raw,echo=0");
	}else if(argc==3){
		char aux[300];
		strcpy(aux,"socat -d -d pty,raw,echo=0,link=");
		strcat(aux,argv[1]);
		strcat(aux," pty,raw,echo=0,link=");
		strcat(aux,argv[2]);
		system(aux);
	}else if(argc==2 && strcmp(argv[1],HELP)==0){
		printf("MANUAL DE USUARIO:\n");
		printf("-------------------\n");
		printf(" Se recomienda crear una carpeta temporal 'tmp' para almacenar los enlaces a los puertos virtuales. [mkdir tmp]\n");
		printf("\t Modo de Uso:\n");
		printf("\t\t1-.\t./virtualSocket\n");
		printf("\t\t2-.\t./virtualSocket puerto1 puerto2\n\n");
		printf("\t Ejemplo:\n");
		printf("\t\t2-.\t./virtualSocket tmp/p1 tmp/p2\n");
	}else
		printf("Error de argumentos!\n\t Use: ./virtualSocket -h\n");
	return 0;
}