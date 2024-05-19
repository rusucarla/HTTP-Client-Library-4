#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

#define PORT_PRINCIPAL 8080
#define PRINCIPAL_SERVER_ADDR "54.170.241.232"
#define WEATHER_SERVER_ADDR "api.openweathermap.org"
#define PORT_WEATHER 80

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
        
    // Ex 1.1: GET dummy from main server
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(PRINCIPAL_SERVER_ADDR, "/api/v1/dummy", NULL, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);
    printf("\n");
    // Ex 1.2: POST dummy and print response from main server
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    char *text = "Buna ziua!";
    // adresa /api/v1/dummy de la serverul principal cu cu orice conținut pentru date de forma application/x-www-form-urlencoded.
    message = compute_post_request(PRINCIPAL_SERVER_ADDR, "/api/v1/dummy", "application/x-www-form-urlencoded", &text, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);
    printf("\n");
    // Ex 2: Login into main server
    // Implementați folosind instrucțiunile din îndrumarul de laborator o cerere de tip POST pentru adresa /api/v1/auth/login de pe serverul principal folosind username student si password student. Similar cu task-ul precedent datele trebuie să fie de forma application/x-www-form-urlencoded.
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    char *combined = "username=student&password=student";
    message = compute_post_request(PRINCIPAL_SERVER_ADDR, "/api/v1/auth/login", "application/x-www-form-urlencoded", &combined, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);
    // Ex 3: GET weather key from main server
    // Acum ca ne-am logat cu succes pe serverul principal, am primit un cookie inapoi in response. Cu acest cookie, facem o cerere de tip GET pentru adresa /api/v1/weather/key de pe serverul principal pentru a obtine cheia de acces catre serverul de weather.
    // extrag cookie-ul din response
    char *cookie = strstr(response, "Set-Cookie: ");
    cookie += strlen("Set-Cookie: ");
    char *cookie_end = strstr(cookie, ";");
    *cookie_end = '\0';
    // arat cookie-ul : connect.sid=s%3Aelq9QiJaECAXK8PI0LbgBT2Fqs5GGD6S.vG0Hg8MdZpHbjIlYIZFVqRqppQR%2F4FAKVta3MWV14lI
    printf("Cookie: %s\n", cookie);
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(PRINCIPAL_SERVER_ADDR, "/api/v1/weather/key", NULL, &cookie, 1);
    send_to_server(sockfd, message);
    // Acum ar trebui sa primim un mesaj de forma {"key":"weather_key"}.
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);
    // vreau sa extrag cheia din response
    char *key = strstr(response, "key");
    key += strlen("key\":\"");
    char *key_end = strstr(key, "\"");
    *key_end = '\0';
    printf("Key: %s\n", key);
    // Ex 4: GET weather data from OpenWeather API
    // Acum ca avem weather key ne putem conecta la serverul de weather. Facem o cerere de tip GET la serverul Openweather Map si ne alegem longitudinea si latitudinea Bucurestiului
    // mai intai trebuie sa aflu adresa ip a serverului de weather
    struct hostent *host = gethostbyname(WEATHER_SERVER_ADDR);
    char *weather_ip = inet_ntoa(*((struct in_addr *)host->h_addr_list[0]));
    sockfd = open_connection(weather_ip, PORT_WEATHER, AF_INET, SOCK_STREAM, 0);
    // Pentru a afla ce date de long si lat au ei pentru o anumita locatie fac acest apel
    // Exemplu de API call: http://api.openweathermap.org/geo/1.0/direct?q={city name},{state code},{country code}&limit={limit}&appid={API key}
    // vreau sa fac o cerere de tip GET la adresa /geo/1.0/direct?q=Bucharest&limit=1&appid=weather_key
    char *weather_query = "q=Bucharest&limit=1&appid=";
    char *combined_query = calloc(strlen(weather_query) + strlen(key) + 1, sizeof(char));
    strcpy(combined_query, weather_query);
    strcat(combined_query, key);
    message = compute_get_request(WEATHER_SERVER_ADDR, "/geo/1.0/direct", combined_query, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);
    // avem "lat":44.4361414,"lon":26.1027202, deci latitudinea si longitudinea sunt 44.4361414 si 26.1027202
    // Exemplu de API call: http://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&appid={API key}
    // vreau sa fac o cerere de tip GET la adresa /data/2.5/weather?lat=44.4361414&lon=26.1027202&appid=weather_key
    // 44.7398/22.2767 -> exemplul dat de ei 
    char *weather_query2 = "lat=44.7398&lon=22.2767&appid=";
    char *combined_query2 = calloc(strlen(weather_query2) + strlen(key) + 1, sizeof(char));
    strcpy(combined_query2, weather_query2);
    strcat(combined_query2, key);
    sockfd = open_connection(weather_ip, PORT_WEATHER, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(WEATHER_SERVER_ADDR, "/data/2.5/weather", combined_query2, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);
    // Ex 5: POST weather data for verification to main server
    // Acum cam am reusit sa extrag date de la serverul de weather, voi face o cere post la serverul principal, calea /api/v1/weather/{latitudine}/{longitudine} cu datele primite de la serverul de weather.

    // Pentru acest task, pentru identificarea mai facilă a începutului payload-ului, puteți să țineți cont de faptul că datele servite de serverul Openweather Map încep cu o acoladă ({), fiind un obiect în formatul JSON și nu vor apărea alte acolade în antetul răspunsului. Desigur, nu este obligatoriu să vă folosiți de asta, parsarea răspunsului HTTP fiind o alternativă cel puțin la fel de bună.

    // vreau sa extrag datele de la serverul de weather
    char *weather_data = basic_extract_json_response(response);
    printf("Weather data: %s\n", weather_data);
    printf("\n");
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    // Am pus aici exemplul din cerinta lor ca sa ma asigur ca merge, ca nu stiu ce date au ei
    message = compute_post_request(PRINCIPAL_SERVER_ADDR, "/api/v1/weather/44.7398/22.2767", "application/json", &weather_data, 1, &cookie, 1);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);

    // Ex 6: Logout from main server
    // Implementați folosind instrucțiunile din îndrumarul de laborator o cerere de tip GET pentru adresa /api/v1/auth/logout de pe serverul principal.
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(PRINCIPAL_SERVER_ADDR, "/api/v1/auth/logout", NULL, &cookie, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);

    // BONUS: make the main server return "Already logged in!"
    // Implementați mecanismul necesar de păstrare a cookies-urilor, astfel încât dacă un client deja autentificat încearcă să facă login, el să primească un mesaj de forma "Already logged in!". 
    
    // Pentru a testa asta, ne conectam la serverul principal si facem login
    // Primim un cookie cu care incercam sa facem login din nou

    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    message = compute_post_request(PRINCIPAL_SERVER_ADDR, "/api/v1/auth/login", "application/x-www-form-urlencoded", &combined, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);

    // Extragem cookie-ul din response
    char *cookie2 = strstr(response, "Set-Cookie: ");
    cookie2 += strlen("Set-Cookie: ");
    char *cookie_end2 = strstr(cookie2, ";");
    *cookie_end2 = '\0';
    // acum incerc sa fac login din nou
    sockfd = open_connection(PRINCIPAL_SERVER_ADDR, PORT_PRINCIPAL, AF_INET, SOCK_STREAM, 0);
    message = compute_post_request(PRINCIPAL_SERVER_ADDR, "/api/v1/auth/login", "application/x-www-form-urlencoded", &combined, 1, &cookie2, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("Response from server:\n%s\n", response);
    close_connection(sockfd);

    // Ar trebui sa primesc un mesaj de forma "Already logged in!" -> YES

    // free the allocated data at the end!
    free(message);
    free(response);
    return 0;
}
