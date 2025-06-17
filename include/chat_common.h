/**
 * @file chat_common.h
 * @brief Definiciones comunes para el sistema de chat cliente-servidor
 * @author Sistema de Chat Socket
 * @date 2025
 * 
 * Este archivo contiene todas las definiciones, estructuras y constantes
 * compartidas entre el cliente y el servidor del sistema de chat.
 */

#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

/* ========== CONSTANTES DE CONFIGURACIÓN ========== */

/* Configuración de red */
#define DEFAULT_PORT        8080        /* Puerto por defecto del servidor */
#define MAX_CLIENTS         50          /* Límite máximo de clientes concurrentes */
#define BUFFER_SIZE         1024        /* Tamaño del buffer para mensajes */
#define USERNAME_SIZE       32          /* Tamaño máximo del nombre de usuario */
#define MESSAGE_SIZE        (BUFFER_SIZE - USERNAME_SIZE - 64) /* Tamaño del mensaje */

/* Configuración de timeout y reintentos */
#define CONNECTION_TIMEOUT  30          /* Timeout de conexión en segundos */
#define KEEPALIVE_INTERVAL  60          /* Intervalo de keepalive en segundos */

/* Códigos de retorno */
#define SUCCESS             0
#define ERROR_SOCKET        -1
#define ERROR_BIND          -2
#define ERROR_LISTEN        -3
#define ERROR_ACCEPT        -4
#define ERROR_CONNECT       -5
#define ERROR_THREAD        -6
#define ERROR_MEMORY        -7

/* ========== TIPOS DE MENSAJES ========== */

typedef enum {
    MSG_CONNECT,        /* Mensaje de conexión inicial */
    MSG_DISCONNECT,     /* Mensaje de desconexión */
    MSG_CHAT,          /* Mensaje de chat normal */
    MSG_NOTIFICATION,   /* Notificación del sistema */
    MSG_ERROR,         /* Mensaje de error */
    MSG_KEEPALIVE      /* Mensaje de keepalive */
} message_type_t;

/* ========== ESTRUCTURAS DE DATOS ========== */

/**
 * @brief Estructura para representar un mensaje en el chat
 * 
 * Esta estructura encapsula toda la información necesaria para
 * un mensaje en el sistema de chat, incluyendo tipo, remitente y contenido.
 */
typedef struct {
    message_type_t type;                    /* Tipo de mensaje */
    char username[USERNAME_SIZE];           /* Nombre del usuario remitente */
    char content[MESSAGE_SIZE];             /* Contenido del mensaje */
    time_t timestamp;                       /* Timestamp del mensaje */
    size_t length;                          /* Longitud total del mensaje */
} chat_message_t;

/**
 * @brief Estructura para representar un cliente conectado
 * 
 * Mantiene la información necesaria para gestionar cada cliente
 * conectado al servidor, incluyendo socket y datos de identificación.
 */
typedef struct {
    int socket_fd;                          /* File descriptor del socket */
    char username[USERNAME_SIZE];           /* Nombre de usuario */
    struct sockaddr_in address;             /* Dirección IP del cliente */
    time_t connect_time;                    /* Tiempo de conexión */
    pthread_t thread_id;                    /* ID del thread del cliente */
    int active;                             /* Flag de estado activo */
    int disconnect_notified;                /* Flag para evitar notificaciones duplicadas */
} client_info_t;

/**
 * @brief Estructura para el contexto del servidor
 * 
 * Mantiene el estado global del servidor incluyendo la lista de clientes
 * conectados y los mecanismos de sincronización.
 */
typedef struct {
    client_info_t clients[MAX_CLIENTS];     /* Array de clientes conectados */
    int client_count;                       /* Número actual de clientes */
    pthread_mutex_t clients_mutex;          /* Mutex para acceso a lista de clientes */
    int server_socket;                      /* Socket del servidor */
    int running;                            /* Flag de estado del servidor */
} server_context_t;

/* ========== PROTOTIPOS DE FUNCIONES COMUNES ========== */

/**
 * @brief Inicializa una estructura de mensaje
 * @param msg Puntero al mensaje a inicializar
 * @param type Tipo de mensaje
 * @param username Nombre de usuario (puede ser NULL)
 * @param content Contenido del mensaje (puede ser NULL)
 */
void init_message(chat_message_t *msg, message_type_t type, 
                  const char *username, const char *content);

/**
 * @brief Serializa un mensaje para envío por red
 * @param msg Mensaje a serializar
 * @param buffer Buffer de salida
 * @param buffer_size Tamaño del buffer
 * @return Número de bytes serializados o -1 en error
 */
ssize_t serialize_message(const chat_message_t *msg, char *buffer, size_t buffer_size);

/**
 * @brief Deserializa un mensaje recibido por red
 * @param buffer Buffer con datos serializados
 * @param buffer_size Tamaño de los datos
 * @param msg Estructura de mensaje de salida
 * @return 0 en éxito, -1 en error
 */
int deserialize_message(const char *buffer, size_t buffer_size, chat_message_t *msg);

/**
 * @brief Formatea un timestamp para mostrar
 * @param timestamp Timestamp a formatear
 * @param buffer Buffer de salida
 * @param buffer_size Tamaño del buffer
 */
void format_timestamp(time_t timestamp, char *buffer, size_t buffer_size);

/**
 * @brief Valida un nombre de usuario
 * @param username Nombre de usuario a validar
 * @return 1 si es válido, 0 si no
 */
int validate_username(const char *username);

/**
 * @brief Función de logging thread-safe
 * @param level Nivel de log
 * @param format Formato del mensaje
 * @param ... Argumentos del formato
 */
void safe_log(const char *level, const char *format, ...);

/* ========== MACROS DE UTILIDAD ========== */

/* Macros para logging */
#define LOG_INFO(...)    safe_log("INFO", __VA_ARGS__)
#define LOG_ERROR(...)   safe_log("ERROR", __VA_ARGS__)
#define LOG_DEBUG(...)   safe_log("DEBUG", __VA_ARGS__)

/* Macro para limpieza de recursos */
#define SAFE_CLOSE(fd) do { \
    if ((fd) >= 0) { \
        close(fd); \
        (fd) = -1; \
    } \
} while(0)

/* Macro para verificación de errores de pthread */
#define PTHREAD_CHECK(call) do { \
    int ret = (call); \
    if (ret != 0) { \
        fprintf(stderr, "Error en %s: %s\n", #call, strerror(ret)); \
        return ERROR_THREAD; \
    } \
} while(0)

#endif /* CHAT_COMMON_H */
