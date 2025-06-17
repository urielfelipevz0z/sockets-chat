# Sistema de Chat TCP Cliente-Servidor

Un sistema completo de chat en tiempo real implementado en C utilizando sockets TCP, diseñado para manejar múltiples clientes concurrentes con arquitectura basada en threads.

## 🚀 Características Principales

- **Servidor TCP multihilo** que soporta hasta 50 clientes concurrentes
- **Cliente con interfaz de terminal** intuitiva y fácil de usar
- **Comunicación bidireccional** en tiempo real
- **Notificaciones automáticas** de conexión y desconexión de usuarios
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
# Clonar/descargar el proyecto
cd /ruta/al/proyecto/sockets

# Compilar (modo release)
make all

# Verificar archivos generados
ls -la bin/
# Debería mostrar: chat_server y chat_client
```

### Modos de Compilación

| Comando | Descripción |
|---------|-------------|
| `make all` | Compilación estándar (release) |
| `make debug` | Compilación con símbolos de debug |
| `make release` | Compilación optimizada para producción |
| `make clean` | Limpiar archivos compilados |

## 🎯 Uso Rápido

### Iniciar el Servidor
```bash
# Puerto por defecto (8080)
./bin/chat_server

# Puerto específico
./bin/chat_server 9090
```

### Conectar Cliente
```bash
# Conexión básica (servidor local)
./bin/chat_client mi_usuario

# Conexión a servidor específico
./bin/chat_client juan 192.168.1.100 8080
```

### Comandos del Cliente
| Comando | Descripción |
|---------|-------------|
| `/help` | Mostrar ayuda |
| `/quit` | Salir del chat |
| `/status` | Ver estado de conexión |

## 💡 Ejemplo de Uso

### Terminal 1 (Servidor):
```bash
$ ./bin/chat_server 8080
[10:30:15] [INFO] Servidor iniciado correctamente. Esperando conexiones...

=== ESTADÍSTICAS DEL SERVIDOR ===
Estado: Ejecutándose
Clientes conectados: 0/50
Puerto: 8080
===============================

[10:30:45] [INFO] Nueva conexión desde 127.0.0.1:54321
[10:30:45] [INFO] Cliente 'juan' agregado (total: 1/50)
```

### Terminal 2 (Cliente):
```bash
$ ./bin/chat_client juan
┌─────────────────────────────────────────────────────────────┐
│                    CLIENTE DE CHAT TCP                     │
│                                                             │
│  • Escriba mensajes y presione Enter para enviarlos        │
│  • Use /help para ver comandos disponibles                 │
│  • Use /quit para salir del chat                           │
└─────────────────────────────────────────────────────────────┘

[10:30:45] Conectado al chat. ¡Bienvenido!
> Hola a todos!
[10:30:55] <juan> Hola a todos!
```

## 🏗️ Arquitectura del Sistema

### Estructura del Proyecto
```
sockets/
├── src/                    # Código fuente
│   ├── chat_common.c      # Funciones comunes
│   ├── chat_server.c      # Implementación del servidor
│   └── chat_client.c      # Implementación del cliente
├── include/                # Archivos header
│   ├── chat_common.h      # Definiciones comunes
│   ├── chat_server.h      # Definiciones del servidor
│   └── chat_client.h      # Definiciones del cliente
├── bin/                    # Ejecutables compilados
├── docs/                   # Documentación detallada
├── Makefile               # Sistema de compilación
└── README.md              # Este archivo
```

### Modelo de Concurrencia

**Servidor**: Utiliza un thread por cliente (thread-per-client model)
- Thread principal acepta conexiones
- Cada cliente maneja en thread separado
- Sincronización con mutexes para operaciones compartidas

**Cliente**: Utiliza dos threads concurrentes
- Thread de recepción: Maneja mensajes del servidor
- Thread de entrada: Procesa input del usuario

## 🔧 Decisiones Técnicas

### ¿Por qué Threads en lugar de Fork?

| Aspecto | Threads | Fork |
|---------|---------|------|
| **Overhead** | Bajo | Alto |
| **Memoria** | Compartida | Duplicada |
| **Comunicación** | Directa | Requiere IPC |
| **Escalabilidad** | Excelente (50 clientes) | Limitada |

### Protocolo de Comunicación

- **Formato**: Estructura binaria serializada
- **Tipos de mensaje**: CONNECT, CHAT, NOTIFICATION, DISCONNECT, ERROR
- **Tamaño fijo**: 980 bytes por mensaje
- **Thread-safe**: Operaciones atómicas con mutexes

## 📊 Capacidades y Límites

| Característica | Valor | Configurable |
|----------------|-------|--------------|
| Clientes máximos | 50 | Sí (recompilación) |
| Tamaño de mensaje | 928 bytes | Sí (recompilación) |
| Nombre de usuario | 31 caracteres | Sí (recompilación) |
| Puerto por defecto | 8080 | Sí (línea de comandos) |

## 🐛 Resolución de Problemas

### Problemas Comunes

**Error: "Connection refused"**
```bash
# Verificar que el servidor esté ejecutándose
ps aux | grep chat_server

# Verificar puerto
netstat -ln | grep 8080
```

**Error: "Address already in use"**
```bash
# Encontrar proceso usando el puerto
sudo lsof -i :8080

# Terminar proceso anterior
kill -9 <PID>
```

**Error: "Invalid username"**
- Solo usar letras, números y guión bajo (_)
- Máximo 31 caracteres

### Debugging

```bash
# Compilar en modo debug
make debug

# Ejecutar con GDB
gdb ./bin/chat_server
(gdb) run 8080
```

## 📚 Documentación Completa

Para documentación técnica detallada, arquitectura del sistema, y ejemplos avanzados, consulte:

- [`docs/documentacion_tecnica.qmd`](docs/documentacion_tecnica.qmd) - Documentación completa en formato Quarto

### Temas Cubiertos en la Documentación

- ✅ Arquitectura detallada del sistema
- ✅ Diagramas de flujo de comunicación  
- ✅ Análisis de concurrencia y thread safety
- ✅ Protocolo de comunicación completo
- ✅ Manual de compilación paso a paso
- ✅ Guía de resolución de problemas
- ✅ Casos de uso avanzados
- ✅ Estándares de código y mejores prácticas

## 🧪 Testing

### Test Básico
```bash
# Terminal 1: Servidor
make test-server

# Terminal 2: Cliente 1
make test-client

# Terminal 3: Cliente 2  
./bin/chat_client usuario2 127.0.0.1 8080
```

### Test de Carga (Script de ejemplo)
```bash
#!/bin/bash
# Conectar múltiples clientes para testing
for i in {1..10}; do
    ./bin/chat_client "user$i" 127.0.0.1 8080 &
done
wait
```

## 🤝 Contribuciones

Este proyecto sigue las mejores prácticas de desarrollo en C:

- **Estilo de código**: GNU C Standards
- **Documentación**: Comentarios completos en español
- **Testing**: Pruebas manuales y casos edge cubiertos
- **Portabilidad**: Compatible con distribuciones Linux modernas

## 📄 Licencia

Proyecto educativo - Sistema de Chat Socket 2025

## 🎯 Casos de Uso

- **Desarrollo**: Chat para equipos de desarrollo local
- **Educación**: Ejemplo de programación de sockets en C
- **Prototipado**: Base para aplicaciones de chat más complejas
- **Testing**: Herramienta para probar redes y conectividad

## 📞 Soporte

Para problemas técnicos:

1. Consultar la [documentación completa](docs/documentacion_tecnica.qmd)
2. Verificar los [problemas comunes](#-resolución-de-problemas)
3. Revisar los logs del sistema con modo debug

---

**Desarrollado con** ❤️ **usando C, sockets TCP y threads POSIX**
