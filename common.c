#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

void exibir_log_saida(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int analisar_endereco(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
    if(addrstr == NULL || portstr == NULL)
        return -1;
    uint16_t port = (uint16_t) atoi(portstr); // Unsigned short
    if(port == 0)
        return -1;
    port = htons(port); // Host para network short

    struct in_addr inaddr4; // Endereço IP de 32 bits
    if(inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }
    return -1;
}

void converter_endereco_em_string(const struct sockaddr *addr, char *str, size_t strsize) {
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;

    if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        exibir_log_saida("ntop");
    uint16_t port = ntohs(addr4->sin_port);

    if (str) {
        snprintf(str, strsize, "%s %hu", addrstr, port);
    }
}

int inicializar_sock_addr_server(const char *portstr, struct sockaddr_storage *storage) {
    
    uint16_t port = (uint16_t) atoi(portstr); // unsigned short
    if (port == 0)
        return -1;

    port = htons(port); // host para network short

    memset(storage, 0, sizeof(*storage));

    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_addr.s_addr = INADDR_ANY;
    addr4->sin_port = port;

    return 0;
}
