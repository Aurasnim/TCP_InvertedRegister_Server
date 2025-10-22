#include <stdio.h>
#include <string.h>
#include "xil_printf.h"
#include "xparameters.h"

#define BUFFER_SIZE 256

/* External functions from custom_app.c */
extern void get_network_stats(int *connections, int *data_flag, char *last_data);
extern void process_received_data(char *data, int length);

/* Command structure */
typedef struct {
    const char *name;
    const char *description;
    void (*function)(void);
} command_t;

/* Function prototypes */
void console_init(void);
void console_demo_mode(void);
void print_welcome_message(void);
void print_help(void);
void show_status(void);
void test_processing(void);
void show_network_info(void);
void show_test_examples(void);

/* Command table */
command_t commands[] = {
    {"help",    "Show this help", print_help},
    {"status",  "Show system status", show_status},
    {"test",    "Test data processing", test_processing},
    {"netinfo", "Show network info", show_network_info},
    {"examples","Test examples", show_test_examples},
    {NULL, NULL, NULL}
};

/* Global variables */
static int demo_counter = 0;
static int demo_state = 0;

/* Console initialization */
void console_init(void) {
    print_welcome_message();
}

/* Demo mode */
void console_demo_mode(void) {
    /* Show information every ~3 seconds */
    if (demo_counter++ > 3000000) {
        demo_counter = 0;

        switch(demo_state) {
            case 0:
                show_status();
                break;
            case 1:
                test_processing();
                break;
            case 2:
                show_network_info();
                break;
            case 3:
                show_test_examples();
                demo_state = -1; // Restart cycle
                break;
        }
        demo_state++;
    }
}

/* Welcome message */
void print_welcome_message(void) {
    xil_printf("\r\n\n");
    xil_printf("=============================================\r\n");
    xil_printf("          CONSOLE APPLICATION\r\n");
    xil_printf("          TCP SERVER + DATA PROCESSING\r\n");
    xil_printf("=============================================\r\n");
    xil_printf("\r\n");
    xil_printf("IP Address: 192.168.1.10\r\n");
    xil_printf("TCP Port: 12345\r\n");
    xil_printf("Function: Case inversion + prefix\r\n");
    xil_printf("\r\n");
    xil_printf("System started and ready!\r\n");
    xil_printf("\r\n");
    xil_printf("Auto demo mode active...\r\n");
    xil_printf("Info will be shown every 3 seconds\r\n");
}

/* Command: help */
void print_help(void) {
    int i;

    xil_printf("\r\nAVAILABLE COMMANDS:\r\n");
    for (i = 0; commands[i].name != NULL; i++) {
        xil_printf("  %-9s - %s\r\n", commands[i].name, commands[i].description);
    }

    xil_printf("\r\nFUNCTIONALITY:\r\n");
    xil_printf("- Letter case inversion\r\n");
    xil_printf("- Adds prefix 'PROCESSED:'\r\n");
    xil_printf("- TCP server on port 12345\r\n");
}

/* Command: status */
void show_status(void) {
    int connections, data_flag;
    char last_data[256];

    get_network_stats(&connections, &data_flag, last_data);

    xil_printf("\r\n=== SYSTEM STATUS ===\r\n");
    xil_printf("Active connections: %d\r\n", connections);
    xil_printf("TCP Port: 12345\r\n");
    xil_printf("IP Address: 192.168.1.10\r\n");

    if (data_flag && last_data[0] != '\0') {
        xil_printf("Last received data: %s\r\n", last_data);
    } else {
        xil_printf("Status: Waiting for data\r\n");
    }
    xil_printf("Mode: Demo\r\n");
}

/* Command: test processing */
void test_processing(void) {
    char test_samples[][32] = {
        "Hello World",
        "Test Data",
        "MiXeD CaSe",
        "123 Numbers",
        "ABCdefGHI"
    };

    xil_printf("\r\n=== DATA PROCESSING TEST ===\r\n");

    for (int i = 0; i < 5; i++) {
        char original[32];
        char processed[64];

        strcpy(original, test_samples[i]);

        /* Processing */
        char temp[32];
        strcpy(temp, original);
        process_received_data(temp, strlen(temp));
        snprintf(processed, sizeof(processed), "PROCESSED: %s", temp);

        xil_printf("%-15s -> %s\r\n", original, processed);
    }
}

/* Command: network info */
void show_network_info(void) {
    xil_printf("\r\n=== NETWORK INFORMATION ===\r\n");
    xil_printf("MAC Address: 00:0A:35:00:01:02\r\n");
    xil_printf("IP Address: 192.168.1.10\r\n");
    xil_printf("Subnet Mask: 255.255.255.0\r\n");
    xil_printf("Gateway: 192.168.1.1\r\n");
    xil_printf("Port: 12345\r\n");
    xil_printf("Protocol: TCP\r\n");
}

/* Command: test examples */
void show_test_examples(void) {
    xil_printf("\r\n=== TESTING EXAMPLES ===\r\n");
    xil_printf("Send to port 12345:\r\n");
    xil_printf("Input           -> Output\r\n");
    xil_printf("Hello           -> PROCESSED: hELLO\r\n");
    xil_printf("Test Data       -> PROCESSED: tEST dATA\r\n");
    xil_printf("MiXeD CaSe      -> PROCESSED: mIxEd cAsE\r\n");
    xil_printf("123 Numbers     -> PROCESSED: 123 nUMBERS\r\n");
    xil_printf("\r\n");
    xil_printf("Test commands (run on PC):\r\n");
    xil_printf("  telnet 192.168.1.10 12345\r\n");
    xil_printf("  echo 'Hello' | nc 192.168.1.10 12345\r\n");
    xil_printf("\r\n--- Demo cycle completed ---\r\n");
}
