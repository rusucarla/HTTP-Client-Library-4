#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

#include <iostream>
#include <regex>

#include "json.hpp"
using json = nlohmann::json;

#define PORT 8080
#define SERVER_ADDR "34.246.184.49"
#define REGISTER_ROUTE "/api/v1/tema/auth/register"      // POST
#define LOGIN_ROUTE "/api/v1/tema/auth/login"            // POST
#define ACCESS_ROUTE "/api/v1/tema/library/access"       // GET
#define SEE_BOOKS_ROUTE "/api/v1/tema/library/books"     // GET
#define GET_BOOK_ROUTE "/api/v1/tema/library/books/"     // GET cu id-ul cartii
#define ADD_BOOK_ROUTE "/api/v1/tema/library/books"      // POST
#define DELETE_BOOK_ROUTE "/api/v1/tema/library/books/"  // DELETE cu id-ul cartii
#define LOGOUT_ROUTE "/api/v1/tema/auth/logout"          // GET

#define ERROR_MESSAGE "Operation FAILED or needs attention | "
#define SUCCESS_MESSAGE "Operation SUCCESS | "
#define OUTPUT_ASCII "< "

// dimensiunea buffer-ului pentru citirea datelor de la server este luata din laborator
#define BUFFER_SIZE 4096

// o functie care trimite un request HTTP catre server
// primeste toate datele necesare pentru a trimite un request (GET, POST,
// DELETE)
// functia primeste si raspunsul de la server si il returneaza
std::string send_http_request(const std::string& method, const std::string& host, const std::string& route, const std::string& payload = "", const std::string& cookie = "", const std::string& token = "") {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return "ERROR: Cannot open socket";
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
		close(sockfd);
		return "ERROR: Invalid address";
	}

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		close(sockfd);
		return "ERROR: Connection failed";
	}

	std::string request = method + " " + route + " HTTP/1.1\r\n";
	request += "Host: " + host + "\r\n";
	request += "Content-Type: application/json\r\n";
	if (!cookie.empty()) {
		request += "Cookie: " + cookie + "\r\n";
	}
	if (!token.empty()) {
		request += "Authorization: Bearer " + token + "\r\n";
	}
	if (!payload.empty()) {
		request += "Content-Length: " + std::to_string(payload.length()) + "\r\n";
	}
	request += "Connection: close\r\n\r\n";
	request += payload;

	if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
		close(sockfd);
		return "ERROR: Failed to send request";
	}

	char buffer[BUFFER_SIZE];
	std::string response;
	int received = 0;
	while ((received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
		buffer[received] = '\0';
		response += buffer;
	}

	close(sockfd);
	return response;
}

// functie wrapper pentru a trimite un request de tip POST
std::string send_post_request(const std::string& host, const std::string& route, const std::string& payload, const std::string& jwt_token = "") {
	return send_http_request("POST", host, route, payload, "", jwt_token);
}

// functie wrapper pentru a trimite un request de tip GET
std::string send_get_request(const std::string& host, const std::string& route, const std::string& session_cookie = "", const std::string& jwt_token = "") {
	return send_http_request("GET", host, route, "", session_cookie, jwt_token);
}

// functie wrapper pentru a trimite un request de tip DELETE
std::string send_delete_request(const std::string& server, const std::string& route, const std::string& session_cookie, const std::string& jwt_token) {
	return send_http_request("DELETE", server, route, "", session_cookie, jwt_token);
}

// vreau un mic struct cu status code si status message
struct Response {
	int status_code;
	std::string status_message;
};

// functie pentru afisarea mesajelor de raspuns intr-un mod mai frumos
void print_response_message(int status_code, const std::string& status_message, const std::string& additional_info_success = "", const std::string& additional_info_error = "", bool show_success = true) {
	// verific daca status code-ul este unul de succes
	if (status_code >= 200 && status_code < 300) {
		// pot sa am cazuri (afisarea cartilor) unde sa nu ma intereseze
		// afisajul de succes
		if (show_success) {
			std::cout << OUTPUT_ASCII << SUCCESS_MESSAGE << status_code << " " << status_message;
			if (!additional_info_success.empty()) {
				std::cout << " - " << additional_info_success;
			}
			std::cout << std::endl;
		}
	} else {
		std::cout << OUTPUT_ASCII << ERROR_MESSAGE << status_code << " " << status_message;
		if (!additional_info_error.empty()) {
			std::cout << " - " << additional_info_error;
		}
		std::cout << std::endl;
	}
}

struct Response parse_http_response(const std::string& response, std::string* good_to_know = nullptr) {
	// vreau un struct cu status code si status message
	struct Response response_struct;
	response_struct.status_code = 0;
	response_struct.status_message = "";
	// caut sfarsitul header-ului
	size_t header_end = response.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		return response_struct;
	}
	// extrag header-ul si body-ul
	std::string headers = response.substr(0, header_end);
	std::string body = response.substr(header_end + 4);  // dupa "\r\n\r\n"
	// defintesc regex-uri pentru status line si cookie
	std::regex status_line_regex(R"(HTTP\/1\.1 (\d{3}) (.+))");
	std::regex cookie_regex(R"(Set-Cookie: ([^;]+))");
	std::smatch matches;

	if (std::regex_search(headers, matches, status_line_regex) && matches.size() > 2) {
		std::string status_code = matches[1];
		std::string status_message = matches[2];

		response_struct.status_code = std::stoi(status_code);
		response_struct.status_message = status_message;

		// caut cookie-ul de sesiune
		std::smatch cookie_match;
		if (std::regex_search(headers, cookie_match, cookie_regex) && good_to_know) {
			*good_to_know = cookie_match[1].str();
		}
		// verific daca am primit un raspuns bun
		if (status_code[0] == '2') {
			try {
				if (!body.empty() && (body.front() == '{' || body.front() == '[')) {
					json j = json::parse(body);
					// verific tipul de date primit
					if (j.is_array() || j.is_object()) {
						// trebuie sa vad daca am primit un token sau nu
						if (j.contains("token") && good_to_know) {
							*good_to_know = j["token"].get<std::string>();
						} else {
							// in cazul in care am primit informatii despre
							// carti, le afisez
							std::cout << j.dump(2) << std::endl;
						}
					}
				}
			} catch (const json::parse_error& e) {
			}
		} else {
		}
	}
	return response_struct;
}
int main(int argc, char* argv[]) {
	std::string command, username, password;
	std::string session_cookie;
	std::string jwt_token;
	struct Response response;
	// bucla pentru primirea comenzilor de la tastatura
	while (true) {
		std::cin >> command;

		if (command == "exit") {
			// iesirea din bucla
			break;
		} else if (command == "register" || command == "login") {
			// daca sunt deja logat, nu pot sa ma loghez din nou
			if (!session_cookie.empty()) {
				std::cout << OUTPUT_ASCII << ERROR_MESSAGE << "Already logged in." << std::endl;
				continue;
			}
			std::cin.ignore();
			std::cout << "username=";
			std::getline(std::cin, username);
			std::cout << "password=";
			std::getline(std::cin, password);

			// verific validitatea datelor introduse (sa fie nevide si fara spatii)
			if (username.empty() || password.empty() || std::find_if(username.begin(), username.end(), ::isspace) != username.end() || std::find_if(password.begin(), password.end(), ::isspace) != password.end()) {
				std::cout << OUTPUT_ASCII << ERROR_MESSAGE << "Invalid input (no spaces/empty input)" << std::endl;
				continue;
			}

			// creez un json cu username-ul si parola
			json user;
			user["username"] = username;
			user["password"] = password;

			std::string message;
			std::string additional_info_success, additional_info_error;

			if (command == "register") {
				message = send_post_request(SERVER_ADDR, REGISTER_ROUTE, user.dump());
				response = parse_http_response(message);
				additional_info_error = "User already exists";
				additional_info_success = "Registered successfully";
			} else {
				message = send_post_request(SERVER_ADDR, LOGIN_ROUTE, user.dump());
				response = parse_http_response(message, &session_cookie);
				additional_info_error = "Invalid credentials";
				additional_info_success = "Logged in successfully";
			}
			print_response_message(response.status_code, response.status_message, additional_info_success, additional_info_error, true);
		} else if (command == "logout") {
			// daca nu sunt logat, nu pot sa ma deloghez
			if (session_cookie.empty()) {
				std::cout << ERROR_MESSAGE << "Not logged in." << std::endl;
			} else {
				std::string message = send_get_request(SERVER_ADDR, LOGOUT_ROUTE, session_cookie);
				response = parse_http_response(message, &session_cookie);
				// sterg cookie-ul de sesiune si token-ul JWT
				if (response.status_code >= 200 && response.status_code < 300) {
					session_cookie.clear();
					jwt_token.clear();
				}
				std::string additional_info_error = "Could not log out";
				std::string additional_info_success = "Logged out successfully";
				print_response_message(response.status_code, response.status_message, additional_info_success, additional_info_error, true);
			}
		} else if (command == "enter_library") {
			// daca nu sunt logat, nu pot sa intru in biblioteca
			if (session_cookie.empty()) {
				std::cout << ERROR_MESSAGE << "Not logged in." << std::endl;
			} else {
				std::string message = send_get_request(SERVER_ADDR, ACCESS_ROUTE, session_cookie);
				response = parse_http_response(message, &jwt_token);
				std::string additional_info_error = "Could not enter library";
				std::string additional_info_success = "Entered library successfully";
				print_response_message(response.status_code, response.status_message, additional_info_success, additional_info_error, true);
			}
		} else if (command == "get_books") {
			if (session_cookie.empty() && jwt_token.empty()) {
				std::cout << ERROR_MESSAGE << "Not logged in / Don't have token." << std::endl;
			} else {
				// trimit atat cookie-ul de sesiune cat si token-ul JWT
				std::string message = send_get_request(SERVER_ADDR, SEE_BOOKS_ROUTE, session_cookie, jwt_token);
				response = parse_http_response(message);
				// afisez in caz de eroare
				std::string additional_info_error = "Could not find books";
				print_response_message(response.status_code, response.status_message, "", additional_info_error, false);
			}
		} else if (command == "get_book") {
			// daca nu sunt logat, nu pot sa vad o carte
			if (session_cookie.empty() && jwt_token.empty()) {
				std::cout << ERROR_MESSAGE << "Not logged in / Don't have token." << std::endl;
			} else {
				std::string book_id;
				std::cout << "id=";
				std::cin >> book_id;

				// verific validitatea datelor introduse
				if (book_id.empty() || !std::all_of(book_id.begin(), book_id.end(), ::isdigit)) {
					std::cout << OUTPUT_ASCII << ERROR_MESSAGE << "Invalid input (book_id has to be a number/empty input)" << std::endl;
					continue;
				}
				std::string message = send_get_request(SERVER_ADDR, GET_BOOK_ROUTE + book_id, session_cookie, jwt_token);
				response = parse_http_response(message);
				// afisez in caz de eroare
				std::string additional_info_error = "Could not find book with id=" + book_id;
				print_response_message(response.status_code, response.status_message, "", additional_info_error, false);
			}
		} else if (command == "add_book") {
			// daca nu sunt logat, nu pot sa adaug o carte
			if (session_cookie.empty() && jwt_token.empty()) {
				std::cout << ERROR_MESSAGE << "Not logged in / Don't have token." << std::endl;
			} else {
				std::string title, author, genre, publisher, page_count;
				// se poata sa am spatii in numele cartii, autorului, genului si
				// editurii
				std::cin.ignore();  // ignor newline
				std::cout << "title=" << std::flush;
				std::getline(std::cin, title);

				std::cout << "author=" << std::flush;
				std::getline(std::cin, author);

				std::cout << "genre=" << std::flush;
				std::getline(std::cin, genre);

				std::cout << "publisher=" << std::flush;
				std::getline(std::cin, publisher);

				std::cout << "page_count=" << std::flush;
				std::getline(std::cin, page_count);

				// verific validitatea datelor introduse
				if (title.empty() || author.empty() || genre.empty() || publisher.empty() || page_count.empty() || !std::all_of(page_count.begin(), page_count.end(), ::isdigit)) {
					std::cout << OUTPUT_ASCII << ERROR_MESSAGE << "Invalid input (page_count has to be a number/empty input)" << std::endl;
					continue;
				}

				// construiesc un json cu datele cartii
				json book;
				book["title"] = title;
				book["author"] = author;
				book["genre"] = genre;
				book["publisher"] = publisher;
				book["page_count"] = page_count;

				std::string message = send_post_request(SERVER_ADDR, ADD_BOOK_ROUTE, book.dump(), jwt_token);
				response = parse_http_response(message);
				std::string additional_info_error = "Could not add book";
				std::string additional_info_success = "Added book successfully";
				print_response_message(response.status_code, response.status_message, additional_info_success, additional_info_error, true);
			}
		} else if (command == "delete_book") {
			// daca nu sunt logat, nu pot sa sterg o carte
			if (session_cookie.empty() && jwt_token.empty()) {
				std::cout << ERROR_MESSAGE << "Not logged in / Don't have token." << std::endl;
			} else {
				std::string book_id;
				std::cout << "id=";
				std::cin >> book_id;

				// verific validitatea datelor introduse
				if (book_id.empty() || !std::all_of(book_id.begin(), book_id.end(), ::isdigit)) {
					std::cout << OUTPUT_ASCII << ERROR_MESSAGE << "Invalid input (book_id has to be a number/empty input)" << std::endl;
					continue;
				}

				std::string message = send_delete_request(SERVER_ADDR, DELETE_BOOK_ROUTE + book_id, session_cookie, jwt_token);
				response = parse_http_response(message);
				std::string additional_info_error = "Could not delete book with id=" + book_id;
				std::string additional_info_success = "Deleted book successfully";
				print_response_message(response.status_code, response.status_message, additional_info_success, additional_info_error, true);
			}
		} else {
			// daca comanda nu este valida
			std::cout << "Invalid command" << std::endl;
		}
	}
}