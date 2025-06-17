/**
 * @file chat_common.c
 * @brief Implementación de funciones comunes del sistema de chat
 * @author Sistema de Chat Socket
 * @date 2025
 */

#include "../include/chat_common.h"
#include <stdarg.h>

/* Mutex global para logging thread-safe */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Inicializa una estructura de mensaje
 * 
 * Esta función inicializa todos los campos de un mensaje con valores
 * por defecto y establece los valores proporcionados.
 */
void init_message(chat_message_t *msg, message_type_t type, 
                  const char *username, const char *content)
{
    if (!msg) return;
    
    /* Inicializar estructura con ceros */
    memset(msg, 0, sizeof(chat_message_t));
    
    /* Establecer tipo de mensaje */
    msg->type = type;
    
    /* Establecer timestamp actual */
    msg->timestamp = time(NULL);
    
    /* Copiar nombre de usuario si se proporciona */
    if (username) {
        strncpy(msg->username, username, USERNAME_SIZE - 1);
        msg->username[USERNAME_SIZE - 1] = '\0';
    }
    
    /* Copiar contenido si se proporciona */
    if (content) {
        strncpy(msg->content, content, MESSAGE_SIZE - 1);
        msg->content[MESSAGE_SIZE - 1] = '\0';
    }
    
    /* Calcular longitud total */
    msg->length = sizeof(chat_message_t);
}

/**
 * @brief Serializa un mensaje para envío por red
 * 
 * Convierte la estructura de mensaje a un formato de bytes
 * que puede ser transmitido por la red de forma segura.
 */
ssize_t serialize_message(const chat_message_t *msg, char *buffer, size_t buffer_size)
{
    if (!msg || !buffer || buffer_size < sizeof(chat_message_t)) {
        return -1;
    }
    
    /* Copia directa de la estructura (network byte order se manejará si es necesario) */
    memcpy(buffer, msg, sizeof(chat_message_t));
    
    return sizeof(chat_message_t);
}

/**
 * @brief Deserializa un mensaje recibido por red
 * 
 * Convierte los bytes recibidos de la red de vuelta a la
 * estructura de mensaje, validando la integridad de los datos.
 */
int deserialize_message(const char *buffer, size_t buffer_size, chat_message_t *msg)
{
    if (!buffer || !msg || buffer_size < sizeof(chat_message_t)) {
        return -1;
    }
    
    /* Copia directa a la estructura */
    memcpy(msg, buffer, sizeof(chat_message_t));
    
    /* Validaciones básicas */
    if (msg->type < MSG_CONNECT || msg->type > MSG_KEEPALIVE) {
        return -1;
    }
    
    /* Asegurar terminación nula de strings */
    msg->username[USERNAME_SIZE - 1] = '\0';
    msg->content[MESSAGE_SIZE - 1] = '\0';
    
    return 0;
}

/**
 * @brief Formatea un timestamp para mostrar
 * 
 * Convierte un timestamp Unix a una cadena legible
 * en formato [HH:MM:SS] para mostrar en el chat.
 */
void format_timestamp(time_t timestamp, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size < 12) return;
    
    struct tm *tm_info = localtime(&timestamp);
    if (tm_info) {
        snprintf(buffer, buffer_size, "[%02d:%02d:%02d]", 
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    } else {
        strncpy(buffer, "[--:--:--]", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

/**
 * @brief Valida un nombre de usuario
 * 
 * Verifica que el nombre de usuario cumple con los criterios:
 * - No vacío
 * - Solo caracteres alfanuméricos y '_'
 * - Longitud apropiada
 */
int validate_username(const char *username)
{
    if (!username || strlen(username) == 0) {
        return 0;
    }
    
    size_t len = strlen(username);
    if (len >= USERNAME_SIZE) {
        return 0;
    }
    
    /* Verificar caracteres válidos */
    for (size_t i = 0; i < len; i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || 
              (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || 
              c == '_')) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief Función de logging thread-safe
 * 
 * Implementa logging seguro para entornos multi-thread,
 * incluyendo timestamp y nivel de log.
 */
void safe_log(const char *level, const char *format, ...)
{
    /* Adquirir mutex para operación atómica */
    pthread_mutex_lock(&log_mutex);
    
    /* Obtener timestamp actual */
    time_t now = time(NULL);
    char timestamp[32];
    format_timestamp(now, timestamp, sizeof(timestamp));
    
    /* Imprimir prefijo con timestamp y nivel */
    printf("%s [%s] ", timestamp, level);
    
    /* Imprimir mensaje con argumentos variables */
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    /* Liberar mutex */
    pthread_mutex_unlock(&log_mutex);
}
