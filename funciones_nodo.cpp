#include "funciones_nodo.h"
#include <stdio.h>
#include "string.h"




int fcs_IPV4(const BYTE *buff) {
    int fcs_value = 0;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            fcs_value += (buff[i] >> j) & 0x01;
        }
    }
    return fcs_value;
}

int largo_data(const frame_ipv4 *frame) {
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

// vacia la Lista
void borrar_lista(nodo **cabeza) {
    nodo *actual = *cabeza;
    nodo *siguiente = nullptr;

    while (actual != nullptr) {
        siguiente = actual->siguiente;
        delete actual;
        actual = siguiente;
    }

    *cabeza = nullptr;
}

void mensaje(char *nombreIP) {
    printf("chat\n");
    printf("Ya puede escribir sus mensajes !\n");
    printf("Mi IP: %s\n", nombreIP);
    printf("Seleccione IP de destino\n");
    printf("|---SOLO ESCRIBIR ULTIMO VALOR---|\n");
    printf("Recuerde 255 es broadcast\n");
    printf("Rango entre 1-254 usuario\n");
    printf("Ejemplo --> 255 / MENSAJE.\n");
}

void menu() {
    printf("MANUAL DE USUARIO:\n");
    printf("-------------------\n");
    printf("\t Modo de Uso:\n");
    printf("\t\t1-.\t./nodo nombreIP emisor receptor\n");
    printf("\t Ejemplo:\n");
    printf("\t\t1-.\t./nodo 192.168.130.1 ../tmp/p1 ../tmp/p10\n");
}

