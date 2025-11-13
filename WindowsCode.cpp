#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;


// Functie care verifica daca un numar e prim
bool estePrim(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0) return false;
    return true;
}


// Functie care calculeaza numerele prime dintr-un interval
string gasestePrime(int start, int end) {
    ostringstream rez;
    for (int i = start; i < end; ++i)
        if (estePrim(i))
            rez << i << " ";
    return rez.str();
}


// Punctul de intrare principal
int main(int argc, char* argv[])
{

    // PROCES COPIL
    if (argc == 3) {
        int start = stoi(argv[1]);
        int end = stoi(argv[2]);
        string rezultat = gasestePrime(start, end);
        DWORD bytesScrise;
        // Scriem rezultatul in stdout, care e redirectat in pipe
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), rezultat.c_str(),
            (DWORD)rezultat.size(), &bytesScrise, NULL);
        return 0;
    }

    
    // PROCESe PARINTE
    const int NR_PROC = 10;
    const int INTERVAL = 1000;

    HANDLE pipeRead[NR_PROC];
    PROCESS_INFORMATION pi[NR_PROC];
    STARTUPINFOA si[NR_PROC];

    for (int i = 0; i < NR_PROC; ++i) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE; // ca sa poata fi mostenit de copil
        sa.lpSecurityDescriptor = NULL;

        HANDLE hReadPipe = NULL;
        HANDLE hWritePipe = NULL;

        // Cream pipe-ul anonim
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            cerr << "Eroare CreatePipe.\n";
            return 1;
        }

        // Capatul de citire nu trebuie mostenit
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        pipeRead[i] = hReadPipe;

        int start = i * INTERVAL + 1;
        int end = (i + 1) * INTERVAL + 1;

        string cmd = "\"" + string(argv[0]) + "\" " +
            to_string(start) + " " + to_string(end);

        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        si[i].dwFlags = STARTF_USESTDHANDLES;
        si[i].hStdOutput = hWritePipe;  // copilul scrie aici
        si[i].hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si[i].hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        ZeroMemory(&pi[i], sizeof(pi[i]));

        // Cream procesul copil
        if (!CreateProcessA(
            NULL,
            (LPSTR)cmd.c_str(),
            NULL, NULL, TRUE,
            CREATE_NO_WINDOW, // fara consola suplimentara
            NULL, NULL,
            &si[i], &pi[i]
        )) {
            cerr << "Eroare CreateProcess la copilul " << i << endl;
            CloseHandle(hReadPipe);
            CloseHandle(hWritePipe);
            return 1;
        }

        // Inchidem capatul de scriere in parinte
        CloseHandle(hWritePipe);
    }

    
    // Citim rezultatele din fiecare pipe
    cout << "Numere prime pana la 10.000:\n\n";

    for (int i = 0; i < NR_PROC; ++i) {
        char buffer[4096];
        DWORD bytesRead;

        string outputCopil;

        while (ReadFile(pipeRead[i], buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            outputCopil += buffer;
        }

        cout << outputCopil;

        CloseHandle(pipeRead[i]);
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    cout << "\n\n--- GATA ---\n";
    return 0;
}
