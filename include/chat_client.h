/**
 * @file chat_client.h
 * @brief Definiciones específicas del cliente de chat
 * @author Sistema de Chat Socket
 * @date 2025
 * 
 * Este archivo contiene las definiciones, estructuras y prototipos
 * específicos para el componente cliente del sistema de chat.
 */

#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include "chat_common.h"
#include <termios.h>

/* ========== CONSTANTES ESPECÍFICAS DEL CLIENTE ========== */

#define INPUT_BUFFER_SIZE   512         /* Tamaño del buffer de entrada */
#define RECONNECT_ATTEMPTS  3           /* Número de intentos de reconexión */
#define RECONNECT_DELAY     5           /* Delay entre reconexiones en segundos */

/* ========== ESTRUCTURAS ESPECÍFICAS DEL CLIENTE ========== */

/**
 * @brief Contexto del cliente de chat
 * 
 * Mantiene el estado del cliente incluyendo información de conexión,
 * threads de comunicación y configuración del terminal.
 */
typedef struct {
    int server_socket;                      /* Socket de conexión al servidor */
    char username[USERNAME_SIZE];           /* Nombre de usuario del cliente */
    char server_ip[16];                     /* Dirección IP del servidor */
    int server_port;                        /* Puerto del servidor */
    
    pthread_t receive_thread;               /* Thread para recibir mensajes */
    pthread_t input_thread;                 /* Thread para manejar entrada */
    
    int connected;                          /* Estado de conexión */
    int running;                            /* Estado de ejecución */
    
    struct termios original_termios;        /* Configuración original del terminal */
    int terminal_configured;                /* Flag de configuración del terminal */
    
    pthread_mutex_t output_mutex;           /* Mutex para salida thread-safe */
} client_context_t;

/**
 * @brief Argumentos para threads del cliente
 * 
 * Estructura para pasar el contexto del cliente a los threads.
 */
typedef struct {
    client_context_t *ctx;                  /* Contexto del cliente */
} client_thread_args_t;

/* ========== PROTOTIPOS DE FUNCIONES DEL CLIENTE ========== */

/**
 * @brief Inicializa el contexto del cliente
 * @param ctx Contexto del cliente a inicializar
 * @param username Nombre de usuario
 * @param server_ip Dirección IP del servidor
 * @param server_port Puerto del servidor
 * @return 0 en éxito, código de error negativo en fallo
 */
int init_client_context(client_context_t *ctx, const char *username, 
                       const char *server_ip, int server_port);

/**
 * @brief Libera los recursos del contexto del cliente
 * @param ctx Contexto del cliente a limpiar
 */
void cleanup_client_context(client_context_t *ctx);

/**
 * @brief Establece conexión con el servidor
 * @param ctx Contexto del cliente
 * @return 0 en éxito, código de error en fallo
 */
int connect_to_server(client_context_t *ctx);

/**
 * @brief Desconecta del servidor
 * @param ctx Contexto del cliente
 */
void disconnect_from_server(client_context_t *ctx);

/**
 * @brief Envía mensaje de conexión inicial al servidor
 * @param ctx Contexto del cliente
 * @return 0 en éxito, -1 en error
 */
int send_connect_message(client_context_t *ctx);

/**
 * @brief Envía un mensaje de chat al servidor
 * @param ctx Contexto del cliente
 * @param message Contenido del mensaje
 * @return 0 en éxito, -1 en error
 */
int send_chat_message(client_context_t *ctx, const char *message);

/**
 * @brief Thread para recibir mensajes del servidor
 * @param args Argumentos del thread (client_thread_args_t*)
 * @return NULL
 */
void *receive_thread_func(void *args);

/**
 * @brief Thread para manejar entrada del usuario
 * @param args Argumentos del thread (client_thread_args_t*)
 * @return NULL
 */
void *input_thread_func(void *args);

/**
 * @brief Procesa un mensaje recibido del servidor
 * @param ctx Contexto del cliente
 * @param msg Mensaje recibido
 */
void process_server_message(client_context_t *ctx, const chat_message_t *msg);

/**
 * @brief Muestra un mensaje en la consola de forma thread-safe
 * @param ctx Contexto del cliente
 * @param msg Mensaje a mostrar
 */
void display_message(client_context_t *ctx, const chat_message_t *msg);

/**
 * @brief Configura el terminal para entrada no bloqueante
 * @param ctx Contexto del cliente
 * @return 0 en éxito, -1 en error
 */
int setup_terminal(client_context_t *ctx);

/**
 * @brief Restaura la configuración original del terminal
 * @param ctx Contexto del cliente
 */
void restore_terminal(client_context_t *ctx);

/**
 * @brief Lee una línea de entrada del usuario
 * @param buffer Buffer de salida
 * @param buffer_size Tamaño del buffer
 * @return Número de caracteres leídos
 */
int read_user_input(char *buffer, size_t buffer_size);

/**
 * @brief Procesa comandos especiales del cliente
 * @param ctx Contexto del cliente
 * @param input Entrada del usuario
 * @return 1 si es comando especial, 0 si es mensaje normal
 */
int process_client_command(client_context_t *ctx, const char *input);

/**
 * @brief Muestra la ayuda de comandos disponibles
 */
void show_help(void);

/**
 * @brief Muestra información de estado del cliente
 * @param ctx Contexto del cliente
 */
void show_status(client_context_t *ctx);

/**
 * @brief Manejador de señales para el cliente
 * @param sig Número de señal recibida
 */
void client_signal_handler(int sig);

/**
 * @brief Configura manejadores de señales para el cliente
 * @param ctx Contexto del cliente
 */
void setup_client_signal_handlers(client_context_t *ctx);

/**
 * @brief Función principal del cliente
 * @param username Nombre de usuario
 * @param server_ip Dirección IP del servidor
 * @param server_port Puerto del servidor
 * @return 0 en éxito, código de error en fallo
 */
int run_client(const char *username, const char *server_ip, int server_port);

/**
 * @brief Muestra mensaje de bienvenida y comandos básicos
 */
void show_welcome_message(void);

/**
 * @brief Valida los parámetros de entrada del cliente
 * @param username Nombre de usuario a validar
 * @param server_ip IP del servidor a validar
 * @param server_port Puerto del servidor a validar
 * @return 0 si son válidos, -1 si no
 */
int validate_client_params(const char *username, const char *server_ip, int server_port);

#endif /* CHAT_CLIENT_H */
