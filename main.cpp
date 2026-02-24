#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shell32.lib")

std::string get_mime(const std::string& p) {
    if (p.find(".js") != std::string::npos) return "application/javascript";
    if (p.find(".css") != std::string::npos) return "text/css";
    if (p.find(".wasm") != std::string::npos) return "application/wasm";
    if (p.find(".png") != std::string::npos) return "image/png";
    if (p.find(".jpg") != std::string::npos || p.find(".jpeg") != std::string::npos) return "image/jpeg";
    if (p.find(".ogg") != std::string::npos) return "audio/ogg";
    if (p.find(".mp3") != std::string::npos) return "audio/mpeg";
    return "text/html";
}

bool is_edge_running(const std::string& profilePath) {
    std::string lock1 = profilePath + "\\lockfile";
    std::string lock2 = profilePath + "\\lock";
    auto check = [](const std::string& f) {
        HANDLE h = CreateFileA(f.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_SHARING_VIOLATION;
        CloseHandle(h);
        return false;
    };
    return check(lock1) || check(lock2);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    DWORD ftyp = GetFileAttributesA(".\\local");
    if (ftyp == INVALID_FILE_ATTRIBUTES || !(ftyp & FILE_ATTRIBUTE_DIRECTORY)) return 1;

    WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in a = { AF_INET, htons(0) };
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(s, (SOCKADDR*)&a, sizeof(a));
    int l = sizeof(a); getsockname(s, (struct sockaddr*)&a, &l);
    int port = ntohs(a.sin_port);
    listen(s, SOMAXCONN);

    char t[MAX_PATH]; GetTempPathA(MAX_PATH, t);
    std::string prof = std::string(t) + "lume_engine_" + std::to_string(GetCurrentProcessId());
    CreateDirectoryA(prof.c_str(), NULL);

    // INYECCIÓN DE PREFERENCIAS: Matar traductor y sync antes de arrancar
    std::string prefFolder = prof + "\\Default";
    CreateDirectoryA(prefFolder.c_str(), NULL);
    std::ofstream prefFile(prefFolder + "\\Preferences");
    if (prefFile.is_open()) {
        prefFile << "{\"translate\":{\"enabled\":false},\"profile\":{\"content_settings\":{\"exceptions\":{\"translation\":{}}}},\"sync\":{\"managed\":true}}";
        prefFile.close();
    }

    std::string url = "http://127.0.0.1:" + std::to_string(port);
    
    // FLAGS DE BLOQUEO: --no-signin y --disable-features=Translate son vitales
    std::string cmd = "\"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe\" "
                      "--app=" + url + " "
                      "--user-data-dir=\"" + prof + "\" "
                      "--no-first-run --no-signin --bwsi --incognito "
                      "--disable-features=Translate,msEdgeLanguageWebAPI,LanguageDetection "
                      "--disable-translate --disable-sync --no-default-browser-check "
                      "--remote-debugging-port=0";

    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return 1;

    ULONGLONG startTime = GetTickCount64();
    bool gracePeriod = true;

    while (true) {
        // Margen de 5 segundos para que Edge cree el LockFILE
        if (gracePeriod && (GetTickCount64() - startTime) > 5000) gracePeriod = false;
        if (!gracePeriod && !is_edge_running(prof)) break;

        fd_set f; FD_ZERO(&f); FD_SET(s, &f); timeval tv = { 0, 50000 };
        if (select(0, &f, NULL, NULL, &tv) > 0) {
            SOCKET c = accept(s, NULL, NULL);
            if (c != INVALID_SOCKET) {
                char b[2048] = {0}; 
                if (recv(c, b, sizeof(b), 0) > 0) {
                    std::string req(b);
                    size_t st = req.find("GET ") + 4;
                    size_t en = req.find(" ", st);
                    if (st != std::string::npos && en != std::string::npos) {
                        std::string path = req.substr(st, en - st);
                        if (path == "/") path = "/index.html";
                        std::string fullP = ".\\local" + path;
                        std::ifstream file(fullP, std::ios::binary);
                        if (file) {
                            std::vector<char> buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                            // Cabeceras anti-traducción y aislamiento
                            std::string h = "HTTP/1.1 200 OK\r\n"
                                           "Content-Type: " + get_mime(path) + "\r\n"
                                           "Content-Language: en\r\n" 
                                           "Content-Length: " + std::to_string(buf.size()) + "\r\n"
                                           "X-Content-Type-Options: nosniff\r\n"
                                           "Connection: close\r\n\r\n";
                            send(c, h.c_str(), (int)h.size(), 0);
                            send(c, buf.data(), (int)buf.size(), 0);
                        }
                    }
                }
                closesocket(c);
            }
        }
    }

    // Limpieza de carpeta temporal
    char d[MAX_PATH + 2] = { 0 }; memcpy(d, prof.c_str(), prof.size());
    SHFILEOPSTRUCTA fo = { 0, FO_DELETE, d, NULL, FOF_NOCONFIRMATION | FOF_SILENT, FALSE, NULL, NULL };
    SHFileOperationA(&fo);
    
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    closesocket(s); WSACleanup();
    return 0;
}
