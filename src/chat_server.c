/**
 * @file chat_server.c
 * @brief Implementación del servidor de chat con soporte multi-cliente
 * @author Sistema de Chat Socket
 * @date 2025
 * 
 * Implementa un servidor TCP que maneja múltiples clientes concurrentes
 * usando threads, con funcionalidad de broadcast y notificaciones.
 */

#include "../include/chat_server.h"

/* Variable global para el contexto del servidor (para signal handler) */
static server_context_t *g_server_ctx = NULL;

/**
 * @brief Inicializa el contexto del servidor
 * 
 * Configura todas las estructuras de datos necesarias para el servidor,
 * incluyendo la inicialización de mutexes y arrays de clientes.
 */
int init_server_context(server_context_t *ctx)
{
    if (!ctx) return ERROR_MEMORY;
    
    /* Inicializar estructura con ceros */
    memset(ctx, 0, sizeof(server_context_t));
    
    /* Inicializar mutex para sincronización de lista de clientes */
    if (pthread_mutex_init(&ctx->clients_mutex, NULL) != 0) {
        LOG_ERROR("Error al inicializar mutex de clientes: %s", strerror(errno));
        return ERROR_THREAD;
    }
    
    /* Inicializar array de clientes */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ctx->clients[i].socket_fd = -1;
        ctx->clients[i].active = 0;
    }
    
    ctx->client_count = 0;
    ctx->server_socket = -1;
    ctx->running = 1;
    
    LOG_INFO("Contexto del servidor inicializado correctamente");
    return SUCCESS;
}

/**
 * @brief Libera los recursos del contexto del servidor
 * 
 * Realiza limpieza completa de todos los recursos, incluyendo
 * desconexión de clientes y destrucción de mutexes.
 */
void cleanup_server_context(server_context_t *ctx)
{
    if (!ctx) return;
    
    LOG_INFO("Iniciando limpieza del servidor...");
    
    /* Marcar servidor como no ejecutándose */
    ctx->running = 0;
    
    /* Cerrar socket del servidor */
    SAFE_CLOSE(ctx->server_socket);
    
    /* Desconectar todos los clientes agresivamente */
    LOG_INFO("Desconectando todos los clientes...");
    pthread_mutex_lock(&ctx->clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ctx->clients[i].active) {
            LOG_INFO("Desconectando cliente '%s'", ctx->clients[i].username);
            
            /* Cerrar socket inmediatamente para forzar desconexión */
            shutdown(ctx->clients[i].socket_fd, SHUT_RDWR);
            SAFE_CLOSE(ctx->clients[i].socket_fd);
            ctx->clients[i].active = 0;
            
            /* Cancelar thread del cliente si es posible */
            if (ctx->clients[i].thread_id != 0) {
                pthread_cancel(ctx->clients[i].thread_id);
            }
        }
    }
    ctx->client_count = 0;
    pthread_mutex_unlock(&ctx->clients_mutex);
    
    /* Destruir mutex */
    pthread_mutex_destroy(&ctx->clients_mutex);
    
    LOG_INFO("Limpieza del servidor completada");
}

/**
 * @brief Crea y configura el socket del servidor
 * 
 * Establece el socket en modo servidor, configura opciones de socket
 * y realiza bind al puerto especificado.
 */
int create_server_socket(int port)
{
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    /* Crear socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        LOG_ERROR("Error al crear socket: %s", strerror(errno));
        return ERROR_SOCKET;
    }
    
    /* Configurar opciones de socket */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("Error al configurar SO_REUSEADDR: %s", strerror(errno));
        SAFE_CLOSE(server_fd);
        return ERROR_SOCKET;
    }
    
    /* Configurar dirección del servidor */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind del socket */
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Error en bind al puerto %d: %s", port, strerror(errno));
        SAFE_CLOSE(server_fd);
        return ERROR_BIND;
    }
    
    /* Configurar socket para escuchar */
    if (listen(server_fd, LISTEN_BACKLOG) < 0) {
        LOG_ERROR("Error al configurar socket en modo listen: %s", strerror(errno));
        SAFE_CLOSE(server_fd);
        return ERROR_LISTEN;
    }
    
    LOG_INFO("Socket del servidor creado y configurado en puerto %d", port);
    return server_fd;
}

/**
 * @brief Agrega un cliente a la lista de clientes conectados
 * 
 * Busca un slot libre en el array de clientes y agrega la información
 * del nuevo cliente de forma thread-safe.
 */
int add_client(server_context_t *ctx, int client_socket, 
               struct sockaddr_in client_addr, const char *username)
{
    if (!ctx || !username) return -1;
    
    pthread_mutex_lock(&ctx->clients_mutex);
    
    /* Verificar límite de clientes */
    if (ctx->client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&ctx->clients_mutex);
        LOG_ERROR("Límite máximo de clientes alcanzado (%d)", MAX_CLIENTS);
        return -1;
    }
    
    /* Buscar slot libre */
    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!ctx->clients[i].active) {
            client_index = i;
            break;
        }
    }
    
    if (client_index == -1) {
        pthread_mutex_unlock(&ctx->clients_mutex);
        LOG_ERROR("No se encontró slot libre para nuevo cliente");
        return -1;
    }
    
    /* Configurar información del cliente */
    client_info_t *client = &ctx->clients[client_index];
    client->socket_fd = client_socket;
    client->address = client_addr;
    client->connect_time = time(NULL);
    client->active = 1;
    client->disconnect_notified = 0;
    strncpy(client->username, username, USERNAME_SIZE - 1);
    client->username[USERNAME_SIZE - 1] = '\0';
    
    ctx->client_count++;
    
    pthread_mutex_unlock(&ctx->clients_mutex);
    
    LOG_INFO("Cliente '%s' agregado (total: %d/%d)", username, ctx->client_count, MAX_CLIENTS);
    return client_index;
}

/**
 * @brief Remueve un cliente de la lista de clientes conectados
 * 
 * Busca el cliente por socket y lo marca como inactivo de forma thread-safe.
 */
int remove_client(server_context_t *ctx, int client_socket)
{
    if (!ctx) return -1;
    
    /* Si el servidor se está cerrando, no intentar remover */
    if (!ctx->running) {
        return 0; /* Silenciosamente ignorar durante el cierre */
    }
    
    pthread_mutex_lock(&ctx->clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ctx->clients[i].active && ctx->clients[i].socket_fd == client_socket) {
            /* Solo enviar notificación si no se ha enviado ya */
            if (!ctx->clients[i].disconnect_notified) {
                /* Notificar a otros clientes sobre la desconexión */
                char notification_text[MESSAGE_SIZE];
                snprintf(notification_text, sizeof(notification_text), 
                         "[Usuario %s se desconectó]", ctx->clients[i].username);
                
                chat_message_t disconnect_notification;
                init_message(&disconnect_notification, MSG_NOTIFICATION, 
                           "Sistema", notification_text);
                
                /* Marcar que ya se envió la notificación */
                ctx->clients[i].disconnect_notified = 1;
                
                /* Enviar notificación a todos los otros clientes */
                pthread_mutex_unlock(&ctx->clients_mutex);
                int sent = broadcast_message(ctx, &disconnect_notification, client_socket);
                LOG_INFO("Cliente '%s' (socket %d) se desconectó. Notificación enviada a %d clientes", 
                        ctx->clients[i].username, client_socket, sent);
                pthread_mutex_lock(&ctx->clients_mutex);
            }
            
            /* Marcar cliente como inactivo */
            ctx->clients[i].active = 0;
            SAFE_CLOSE(ctx->clients[i].socket_fd);
            ctx->client_count--;
            
            pthread_mutex_unlock(&ctx->clients_mutex);
            LOG_INFO("Cliente removido (total: %d/%d)", ctx->client_count, MAX_CLIENTS);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&ctx->clients_mutex);
    LOG_ERROR("Cliente con socket %d no encontrado para remover", client_socket);
    return -1;
}

/**
 * @brief Encuentra un cliente por su socket
 * 
 * Busca en la lista de clientes el que corresponde al socket dado.
 */
client_info_t *find_client(server_context_t *ctx, int client_socket)
{
    if (!ctx) return NULL;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ctx->clients[i].active && ctx->clients[i].socket_fd == client_socket) {
            return &ctx->clients[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Envía un mensaje a todos los clientes conectados (broadcast)
 * 
 * Itera sobre todos los clientes activos y envía el mensaje,
 * opcionalmente excluyendo un socket específico.
 */
int broadcast_message(server_context_t *ctx, const chat_message_t *msg, int exclude_socket)
{
    if (!ctx || !msg) return 0;
    
    int sent_count = 0;
    char buffer[BUFFER_SIZE];
    ssize_t msg_size;
    
    /* Serializar mensaje */
    msg_size = serialize_message(msg, buffer, sizeof(buffer));
    if (msg_size < 0) {
        LOG_ERROR("Error al serializar mensaje para broadcast");
        return 0;
    }
    
    pthread_mutex_lock(&ctx->clients_mutex);
    
    /* Enviar a todos los clientes activos */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ctx->clients[i].active && ctx->clients[i].socket_fd != exclude_socket) {
            ssize_t sent = send(ctx->clients[i].socket_fd, buffer, msg_size, MSG_NOSIGNAL);
            if (sent > 0) {
                sent_count++;
            } else {
                LOG_ERROR("Error enviando mensaje a cliente '%s': %s", 
                         ctx->clients[i].username, strerror(errno));
                /* Marcar cliente para desconexión */
                ctx->clients[i].active = 0;
            }
        }
    }
    
    pthread_mutex_unlock(&ctx->clients_mutex);
    
    return sent_count;
}

/**
 * @brief Envía un mensaje a un cliente específico
 * 
 * Serializa y envía un mensaje a un socket de cliente particular.
 */
int send_message_to_client(int client_socket, const chat_message_t *msg)
{
    if (client_socket < 0 || !msg) return -1;
    
    char buffer[BUFFER_SIZE];
    ssize_t msg_size = serialize_message(msg, buffer, sizeof(buffer));
    
    if (msg_size < 0) {
        LOG_ERROR("Error al serializar mensaje");
        return -1;
    }
    
    ssize_t sent = send(client_socket, buffer, msg_size, MSG_NOSIGNAL);
    if (sent != msg_size) {
        LOG_ERROR("Error enviando mensaje: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Thread principal para manejar un cliente individual
 * 
 * Función que ejecuta cada thread de cliente, manejando la recepción
 * de mensajes y el procesamiento de los mismos.
 */
void *handle_client_thread(void *args)
{
    client_thread_args_t *client_args = (client_thread_args_t*)args;
    server_context_t *ctx = client_args->server_ctx;
    int client_socket = client_args->client_socket;
    
    char buffer[BUFFER_SIZE];
    chat_message_t msg;
    client_info_t *client = NULL;
    
    LOG_INFO("Thread iniciado para cliente en socket %d", client_socket);
    
    /* Esperar mensaje inicial de conexión con nombre de usuario */
    ssize_t received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        LOG_ERROR("Error recibiendo mensaje inicial del cliente");
        goto cleanup;
    }
    
    /* Deserializar mensaje inicial */
    if (deserialize_message(buffer, received, &msg) < 0 || msg.type != MSG_CONNECT) {
        LOG_ERROR("Mensaje inicial inválido del cliente");
        goto cleanup;
    }
    
    /* Validar nombre de usuario */
    if (!validate_username(msg.username)) {
        LOG_ERROR("Nombre de usuario inválido: '%s'", msg.username);
        chat_message_t error_msg;
        init_message(&error_msg, MSG_ERROR, "Sistema", "Nombre de usuario inválido");
        send_message_to_client(client_socket, &error_msg);
        goto cleanup;
    }
    
    /* Agregar cliente a la lista */
    int client_index = add_client(ctx, client_socket, client_args->client_addr, msg.username);
    if (client_index < 0) {
        LOG_ERROR("Error agregando cliente '%s'", msg.username);
        chat_message_t error_msg;
        init_message(&error_msg, MSG_ERROR, "Sistema", "Servidor lleno. Intente más tarde.");
        send_message_to_client(client_socket, &error_msg);
        goto cleanup;
    }
    
    /* Obtener referencia al cliente */
    client = find_client(ctx, client_socket);
    if (!client) {
        LOG_ERROR("Error encontrando cliente recién agregado");
        goto cleanup;
    }
    
    /* Guardar thread ID */
    client->thread_id = pthread_self();
    
    /* Notificar conexión exitosa al cliente */
    chat_message_t welcome_msg;
    init_message(&welcome_msg, MSG_NOTIFICATION, "Sistema", 
                 "Conectado al chat. ¡Bienvenido!");
    send_message_to_client(client_socket, &welcome_msg);
    
    /* Notificar a otros clientes sobre la nueva conexión */
    notify_user_connected(ctx, msg.username, client_socket);
    
    /* Bucle principal de manejo de mensajes */
    while (ctx->running && client->active) {
        received = recv(client_socket, buffer, sizeof(buffer), 0);
        
        if (received <= 0) {
            if (received == 0) {
                LOG_INFO("Cliente '%s' cerró la conexión", client->username);
            } else {
                LOG_ERROR("Error recibiendo datos del cliente '%s': %s", 
                         client->username, strerror(errno));
            }
            break;
        }
        
        /* Procesar mensaje recibido */
        if (deserialize_message(buffer, received, &msg) == 0) {
            if (process_client_message(ctx, client, &msg) < 0) {
                LOG_ERROR("Error procesando mensaje del cliente '%s'", client->username);
                break;
            }
        } else {
            LOG_ERROR("Error deserializando mensaje del cliente '%s'", client->username);
        }
    }
    
cleanup:
    /* Manejar desconexión */
    if (client) {
        handle_client_disconnect(ctx, client);
    } else {
        SAFE_CLOSE(client_socket);
    }
    
    /* Liberar argumentos del thread */
    free(client_args);
    
    LOG_INFO("Thread de cliente finalizado");
    return NULL;
}

/**
 * @brief Procesa un mensaje recibido de un cliente
 * 
 * Analiza el tipo de mensaje y toma la acción apropiada,
 * incluyendo broadcast de mensajes de chat.
 */
int process_client_message(server_context_t *ctx, client_info_t *client, 
                          const chat_message_t *msg)
{
    if (!ctx || !client || !msg) return -1;
    
    switch (msg->type) {
        case MSG_CHAT:
            /* Broadcast del mensaje de chat a todos los clientes */
            {
                chat_message_t broadcast_msg;
                init_message(&broadcast_msg, MSG_CHAT, client->username, msg->content);
                int sent = broadcast_message(ctx, &broadcast_msg, -1);
                LOG_INFO("Mensaje de '%s' enviado a %d clientes", client->username, sent);
            }
            break;
            
        case MSG_DISCONNECT:
            /* El cliente solicita desconectarse */
            LOG_INFO("Cliente '%s' (socket %d) solicita desconexión", client->username, client->socket_fd);
            
            /* Marcar que ya se procesó la desconexión */
            client->disconnect_notified = 1;
            
            /* Notificar a otros clientes sobre la desconexión */
            {
                chat_message_t disconnect_notification;
                char notification_text[MESSAGE_SIZE];
                
                snprintf(notification_text, sizeof(notification_text), 
                         "[Usuario %s se desconectó]", client->username);
                
                init_message(&disconnect_notification, MSG_NOTIFICATION, 
                           "Sistema", notification_text);
                
                /* Enviar notificación a todos los otros clientes */
                int sent = broadcast_message(ctx, &disconnect_notification, client->socket_fd);
                LOG_INFO("Notificación de desconexión de '%s' enviada a %d clientes", 
                        client->username, sent);
            }
            
            client->active = 0;
            return -1;
            
        case MSG_KEEPALIVE:
            /* Responder al keepalive */
            {
                chat_message_t keepalive_response;
                init_message(&keepalive_response, MSG_KEEPALIVE, "Sistema", "");
                send_message_to_client(client->socket_fd, &keepalive_response);
            }
            break;
            
        default:
            LOG_ERROR("Tipo de mensaje desconocido (%d) del cliente '%s'", 
                     msg->type, client->username);
            break;
    }
    
    return 0;
}

/**
 * @brief Maneja la desconexión de un cliente
 * 
 * Realiza todas las tareas necesarias cuando un cliente se desconecta,
 * incluyendo notificaciones y limpieza de recursos.
 */
void handle_client_disconnect(server_context_t *ctx, client_info_t *client)
{
    if (!ctx || !client) return;
    
    char username[USERNAME_SIZE];
    strncpy(username, client->username, USERNAME_SIZE - 1);
    username[USERNAME_SIZE - 1] = '\0';
    
    /* Remover cliente de la lista */
    remove_client(ctx, client->socket_fd);
    
    /* Notificar desconexión a otros clientes */
    notify_user_disconnected(ctx, username);
}

/**
 * @brief Envía notificación de conexión de usuario
 * 
 * Crea y envía una notificación a todos los clientes sobre
 * la conexión de un nuevo usuario.
 */
void notify_user_connected(server_context_t *ctx, const char *username, int exclude_socket)
{
    if (!ctx || !username) return;
    
    char notification[MESSAGE_SIZE];
    snprintf(notification, sizeof(notification), "[Usuario %s se conectó]", username);
    
    chat_message_t notify_msg;
    init_message(&notify_msg, MSG_NOTIFICATION, "Sistema", notification);
    
    int sent = broadcast_message(ctx, &notify_msg, exclude_socket);
    LOG_INFO("Notificación de conexión de '%s' enviada a %d clientes", username, sent);
}

/**
 * @brief Envía notificación de desconexión de usuario
 * 
 * Crea y envía una notificación a todos los clientes sobre
 * la desconexión de un usuario.
 */
void notify_user_disconnected(server_context_t *ctx, const char *username)
{
    if (!ctx || !username) return;
    
    char notification[MESSAGE_SIZE];
    snprintf(notification, sizeof(notification), "[Usuario %s se desconectó]", username);
    
    chat_message_t notify_msg;
    init_message(&notify_msg, MSG_NOTIFICATION, "Sistema", notification);
    
    int sent = broadcast_message(ctx, &notify_msg, -1);
    LOG_INFO("Notificación de desconexión de '%s' enviada a %d clientes", username, sent);
}

/**
 * @brief Manejador de señales para cierre graceful del servidor
 * 
 * Maneja señales SIGINT y SIGTERM para realizar cierre ordenado.
 */
void signal_handler(int sig)
{
    LOG_INFO("Señal %d recibida, iniciando cierre del servidor...", sig);
    
    if (g_server_ctx) {
        g_server_ctx->running = 0;
        
        /* Forzar salida de accept() cerrando el socket del servidor */
        if (g_server_ctx->server_socket >= 0) {
            LOG_INFO("Cerrando socket del servidor para forzar salida...");
            close(g_server_ctx->server_socket);
            g_server_ctx->server_socket = -1;
        }
    }
}

/**
 * @brief Configura los manejadores de señales
 * 
 * Instala los manejadores de señales para cierre graceful.
 */
void setup_signal_handlers(server_context_t *ctx)
{
    g_server_ctx = ctx;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); /* Ignorar SIGPIPE en sockets */
    
    LOG_INFO("Manejadores de señales configurados");
}

/**
 * @brief Imprime estadísticas del servidor
 * 
 * Muestra información actual sobre el estado del servidor.
 */
void print_server_stats(const server_context_t *ctx)
{
    if (!ctx) return;
    
    printf("\n=== ESTADÍSTICAS DEL SERVIDOR ===\n");
    printf("Estado: %s\n", ctx->running ? "Ejecutándose" : "Detenido");
    printf("Clientes conectados: %d/%d\n", ctx->client_count, MAX_CLIENTS);
    
    if (ctx->client_count > 0) {
        printf("\nClientes activos:\n");
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (ctx->clients[i].active) {
                char time_str[32];
                format_timestamp(ctx->clients[i].connect_time, time_str, sizeof(time_str));
                printf("  - %s (conectado desde %s)\n", 
                       ctx->clients[i].username, time_str);
            }
        }
    }
    printf("===============================\n\n");
}

/**
 * @brief Función principal del servidor
 * 
 * Implementa el bucle principal del servidor, aceptando conexiones
 * y creando threads para manejar cada cliente.
 */
int run_server(int port)
{
    server_context_t server_ctx;
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    LOG_INFO("Iniciando servidor de chat en puerto %d", port);
    
    /* Inicializar contexto del servidor */
    if (init_server_context(&server_ctx) != SUCCESS) {
        LOG_ERROR("Error inicializando contexto del servidor");
        return ERROR_MEMORY;
    }
    
    /* Configurar manejadores de señales */
    setup_signal_handlers(&server_ctx);
    
    /* Crear socket del servidor */
    server_ctx.server_socket = create_server_socket(port);
    if (server_ctx.server_socket < 0) {
        cleanup_server_context(&server_ctx);
        return server_ctx.server_socket;
    }
    
    LOG_INFO("Servidor iniciado correctamente. Esperando conexiones...");
    print_server_stats(&server_ctx);
    
    /* Bucle principal del servidor */
    while (server_ctx.running) {
        /* Aceptar nueva conexión */
        client_socket = accept(server_ctx.server_socket, 
                              (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_socket < 0) {
            if (errno == EINTR) {
                /* Interrupción por señal, continuar */
                continue;
            }
            if (!server_ctx.running) {
                /* El servidor se está cerrando, salir silenciosamente */
                LOG_INFO("Socket del servidor cerrado, terminando bucle principal");
                break;
            }
            LOG_ERROR("Error en accept: %s", strerror(errno));
            break;
        }
        
        LOG_INFO("Nueva conexión desde %s:%d", 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Crear argumentos para el thread del cliente */
        client_thread_args_t *client_args = malloc(sizeof(client_thread_args_t));
        if (!client_args) {
            LOG_ERROR("Error asignando memoria para argumentos de thread");
            SAFE_CLOSE(client_socket);
            continue;
        }
        
        client_args->client_socket = client_socket;
        client_args->client_addr = client_addr;
        client_args->server_ctx = &server_ctx;
        
        /* Crear thread para manejar el cliente */
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client_thread, client_args) != 0) {
            LOG_ERROR("Error creando thread para cliente: %s", strerror(errno));
            free(client_args);
            SAFE_CLOSE(client_socket);
            continue;
        }
        
        /* Detach del thread para limpieza automática */
        pthread_detach(client_thread);
    }
    
    LOG_INFO("Cerrando servidor...");
    cleanup_server_context(&server_ctx);
    
    return SUCCESS;
}

/**
 * @brief Función main del servidor
 * 
 * Punto de entrada principal del programa servidor.
 */
int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    
    /* Procesar argumentos de línea de comandos */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Puerto inválido: %s\n", argv[1]);
            fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    /* Ejecutar servidor */
    int result = run_server(port);
    
    if (result == SUCCESS) {
        LOG_INFO("Servidor terminado correctamente");
        return EXIT_SUCCESS;
    } else {
        LOG_ERROR("Servidor terminado con errores (código: %d)", result);
        return EXIT_FAILURE;
    }
}
