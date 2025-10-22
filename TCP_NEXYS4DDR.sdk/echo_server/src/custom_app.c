#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "xil_printf.h"

/* ��������� ��� ���������� */
typedef struct {
    struct tcp_pcb *pcb;
    char buffer[1024];
    int data_length;
} conn_data_t;

/* ���������� ���������� */
int connection_count = 0;
int data_received_flag = 0;
char last_received_data[256] = "";

/* ������� �������������� ������� ���� */
typedef struct {
    unsigned char utf8_upper[3];  // ��������� ����� � UTF-8
    unsigned char utf8_lower[3];  // �������� ����� � UTF-8
    int size;                     // ������ � ������
} russian_letter_t;

/* ������� ������� ���� �-�, �-�, �, � */
static const russian_letter_t russian_letters[] = {
    /* ��������� �-� */
    {{0xD0, 0x90, 0}, {0xD0, 0xB0, 0}, 2}, // �
    {{0xD0, 0x91, 0}, {0xD0, 0xB1, 0}, 2}, // �
    {{0xD0, 0x92, 0}, {0xD0, 0xB2, 0}, 2}, // �
    {{0xD0, 0x93, 0}, {0xD0, 0xB3, 0}, 2}, // �
    {{0xD0, 0x94, 0}, {0xD0, 0xB4, 0}, 2}, // �
    {{0xD0, 0x95, 0}, {0xD0, 0xB5, 0}, 2}, // �
    {{0xD0, 0x96, 0}, {0xD0, 0xB6, 0}, 2}, // �
    {{0xD0, 0x97, 0}, {0xD0, 0xB7, 0}, 2}, // �
    {{0xD0, 0x98, 0}, {0xD0, 0xB8, 0}, 2}, // �
    {{0xD0, 0x99, 0}, {0xD0, 0xB9, 0}, 2}, // �
    {{0xD0, 0x9A, 0}, {0xD0, 0xBA, 0}, 2}, // �
    {{0xD0, 0x9B, 0}, {0xD0, 0xBB, 0}, 2}, // �
    {{0xD0, 0x9C, 0}, {0xD0, 0xBC, 0}, 2}, // �
    {{0xD0, 0x9D, 0}, {0xD0, 0xBD, 0}, 2}, // �
    {{0xD0, 0x9E, 0}, {0xD0, 0xBE, 0}, 2}, // �
    {{0xD0, 0x9F, 0}, {0xD0, 0xBF, 0}, 2}, // �
    {{0xD0, 0xA0, 0}, {0xD1, 0x80, 0}, 2}, // �
    {{0xD0, 0xA1, 0}, {0xD1, 0x81, 0}, 2}, // �
    {{0xD0, 0xA2, 0}, {0xD1, 0x82, 0}, 2}, // �
    {{0xD0, 0xA3, 0}, {0xD1, 0x83, 0}, 2}, // �
    {{0xD0, 0xA4, 0}, {0xD1, 0x84, 0}, 2}, // �
    {{0xD0, 0xA5, 0}, {0xD1, 0x85, 0}, 2}, // �
    {{0xD0, 0xA6, 0}, {0xD1, 0x86, 0}, 2}, // �
    {{0xD0, 0xA7, 0}, {0xD1, 0x87, 0}, 2}, // �
    {{0xD0, 0xA8, 0}, {0xD1, 0x88, 0}, 2}, // �
    {{0xD0, 0xA9, 0}, {0xD1, 0x89, 0}, 2}, // �
    {{0xD0, 0xAA, 0}, {0xD1, 0x8A, 0}, 2}, // �
    {{0xD0, 0xAB, 0}, {0xD1, 0x8B, 0}, 2}, // �
    {{0xD0, 0xAC, 0}, {0xD1, 0x8C, 0}, 2}, // �
    {{0xD0, 0xAD, 0}, {0xD1, 0x8D, 0}, 2}, // �
    {{0xD0, 0xAE, 0}, {0xD1, 0x8E, 0}, 2}, // �
    {{0xD0, 0xAF, 0}, {0xD1, 0x8F, 0}, 2}, // �

    /* ����� � (������ ������) */
    {{0xD0, 0x81, 0}, {0xD1, 0x91, 0}, 2}, // �/�

    /* ����� ������� */
    {{0, 0, 0}, {0, 0, 0}, 0}
};

/* ������� ��� �������� �������� ������� ���� */
int invert_russian_case(char *data, int pos, int length) {
    if (pos + 1 >= length) return 0;

    unsigned char byte1 = data[pos];
    unsigned char byte2 = data[pos + 1];

    // ���� � ������� ������� ����
    const russian_letter_t *letter = russian_letters;
    while (letter->size > 0) {
        // ��������� ��������� �����
        if (byte1 == letter->utf8_upper[0] && byte2 == letter->utf8_upper[1]) {
            data[pos] = letter->utf8_lower[0];
            data[pos + 1] = letter->utf8_lower[1];
            return 2;
        }
        // ��������� �������� �����
        else if (byte1 == letter->utf8_lower[0] && byte2 == letter->utf8_lower[1]) {
            data[pos] = letter->utf8_upper[0];
            data[pos + 1] = letter->utf8_upper[1];
            return 2;
        }
        letter++;
    }

    return 0;
}

/* �������� ������� ��������� ������ � ���������� UTF-8 */
void process_received_data(char *data, int length) {
    int i = 0;

    while (i < length && data[i] != '\0') {
        unsigned char c = data[i];

        // ������������ ASCII ������� (���������� �����)
        if ((c & 0x80) == 0) {
            if (c >= 'a' && c <= 'z') {
                data[i] = c - 'a' + 'A';  // �������� -> ���������
                i++;
            }
            else if (c >= 'A' && c <= 'Z') {
                data[i] = c - 'A' + 'a';  // ��������� -> ��������
                i++;
            }
            else {
                i++;  // ������ ASCII �������
            }
        }
        // ������������ 2-�������� UTF-8 ������� (������� �����)
        else if ((c & 0xE0) == 0xC0) {
            int processed = invert_russian_case(data, i, length);
            if (processed > 0) {
                i += processed;
            } else {
                i++;  // ���������� ����������� UTF-8 ������
            }
        }
        // ���������� ������ UTF-8 �������
        else {
            i++;
        }
    }
}

/* ���������� ������ ��� ������� */
void process_received_data_simple(char *data, int length) {
    int i;

    xil_printf("[���������] ������� ������: %s\r\n", data);

    // ������ ���������� ����� ��� ��������
    for (i = 0; i < length && data[i] != '\0'; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] = data[i] - 'a' + 'A';
        }
        else if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] = data[i] - 'A' + 'a';
        }
        // ������� ����� �� ������� � ���������� ������
    }

    xil_printf("[���������] �������� ������: %s\r\n", data);
}

/* Callback ��� ��������� ������ */
err_t data_received_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    conn_data_t *conn_data = (conn_data_t *)arg;

    if (!p) {
        /* ���������� ������� */
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        if (conn_data) {
            mem_free(conn_data);
        }
        connection_count--;
        xil_printf("[����] ���������� �������. ��������: %d\r\n", connection_count);
        return ERR_OK;
    }

    /* �������� ������ */
    conn_data->data_length = p->len;
    if (conn_data->data_length > sizeof(conn_data->buffer) - 1) {
        conn_data->data_length = sizeof(conn_data->buffer) - 1;
    }

    memcpy(conn_data->buffer, p->payload, conn_data->data_length);
    conn_data->buffer[conn_data->data_length] = '\0';

    /* ��������� ��� ������� */
    data_received_flag = 1;
    strncpy(last_received_data, conn_data->buffer, sizeof(last_received_data)-1);
    last_received_data[sizeof(last_received_data)-1] = '\0';

    xil_printf("[����] �������� %d ����: %s\r\n", conn_data->data_length, conn_data->buffer);

    /* ������������ ������ - ���������� ���������� ������ ��� ���������� */
    process_received_data_simple(conn_data->buffer, conn_data->data_length);

    /* ������� ��������� ������ � ��������� */
    char final_buffer[1024];
    int final_length = snprintf(final_buffer, sizeof(final_buffer), "PROCESSED: %s", conn_data->buffer);

    if (final_length >= sizeof(final_buffer)) {
        final_length = sizeof(final_buffer) - 1;
        final_buffer[final_length] = '\0';
    }

    /* ��������� ����� � ������ �������� */
    if (tcp_sndbuf(tpcb) < final_length) {
        xil_printf("[����] ���� ����� � ������ ��������: %d < %d\r\n", tcp_sndbuf(tpcb), final_length);
        pbuf_free(p);
        return ERR_MEM;
    }

    /* ���������� ������������ ������ */
    err_t write_err = tcp_write(tpcb, final_buffer, final_length, TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        xil_printf("[����] ������ ��������: %d\r\n", write_err);
        pbuf_free(p);
        return write_err;
    }

    /* ������������ ��������� */
    tcp_recved(tpcb, p->len);
    pbuf_free(p);

    xil_printf("[����] ���������� %d ����: %s\r\n", final_length, final_buffer);

    return ERR_OK;
}

/* Callback ��� ����� ���������� */
err_t connection_accepted_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    conn_data_t *conn_data;

    if (err != ERR_OK || newpcb == NULL) {
        xil_printf("[����] ������ �������� ����������: %d\r\n", err);
        return ERR_VAL;
    }

    /* �������� ������ */
    conn_data = (conn_data_t *)mem_malloc(sizeof(conn_data_t));
    if (conn_data == NULL) {
        xil_printf("[����] ������ ��������� ������\r\n");
        return ERR_MEM;
    }

    memset(conn_data, 0, sizeof(conn_data_t));
    conn_data->pcb = newpcb;
    conn_data->data_length = 0;

    tcp_recv(newpcb, data_received_callback);
    tcp_arg(newpcb, conn_data);

    connection_count++;
    xil_printf("[����] ����� ����������. ��������: %d\r\n", connection_count);

    return ERR_OK;
}

/* ������� ��� ������� */
void get_network_stats(int *connections, int *data_flag, char *last_data) {
    *connections = connection_count;
    *data_flag = data_received_flag;
    if (data_received_flag) {
        strncpy(last_data, last_received_data, sizeof(last_received_data)-1);
        last_data[sizeof(last_received_data)-1] = '\0';
        data_received_flag = 0;
    } else {
        last_data[0] = '\0';
    }
}

/* ������ TCP-������� */
void start_custom_application(void) {
    struct tcp_pcb *pcb;
    err_t err;

    /* ������� PCB */
    pcb = tcp_new();
    if (!pcb) {
        xil_printf("[����] ������ �������� PCB\r\n");
        return;
    }

    /* ������ �� ���� 12345 */
    err = tcp_bind(pcb, IP_ADDR_ANY, 12345);
    if (err != ERR_OK) {
        xil_printf("[����] ������ bind �� ���� 12345: %d\r\n", err);
        tcp_close(pcb);
        return;
    }

    /* ������� ���������� */
    pcb = tcp_listen(pcb);
    if (!pcb) {
        xil_printf("[����] ������ listen\r\n");
        return;
    }

    /* ������������� callback ��� �������� ���������� */
    tcp_accept(pcb, connection_accepted_callback);

    xil_printf("[����] TCP-������ ������� �� ����� 12345\r\n");
    xil_printf("[����] �����: ������ ���������� �����\r\n");
    xil_printf("[����] ����� � ������ �����������\r\n");
}
