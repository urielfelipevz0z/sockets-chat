/**
 * @file chat_server.h
 * @brief Definiciones específicas del servidor de chat
 * @author Sistema de Chat Socket
 * @date 2025
 * 
 * Este archivo contiene las definiciones, estructuras y prototipos
 * específicos para el componente servidor del sistema de chat.
 */

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include "chat_common.h"

/* ========== CONSTANTES ESPECÍFICAS DEL SERVIDOR ========== */

#define LISTEN_BACKLOG      10          /* Tamaño de la cola de conexiones pendientes */
#define CLEANUP_INTERVAL    300         /* Intervalo de limpieza en segundos */

/* ========== ESTRUCTURAS ESPECÍFICAS DEL SERVIDOR ========== */

/**
 * @brief Argumentos para el thread de manejo de cliente
 * 
 * Estructura que se pasa al thread que maneja cada cliente individual,
 * conteniendo toda la información necesaria para la comunicación.
 */
typedef struct {
    int client_socket;                      /* Socket del cliente */
    struct sockaddr_in client_addr;         /* Dirección del cliente */
    server_context_t *server_ctx;           /* Contexto del servidor */
} client_thread_args_t;

/* ========== PROTOTIPOS DE FUNCIONES DEL SERVIDOR ========== */

/**
 * @brief Inicializa el contexto del servidor
 * @param ctx Contexto del servidor a inicializar
 * @return 0 en éxito, código de error negativo en fallo
 */
int init_server_context(server_context_t *ctx);

/**
 * @brief Libera los recursos del contexto del servidor
 * @param ctx Contexto del servidor a limpiar
 */
void cleanup_server_context(server_context_t *ctx);

/**
 * @brief Crea y configura el socket del servidor
 * @param port Puerto en el que escuchar
 * @return File descriptor del socket o -1 en error
 */
int create_server_socket(int port);

/**
 * @brief Agrega un cliente a la lista de clientes conectados
 * @param ctx Contexto del servidor
 * @param client_socket Socket del cliente
 * @param client_addr Dirección del cliente
 * @param username Nombre de usuario del cliente
 * @return Índice del cliente en la lista o -1 en error
 */
int add_client(server_context_t *ctx, int client_socket, 
               struct sockaddr_in client_addr, const char *username);

/**
 * @brief Remueve un cliente de la lista de clientes conectados
 * @param ctx Contexto del servidor
 * @param client_socket Socket del cliente a remover
 * @return 0 en éxito, -1 en error
 */
int remove_client(server_context_t *ctx, int client_socket);

/**
 * @brief Encuentra un cliente por su socket
 * @param ctx Contexto del servidor
 * @param client_socket Socket del cliente a buscar
 * @return Puntero al cliente o NULL si no se encuentra
 */
client_info_t *find_client(server_context_t *ctx, int client_socket);

/**
 * @brief Envía un mensaje a todos los clientes conectados (broadcast)
 * @param ctx Contexto del servidor
 * @param msg Mensaje a enviar
 * @param exclude_socket Socket a excluir del broadcast (opcional, -1 para incluir todos)
 * @return Número de clientes que recibieron el mensaje
 */
int broadcast_message(server_context_t *ctx, const chat_message_t *msg, int exclude_socket);

/**
 * @brief Envía un mensaje a un cliente específico
 * @param client_socket Socket del cliente destinatario
 * @param msg Mensaje a enviar
 * @return 0 en éxito, -1 en error
 */
int send_message_to_client(int client_socket, const chat_message_t *msg);

/**
 * @brief Thread principal para manejar un cliente individual
 * @param args Argumentos del thread (client_thread_args_t*)
 * @return NULL
 */
void *handle_client_thread(void *args);

/**
 * @brief Procesa un mensaje recibido de un cliente
 * @param ctx Contexto del servidor
 * @param client Cliente que envió el mensaje
 * @param msg Mensaje recibido
 * @return 0 en éxito, -1 en error
 */
int process_client_message(server_context_t *ctx, client_info_t *client, 
                          const chat_message_t *msg);

/**
 * @brief Maneja la desconexión de un cliente
 * @param ctx Contexto del servidor
 * @param client Cliente que se desconecta
 */
void handle_client_disconnect(server_context_t *ctx, client_info_t *client);

/**
 * @brief Envía notificación de conexión de usuario
 * @param ctx Contexto del servidor
 * @param username Nombre del usuario que se conectó
 * @param exclude_socket Socket a excluir de la notificación
 */
void notify_user_connected(server_context_t *ctx, const char *username, int exclude_socket);

/**
 * @brief Envía notificación de desconexión de usuario
 * @param ctx Contexto del servidor
 * @param username Nombre del usuario que se desconectó
 */
void notify_user_disconnected(server_context_t *ctx, const char *username);

/**
 * @brief Manejador de señales para cierre graceful del servidor
 * @param sig Número de señal recibida
 */
void signal_handler(int sig);

/**
 * @brief Configura los manejadores de señales
 * @param ctx Contexto del servidor
 */
void setup_signal_handlers(server_context_t *ctx);

/**
 * @brief Imprime estadísticas del servidor
 * @param ctx Contexto del servidor
 */
void print_server_stats(const server_context_t *ctx);

/**
 * @brief Función principal del servidor
 * @param port Puerto en el que ejecutar el servidor
 * @return 0 en éxito, código de error en fallo
 */
int run_server(int port);

#endif /* CHAT_SERVER_H */
