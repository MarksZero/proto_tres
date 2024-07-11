#include <stdio.h>
#include "serial.h"
#include "slip.h"
#include "string.h"

#define LEN 1500
#define BYTE unsigned char
#define HELP "-h"
BYTE buffer[1516] = {0};

struct frame_ipv4 {
    BYTE flag_fragmento = 0; //4bits  0 no fragmentado, 1 fragmentado, 2 ultimo fragmento
    int offset_fragmento = 0; //12bits   si esta fragmentado
    int len_datos = 0; //16bits 2bytes largo total del paquete
    BYTE TTL = 0; //8bits 1byte
    bool es_ping = false;
    BYTE identificacion = 0; //8bits 1byte    si esta fragmentado
    int s_verificacion = 0; //16bits 2bytes
    BYTE ip_origen[4] = {0}; //32bits 4bytes
    BYTE ip_destino[4] = {0}; //32 bits 4bytes
    BYTE DATA[LEN] = {0}; //  memcpy(void*(org),void*(dst),len)
};

struct nodo {
    BYTE ip_origen[3] = {0};
    BYTE ip_destino[3] = {0};
    int TTL = 0;
    nodo *siguiente = nullptr;
};

void instrucciones(int op, char *nombreIP);

void doom();

int largo_data(frame_ipv4 *frame);

int fcs_IPV4(BYTE *buff);

void empaquetar_IPV4(BYTE *buff, frame_ipv4 *trama);

void desempaquetar_IPV4(BYTE *buff, frame_ipv4 *recuperado);

void agregarNodo(nodo **cabeza, BYTE ip_origen[3], BYTE ip_destino[3], int TTL);

nodo *buscarNodo(nodo *cabeza, BYTE ip_destino[3]);

void eliminarNodo(nodo **cabeza, BYTE ip_destino[3]);

void liberarLista(nodo **cabeza);

nodo *encontrarRutaMasCorta(nodo *cabeza);

void enviar_a_todos(nodo **tabla_ruta, const char *virtual_port_tx, const char *virtual_port_rx, BYTE ip_origen_final, BYTE ip_destino_final, BYTE TTL);

void ping_rutas(nodo **tabla_ruta, const char *virtual_port_tx, const char *virtual_port_rx, BYTE ip_origen_final);


bool flag = true;

int main(int nargs, char *arg_arr[]) {
    if (nargs == 4) {
        //./nodo "nombreIp" Emisor Receptor
        char msg[500];
        char msg_rx[1000];

        nodo *tabla_ruta = new nodo;

        frame_ipv4 frame;
        int len = 0;
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

        //Inicia la capa 1 y 2c
        //Obtiene descriptores
        int stdin_desc = fileno(stdin);
        //Obtiene descriptores

        //Indica inicio del chat
        int opcion;
        printf("opciones\n");
        printf("0.- Enviar mensaje\n");
        printf("1.-Instructivo\n");
        printf("2.- Mostrar rutas\n");
        printf("3.- Ping\n");
        printf("|------------------------------------|\n");
        sscanf(msg,"%d", &opcion);
        if (opcion == 0) {
            instrucciones(0, nombreIP);
        } else if (opcion == 1) {
            instrucciones(1, nombreIP);
        } else if (opcion == 2) {
            //mostrar rutas
        } else if (opcion == 3) {
            ping_rutas(&tabla_ruta, virtual_port_tx, virtual_port_rx, frame.ip_origen[3]);
        } else {
            printf("Error\n");
        }

        memset(msg, 0, sizeof(msg));
        memset(msg_rx, 0, sizeof(msg_rx));

        while (flag) {



            //Lee mensajes del puerto virtual y los muestra
            len = readSlip((BYTE *) msg_rx, 1000, vport_dos);
            msg_rx[len - 1] = '\0';
            if (len > 0) {
                //y si es 255 entonces broadcast
                frame_ipv4 recibido;
                desempaquetar_IPV4((BYTE *) msg_rx, &recibido);

                if (recibido.es_ping) {
                    if (recibido.ip_destino[3] == frame.ip_origen[3]) {
                        // Es un ping para este nodo, responder con el TTL actual
                        frame_ipv4 respuesta;
                        memcpy(respuesta.ip_origen, recibido.ip_destino, 4);
                        memcpy(respuesta.ip_destino, recibido.ip_origen, 4);
                        respuesta.TTL = recibido.TTL;
                        respuesta.es_ping = false;
                        strcpy((char*)respuesta.DATA, "PONG");
                        respuesta.len_datos = strlen((char*)respuesta.DATA) + 1;

                        empaquetar_IPV4(buffer, &respuesta);
                        writeSlip((BYTE *)buffer, (respuesta.len_datos + 16), vport_uno);
                    } else {
                        // Es un ping para otro nodo, reenviar decrementando el TTL
                        recibido.TTL--;
                        if (recibido.TTL > 0) {
                            empaquetar_IPV4(buffer, &recibido);
                            writeSlip((BYTE *)buffer, (recibido.len_datos + 16), vport_uno);
                        }
                    }
                }else {
                    if (strcmp((char *) recibido.DATA, "cerrar") == 0) {
                        flag = false;
                    }
                    if (recibido.ip_destino[3] == 255) {
                        if (frame.ip_origen[3] == recibido.ip_origen[3]) {
                            //si es el mismo nodo
                            continue;
                        }
                        printf("MENSAJE BROADCAST RECIBIDO\n");
                        printf("IP ORIGEN: %hhu.%hhu.%hhu.%hhu\n", recibido.ip_origen[0], recibido.ip_origen[1],
                               recibido.ip_origen[2], recibido.ip_origen[3]);
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
                        printf("IP ORIGEN: %hhu.%hhu.%hhu.%hhu\n", recibido.ip_origen[0], recibido.ip_origen[1],
                               recibido.ip_origen[2], recibido.ip_origen[3]);
                        printf("TTL: %hhu\n", (13 - recibido.TTL));
                        printf("MENSAJE: %s\n", recibido.DATA);
                        printf("-------------------------------------\n");
                    } else {
                        recibido.TTL--;
                        empaquetar_IPV4(buffer, &recibido);
                        writeSlip((BYTE *) buffer, (recibido.len_datos + 16), vport_uno);
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
                memset(msg_rx, 0, sizeof(msg_rx));
            }
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
            //Lee consola y envía el mensaje por el puerto virtual
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

//**********************************************************************************************************************

    } else if (nargs == 2 && strcmp(arg_arr[1],HELP) == 0) {
        printf("MANUAL DE USUARIO:\n");
        printf("-------------------\n");
        printf("\t Modo de Uso:\n");
        printf("\t\t1-.\t./nodo nombreIP emisor receptor\n");
        printf("\t Ejemplo:\n");
        printf("\t\t1-.\t./nodo 192.168.130.1 ../tmp/p1 ../tmp/p10\n");
    } else {
        //Se requiere un y solo un argumento; el puerto virtual
        printf("Debe indicar el puerto virtual y/o IP!\n");
        printf("Para mas ayuda escriba ./nodo -h\n");
    }
    return 0;
}


int fcs_IPV4(BYTE *buff) {
    int fcs_value = 0;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            fcs_value += (buff[i] >> j) & 0x01;
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

void empaquetar_IPV4(BYTE *buff, frame_ipv4 *trama) {
    int posicion = 0;
    buff[0] |= ((trama->flag_fragmento & 0x0F) << 4) | ((trama->offset_fragmento) & 0x0F);
    buff[1] |= ((trama->offset_fragmento >> 4) & 0xFF);
    posicion += 2;
    memcpy(buff + posicion, &trama->len_datos, 2);
    posicion += 2;
    buff[posicion++] = trama->TTL;
    buff[posicion++] = trama->identificacion;

    posicion += 2;
    memcpy(buff + posicion, trama->ip_origen, sizeof(trama->ip_origen));
    posicion += sizeof(trama->ip_origen);
    memcpy(buff + posicion, trama->ip_destino, sizeof(trama->ip_destino));
    posicion += sizeof(trama->ip_destino);
    memcpy(buff + posicion, trama->DATA, trama->len_datos);

    trama->s_verificacion = fcs_IPV4(buff);
    memcpy(buff + 6, &trama->s_verificacion, 2);
}

void desempaquetar_IPV4(BYTE *buff, frame_ipv4 *recuperado) {
    recuperado->flag_fragmento = (buff[0] >> 4) & 0x0F;
    recuperado->offset_fragmento = (buff[1] & 0xFF) << 4 | (buff[0] & 0x0F);
    int p = 2;
    memcpy(&recuperado->len_datos, buff + p, 2);
    p += 2;
    recuperado->TTL = buff[p++];
    recuperado->identificacion = buff[p++];
    memcpy(&recuperado->s_verificacion, buff + p, 2);
    p += 2;
    memcpy(recuperado->ip_origen, buff + p, sizeof(recuperado->ip_origen));
    p += sizeof(recuperado->ip_origen);
    memcpy(recuperado->ip_destino, buff + p, sizeof(recuperado->ip_destino));
    p += sizeof(recuperado->ip_destino);
    memcpy(recuperado->DATA, buff + p, recuperado->len_datos);
}

void doom() {
    FILE *archivo = fopen("banner.txt", "r");
    char buff[1000];
    while (fgets(buff, sizeof(buff), archivo)) {
        printf("%s", buff);
    }
    fclose(archivo);
}

// Agregar un Nodo al Final
void agregarNodo(nodo **cabeza, BYTE ip_origen[3], BYTE ip_destino[3], int TTL) {
    nodo *nuevoNodo = new nodo;
    memcpy(nuevoNodo->ip_origen, ip_origen, 3);
    memcpy(nuevoNodo->ip_destino, ip_destino, 3);
    nuevoNodo->TTL = TTL;
    nuevoNodo->siguiente = nullptr;

    if (*cabeza == nullptr) {
        *cabeza = nuevoNodo;
    } else {
        nodo *actual = *cabeza;
        while (actual->siguiente != nullptr) {
            actual = actual->siguiente;
        }
        actual->siguiente = nuevoNodo;
    }
}

// Buscar un Nodo
nodo *buscarNodo(nodo *cabeza, BYTE ip_destino[3]) {
    while (cabeza != nullptr) {
        if (memcmp(cabeza->ip_destino, ip_destino, 3) == 0) {
            return cabeza;
        }
        cabeza = cabeza->siguiente;
    }
    return nullptr;
}

// Eliminar un Nodo
void eliminarNodo(nodo **cabeza, BYTE ip_destino[3]) {
    nodo *actual = *cabeza;
    nodo *anterior = nullptr;

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
void liberarLista(nodo **cabeza) {
    nodo *actual = *cabeza;
    nodo *siguiente = nullptr;

    while (actual != nullptr) {
        siguiente = actual->siguiente;
        delete actual;
        actual = siguiente;
    }

    *cabeza = nullptr;
}

nodo *encontrarRutaMasCorta(nodo *cabeza) {
    if (cabeza == nullptr) return nullptr; // Lista vacía

    nodo *rutaMasCorta = cabeza;
    nodo *actual = cabeza->siguiente;

    while (actual != nullptr) {
        if (actual->TTL < rutaMasCorta->TTL) {
            rutaMasCorta = actual;
        }
        actual = actual->siguiente;
    }

    return rutaMasCorta;
}

void instrucciones(int op, char *nombreIP) {
    if (op == 0) {
        printf("chat\n");
        printf("Ya puede escribir sus mensajes !\n");
        printf("Mi IP: %s\n", nombreIP);
        printf("Seleccione IP de destino\n");
    } else if (op == 1) {
        printf("|---SOLO ESCRIBIR ULTIMO VALOR---|\n");
        printf("Recuerde 255 es broadcast\n");
        printf("Rango entre 1-254 usuario\n");
        printf("Ejemplo --> 255 / Puerto / MENSAJE.\n");
        printf("|----Para-obtener-las-rutas---> rutas**----|\n");
    }
}

void enviar_a_todos(nodo **tabla_ruta, const char *virtual_port_tx, const char *virtual_port_rx, BYTE ip_origen_final, BYTE ip_destino_final, BYTE TTL){
    frame_ipv4 frame;
    BYTE ip_base[3] = {192, 168, 130};
    memcpy(frame.ip_origen, ip_base, 3);
    frame.ip_origen[3] = ip_origen_final;
    memcpy(frame.ip_destino, ip_base, 3);
    frame.ip_destino[3] = ip_destino_final;
    frame.TTL = TTL;

    // Determinar el mensaje basado en el puerto virtual utilizado
    if (strcmp(virtual_port_tx, "vport_uno") == 0) {
        strcpy((char *) frame.DATA, "ruta_*_1");
    } else if (strcmp(virtual_port_rx, "vport_dos") == 0) {
        strcpy((char *) frame.DATA, "ruta_*_2");
    }

    frame.len_datos = largo_data(&frame);

    BYTE buffer[1516] = {0};
    empaquetar_IPV4(buffer, &frame);

    FILE *vport_uno = fopen(virtual_port_tx, "w+");
    FILE *vport_dos = fopen(virtual_port_rx, "w+");
    if (vport_uno != nullptr && vport_dos != nullptr) {
        writeSlip(buffer, (frame.len_datos + 16), vport_uno);
        writeSlip(buffer, (frame.len_datos + 16), vport_dos);
        fclose(vport_uno);
        fclose(vport_dos);
    } else {
        if (vport_uno != nullptr) fclose(vport_uno);
        if (vport_dos != nullptr) fclose(vport_dos);
    }

    agregarNodo(tabla_ruta, &frame.ip_origen[3], &frame.ip_destino[3], frame.TTL);
}
void ping_rutas(nodo **tabla_ruta, const char *virtual_port_tx, const char *virtual_port_rx, BYTE ip_origen_final) {
    BYTE ip_destino_final;
    printf("Ingrese el último octeto de la IP de destino (1-254): ");
    scanf("%hhu", &ip_destino_final);

    if (ip_destino_final == ip_origen_final || ip_destino_final < 1 || ip_destino_final > 254) {
        printf("IP de destino inválida.\n");
        return;
    }

    frame_ipv4 frame_tx, frame_rx;
    BYTE buffer[1516] = {0};
    char msg_rx[1000];
    int len;

    FILE *vport_uno = fopen(virtual_port_tx, "w+");
    FILE *vport_dos = fopen(virtual_port_rx, "w+");

    if (vport_uno == NULL || vport_dos == NULL) {
        printf("Error al abrir los puertos virtuales.\n");
        if (vport_uno) fclose(vport_uno);
        if (vport_dos) fclose(vport_dos);
        return;
    }

    // Preparar el frame de ping
    BYTE ip_base[3] = {192, 168, 130};
    memcpy(frame_tx.ip_origen, ip_base, 3);
    frame_tx.ip_origen[3] = ip_origen_final;
    memcpy(frame_tx.ip_destino, ip_base, 3);
    frame_tx.ip_destino[3] = ip_destino_final;
    frame_tx.TTL = 13;
    frame_tx.es_ping = true;
    strcpy((char*)frame_tx.DATA, "PING");
    frame_tx.len_datos = strlen((char*)frame_tx.DATA) + 1;

    // Enviar por vport_uno
    empaquetar_IPV4(buffer, &frame_tx);
    writeSlip(buffer, (frame_tx.len_datos + 16), vport_uno);

    // Esperar y leer la respuesta
    len = readSlip((BYTE *)msg_rx, 1000, vport_uno);
    int ttl_vport_uno = -1;
    if (len > 0) {
        desempaquetar_IPV4((BYTE *)msg_rx, &frame_rx);
        ttl_vport_uno = frame_rx.TTL;
    }

    // Enviar por vport_dos
    writeSlip(buffer, (frame_tx.len_datos + 16), vport_dos);

    // Esperar y leer la respuesta
    len = readSlip((BYTE *)msg_rx, 1000, vport_dos);
    int ttl_vport_dos = -1;
    if (len > 0) {
        desempaquetar_IPV4((BYTE *)msg_rx, &frame_rx);
        ttl_vport_dos = frame_rx.TTL;
    }

    fclose(vport_uno);
    fclose(vport_dos);

    // Comparar y mostrar resultados
    if (ttl_vport_uno > 0 && ttl_vport_dos > 0) {
        if (ttl_vport_uno >= ttl_vport_dos) {
            printf("La ruta más rápida a %d.%d.%d.%d es por vport_uno con TTL %d\n",
                   192, 168, 130, ip_destino_final, ttl_vport_uno);
            agregarNodo(tabla_ruta, &ip_origen_final, &ip_destino_final, ttl_vport_uno);
        } else {
            printf("La ruta más rápida a %d.%d.%d.%d es por vport_dos con TTL %d\n",
                   192, 168, 130, ip_destino_final, ttl_vport_dos);
            agregarNodo(tabla_ruta, &ip_origen_final, &ip_destino_final, ttl_vport_dos);
        }
    } else if (ttl_vport_uno > 0) {
        printf("Solo se recibió respuesta por vport_uno. TTL: %d\n", ttl_vport_uno);
        agregarNodo(tabla_ruta, &ip_origen_final, &ip_destino_final, ttl_vport_uno);
    } else if (ttl_vport_dos > 0) {
        printf("Solo se recibió respuesta por vport_dos. TTL: %d\n", ttl_vport_dos);
        agregarNodo(tabla_ruta, &ip_origen_final, &ip_destino_final, ttl_vport_dos);
    } else {
        printf("No se recibió respuesta por ningún puerto.\n");
    }
}