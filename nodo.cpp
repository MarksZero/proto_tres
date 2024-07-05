#include "funciones_nodo.h"
#include <cstdio>
#include "serial.h"
#include "slip.h"
#include <cstring>

#define LEN 1500
#define BYTE unsigned char
#define HELP "-h"
BYTE buffer[1516] = {0};



bool flag = true;

int main(int nargs, char *arg_arr[]) {
    if (nargs == 4) {
        //./nodo "nombreIp" Emisor Receptor
        char msg[500];
        char msg_rx[1000];


        frame_ipv4 frame;
        int len;
        //Obtiene nombre IP
        char *nombreIP = arg_arr[1];
        sscanf(nombreIP, "%hhu.%hhu.%hhu.%hhu", &frame.ip_origen[0], &frame.ip_origen[1], &frame.ip_origen[2],
               &frame.ip_origen[3]);

        //Obtiene puerto virtual tx
        char *virtual_port_tx = arg_arr[2];
        //Obtiene puerto virtual rx
        char *virtual_port_rx = arg_arr[3];

        FILE *vport_uno = fopen(virtual_port_tx, "w+");
        FILE *vport_dos = fopen(virtual_port_rx, "w+");


        int stdin_desc = fileno(stdin);


        mensaje(nombreIP); //Indica inicio del chat
        memset(msg, 0, sizeof(msg));
        memset(msg_rx, 0, sizeof(msg_rx));


        while (flag) {

            len = readSlip((BYTE *) msg_rx, 1000, vport_dos);
            msg_rx[len - 1] = '\0';

            if (len > 0) {
                //y si es 255 entonces broadcast
                frame_ipv4 recibido;
                desempaquetar_IPV4((
                    BYTE *) msg_rx, &recibido);
                if (strcmp((char *) recibido.DATA, "cerrar") == 0) {
                    bool flag = false;
                }
                if (recibido.ip_destino[3] == 255) {
                    printf("MENSAJE BROADCAST RECIBIDO\n");
                    printf("IP ORIGEN: %hhu.%hhu.%hhu.%hhu\n", recibido.ip_origen[0], recibido.ip_origen[1], recibido.ip_origen[2],
                           recibido.ip_origen[3]);
                    printf("TTL: %hhu\n", (13 - recibido.TTL));
                    printf("MENSAJE: %s\n", recibido.DATA);
                    printf("-------------------------------------\n");
                    if (strcmp((char *) recibido.DATA, "DOOM") == 0) {
                        doom();
                    }
                    recibido.TTL--;
                    if (recibido.TTL > 0) {
                        empaquetar_IPV4(buffer, &recibido);
                        writeSlip((BYTE *) buffer, (recibido.len_datos + 16), vport_uno);
                        memset(buffer, 0, sizeof(buffer));
                    } else {
                        printf("ERROR TTL = 0\n");
                    }
                } else if (recibido.ip_destino[3] == frame.ip_origen[3] && recibido.ip_destino[3] != 255) {
                    printf("MENSAJE UNICAST RECIBIDO\n");
                    printf("IP ORIGEN: %hhu.%hhu.%hhu.%hhu\n", recibido.ip_origen[0], recibido.ip_origen[1], recibido.ip_origen[2],
                           recibido.ip_origen[3]);
                    printf("TTL: %hhu\n", (13 - recibido.TTL));
                    printf("MENSAJE: %s\n", recibido.DATA);
                    printf("-------------------------------------\n");
                } else {
                    recibido.TTL--;
                    empaquetar_IPV4(buffer, &recibido);
                    writeSlip((BYTE *) buffer, (recibido.len_datos + 16), vport_uno);
                    memset(buffer, 0, sizeof(buffer));
                }
                memset(msg_rx, 0, sizeof(msg_rx));
            }
//--------------------------------------------------------------------------------------------------------------

            len = readPort(stdin_desc, (BYTE *) msg, 500, 100);
            msg[len] = '\0';
            if (len > 0) {
                if (msg[len - 1] == '\n') {
                    msg[len - 1] = '\0';
                    len--;
                }
                int ruta;
                printf("escribio -> %s\n", msg);
                frame.flag_fragmento = 0;
                frame.offset_fragmento = 0;
                frame.ip_destino[0] = 192;
                frame.ip_destino[1] = 168;
                frame.ip_destino[2] = 130;
                sscanf(msg, "%hhu / %d / %499[^\n]", &frame.ip_destino[3], &ruta, frame.DATA);
                frame.len_datos = largo_data(&frame);
                frame.TTL = 12;
                frame.identificacion = 0;
                if (frame.ip_destino[3] == 255) {
                    printf("BROADCAST DETECTADO %d -", frame.ip_destino[3]);
                    empaquetar_IPV4(buffer, &frame);
                    writeSlip((BYTE *) buffer, (frame.len_datos + 16), vport_uno);
                    printf("ENVIANDO DATA --> %s\n", frame.DATA);
                } else if (frame.ip_destino[3] > 0 && frame.ip_destino[3] < 255) {
                    printf("UNICAST\n");
                    empaquetar_IPV4(buffer, &frame);
                    writeSlip((BYTE *) buffer, (frame.len_datos + 16), vport_uno);
                    printf("ENVIANDO DATA --> %s\n", frame.DATA);
                } else {
                    printf("Error\n");
                }
                memset(buffer, 0, sizeof(buffer));
                memset(msg, 0, sizeof(msg));
            }
        }


    }
    else if (nargs == 2 && strcmp(arg_arr[1],HELP) == 0) {
      menu();
    }

    else {
        printf("Debe indicar el puerto virtual y/o IP!\n");
        printf("Para mas ayuda escriba ./nodo -h\n");
    }
    return 0;
}

