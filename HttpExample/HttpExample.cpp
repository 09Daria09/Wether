#pragma comment (lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
using namespace std;

int main()
{

    //1. инициализация "Ws2_32.dll" для текущего процесса
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {

        cout << "WSAStartup failed with error: " << err << endl;
        return 1;
    }  

    //инициализация структуры, для указания ip адреса и порта сервера с которым мы хотим соединиться
   
    // сайт с которого хотим взять информацию
    char hostname[255] = "api.openweathermap.org"; 
    
    // сокет для выхода в сеть
    addrinfo* result = NULL;    
    
    addrinfo hints;
    // обнуляем поля 
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // 
    int iResult = getaddrinfo(hostname, "http", &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << endl;
        WSACleanup();
        return 3;
    }  
   
    // создается сокет 
    SOCKET connectSocket = INVALID_SOCKET;
    addrinfo* ptr = NULL;

    //Пробуем присоединиться к полученному адресу
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        //2. создание клиентского сокета
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

       //3. Соединяемся с сервером
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    //4. HTTP Request

    // формирование запроса
    // после знака ? начинается передача параметров на сайт 
    // 75 - код города 
    // f6e64d49db78658d09cb5ab201e483 - уникальный ключ 
    // прописываем формат в котором мы хотим получить данные
    string uri = "/data/2.5/weather?q=Odessa&appid=75f6e64d49db78658d09cb5ab201e483&mode=JSON";

    // Формируем полноценную строку 
    string request = "GET " + uri + " HTTP/1.1\n"; 
    request += "Host: " + string(hostname) + "\n";
    request += "Accept: */*\n";
    request += "Accept-Encoding: gzip, deflate, br\n";   
    request += "Connection: close\n";   
    request += "\n";

    //отправка сообщения
    if (send(connectSocket, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
        cout << "send failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 5;
    }
    cout << "send data" << endl;

    //5. HTTP Response
    // получение и обработка ответа 
    string response;

    const size_t BUFFERSIZE = 1024;
    char resBuf[BUFFERSIZE];

    int respLength;

    do {
        respLength = recv(connectSocket, resBuf, BUFFERSIZE, 0);
        if (respLength > 0) {
            response += string(resBuf).substr(0, respLength);    //   substr - от 0 позиции до respLength      
        }
        else {
            cout << "recv failed: " << WSAGetLastError() << endl;
            closesocket(connectSocket);
            WSACleanup();
            return 6;
        }

    } while (respLength == BUFFERSIZE);

    // получили огромную строку и показали без обработки 
    // ПОЛУЧИЛИ ДЛЯ XML файла
    // response - строка в которой записана информация.
    string data;
    int posS;

    // Страна 
    {
        data = "Country: ";
        posS = response.find("country");
        for (int i = posS + 10; response[i] != '"'; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\n";
    }
    // Город 
    {
        data += "City: ";
        posS = response.find("name");
        for (int i = posS + 7; response[i] != '"'; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\n";
    }
    // ID: 
    {
        data += "ID: ";
        posS = response.find("timezone");
        posS = response.find("id",posS); // начиная с  timezone
        for (int i = posS + 4; response[i] != ','; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\n";
    }
    // Координаты: X
    {
        data += "Coord: X: ";
        posS = response.find("lon");
        for (int i = posS + 5; response[i] != ','; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\t";
    }
    // Координаты: Y
    {
        data += "Y: ";
        posS = response.find("lat");
        //posE = response.find("weather");
        for (int i = posS + 5; response[i] != '}'; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\n";
    }

    char buf[5];
    string temp;
    double a;
    // Температура
    {
        data += "Temperature: ";
        posS = response.find("temp");
        for (int i = posS+6; response[i] != ','; i++)
            temp += response[i];                    // получаем строку
        a = atof(temp.c_str());      // преобразование типов
        a -= 273,15;                                                           // создаем буфер
        sprintf_s(buf, "%.2f", a); // преобразовываем в строку
        data += buf;                            // соединяем
        data += "\n";
    }
    // Температура максимум 
    {
        data += "Temperature MAX: ";
        posS = response.find("temp_max");
        for (int i = posS; response[i] != ','; i++)
            temp += response[i];                    // получаем строку
        a = atof(temp.c_str());      // преобразование типов
        a -= 273,15;                                                           // создаем буфер
        sprintf_s(buf, "%.2f", a); // преобразовываем в строку
        data += buf;                            // соединяем
        data += "\n";
    }
    // Температура минимум 
    {
        data += "Temperature MIN: ";
        posS = response.find("temp_min");
        for (int i = posS; response[i] != ','; i++)
            temp += response[i];                    // получаем строку
        a = atof(temp.c_str());      // преобразование типов
        a -= 273, 15;                                                           // создаем буфер
        sprintf_s(buf, "%.2f", a); // преобразовываем в строку
        data += buf;                            // соединяем
        data += "\n";
    }
    // Восход
    {
        data += "Sunrise: ";
        posS = response.find("sunrise");
        for (int i = posS + 9; response[i] != ','; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\n";
    }
    // Закат
    {
        data += "Sunset: ";
        posS = response.find("sunset");
        for (int i = posS + 8; response[i] != '}'; i++) // 9- кол-во символов в <country>
            data += response[i];
        data += "\n";
    }



    cout << data << endl;



    //отключает отправку и получение сообщений сокетом
    iResult = shutdown(connectSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR) {
        cout << "shutdown failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 7;
    }
    // закрываем сокет 
    closesocket(connectSocket);
    WSACleanup();
}
// страна 
// город
// температура по Цельсиям
// Минимальная, максимальная 