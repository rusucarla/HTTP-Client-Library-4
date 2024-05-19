# README TEMA 4

## Implementare

Baza acestei teme a fost laboratorul 9, unde am implementat un client in C care
transmitea date textuale catre un server. Am folosit acel client ca baza pentru
a-l extinde (cu ajutorul functionalitatilor din C++) si a-l face capabil sa
transmita payload-uri de tip JSON.

### Cereri HTTP

Am avut de implementat 3 tipuri de cereri HTTP: GET, POST si DELETE.

Pentru a avea o abordare mai generala, am creat o functie `send_http_request`
care primeste ca parametrii :

- tipul cererii (GET, POST, DELETE)
- host-ul (adresa IP a serverului)
- route-ul (cele mentionate in enunt)
- payload-ul (un JSON transformat in string)
- cookie-ul
- token-ul JWT

In aceasta functie, nu numai ca se trimite cererea, dar se si primeste raspunsul
de la server si se returneaza acest raspuns.

Ceva important de mentionat este ca socket-ul este inchis dupa fiecare cerere si
nu se foloseste mecanismul de keep-alive (nu am vrut sa imi complic existenta).

Pentru ca am vrut sa extrag intr-un mod "eficient" informatiile din raspuns, am
creat functia `parse_http_response` care folosind regex-uri, extrage
informatiile necesare (status code, status message, body, cookie, token JWT).

Pe langa informatiile extrase, aceasta afiseaza si JSON-urile primite de la
server si le afiseaza in format frumos (sau cel putin, verificabil de catre
checker).

Functia intoarce un obiect de tipul `Response` care contine informatiile
preliminare despre raspunsul serverului : status code si status message.

Acesta este folosit pentru a afisa niste mesaje de eroare sau de succes dar care
sa contina si informatii pertinente in cazul unei erori. (functia
`print_response_message`)

### Comenzile

In bucla principala a programului, se citeste comanda de la tastatura si se fac
anumite verificari pentru a vedea ce actiune trebuie sa se intample.

In principiu, se respecta enuntul si se trimit cererile corespunzatoare in
functie de comanda primita.

Se verifica pentru toate comenzile valide daca user-ul este logat sau nu sau
daca incearca sa se logheze de 2 ori, prin verificarea cookie-ului si a
token-ului JWT (in cazurile in care e vorba de manipularea bibliotecii).

Pentru a gestiona spatiile din input-urile de la `add_book` si de la
`login`/`register` am folosit citirea prin `std::getline` si
`std::cin.ignore()`, fata de uzualul `std::cin`.

### JSON

Pentru ca am lucrat in C++, am ales sa folosesc biblioteca nlohmann/json pentru
a manipula fisierele JSON, cunoscuta si sub numele de "JSON for Modern C++".

A fost esentiala pentru a putea comunica cu serverul avand in vedere ca fiecare
request era de tipul "application/json".

Sumar, a fost foarte usor de folosit si a facut lucrurile mult mai simple.

## Concluzii

In concluzie, am invatat cum sa fac un client care sa comunice cu un server prin
intermediul HTTP cu REST API. Am avut cateva probleme la inceput (cu modul in
care verifica checker-ul anumite input-uri), dar le-am rezolvat destul de
repede. O tema interesanta si care m-a facut sa invat lucruri noi.
