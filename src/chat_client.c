/**
 * @file chat_client.c
 * @brief Implementación del cliente de chat con interfaz de terminal
 * @author Sistema de Chat Socket
 * @date 2025
 * 
 * Implementa un cliente TCP que se conecta al servidor de chat,
 * con threads separados para entrada y recepción de mensajes.
 */

#include "../include/chat_client.h"

/* Variable global para el contexto del cliente (para signal handler) */
static client_context_t *g_client_ctx = NULL;

/**
 * @brief Inicializa el contexto del cliente
 * 
 * Configura todas las estructuras de datos necesarias para el cliente,
 * incluyendo parámetros de conexión y configuración inicial.
 */
int init_client_context(client_context_t *ctx, const char *username, 
                       const char *server_ip, int server_port)
{
    if (!ctx || !username || !server_ip) return ERROR_MEMORY;
    
    /* Inicializar estructura con ceros */
    memset(ctx, 0, sizeof(client_context_t));
    
    /* Configurar parámetros de conexión */
    strncpy(ctx->username, username, USERNAME_SIZE - 1);
    ctx->username[USERNAME_SIZE - 1] = '\0';
    
    strncpy(ctx->server_ip, server_ip, sizeof(ctx->server_ip) - 1);
    ctx->server_ip[sizeof(ctx->server_ip) - 1] = '\0';
    
    ctx->server_port = server_port;
    ctx->server_socket = -1;
    ctx->connected = 0;
    ctx->running = 1;
    ctx->terminal_configured = 0;
    
    /* Inicializar mutex para salida thread-safe */
    if (pthread_mutex_init(&ctx->output_mutex, NULL) != 0) {
        LOG_ERROR("Error al inicializar mutex de salida: %s", strerror(errno));
        return ERROR_THREAD;
    }
    
    LOG_INFO("Contexto del cliente inicializado para usuario '%s'", username);
    return SUCCESS;
}

/**
 * @brief Libera los recursos del contexto del cliente
 * 
 * Realiza limpieza completa de todos los recursos del cliente.
 */
void cleanup_client_context(client_context_t *ctx)
{
    if (!ctx) return;
    
    LOG_INFO("Iniciando limpieza del cliente...");
    
    /* Marcar como no ejecutándose */
    ctx->running = 0;
    
    /* Desconectar del servidor */
    disconnect_from_server(ctx);
    
    /* Restaurar terminal */
    restore_terminal(ctx);
    
    /* Destruir mutex */
    pthread_mutex_destroy(&ctx->output_mutex);
    
    LOG_INFO("Limpieza del cliente completada");
}

/**
 * @brief Establece conexión con el servidor
 * 
 * Crea socket y establece conexión TCP con el servidor especificado.
 */
int connect_to_server(client_context_t *ctx)
{
    if (!ctx) return ERROR_MEMORY;
    
    struct sockaddr_in server_addr;
    
    LOG_INFO("Conectando a servidor %s:%d...", ctx->server_ip, ctx->server_port);
    
    /* Crear socket */
    ctx->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->server_socket < 0) {
        LOG_ERROR("Error al crear socket: %s", strerror(errno));
        return ERROR_SOCKET;
    }
    
    /* Configurar dirección del servidor */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ctx->server_port);
    
    if (inet_pton(AF_INET, ctx->server_ip, &server_addr.sin_addr) <= 0) {
        LOG_ERROR("Dirección IP inválida: %s", ctx->server_ip);
        SAFE_CLOSE(ctx->server_socket);
        return ERROR_CONNECT;
    }
    
    /* Establecer conexión */
    if (connect(ctx->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Error conectando al servidor: %s", strerror(errno));
        SAFE_CLOSE(ctx->server_socket);
        return ERROR_CONNECT;
    }
    
    ctx->connected = 1;
    LOG_INFO("Conexión establecida exitosamente");
    
    return SUCCESS;
}

/**
 * @brief Desconecta del servidor
 * 
 * Cierra la conexión con el servidor de forma ordenada.
 */
void disconnect_from_server(client_context_t *ctx)
{
    if (!ctx || !ctx->connected) return;
    
    LOG_INFO("Desconectando del servidor...");
    
    /* Enviar mensaje de desconexión si es posible */
    if (ctx->server_socket >= 0) {
        chat_message_t disconnect_msg;
        init_message(&disconnect_msg, MSG_DISCONNECT, ctx->username, "");
        
        /* Serializar y enviar mensaje de desconexión */
        char buffer[BUFFER_SIZE];
        ssize_t msg_size = serialize_message(&disconnect_msg, buffer, sizeof(buffer));
        if (msg_size > 0) {
            send(ctx->server_socket, buffer, msg_size, 0);
        }
    }
    
    /* Cerrar socket */
    SAFE_CLOSE(ctx->server_socket);
    ctx->connected = 0;
    
    LOG_INFO("Desconectado del servidor");
}

/**
 * @brief Envía mensaje de conexión inicial al servidor
 * 
 * Envía el mensaje de handshake inicial con el nombre de usuario.
 */
int send_connect_message(client_context_t *ctx)
{
    if (!ctx || !ctx->connected) return -1;
    
    chat_message_t connect_msg;
    init_message(&connect_msg, MSG_CONNECT, ctx->username, "");
    
    char buffer[BUFFER_SIZE];
    ssize_t msg_size = serialize_message(&connect_msg, buffer, sizeof(buffer));
    
    if (msg_size < 0) {
        LOG_ERROR("Error serializando mensaje de conexión");
        return -1;
    }
    
    ssize_t sent = send(ctx->server_socket, buffer, msg_size, 0);
    if (sent != msg_size) {
        LOG_ERROR("Error enviando mensaje de conexión: %s", strerror(errno));
        return -1;
    }
    
    LOG_INFO("Mensaje de conexión enviado al servidor");
    return 0;
}

/**
 * @brief Envía un mensaje de chat al servidor
 * 
 * Serializa y envía un mensaje de chat al servidor.
 */
int send_chat_message(client_context_t *ctx, const char *message)
{
    if (!ctx || !message || !ctx->connected) return -1;
    
    chat_message_t chat_msg;
    init_message(&chat_msg, MSG_CHAT, ctx->username, message);
    
    char buffer[BUFFER_SIZE];
    ssize_t msg_size = serialize_message(&chat_msg, buffer, sizeof(buffer));
    
    if (msg_size < 0) {
        LOG_ERROR("Error serializando mensaje de chat");
        return -1;
    }
    
    ssize_t sent = send(ctx->server_socket, buffer, msg_size, 0);
    if (sent != msg_size) {
        LOG_ERROR("Error enviando mensaje de chat: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Thread para recibir mensajes del servidor
 * 
 * Bucle principal que recibe y procesa mensajes del servidor.
 */
void *receive_thread_func(void *args)
{
    client_thread_args_t *thread_args = (client_thread_args_t*)args;
    client_context_t *ctx = thread_args->ctx;
    
    char buffer[BUFFER_SIZE];
    chat_message_t msg;
    
    LOG_INFO("Thread de recepción iniciado");
    
    while (ctx->running && ctx->connected) {
        ssize_t received = recv(ctx->server_socket, buffer, sizeof(buffer), 0);
        
        if (received <= 0) {
            if (received == 0) {
                LOG_INFO("Servidor cerró la conexión");
            } else {
                LOG_ERROR("Error recibiendo datos del servidor: %s", strerror(errno));
            }
            ctx->connected = 0;
            ctx->running = 0;
            break;
        }
        
        /* Procesar mensaje recibido */
        if (deserialize_message(buffer, received, &msg) == 0) {
            process_server_message(ctx, &msg);
        } else {
            LOG_ERROR("Error deserializando mensaje del servidor");
        }
    }
    
    LOG_INFO("Thread de recepción finalizado");
    return NULL;
}

/**
 * @brief Thread para manejar entrada del usuario
 * 
 * Maneja la entrada del usuario desde el terminal de forma asíncrona.
 */
void *input_thread_func(void *args)
{
    client_thread_args_t *thread_args = (client_thread_args_t*)args;
    client_context_t *ctx = thread_args->ctx;
    
    char input_buffer[INPUT_BUFFER_SIZE];
    
    LOG_INFO("Thread de entrada iniciado");
    
    /* Mostrar prompt inicial */
    pthread_mutex_lock(&ctx->output_mutex);
    printf("> ");
    fflush(stdout);
    pthread_mutex_unlock(&ctx->output_mutex);
    
    while (ctx->running && ctx->connected) {
        /* Leer entrada del usuario con timeout usando select */
        fd_set readfds;
        struct timeval timeout;
        int stdin_fd = STDIN_FILENO;
        
        FD_ZERO(&readfds);
        FD_SET(stdin_fd, &readfds);
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int select_result = select(stdin_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (select_result > 0 && FD_ISSET(stdin_fd, &readfds)) {
            /* Hay entrada disponible */
            if (read_user_input(input_buffer, sizeof(input_buffer)) > 0) {
                /* Verificar si aún estamos ejecutándose */
                if (!ctx->running || !ctx->connected) {
                    break;
                }
                
                /* Procesar comando o mensaje */
                if (!process_client_command(ctx, input_buffer)) {
                    /* Es un mensaje normal, enviarlo al servidor */
                    if (send_chat_message(ctx, input_buffer) < 0) {
                        LOG_ERROR("Error enviando mensaje al servidor");
                        break;
                    }
                }
                
                /* Mostrar nuevo prompt solo después de procesar entrada */
                if (ctx->running && ctx->connected) {
                    pthread_mutex_lock(&ctx->output_mutex);
                    printf("> ");
                    fflush(stdout);
                    pthread_mutex_unlock(&ctx->output_mutex);
                }
            }
        } else if (select_result < 0 && errno != EINTR) {
            LOG_ERROR("Error en select: %s", strerror(errno));
            break;
        }
        /* Si select_result == 0, fue timeout, continuamos el loop para verificar ctx->running */
    }
    
    LOG_INFO("Thread de entrada finalizado");
    return NULL;
}

/**
 * @brief Procesa un mensaje recibido del servidor
 * 
 * Analiza el tipo de mensaje y toma la acción apropiada.
 */
void process_server_message(client_context_t *ctx, const chat_message_t *msg)
{
    if (!ctx || !msg) return;
    
    switch (msg->type) {
        case MSG_CHAT:
        case MSG_NOTIFICATION:
            display_message(ctx, msg);
            break;
            
        case MSG_ERROR:
            pthread_mutex_lock(&ctx->output_mutex);
            printf("\n[ERROR] %s\n", msg->content);
            fflush(stdout);
            pthread_mutex_unlock(&ctx->output_mutex);
            break;
            
        case MSG_KEEPALIVE:
            /* Responder al keepalive */
            {
                chat_message_t response;
                init_message(&response, MSG_KEEPALIVE, ctx->username, "");
                char buffer[BUFFER_SIZE];
                ssize_t msg_size = serialize_message(&response, buffer, sizeof(buffer));
                if (msg_size > 0) {
                    send(ctx->server_socket, buffer, msg_size, 0);
                }
            }
            break;
            
        default:
            LOG_ERROR("Tipo de mensaje desconocido recibido: %d", msg->type);
            break;
    }
}

/**
 * @brief Muestra un mensaje en la consola de forma thread-safe
 * 
 * Formatea y muestra mensajes con timestamp y formato apropiado.
 */
void display_message(client_context_t *ctx, const chat_message_t *msg)
{
    if (!ctx || !msg) return;
    
    char timestamp[32];
    format_timestamp(msg->timestamp, timestamp, sizeof(timestamp));
    
    pthread_mutex_lock(&ctx->output_mutex);
    
    /* Borrar línea actual del prompt */
    printf("\r\033[K");
    
    if (msg->type == MSG_NOTIFICATION) {
        printf("%s %s\n", timestamp, msg->content);
    } else {
        printf("%s <%s> %s\n", timestamp, msg->username, msg->content);
    }
    
    fflush(stdout);
    pthread_mutex_unlock(&ctx->output_mutex);
}

/**
 * @brief Configura el terminal para entrada no bloqueante
 * 
 * Modifica la configuración del terminal para mejor experiencia de usuario.
 */
int setup_terminal(client_context_t *ctx)
{
    if (!ctx) return -1;
    
    /* Verificar si estamos en un terminal interactivo */
    if (!isatty(STDIN_FILENO)) {
        LOG_INFO("No se detectó terminal interactivo, saltando configuración");
        return 0;
    }
    
    /* Obtener configuración actual del terminal */
    if (tcgetattr(STDIN_FILENO, &ctx->original_termios) < 0) {
        LOG_ERROR("Error obteniendo configuración del terminal: %s", strerror(errno));
        return -1;
    }
    
    ctx->terminal_configured = 1;
    LOG_INFO("Terminal configurado correctamente");
    return 0;
}

/**
 * @brief Restaura la configuración original del terminal
 * 
 * Restaura la configuración del terminal al estado original.
 */
void restore_terminal(client_context_t *ctx)
{
    if (!ctx || !ctx->terminal_configured) return;
    
    /* Restaurar configuración original */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ctx->original_termios) < 0) {
        LOG_ERROR("Error restaurando configuración del terminal: %s", strerror(errno));
    } else {
        LOG_INFO("Configuración del terminal restaurada");
    }
    
    ctx->terminal_configured = 0;
}

/**
 * @brief Lee una línea de entrada del usuario
 * 
 * Lee entrada del usuario desde stdin con manejo de errores.
 */
int read_user_input(char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0) return -1;
    
    if (!fgets(buffer, buffer_size, stdin)) {
        return -1;
    }
    
    /* Remover salto de línea */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    return (int)len;
}

/**
 * @brief Procesa comandos especiales del cliente
 * 
 * Maneja comandos especiales como /help, /quit, /status, etc.
 */
int process_client_command(client_context_t *ctx, const char *input)
{
    if (!ctx || !input) return 0;
    
    /* Verificar si es un comando (inicia con /) */
    if (input[0] != '/') {
        return 0; /* No es un comando */
    }
    
    if (strcmp(input, "/help") == 0 || strcmp(input, "/h") == 0) {
        show_help();
        return 1;
    }
    
    if (strcmp(input, "/quit") == 0 || strcmp(input, "/q") == 0) {
        pthread_mutex_lock(&ctx->output_mutex);
        printf("Desconectando del chat...\n");
        pthread_mutex_unlock(&ctx->output_mutex);
        
        /* Enviar mensaje de desconexión al servidor */
        chat_message_t disconnect_msg;
        init_message(&disconnect_msg, MSG_DISCONNECT, ctx->username, "");
        char buffer[BUFFER_SIZE];
        ssize_t msg_size = serialize_message(&disconnect_msg, buffer, sizeof(buffer));
        if (msg_size > 0) {
            send(ctx->server_socket, buffer, msg_size, 0);
        }
        
        ctx->running = 0;
        ctx->connected = 0;
        
        /* Forzar salida inmediata cerrando el socket */
        SAFE_CLOSE(ctx->server_socket);
        
        return 1;
    }
    
    if (strcmp(input, "/status") == 0 || strcmp(input, "/s") == 0) {
        show_status(ctx);
        return 1;
    }
    
    /* Comando no reconocido */
    pthread_mutex_lock(&ctx->output_mutex);
    printf("Comando no reconocido: %s\nUse /help para ver comandos disponibles.\n", input);
    pthread_mutex_unlock(&ctx->output_mutex);
    
    return 1;
}

/**
 * @brief Muestra la ayuda de comandos disponibles
 * 
 * Imprime la lista de comandos disponibles para el usuario.
 */
void show_help(void)
{
    printf("\n=== COMANDOS DISPONIBLES ===\n");
    printf("/help, /h     - Mostrar esta ayuda\n");
    printf("/quit, /q     - Salir del chat\n");
    printf("/status, /s   - Mostrar estado de conexión\n");
    printf("\nPara enviar un mensaje, simplemente escriba el texto y presione Enter.\n");
    printf("===========================\n\n");
}

/**
 * @brief Muestra información de estado del cliente
 * 
 * Muestra información actual sobre el estado de la conexión.
 */
void show_status(client_context_t *ctx)
{
    if (!ctx) return;
    
    printf("\n=== ESTADO DEL CLIENTE ===\n");
    printf("Usuario: %s\n", ctx->username);
    printf("Servidor: %s:%d\n", ctx->server_ip, ctx->server_port);
    printf("Estado: %s\n", ctx->connected ? "Conectado" : "Desconectado");
    printf("Ejecutándose: %s\n", ctx->running ? "Sí" : "No");
    printf("==========================\n\n");
}

/**
 * @brief Manejador de señales para el cliente
 * 
 * Maneja señales para cierre graceful del cliente.
 */
void client_signal_handler(int sig)
{
    LOG_INFO("Señal %d recibida, cerrando cliente...", sig);
    
    if (g_client_ctx) {
        g_client_ctx->running = 0;
    }
}

/**
 * @brief Configura manejadores de señales para el cliente
 * 
 * Instala los manejadores de señales apropiados.
 */
void setup_client_signal_handlers(client_context_t *ctx)
{
    g_client_ctx = ctx;
    
    signal(SIGINT, client_signal_handler);
    signal(SIGTERM, client_signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    LOG_INFO("Manejadores de señales del cliente configurados");
}

/**
 * @brief Muestra mensaje de bienvenida y comandos básicos
 * 
 * Muestra información inicial al usuario sobre cómo usar el cliente.
 */
void show_welcome_message(void)
{
    printf("\n");
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│                    CLIENTE DE CHAT TCP                      │\n");
    printf("│                                                             │\n");
    printf("│  • Escriba mensajes y presione Enter para enviarlos         │\n");
    printf("│  • Use /help para ver comandos disponibles                  │\n");
    printf("│  • Use /quit para salir del chat                            │\n");
    printf("│                                                             │\n");
    printf("└─────────────────────────────────────────────────────────────┘\n");
    printf("\n");
}

/**
 * @brief Valida los parámetros de entrada del cliente
 * 
 * Verifica que los parámetros proporcionados sean válidos.
 */
int validate_client_params(const char *username, const char *server_ip, int server_port)
{
    /* Validar nombre de usuario */
    if (!validate_username(username)) {
        fprintf(stderr, "Error: Nombre de usuario inválido '%s'\n", username);
        fprintf(stderr, "El nombre debe contener solo letras, números y '_'\n");
        return -1;
    }
    
    /* Validar puerto */
    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Error: Puerto inválido %d\n", server_port);
        return -1;
    }
    
    /* Validar IP (básico) */
    if (!server_ip || strlen(server_ip) == 0) {
        fprintf(stderr, "Error: Dirección IP inválida\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Función principal del cliente
 * 
 * Implementa el bucle principal del cliente con threads para E/S.
 */
int run_client(const char *username, const char *server_ip, int server_port)
{
    client_context_t client_ctx;
    client_thread_args_t thread_args;
    
    LOG_INFO("Iniciando cliente de chat para usuario '%s'", username);
    
    /* Validar parámetros */
    if (validate_client_params(username, server_ip, server_port) < 0) {
        return ERROR_MEMORY;
    }
    
    /* Inicializar contexto del cliente */
    if (init_client_context(&client_ctx, username, server_ip, server_port) != SUCCESS) {
        LOG_ERROR("Error inicializando contexto del cliente");
        return ERROR_MEMORY;
    }
    
    /* Configurar manejadores de señales */
    setup_client_signal_handlers(&client_ctx);
    
    /* Configurar terminal (opcional) */
    setup_terminal(&client_ctx);  /* No fallar si no es un terminal interactivo */
    
    /* Conectar al servidor */
    if (connect_to_server(&client_ctx) != SUCCESS) {
        cleanup_client_context(&client_ctx);
        return ERROR_CONNECT;
    }
    
    /* Enviar mensaje de conexión inicial */
    if (send_connect_message(&client_ctx) < 0) {
        LOG_ERROR("Error enviando mensaje de conexión inicial");
        cleanup_client_context(&client_ctx);
        return ERROR_CONNECT;
    }
    
    /* Mostrar mensaje de bienvenida */
    show_welcome_message();
    
    /* Configurar argumentos para threads */
    thread_args.ctx = &client_ctx;
    
    /* Crear thread de recepción */
    if (pthread_create(&client_ctx.receive_thread, NULL, receive_thread_func, &thread_args) != 0) {
        LOG_ERROR("Error creando thread de recepción: %s", strerror(errno));
        cleanup_client_context(&client_ctx);
        return ERROR_THREAD;
    }
    
    /* Crear thread de entrada */
    if (pthread_create(&client_ctx.input_thread, NULL, input_thread_func, &thread_args) != 0) {
        LOG_ERROR("Error creando thread de entrada: %s", strerror(errno));
        cleanup_client_context(&client_ctx);
        return ERROR_THREAD;
    }
    
    /* Esperar a que terminen los threads normalmente */
    /* Solo usar timeout si explícitamente se solicitó salida */
    while (client_ctx.running && client_ctx.connected) {
        usleep(500000); /* 500ms */
    }
    
    /* Ahora que running=0 o connected=0, esperar que threads terminen */
    int timeout_count = 0;
    const int max_timeout = 50; /* 5 segundos máximo para terminación */
    
    /* Esperar a que los threads realmente terminen */
    while (timeout_count < max_timeout) {
        usleep(100000); /* 100ms */
        timeout_count++;
        
        /* Verificar si los threads aún están activos */
        int receive_alive = (pthread_kill(client_ctx.receive_thread, 0) == 0);
        int input_alive = (pthread_kill(client_ctx.input_thread, 0) == 0);
        
        if (!receive_alive && !input_alive) {
            break; /* Ambos threads han terminado */
        }
    }
    
    if (timeout_count >= max_timeout) {
        LOG_INFO("Timeout esperando terminación de threads, cancelando...");
        pthread_cancel(client_ctx.receive_thread);
        pthread_cancel(client_ctx.input_thread);
    }
    
    /* Hacer join a los threads */
    pthread_join(client_ctx.receive_thread, NULL);
    pthread_join(client_ctx.input_thread, NULL);
    
    /* Limpieza */
    cleanup_client_context(&client_ctx);
    
    printf("\nCliente terminado.\n");
    return SUCCESS;
}

/**
 * @brief Función main del cliente
 * 
 * Punto de entrada principal del programa cliente.
 */
int main(int argc, char *argv[])
{
    const char *username;
    const char *server_ip = "127.0.0.1";
    int server_port = DEFAULT_PORT;
    
    /* Procesar argumentos de línea de comandos */
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <nombre_usuario> [ip_servidor] [puerto]\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s juan 192.168.1.100 8080\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    username = argv[1];
    
    if (argc > 2) {
        server_ip = argv[2];
    }
    
    if (argc > 3) {
        server_port = atoi(argv[3]);
        if (server_port <= 0 || server_port > 65535) {
            fprintf(stderr, "Puerto inválido: %s\n", argv[3]);
            return EXIT_FAILURE;
        }
    }
    
    /* Ejecutar cliente */
    int result = run_client(username, server_ip, server_port);
    
    if (result == SUCCESS) {
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Cliente terminado con errores (código: %d)\n", result);
        return EXIT_FAILURE;
    }
}
