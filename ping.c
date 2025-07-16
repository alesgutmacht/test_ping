#include "ping.h"

char address_ip[16];
char abuf[INET_ADDRSTRLEN];
int error_code = 0;
int req_soc;
int rec_soc;
struct sockaddr_in addr_soc;

int run_ping(char *addr_ip)
{
    // Если адрес не был передан параметром при запуске,
    // то он запашивается в цикле (пока не будет введен корректный) и применяется
    while (1)
    {
        if (set_address_ip(addr_ip) == 0)
        {
            break;
        }
        addr_ip = "";
    }

    // Сокет для получения ICMP ответов
    if ((rec_soc = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
    {
        perror("rec_soc");
        close_socs();
        return -1;
    }

    // Сокет для отправки ICMP запроса
    if ((req_soc = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("socket");
        close_socs();
        return -1;
    }

    // Формирование ICMP пакета
    char packet[64];
    struct icmphdr *icmp_req = (struct icmphdr *)packet;
    memset(packet, 0, sizeof(packet));
    icmp_req->type = ICMP_ECHO;
    icmp_req->code = 0;
    icmp_req->un.echo.id = getpid();
    icmp_req->un.echo.sequence = 0;
    icmp_req->checksum = get_checksum(icmp_req, sizeof(struct icmphdr));

    // Выполнение ping
    char addr_buf[256];
    struct sockaddr *addr = (struct sockaddr *)addr_buf;
    if (sendto(req_soc, packet, sizeof(packet), 0, (struct sockaddr *)&addr_soc, sizeof(addr_soc)) <= 0)
    {
        perror("sendto");
        close_socs();
        return -1;
    }

    // Получаем ответ
    unsigned char buf[ETH_FRAME_LEN];
    struct sockaddr_ll s_rec_addr;
    socklen_t s_rec_addr_len = sizeof(s_rec_addr);

    while (1)
    {
        // Получение сведений об отправителе ICMP ответа
        if (recvfrom(rec_soc, buf, sizeof(buf), 0, (struct sockaddr *)&s_rec_addr, &s_rec_addr_len) < 0)
        {
            perror("recvfrom");
            close_socs();
            return -1;
        }

        // Проверяем что это IP пакет
        if (ntohs(s_rec_addr.sll_protocol) == ETH_P_IP)
        {
            struct ethhdr *ehdr = (struct ethhdr *)buf;
            struct iphdr *ip = (struct iphdr *)(buf + sizeof(struct ethhdr));

            // Проверяем что это ICMP ответ
            if (ip->protocol == IPPROTO_ICMP)
            {
                struct icmphdr *icmp_rep = (struct icmphdr *)(buf + sizeof(struct ethhdr) + (ip->ihl << 2));

                // Проверяем что это ответ в своем процессе
                if (icmp_rep->type == ICMP_ECHOREPLY && icmp_rep->un.echo.id == getpid())
                {
                    // Получаем MAC адрес отправителя ICMP ответа
                    unsigned char mac_s[ETH_ALEN];
                    memcpy(mac_s, ehdr->h_source, ETH_ALEN);

                    printf("IP-адреc: %s\n", address_ip);
                    printf("mac-адрес: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_s[0], mac_s[1], mac_s[2], mac_s[3], mac_s[4], mac_s[5]);

                    close_socs();
                    return 0;
                }
            }
        }
    }

    close_socs();
    return 0;
}

// Контрольная сумма ICMP пакета
unsigned short get_checksum(void *hdr, int len)
{
    unsigned short *buf = hdr;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
    {
        sum += *buf++;
    }

    if (len == 1)
    {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

int set_address_ip(char *addr_ip)
{
    if (strlen(addr_ip) == 0)
    {
        printf("Введите IP-адрес\n");
        memset(address_ip, 0, sizeof(address_ip));
        scanf("%16s", address_ip);
        // Для отладки. Принудительно передается адрес
        //strncpy(address_ip, "192.168.1.1", strlen("192.168.1.1"));
    }
    else
    {
        memset(address_ip, 0, sizeof(address_ip));
        strncpy(address_ip, addr_ip, strlen(addr_ip));
    }

    memset(&addr_soc, 0, sizeof(addr_soc));
    if ((error_code = inet_pton(AF_INET, address_ip, &addr_soc.sin_addr)) <= 0)
    {
        printf("Ошибка в IP \"%s\" при преобразовании. Error code: %i.\nФормат адреса: 0.0.0.0\n", address_ip, error_code);
        return -1;
    }

    return 0;
}

void close_socs()
{
    shutdown(req_soc, SHUT_RDWR);
    shutdown(rec_soc, SHUT_RDWR);
    close(req_soc);
    close(rec_soc);
}
