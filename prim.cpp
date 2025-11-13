#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>
#include <sstream>

bool isPrime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0) return false;
    return true;
}

int main() {
    const int NUM_PROCESSES = 10;
    const int INTERVAL_SIZE = 1000;
    int pipes[NUM_PROCESSES][2]; // un pipe pentru fiecare proces

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            // === CODUL COPILULUI ===
            close(pipes[i][0]); // copilul nu citește, doar scrie

            int start = i * INTERVAL_SIZE + 1;
            int end = (i + 1) * INTERVAL_SIZE;

            std::ostringstream primes;
            for (int n = start; n <= end; ++n) {
                if (isPrime(n)) primes << n << " ";
            }

            std::string result = primes.str();
            write(pipes[i][1], result.c_str(), result.size());
            close(pipes[i][1]);
            _exit(0);
        } 
        else {
            // === CODUL PĂRINTELUI ===
            close(pipes[i][1]); // părintele nu scrie, doar citește
        }
    }

    // === PĂRINTELE CITEȘTE DATELE ===
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        char buffer[4096];
        ssize_t bytesRead = read(pipes[i][0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::cout << "Proc " << i << ": " << buffer << "\n";
        }
        close(pipes[i][0]);
    }

    // Așteptăm toți copiii să se termine
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        wait(nullptr);
    }

    return 0;
}
