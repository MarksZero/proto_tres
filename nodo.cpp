#include <stdio.h>
#include "serial.h"
#include "slip.h"
#include "string.h"

#define LEN 1500
#define BYTE unsigned char
#define HELP "-h"
struct frame_ipv4
{
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

void doom();
int largo_data(frame_ipv4 *frame);
int fcs_IPV4(BYTE* buff);
void empaquetar_IPV4(BYTE *buff, frame_ipv4* trama);
void desempaquetar_IPV4(BYTE *buff, frame_ipv4* recuperado);

BYTE buffer[1516]={0};
bool flag = true;

int main(int nargs, char* arg_arr[]){
    if(nargs == 4){ //./nodo "nombreIp" Emisor Receptor
        char msg[500];
        char msg_rx[1000];

        nodo* tabla_ruta=new nodo;

        frame_ipv4 frame;
        int len = 0;
        //Obtiene nombre IP
        char* nombreIP = arg_arr[1];
        sscanf(nombreIP, "%hhu.%hhu.%hhu.%hhu", &frame.ip_origen[0], &frame.ip_origen[1], &frame.ip_origen[2], &frame.ip_origen[3]);

        //Obtiene puerto virtual tx
        char* virtual_port_tx = arg_arr[2];
        //Obtiene puerto virtual rx
        char* virtual_port_rx = arg_arr[3];

        FILE* vport_uno = fopen(virtual_port_tx, "w+");
        FILE* vport_dos = fopen(virtual_port_rx, "w+");

        //Inicia la capa 1 y 2
        //Obtiene descriptores
        int stdin_desc = fileno(stdin);
        //Obtiene descriptores
  
        //Indica inicio del chat
        printf("chat\n");
        printf("Ya puede escribir sus mensajes !\n");
        printf("Mi IP: %s\n", nombreIP);
        printf("Seleccione IP de destino\n");
        printf("|---SOLO ESCRIBIR ULTIMO VALOR---|\n");
        printf("Recuerde 255 es broadcast\n");
        printf("Rango entre 1-254 usuario\n");
        printf("Ejemplo --> 255 / MENSAJE.\n");
        memset(msg,0,sizeof(msg));
        memset(msg_rx,0,sizeof(msg_rx));
        while(flag){
            
            //Lee mensajes del puerto virtual y los muestra
            len = readSlip((BYTE*)msg_rx, 1000, vport_dos);
            msg_rx[len-1] = '\0';
            if(len>0){ //y si es 255 entonces broadcast
                frame_ipv4 recibido;
                desempaquetar_IPV4((BYTE*)msg_rx,&recibido);
                if(strcmp((char*)recibido.DATA, "cerrar") == 0){
                    flag=false;
                }
                if(recibido.ip_destino[3] == 255){
                    if(frame.ip_origen[3] == recibido.ip_origen[3]){
                        continue;
                    }
                    printf("MENSAJE BROADCAST RECIBIDO\n");
                    printf("IP ORIGEN: %hhu.%hhu.%hhu.%hhu\n", recibido.ip_origen[0], recibido.ip_origen[1], recibido.ip_origen[2], recibido.ip_origen[3]);
                    printf("TTL: %hhu\n", (13 - recibido.TTL));
                    printf("MENSAJE: %s\n", recibido.DATA);
                    printf("-------------------------------------\n");
                    if(strcmp((char*)recibido.DATA, "DOOM") == 0){
                    doom();
                    }
                    recibido.TTL--;
                    if (recibido.TTL > 0){
                        empaquetar_IPV4(buffer, &recibido);
                        writeSlip((BYTE*)buffer, (recibido.len_datos + 16), vport_uno);
                        memset(buffer,0,sizeof(buffer));
                    }
                    else{
                        printf("ERROR TTL = 0\n");
                    }
                }
                else if(recibido.ip_destino[3] == frame.ip_origen[3] && recibido.ip_destino[3] != 255){
                    printf("MENSAJE UNICAST RECIBIDO\n");
                    printf("IP ORIGEN: %hhu.%hhu.%hhu.%hhu\n", recibido.ip_origen[0], recibido.ip_origen[1], recibido.ip_origen[2], recibido.ip_origen[3]);
                    printf("TTL: %hhu\n", (13 - recibido.TTL));
                    printf("MENSAJE: %s\n", recibido.DATA);
                    printf("-------------------------------------\n");
                }
                else{
                    recibido.TTL--;
                    empaquetar_IPV4(buffer, &recibido);
                    writeSlip((BYTE*)buffer, (recibido.len_datos + 16), vport_uno);
                    memset(buffer,0,sizeof(buffer));
                }
                memset(msg_rx, 0, sizeof(msg_rx));
            }
            //Lee consola y envía el mensaje por el puerto virtual
            len = readPort(stdin_desc, (BYTE*)msg, 500, 100);
            msg[len] = '\0';
            if(len>0){
                if (msg[len - 1] == '\n') {
                    msg[len - 1] = '\0';
                    len--;
                }
                int ruta;
                printf("escribio -> %s\n",msg); 
                frame.flag_fragmento=0;
                frame.offset_fragmento=0;
                frame.ip_destino[0]=192;
                frame.ip_destino[1]=168;
                frame.ip_destino[2]=130;
                sscanf(msg, "%hhu / %d / %499[^\n]", &frame.ip_destino[3], &ruta ,frame.DATA);
                frame.len_datos=largo_data(&frame);
                frame.TTL=12;
                frame.identificacion=0;
                if(frame.ip_destino[3]==255){
                    printf("BROADCAST DETECTADO %d -", frame.ip_destino[3]);
                    empaquetar_IPV4(buffer, &frame);
                    writeSlip((BYTE*)buffer, (frame.len_datos+16), vport_uno);
                    printf("ENVIANDO DATA --> %s\n", frame.DATA);
                }else if(frame.ip_destino[3]>0 && frame.ip_destino[3]<255){
                    printf("UNICAST\n");
                    empaquetar_IPV4(buffer, &frame);
                    writeSlip((BYTE*)buffer, (frame.len_datos+16), vport_uno);
                    printf("ENVIANDO DATA --> %s\n", frame.DATA);
                }else{
                    printf("Error\n");
                }
                memset(buffer, 0, sizeof(buffer));
                memset(msg,0,sizeof(msg));
            }
        }
    }else if(nargs==2 && strcmp(arg_arr[1],HELP)==0){
        printf("MANUAL DE USUARIO:\n");
		printf("-------------------\n");
		printf("\t Modo de Uso:\n");
		printf("\t\t1-.\t./nodo nombreIP emisor receptor\n");
		printf("\t Ejemplo:\n");
		printf("\t\t1-.\t./nodo 192.168.130.1 ../tmp/p1 ../tmp/p10\n");
    }else{
        //Se requiere un y solo un argumento; el puerto virtual
        printf("Debe indicar el puerto virtual y/o IP!\n");
        printf("Para mas ayuda escriba ./nodo -h\n");
    }    
    return 0;
}


int fcs_IPV4(BYTE* buff){
    int fcs_value=0;
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            fcs_value += (buff[i]>>j)&0x01;
        }
    }
    return fcs_value;
}

int largo_data(frame_ipv4 *frame) {
    int i = 0;
    while (frame->DATA[i] != '\0') {
        i++;
    }
    return i;
}

void empaquetar_IPV4(BYTE *buff, frame_ipv4* trama){
    int posicion=0;
    buff[0] |= ((trama->flag_fragmento & 0x0F)<<4)|((trama->offset_fragmento)&0x0F);
    buff[1] |= ((trama->offset_fragmento>>4)&0xFF);
    posicion+=2;
    memcpy(buff+posicion , &trama->len_datos,2);
    posicion+=2;
    buff[posicion++]=trama->TTL;
    buff[posicion++]=trama->identificacion;
    
    posicion+=2;
    memcpy(buff+posicion, trama->ip_origen, sizeof(trama->ip_origen));
    posicion+=sizeof(trama->ip_origen);
    memcpy(buff+posicion, trama->ip_destino, sizeof(trama->ip_destino));
    posicion+=sizeof(trama->ip_destino);
    memcpy(buff+posicion, trama->DATA, trama->len_datos);

    trama->s_verificacion=fcs_IPV4(buff);
    memcpy(buff+6, &trama->s_verificacion, 2);

}

void desempaquetar_IPV4(BYTE *buff, frame_ipv4* recuperado){
    recuperado->flag_fragmento = (buff[0]>>4)&0x0F;
    recuperado->offset_fragmento = (buff[1]&0xFF)<<4 | (buff[0]&0x0F);
    int p=2;
    memcpy(&recuperado->len_datos, buff+p, 2);
    p+=2;
    recuperado->TTL = buff[p++];
    recuperado->identificacion = buff[p++];
    memcpy(&recuperado->s_verificacion, buff+p, 2);
    p+=2;
    memcpy(recuperado->ip_origen, buff+p, sizeof(recuperado->ip_origen));
    p+=sizeof(recuperado->ip_origen);
    memcpy(recuperado->ip_destino, buff+p, sizeof(recuperado->ip_destino));
    p+=sizeof(recuperado->ip_destino);
    memcpy(recuperado->DATA, buff+p, recuperado->len_datos);
}

void doom(){
    FILE* archivo = fopen("banner.txt", "r");
    char buff[1000];
    while (fgets(buff,sizeof(buff),archivo))
    {
        printf("%s", buff);
    }
    fclose(archivo);
}
// Agregar un Nodo al Final
void agregarNodo(nodo** cabeza, BYTE ip_origen[3], BYTE ip_destino[3], int TTL) {
    nodo* nuevoNodo = new nodo;
    memcpy(nuevoNodo->ip_origen, ip_origen, 3);
    memcpy(nuevoNodo->ip_destino, ip_destino, 3);
    nuevoNodo->TTL = TTL;
    nuevoNodo->siguiente = nullptr;

    if (*cabeza == nullptr) {
        *cabeza = nuevoNodo;
    } else {
        nodo* actual = *cabeza;
        while (actual->siguiente != nullptr) {
            actual = actual->siguiente;
        }
        actual->siguiente = nuevoNodo;
    }
}

// Buscar un Nodo
nodo* buscarNodo(nodo* cabeza, BYTE ip_destino[3]) {
    while (cabeza != nullptr) {
        if (memcmp(cabeza->ip_destino, ip_destino, 3) == 0) {
            return cabeza;
        }
        cabeza = cabeza->siguiente;
    }
    return nullptr;
}

// Eliminar un Nodo
void eliminarNodo(nodo** cabeza, BYTE ip_destino[3]) {
    nodo* actual = *cabeza;
    nodo* anterior = nullptr;

    while (actual != nullptr && memcmp(actual->ip_destino, ip_destino, 3) != 0) {
        anterior = actual;
        actual = actual->siguiente;
    }

    if (actual == nullptr) return; // No se encontró el nodo

    if (anterior == nullptr) {
        // Eliminar el primer nodo
        *cabeza = actual->siguiente;
    } else {
        anterior->siguiente = actual->siguiente;
    }

    delete actual;
}

// Liberar la Lista
void liberarLista(nodo** cabeza) {
    nodo* actual = *cabeza;
    nodo* siguiente = nullptr;

    while (actual != nullptr) {
        siguiente = actual->siguiente;
        delete actual;
        actual = siguiente;
    }

    *cabeza = nullptr;
}