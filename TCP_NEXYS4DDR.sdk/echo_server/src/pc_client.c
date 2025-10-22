#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <locale.h>

#pragma comment(lib, "ws2_32.lib")

#define BOARD_IP "192.168.1.10"
#define BOARD_PORT 12345
#define BUFFER_SIZE 1024

// Определяем цвета для Windows Console
#define COLOR_RED     FOREGROUND_RED | FOREGROUND_INTENSITY
#define COLOR_GREEN   FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define COLOR_BLUE    FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_YELLOW  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define COLOR_CYAN    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_WHITE   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY

// Функция для настройки кодировки консоли Windows
void setup_console_encoding() {
    // Устанавливаем локаль для поддержки UTF-8
    setlocale(LC_ALL, "ru_RU.UTF-8");

    // Для Windows устанавливаем кодировку консоли в UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
}

void print_colored(const char* text, WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;

    // Сохраняем текущие атрибуты
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

    // Устанавливаем новый цвет
    SetConsoleTextAttribute(hConsole, color);
    printf("%s", text);

    // Восстанавливаем оригинальные атрибуты
    SetConsoleTextAttribute(hConsole, saved_attributes);
}

void print_menu() {
    printf("\n");
    printf("=================================================\n");
    printf("        КЛИЕНТ ДЛЯ ПЛАТЫ NEXYS 4 DDR\n");
    printf("=================================================\n");
    printf("Команды:\n");
    printf("  <слово>    - Отправить слово на плату\n");
    printf("  test       - Тестовый набор слов\n");
    printf("  clear      - Очистить экран\n");
    printf("  help       - Показать это меню\n");
    printf("  quit       - Выход из программы\n");
    printf("=================================================\n");
}

int send_to_board(SOCKET sock, const char* word, char* response) {
    char buffer[BUFFER_SIZE];

    // Отправляем слово на плату
    sprintf(buffer, "%s\n", word);
    if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
        print_colored("Ошибка отправки данных!\n", COLOR_RED);
        return 0;
    }

    print_colored(">> Отправлено: ", COLOR_GREEN);
    printf("'%s'\n", word);

    // Получаем ответ от платы
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        // Убираем символы новой строки из ответа
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';

        strcpy(response, buffer);

        print_colored("<< Получено: ", COLOR_BLUE);
        printf("'%s'\n", response);
        return 1;
    }

    print_colored("Не удалось получить ответ от платы!\n", COLOR_RED);
    return 0;
}

void test_suite(SOCKET sock) {
    // Тестовый набор слов
    char* test_words[] = {
        "Hello",
        "World",
        "TEST",
        "MiXeD",
        "123Numbers",
        "Привет",
        "Тест",
        "Русский",
        NULL
    };

    printf("\n");
    print_colored("=== ЗАПУСК ТЕСТОВОГО НАБОРА ===\n", COLOR_CYAN);

    for (int i = 0; test_words[i] != NULL; i++) {
        char response[BUFFER_SIZE];
        printf("\nТест %d: ", i + 1);

        if (send_to_board(sock, test_words[i], response)) {
            print_colored("УСПЕХ", COLOR_GREEN);
            printf(" - '%s' -> '%s'", test_words[i], response);
        } else {
            print_colored("ОШИБКА", COLOR_RED);
            printf(" - '%s'", test_words[i]);
        }

        Sleep(300); // Пауза между запросами
    }

    printf("\n\n");
    print_colored("=== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ===\n", COLOR_CYAN);
}

void clear_screen() {
    system("cls");
}

void print_status(SOCKET sock, int is_connected) {
    printf("\n");
    print_colored("=== СТАТУС СИСТЕМЫ ===\n", COLOR_GREEN);

    if (is_connected) {
        print_colored("Подключение: ", COLOR_GREEN);
        printf("АКТИВНО\n");
        print_colored("Адрес платы: ", COLOR_GREEN);
        printf("%s:%d\n", BOARD_IP, BOARD_PORT);
    } else {
        print_colored("Подключение: ", COLOR_RED);
        printf("НЕТ\n");
    }

    print_colored("Протокол: ", COLOR_GREEN);
    printf("TCP\n");
    print_colored("Порт: ", COLOR_GREEN);
    printf("%d\n", BOARD_PORT);
}

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char input[256];
    char response[BUFFER_SIZE];
    int is_connected = 0;

    // Настройка кодировки консоли
    setup_console_encoding();

    printf("Клиент для платы Nexys 4 DDR\n");
    printf("Поддержка русского языка включена\n\n");

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        print_colored("Ошибка инициализации Winsock!\n", COLOR_RED);
        return 1;
    }

    // Создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        print_colored("Ошибка создания сокета!\n", COLOR_RED);
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(BOARD_PORT);
    server_addr.sin_addr.s_addr = inet_addr(BOARD_IP);

    // Подключение к плате
    print_colored("Подключение к плате ", COLOR_GREEN);
    printf("%s:%d...\n", BOARD_IP, BOARD_PORT);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        print_colored("Ошибка подключения к плате!\n", COLOR_RED);
        print_colored("Убедитесь, что:\n", COLOR_YELLOW);
        printf("  1. Плата включена и работает\n");
        printf("  2. IP-адрес правильный: %s\n", BOARD_IP);
        printf("  3. Порт правильный: %d\n", BOARD_PORT);
        printf("  4. Сетевой кабель подключен\n");

        closesocket(client_socket);
        WSACleanup();

        printf("\nНажмите Enter для выхода...");
        getchar();
        return 1;
    }

    is_connected = 1;
    print_colored("Успешно подключено к плате!\n\n", COLOR_GREEN);

    print_menu();

    // Главный цикл взаимодействия
    while (1) {
        printf("\n");
        print_colored("Введите слово или команду: ", COLOR_YELLOW);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Убираем символ новой строки
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) {
            continue;
        }

        // Обработка команд
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            print_colored("Завершение работы...\n", COLOR_YELLOW);
            break;
        }
        else if (strcmp(input, "test") == 0) {
            test_suite(client_socket);
        }
        else if (strcmp(input, "clear") == 0) {
            clear_screen();
            print_menu();
        }
        else if (strcmp(input, "help") == 0) {
            print_menu();
        }
        else if (strcmp(input, "status") == 0) {
            print_status(client_socket, is_connected);
        }
        else {
            // Отправка произвольного слова на плату
            if (send_to_board(client_socket, input, response)) {
                printf("\n");
                print_colored("РЕЗУЛЬТАТ: ", COLOR_CYAN);
                printf("'%s' -> '%s'\n", input, response);
            } else {
                print_colored("Не удалось отправить слово на плату\n", COLOR_RED);
            }
        }
    }

    // Завершение работы
    print_colored("\nЗакрытие соединения...\n", COLOR_YELLOW);
    closesocket(client_socket);
    WSACleanup();

    print_colored("Работа программы завершена.\n", COLOR_GREEN);
    printf("Нажмите Enter для выхода...");
    getchar();

    return 0;
}
