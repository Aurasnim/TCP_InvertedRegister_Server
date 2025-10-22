#include <stdio.h>
#include "xparameters.h"
#include "netif/xadapter.h"
#include "platform.h"
#include "platform_config.h"
#include "lwip/tcp.h"
#include "xil_cache.h"
#include "console_app.h"

#if LWIP_IPV6==0
#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif
#endif

/* Глобальные переменные */
#if LWIP_IPV6==0
#if LWIP_DHCP==1
volatile int dhcp_timoutcntr = 24;
#endif
#endif

volatile int TcpFastTmrFlag = 0;
volatile int TcpSlowTmrFlag = 0;

static struct netif server_netif;
struct netif *custom_netif = NULL;

/* Прототипы функций */
void start_custom_application(void);

void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
    xil_printf("Board IP: %d.%d.%d.%d\n\r",
               ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
    xil_printf("Netmask : %d.%d.%d.%d\n\r",
               ip4_addr1(mask), ip4_addr2(mask), ip4_addr3(mask), ip4_addr4(mask));
    xil_printf("Gateway : %d.%d.%d.%d\n\r",
               ip4_addr1(gw), ip4_addr2(gw), ip4_addr3(gw), ip4_addr4(gw));
}

int main()
{
#if LWIP_IPV6==0
    ip_addr_t ipaddr, netmask, gw;
#endif
    unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 35, 0x00, 0x01, 0x02 };

    custom_netif = &server_netif;

    /* Инициализация платформы */
    init_platform();

#if LWIP_IPV6==0
    /* Статический IP */
    IP4_ADDR(&ipaddr,  192, 168,   1, 10);
    IP4_ADDR(&netmask, 255, 255, 255,  0);
    IP4_ADDR(&gw,      192, 168,   1,  1);
#endif

    /* Инициализация консоли */
    console_init();

    xil_printf("\r\n=== Запуск приложения ===\r\n");
    xil_printf("Инициализация сети...\r\n");

    /* Инициализация lwIP */
    lwip_init();

#if (LWIP_IPV6 == 0)
    /* Добавляем сетевой интерфейс */
    if (!xemac_add(custom_netif, &ipaddr, &netmask,
                    &gw, mac_ethernet_address,
                    PLATFORM_EMAC_BASEADDR)) {
        xil_printf("Ошибка добавления сетевого интерфейса\n\r");
        return -1;
    }
#endif

    netif_set_default(custom_netif);
    platform_enable_interrupts();
    netif_set_up(custom_netif);

    /* Показываем сетевые настройки */
    print_ip_settings(&ipaddr, &netmask, &gw);

    /* Запускаем TCP-сервер */
    start_custom_application();

    xil_printf("Приложение запущено успешно!\r\n");
    xil_printf("Ожидание подключений на порт 12345...\r\n");

    /* Главный цикл */
    while (1) {
        /* Таймеры lwIP */
        if (TcpFastTmrFlag) {
            tcp_fasttmr();
            TcpFastTmrFlag = 0;
        }
        if (TcpSlowTmrFlag) {
            tcp_slowtmr();
            TcpSlowTmrFlag = 0;
        }

        /* Обработка сетевых пакетов */
        xemacif_input(custom_netif);

        /* Демо-режим консоли */
        console_demo_mode();
    }

    cleanup_platform();
    return 0;
}
