/**
 *	@author		:	Spl3en (Moreau Cyril) <spl3en.contact@gmail.com>
 *	@file		:	EasySocket.c
 *
 *	Furthermore informations in EasySocket.h
*/
#include "EasySocket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef ES_WITHOUT_LINKED_LIBS
	int (*_WSAStartup) (WORD,LPWSADATA);
	int (*_WSACleanup) (void);
	int (*_listen)(SOCKET,int);
	int (*_bind) (SOCKET,const struct sockaddr*,int);
	int (*_recv)(SOCKET,char*,int,int);
	int (*_send)(SOCKET,const char*,int,int);
	int (*_connect) (SOCKET,const struct sockaddr*,int);
	int (*_closesocket) (SOCKET);
	SOCKET (*_accept) (SOCKET,struct sockaddr*,int*);
	SOCKET (*_socket)(int,int,int);
	u_long (*_htonl) (u_long);
	u_short (*_htons) (u_short);
	struct hostent * (*_gethostbyname) (const char*);
	unsigned long (*_inet_addr) (const char*);
	char * (*_inet_ntoa) (struct in_addr);

	#define recv _recv
	#define accept _accept
	#define send _send
	#define bind _bind
	#define listen _listen
	#define inet_ntoa _inet_ntoa
	#define gethostbyname _gethostbyname
	#define socket _socket
	#define htonl _htonl
	#define htons _htons
	#define inet_addr _inet_addr
	#define connect _connect
	#define WSAStartup _WSAStartup
	#define WSACleanup _WSACleanup
	#define closesocket _closesocket
#endif

/* Private variables */
bool wsadata_initialized = FALSE;

/* Private Methods */

static void
_ex_es_listener (EasySocketListenerArgs *esla)
{
    EasySocketListened *esl = esla->esl;
    void (*_callback)() = esla->callback;
    void (*_finish_callback)() = esla->finish_callback;
    int b_read;

    while (esl->is_connected)
    {
		if ((b_read = recv(esl->sock, esl->buffer, esl->bsize, 0)) <= 0)
        {
            esl->is_connected = 0;
            continue;
        }

        esl->buffer[b_read-1] = '\0';
		_callback(esl);
    }

    _finish_callback(esl);
}

/* Public Methods */

/**
 *    @Constructors
 */
EasySocketListenerArgs *
esla_new(EasySocketListened *esl, void (*callback)(), void (*finish_callback)())
{
    EasySocketListenerArgs *esla = NULL;
    if ((esla = malloc(sizeof(EasySocketListenerArgs))) == NULL)
        return NULL;

    esla->esl = esl;
    esla->callback = callback;
    esla->finish_callback = finish_callback;

    return esla;
}

EasySocket *
es_server_new (int port, int max_connection)
{
    if (!wsadata_initialized)
        es_init();

    EasySocket *p = NULL;

    SOCKET sock = 0;
    SOCKADDR_IN server_context;
    SOCKADDR_IN csin;
    int csin_size = sizeof(csin);

	if ((p = (EasySocket *) malloc (sizeof(EasySocket))) == NULL)
		return NULL;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert (sock != INVALID_SOCKET);

    server_context.sin_family      = AF_INET;
    server_context.sin_addr.s_addr = htonl(INADDR_ANY);
    server_context.sin_port        = htons(port);

    if (bind(sock, (SOCKADDR*)&server_context, csin_size) == SOCKET_ERROR)
        return NULL;

    if (listen(sock, max_connection) == SOCKET_ERROR)
        return NULL;

    p->sock = sock;
    p->is_connected = 1;
	p->abortEvent = NULL;

    return p;
}

int
es_set_blocking (EasySocket *es, bool blocking)
{
	SOCKET sock = es->sock;

	// Set non blocking
	#ifdef WIN32
		unsigned long on = (blocking) ? 0 : 1;
		if (0 != ioctlsocket (sock, FIONBIO, &on))
			return -1;
	#else
		int flags = fcntl (sock, F_GETFL, 0);
		if (flags < 0)
			return -1;
		flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
		if (fcntl (sock, F_SETFL, flags) != 0)
			return -1;
	#endif

	return 0;
}

EasySocket *
es_client_new_from_ip (char *ip, int port)
{
    if (!wsadata_initialized)
        es_init();

    EasySocket *p = NULL;

    SOCKET sock = 0;
    SOCKADDR_IN server_context;
    SOCKADDR_IN csin;
    int csin_size = sizeof(csin);

	if ((p = (EasySocket *) malloc (sizeof(EasySocket))) == NULL)
		return NULL;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert (sock != INVALID_SOCKET);

    server_context.sin_family      = AF_INET;
    server_context.sin_addr.s_addr = inet_addr(ip);
    server_context.sin_port        = htons (port);

    if (connect(sock, (SOCKADDR*)&server_context, csin_size) == SOCKET_ERROR)
        return NULL;

    p->sock = sock;
    p->is_connected = 1;
    p->hostname = NULL;
    p->ip = ip;

    return p;
}

EasySocket *
es_client_new_from_host (char *hostname, int port)
{
    if (!wsadata_initialized)
        es_init();

    EasySocket *p = NULL;
    char *ip = NULL;

    if ((ip = es_get_ip_from_hostname(hostname)) == NULL) {
        printf("[!] Cannot connect to %s:%d.\n", hostname, port);
        return NULL;
    }

    p = es_client_new_from_ip(ip, port);
    p->hostname = hostname;

	return p;
}

/**
 *  @Accessors
 */
// Existe en macro
void
_es_func_set_data (EasySocketListened *esl, void *data)
{
    esl->_data = data;
}

void *
_es_func_get_data (EasySocketListened *esl)
{
    return esl->_data;
}



/**
 *  @Methods
 */

int
es_init()
{
    wsadata_initialized = TRUE;
    WSADATA wsaData;

	#ifdef ES_WITHOUT_LINKED_LIBS
	HINSTANCE ws2_32 = LoadLibrary("ws2_32.dll");
	_recv = (void *) GetProcAddress (ws2_32, "recv");
	_send = (void *) GetProcAddress (ws2_32, "send");
	_bind = (void *) GetProcAddress (ws2_32, "bind");
	_accept = (void *) GetProcAddress (ws2_32, "accept");
	_gethostbyname = (void *) GetProcAddress (ws2_32, "gethostbyname");
	_socket = (void *) GetProcAddress (ws2_32, "socket");
	_listen = (void *) GetProcAddress (ws2_32, "listen");
	_htons = (void *) GetProcAddress (ws2_32, "htons");
	_htonl = (void *) GetProcAddress (ws2_32, "htonl");
	_connect = (void *) GetProcAddress (ws2_32, "connect");
	_WSAStartup = (void *) GetProcAddress (ws2_32, "WSAStartup");
	_WSACleanup = (void *) GetProcAddress (ws2_32, "WSACleanup");
	_closesocket = (void *) GetProcAddress (ws2_32, "closesocket");
	_inet_ntoa = (void *) GetProcAddress (ws2_32, "inet_ntoa");
	_inet_addr = (void *) GetProcAddress (ws2_32, "inet_addr");
	#endif

    return (WSAStartup (MAKEWORD(2, 0), &wsaData) == 0);
}


EasySocketListened *
es_accept (EasySocket *server, int buffer_size_allocated, bool abortable)
{
    SOCKET sock;
    SOCKADDR_IN csin;
    int csin_size = sizeof(csin);
    EasySocketListened *esl = NULL;

	if (abortable) {
		server->abortEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	}

    sock = accept (server->sock, (SOCKADDR *) &csin, &csin_size);

    if (sock == (unsigned) SOCKET_ERROR)
        return NULL;

    if ((esl = malloc(sizeof(EasySocketListened))) == NULL)
        return NULL;

    esl->sock         = sock;
    esl->is_connected = true;
    esl->buffer       = str_malloc_clear(buffer_size_allocated);
    esl->bsize        = buffer_size_allocated;
    esl->_data        = NULL;

    return esl;
}

char *
es_get_ip_from_hostname (char *addr)
{
    struct hostent *h;

	if ((h = gethostbyname(addr)) == NULL)
        return NULL;

	return inet_ntoa(*((struct in_addr *)h->h_addr));
}

void
es_set_connected (EasySocket *es, int is_connected)
{
    es->is_connected = is_connected;
}

void
es_listener (EasySocketListened *esl, void (*recv_callback)(EasySocketListened *sock), void (*finish_callback)(EasySocketListened *sock))
{
    EasySocketListenerArgs *esla = esla_new(esl, recv_callback, finish_callback);

    esl->thread = CreateThread(NULL, 0, (void*)_ex_es_listener, esla, 0, NULL);
}

int
es_send (EasySocket *es, void *msg, int len)
{
    if (len == -1)
        len = strlen(msg);

    return send(es->sock, msg, len, 0);
}

void
es_set_timeout (EasySocket *es, long int milliseconds)
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = milliseconds;

    setsockopt(es->sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
}


unsigned char *
es_recv (EasySocket *es, int *_out_size)
{
	int bytes;
	unsigned char data[1024] = {};
	unsigned int total_bytes = 0;
	BbQueue msg_recved = bb_queue_local_decl();

    do {
    	if ((bytes = recv (es->sock, data, (sizeof data) - 1, 0)) <= 0) {
			// Server disconnected
			break;
    	}

        Buffer *buffer = buffer_new_from_ptr(data, bytes);
        bb_queue_add(&msg_recved, buffer);
        total_bytes += bytes;

    } while (bytes == sizeof(data));

    *_out_size = total_bytes;

    if (total_bytes == 0)
        return NULL;

    unsigned int write_pos = 0;
    char *response = malloc(total_bytes);

    foreach_bbqueue_item (&msg_recved, Buffer *buffer)
    {
        memcpy(&response[write_pos], buffer->data, buffer->size);
        write_pos += buffer->size;
        buffer_free(buffer);
    }

    return response;
}

bool
es_recv_buffer (EasySocket *es, void *data, int sizeData)
{
	int bytes = recv (es->sock, data, sizeData, 0);

	if (bytes <= 0 || bytes > sizeData) {
		// Server disconnected or data too long
		return false;
	}

	return true;
}

int
es_close(EasySocket *es)
{
    return (closesocket(es->sock) != SOCKET_ERROR);
}

void
es_end()
{
    WSACleanup();
    wsadata_initialized = FALSE;
}

/**
 *  @Destructors
 */

void
es_free (EasySocket *p)
{
	if (p != NULL)
	{
		free(p);
	}
}

void
es_listener_free (EasySocketListened *esl, void (*free_data_func)())
{
    if (esl != NULL)
    {
        free(esl->buffer);

        if (esl->_data != NULL && free_data_func != NULL)
            free_data_func(esl->_data);

        closesocket(esl->sock);

        free(esl);
    }
}
