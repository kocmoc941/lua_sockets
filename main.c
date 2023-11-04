#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __linux__
    #include <sys/socket.h>
    #define ioctl ioctl
#elif _WIN32
    #include <winsock2.h>
    #define ioctl ioctlsocket
#endif

#define port 32000
#define address "127.0.0.1"

struct SPacket {
    int16_t cmd;
    int16_t len;
    char data[0];
};

enum ConTypes {
    NONE,
    UDP,
    TCP,
    UNDEFINED,
};

struct Sthreadctx {
    char run;
    int socket;
    struct sockaddr_in *addr;
    enum ConTypes type;
    uint8_t blocked;
    uint8_t is_client;
};


int8_t create_server(struct Sthreadctx *ctx)
{
    uint8_t t = 0;
    uint8_t p = 0;
    switch(ctx->type) {
        case UDP: {
            t = SOCK_DGRAM;
			p = IPPROTO_UDP;
            break;
        }

        case TCP: {
            t = SOCK_STREAM;
			p = IPPROTO_TCP;
            break;
        }

        default: {
            return -1;
        }
    }
    ctx->socket = socket(AF_INET, t, p);
    if(ctx->socket == INVALID_SOCKET) {
        perror("error create socket");
        exit(-1);
    }

    u_long mode = !ctx->blocked;
    ioctl(ctx->socket, FIONBIO, &mode);

    static struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int res = bind(ctx->socket, (struct sockaddr *)&addr, sizeof(addr));
    if(res) {
        perror("bind error");
        exit(-1);
    }
    ctx->addr = &addr;

    if(t == TCP) {
        res = listen(ctx->socket, 10);
    }

    return res;
}

int8_t create_client(struct Sthreadctx *ctx)
{
    uint8_t t = 0;
    uint8_t p = 0;
    switch(ctx->type) {
        case UDP: {
            t = SOCK_DGRAM;
			p = IPPROTO_UDP;
            break;
        }

        case TCP: {
            t = SOCK_STREAM;
			p = IPPROTO_TCP;
            break;
        }

        default: {
            return -1;
        }
    }
    ctx->socket = socket(AF_INET, t, p);
    if(ctx->socket == INVALID_SOCKET) {
        perror("error create socket");
        exit(-1);
    }

    u_long mode = !ctx->blocked;
    ioctl(ctx->socket, FIONBIO, &mode);

    static struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address);
    int res = bind(ctx->socket, (struct sockaddr *)&addr, sizeof(addr));
    if(res) {
        perror("bind error");
        exit(-1);
    }
    ctx->addr = &addr;

    if(t == TCP) {
        res = connect(ctx->socket, (struct sockaddr *)ctx->addr, sizeof(*ctx->addr));
        if(res) {
            perror("fail connect");
            exit(-1);
        }
    }

    return res;
}


void *sv_thread(void *arg)
{
    struct Sthreadctx *sth = (struct Sthreadctx *)arg;
    while(sth->run) {
        struct sockaddr_in claddr;
        int cllen = sizeof(claddr);
        int scl = accept(sth->socket, (struct sockaddr *)&claddr, &cllen);
    }
}

void *cl_thread(void *arg)
{
    struct Sthreadctx *sth = (struct Sthreadctx *)arg;
    while(sth->run) {
        struct sockaddr_in claddr;
        int cllen = sizeof(claddr);
        int scl = accept(sth->socket, (struct sockaddr *)&claddr, &cllen);
    }
}

void handle_clients(struct Sthreadctx *ctx)
{
    pthread_t thid;
    if(!ctx->is_client) {
        pthread_create(&thid, NULL, &sv_thread, &ctx);
        puts("server started...");
    } else {
        pthread_create(&thid, NULL, &cl_thread, &ctx);
        puts("client started...");
    }
    pthread_join(thid, NULL);
}

void handle_signal(int signal) {
    switch (signal) {
#ifdef _WIN32
    case SIGTERM:
    case SIGABRT:
    case SIGBREAK:
#else
    case SIGHUP:
#endif
      break;
    case SIGINT:
      break;
    }
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
#else
	struct sigaction sa;
    sa.sa_handler = &handle_signal;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
      puts("Cannot install SIGHUP handler.");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
      puts("Cannot install SIGINT handler.");
    }
#endif

    struct Sthreadctx ctx;
    if(argc == 1) {
        ctx.is_client = 0;
        create_server(&ctx);
    } else {
        ctx.is_client = 1;
        create_client(&ctx);
    }
    handle_clients(&ctx);
    puts("bye...");

    return EXIT_SUCCESS;
}
