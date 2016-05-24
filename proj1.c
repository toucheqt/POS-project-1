/* ========================================================================== */
/*                                                                            */
/*   proj01.c                                                                 */
/*   (c) 2016 Bc. Ondrej Krpec, xkrpec01@stud.fit.vutbr.cz                    */
/*                                                                            */
/*   Prvni projekt do predmetu Pokrocile operacni systemy na fakulte FIT VUT  */
/*   v Brne. Projekt po spusteni vytvori proces otce a syna, kteri cyklicky   */
/*   vypisuji znaky abecedy.                                                  */
/*                                                                            */
/* ========================================================================== */

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1 /* XPG 4.2 - needed for usleep() */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


volatile int intr;
int ch;

/**
 * Obsluzna funkce, pokud je zachycen signal SIGUSR2 je hodnota promenne ch nastavena na zacatek abecedy.
 * @param int sig Id signalu
 */
void catcher(int sig);


int main() {

    ch = (int)'A';

    struct sigaction sigact;
    sigset_t setint;
    
    // inicializiace signalu
    sigemptyset(&setint);
    sigaddset(&setint, SIGUSR1);
    sigaddset(&setint, SIGUSR2);
    sigprocmask(SIG_BLOCK, &setint, NULL);
    
    sigact.sa_handler = catcher;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
     
    if (sigaction(SIGUSR1, &sigact, NULL) == -1) {
        perror("Error: In sigaction(), receiving SIGUSR1");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGUSR2, &sigact, NULL) == -1) {
        perror("Error: In sigaction(), receiving SIGUSR2");
        return EXIT_FAILURE;
    }

    sigprocmask(SIG_UNBLOCK, &setint, NULL);
    
    // vytvoreni synovskeho procesu
    pid_t pid = fork();
    
    if (pid > 0) { // proces rodice
      
        bool init = true;        
        sigemptyset(&setint);
        
        while(true) { // koncime az s prichodem signalu sigkill
            while (init || ((sigsuspend(&setint) == -1) && errno == EINTR)) {
                if (intr || init) {
                    intr = 0;  
                    break;
                }
            }

            if (!init) {
                sigprocmask(SIG_UNBLOCK, &setint, NULL);
                printf("Press enter...");
                while ((getchar()) != '\n') {}
                sigprocmask(SIG_BLOCK, &setint, NULL);
            } else {
                init = false;
            }
            
            printf("Parent (%d): '%c'\n", getpid(), (char) ch);
            
            if (ch == 'Z') {
                ch = (int) 'A';
            } else {
                ch++;
            }
                      
            // posli signal potomku
            if (kill(pid, SIGUSR1) == -1) {
                perror("Error: In kill(pid, SIGUSR1), Error in sending signal to child process.");
                return EXIT_FAILURE;
            } 
        } // end while(true)
    } else if (pid < 0) { // vytvoreni potomka se nezdarilo
        perror("Error: In fork(), couldnt create child's process");
        return EXIT_FAILURE;
    } else { // proces potomka
        sigemptyset(&setint);
        while (true) {
            while ((sigsuspend(&setint) == -1) && errno == EINTR) {
                if (intr) {
                    intr = 0;
                    break;
                }
            }
            
            sigprocmask(SIG_BLOCK, &setint, NULL);
            printf("Child (%d): '%c'\n", getpid(), (char) ch);
            
            if (ch == 'Z') {
                ch = (int) 'A';
            } else {
                ch++;
            }
            
            if (kill(getppid(), SIGUSR1) == -1) {
                perror("Error: In kill(pid, SIGUSR1), Error in sending signal to parent process.");
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;

}

void catcher(int sig) {
    if (sig == SIGUSR2) {
        ch = (int)'A';    
    } else {
        intr = 1;
    }
}



