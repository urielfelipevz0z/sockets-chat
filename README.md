# Sistema de Chat TCP Cliente-Servidor

Un sistema completo de chat en tiempo real implementado en C utilizando sockets TCP, diseñado para manejar múltiples clientes concurrentes con arquitectura basada en threads y cierre graceful mejorado.

## 🚀 Características Principales

- **Servidor TCP multihilo** que soporta hasta 50 clientes concurrentes
- **Cliente con interfaz de terminal** intuitiva y fácil de usar
- **Comunicación bidireccional** en tiempo real
- **Notificaciones automáticas** de conexión y desconexión de usuarios
- **Puertos configurables** - sin hardcoding, completamente flexible
- **Cierre graceful instantáneo** del servidor con Ctrl+C
- **Cliente termina correctamente** con comandos `/quit` o `/q`
- **Conexiones remotas** - acepta clientes desde cualquier IP
- **Arquitectura modular** con separación clara de responsabilidades
- **Thread safety** con sincronización robusta
- **Manejo completo de errores** y casos edge
- **Código bien documentado** siguiendo estándares GNU C

## 📋 Requisitos del Sistema

### Obligatorios
- **SO**: Linux (cualquier distribución moderna)
- **Compilador**: GCC 4.8+ con soporte para C99
- **Bibliotecas**: pthread, libc estándar

### Verificación de Dependencias
```bash
# Verificar GCC
gcc --version

# Verificar soporte pthread
gcc -pthread --help | grep pthread
```

## 🛠️ Instalación y Compilación

### Compilación Básica
```bash
# Navegar al directorio del proyecto
cd /ruta/al/proyecto/sockets

# Compilar (modo optimizado)
make

# O compilar modo debug
make debug

# Verificar archivos generados
ls -la bin/
# Debería mostrar: chat_server y chat_client
```

### Comandos de Compilación

| Comando | Descripción |
|---------|-------------|
| `make` | Compilación estándar (optimizada) |
| `make debug` | Compilación con símbolos de debug |
| `make clean` | Limpiar archivos compilados |
| `make help` | Mostrar ayuda del Makefile |

## 🎯 Uso del Sistema

### 🖥️ Servidor

#### Sintaxis:
```bash
./bin/chat_server [puerto]
```

#### Ejemplos:
```bash
# Puerto por defecto (8080)
./bin/chat_server

# Puerto específico
./bin/chat_server 9000
./bin/chat_server 3000
./bin/chat_server 12345
```

#### Cerrar servidor:
```bash
# Método recomendado (cierre graceful instantáneo)
Ctrl + C

# El servidor cerrará automáticamente todas las conexiones
```

### 👥 Cliente

#### Sintaxis:
```bash
./bin/chat_client <usuario> [ip_servidor] [puerto]
```

#### Ejemplos:
```bash
# Conexión local básica (127.0.0.1:8080)
./bin/chat_client juan

# Servidor remoto (puerto por defecto)
./bin/chat_client maria 192.168.1.100

# IP y puerto específicos
./bin/chat_client pedro 10.0.0.5 9000

# Conexión desde red local
./bin/chat_client ana servidor.local 3000
```

### 🎮 Comandos del Cliente
| Comando | Alias | Descripción |
|---------|-------|-------------|
| `/help` | `/h` | Mostrar ayuda |
| `/quit` | `/q` | Salir del chat (terminación limpia) |
| `/status` | `/s` | Ver estado de conexión |

## 💡 Ejemplos Completos

### Ejemplo 1: Chat Local (Puerto 8080)
```bash
# Terminal 1 - Servidor
./bin/chat_server

# Terminal 2 - Cliente 1
./bin/chat_client usuario1

# Terminal 3 - Cliente 2  
./bin/chat_client usuario2
```

### Ejemplo 2: Chat en Red Local (Puerto Personalizado)
```bash
# En servidor (IP: 192.168.1.10)
./bin/chat_server 9000

# Cliente desde otra máquina
./bin/chat_client juan 192.168.1.10 9000

# Cliente local
./bin/chat_client maria localhost 9000
```

### Ejemplo 3: Sesión de Chat Típica

#### Terminal del Servidor:
```bash
$ ./bin/chat_server 8080
[INFO] Mensaje entre usuarios intercambiado
^C[INFO] Señal recibida, cerrando servidor...
[INFO] Todos los clientes desconectados correctamente
```

#### Terminal Cliente 1:
```bash
$ ./bin/chat_client juan 192.168.1.10 8080
[INFO] Conectado al chat. ¡Bienvenido!
[Usuario maria se conectó]
> Hola a todos!
[12:30:45] juan: Hola a todos!
[12:30:50] maria: ¡Hola juan! ¿Cómo estás?
> Muy bien, gracias
[12:30:55] juan: Muy bien, gracias
> /quit
Desconectando del chat...
Cliente terminado.
```

#### Terminal Cliente 2:
```bash
$ ./bin/chat_client maria 192.168.1.10 8080
[INFO] Conectado al chat. ¡Bienvenido!
> ¡Hola juan! ¿Cómo estás?
[12:30:45] juan: Hola a todos!
[12:30:50] maria: ¡Hola juan! ¿Cómo estás?
[12:30:55] juan: Muy bien, gracias
[Usuario juan se desconectó]
> /status
Usuario: maria
Servidor: 192.168.1.10:8080
Estado: Conectado
> /quit
```

## 🏗️ Arquitectura del Sistema

### 📂 Estructura del Proyecto
```
sockets/
├── src/                    # Código fuente
│   ├── chat_client.c      # Implementación del cliente
│   ├── chat_server.c      # Implementación del servidor
│   └── chat_common.c      # Funciones comunes
├── include/               # Headers
│   ├── chat_client.h     # Definiciones del cliente
│   ├── chat_server.h     # Definiciones del servidor
│   └── chat_common.h     # Definiciones comunes
├── bin/                   # Ejecutables (generados)
│   ├── chat_client       # Cliente compilado
│   └── chat_server       # Servidor compilado
├── obj/                   # Archivos objeto (generados)
│   ├── *.o               # Archivos objeto compilados
│   └── *.d               # Archivos de dependencias
├── docs/                  # Documentación técnica
├── Makefile              # Script de compilación
└── README.md             # Este archivo
```

### 🧵 Arquitectura de Threads

#### Servidor:
- **Thread Principal**: Acepta nuevas conexiones
- **Thread por Cliente**: Maneja comunicación individual
- **Sincronización**: Mutex para lista de clientes thread-safe

#### Cliente:
- **Thread Principal**: Control general y limpieza
- **Thread de Entrada**: Maneja input del usuario
- **Thread de Recepción**: Procesa mensajes del servidor

## ⚙️ Configuración Avanzada

### Parámetros Configurables (include/chat_common.h)
```c
#define MAX_CLIENTS         50      // Máximo clientes concurrentes
#define BUFFER_SIZE         1024    // Tamaño buffer mensajes
#define USERNAME_SIZE       32      // Tamaño máximo usuario
#define CONNECTION_TIMEOUT  30      // Timeout conexión (segundos)
```

### Puertos Recomendados
| Uso | Puerto | Comando |
|-----|--------|---------|
| **Desarrollo local** | 8080 | `./bin/chat_server` |
| **Testing** | 9000-9999 | `./bin/chat_server 9000` |
| **Producción** | 3000-8000 | `./bin/chat_server 5000` |
| **Múltiples instancias** | Diferentes | `./bin/chat_server 8001` |

## 🐛 Solución de Problemas

### Problemas Comunes

#### "Address already in use"
```bash
# El puerto está ocupado, usar otro puerto
./bin/chat_server 8081

# O esperar que se libere (~2 minutos)
# O matar proceso que usa el puerto:
sudo netstat -tlnp | grep 8080
sudo kill <PID>
```

#### "Connection refused" 
```bash
# Verificar que el servidor esté corriendo
ps aux | grep chat_server

# Verificar puerto correcto
./bin/chat_client usuario 127.0.0.1 8080  # puerto específico
```

#### Cliente no termina con /quit
- ✅ **SOLUCIONADO**: Los clientes ahora terminan correctamente
- Usa `/quit` o `/q` para salida limpia

#### Servidor tarda en cerrar con Ctrl+C
- ✅ **SOLUCIONADO**: Cierre instantáneo implementado
- Ctrl+C ahora cierra el servidor inmediatamente

### Logs y Debugging

#### Habilitar modo debug:
```bash
# Compilar en modo debug
make debug

# Los logs mostrarán más información detallada
```

#### Verificar conexiones activas:
```bash
# Ver conexiones del servidor
netstat -an | grep 8080

# Ver procesos del chat
ps aux | grep chat_
```

## 🔒 Seguridad y Limitaciones

### Consideraciones de Seguridad
- **Solo texto plano**: No hay encriptación (apropiado para LAN)
- **Sin autenticación**: Cualquiera puede conectarse
- **Límite de clientes**: Máximo 50 usuarios simultáneos
- **Validación básica**: Nombres de usuario simples

### Limitaciones Actuales
- Mensajes hasta 1KB por defecto
- No persistencia de mensajes
- No salas/canales separados
- Red local/confiable recomendada

## 🤝 Contribuciones

### Reportar Problemas
1. Describe el problema detalladamente
2. Incluye logs relevantes
3. Especifica SO y versión de GCC
4. Pasos para reproducir

### Mejoras Futuras
- [ ] Encriptación TLS/SSL
- [ ] Sistema de autenticación
- [ ] Múltiples salas de chat
- [ ] Persistencia de mensajes
- [ ] Interfaz gráfica
- [ ] Soporte para archivos/imágenes

## 📚 Documentación Técnica

- **Documentación completa**: Ver `docs/documentacion_tecnica.qmd`
- **Comentarios en código**: Estilo Doxygen
- **Standards**: GNU C99, POSIX threads

## 📄 Licencia

Este proyecto es de código abierto para fines educativos.

---

**Desarrollado para el curso de Redes de Computadoras**  
*Sistema robusto de chat TCP con arquitectura cliente-servidor*
===============================

[10:30:45] [INFO] Nueva conexión desde 127.0.0.1:54321
[10:30:45] [INFO] Cliente 'juan' agregado (total: 1/50)
```

### Terminal 2 (Cliente):
```bash
$ ./bin/chat_client juan
┌─────────────────────────────────────────────────────────────┐
│                    CLIENTE DE CHAT TCP                      │
│                                                             │
│  • Escriba mensajes y presione Enter para enviarlos         │
│  • Use /help para ver comandos disponibles                  │
│  • Use /quit para salir del chat                            │
└─────────────────────────────────────────────────────────────┘

[10:30:45] Conectado al chat. ¡Bienvenido!
> Hola a todos!
[10:30:55] <juan> Hola a todos!
```