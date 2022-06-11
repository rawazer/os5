#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define MAXHOSTNAME 256
#define MAXCLIENTS 5

int establish(unsigned short portnum) {
    char myname[MAXHOSTNAME+1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
    gethostname(myname, MAXHOSTNAME);
    hp = gethostbyname(myname);
    if (hp == NULL){
        fprintf(stderr, "System error: couldn't get host by name\n");
        exit(1);
    }
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_port = htons(portnum);
    if ((s=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "System error: failed to create socket\n");
        return -1;
    }

    if (bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "System error: couldn't bind\n");
        close(s);
        return -1;
    }

    listen(s, MAXCLIENTS);
    return s;
}

int get_connection(int s) {
    int t;

    if ((t = accept(s, NULL, NULL)) < 0) {
        return -1;
    }
    fprintf(stderr, "%d\n", t);
    return t;
}

int call_socket(char *hostname, unsigned short portnum) {
    struct sockaddr_in sa;
    struct hostent *hp;
    int s;

    if ((hp=gethostbyname(hostname)) == NULL) {
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    memcpy((char*)&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);

    if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        close(s);
        return -1;
    }

    return s;
}

int read_data(int s, char *buf, int n) {
    int bcount;
    int br;
    bcount = 0, br = 0;

    while (bcount < n) {
        br = read(s, buf, n-bcount);
        if (br > 0) {
            bcount += br;
            buf += br;
        }

        if (br < 1) {
            return -1;
        }
    }

    return bcount;
}

int write_data(int s, char *buf, int n) {
    int bcount;
    int br;
    bcount = 0, br = 0;

    while (bcount < n) {
        br = write(s, buf, n-bcount);
        if (br > 0) {
            bcount += br;
            buf += br;
        }

        if (br < 1) {
            return -1;
        }
    }

    return bcount;
}

int main (int argc, char* argv[]) {
    if (strcmp(argv[1], "client") == 0) {
        int s = call_socket("localhost", atoi(argv[2]));
        if (write_data(s, argv[3], strlen(argv[3])) == -1) {
            fprintf(stderr, "System error: writing command failed\n");
            exit(1);
        }
    } else if (strcmp(argv[1], "server") == 0) {
        int s = establish(atoi(argv[2]));
        fprintf(stderr, "established %d\n", s);
        while(1) {
            int t = get_connection(s);
            fprintf(stderr, "connected %d\n", t);
            char buf[MAXHOSTNAME] = {0};
            read_data(t, buf, MAXHOSTNAME);
            if(system(buf) != 0) {
                fprintf(stderr, "System error: executing command failed\n");
                exit(1);
            }
        }
    }
}