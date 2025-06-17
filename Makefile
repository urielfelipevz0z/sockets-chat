# Makefile para el Sistema de Chat TCP Cliente-Servidor
# Autor: Sistema de Chat Socket
# Fecha: 2025
#
# Este Makefile compila el sistema completo de chat incluyendo servidor,
# cliente y bibliotecas comunes, siguiendo las mejores prácticas de GNU Make.

# ========== CONFIGURACIÓN DEL COMPILADOR ==========

# Compilador y flags base
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -D_GNU_SOURCE
LDFLAGS = -pthread

# Configuración de debug/release
DEBUG_FLAGS = -g -DDEBUG -O0
RELEASE_FLAGS = -O2 -DNDEBUG

# Flags adicionales de warnings (siguiendo estándares GNU)
WARNING_FLAGS = -Wformat=2 -Wno-unused-parameter -Wshadow \
                -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
                -Wredundant-decls -Wnested-externs -Wmissing-include-dirs

# ========== DIRECTORIOS ==========

SRCDIR = src
INCDIR = include
BINDIR = bin
OBJDIR = obj
DOCDIR = docs

# ========== ARCHIVOS FUENTE ==========

# Archivos fuente comunes
COMMON_SOURCES = $(SRCDIR)/chat_common.c
COMMON_OBJECTS = $(OBJDIR)/chat_common.o

# Archivos fuente del servidor
SERVER_SOURCES = $(SRCDIR)/chat_server.c
SERVER_OBJECTS = $(OBJDIR)/chat_server.o

# Archivos fuente del cliente
CLIENT_SOURCES = $(SRCDIR)/chat_client.c
CLIENT_OBJECTS = $(OBJDIR)/chat_client.o

# Todos los archivos objeto
ALL_OBJECTS = $(COMMON_OBJECTS) $(SERVER_OBJECTS) $(CLIENT_OBJECTS)

# ========== EJECUTABLES ==========

SERVER_EXEC = $(BINDIR)/chat_server
CLIENT_EXEC = $(BINDIR)/chat_client

# ========== REGLAS PRINCIPALES ==========

# Regla por defecto: compilar todo en modo release
all: CFLAGS += $(RELEASE_FLAGS)
all: directories $(SERVER_EXEC) $(CLIENT_EXEC)

# Compilación en modo debug
debug: CFLAGS += $(DEBUG_FLAGS) $(WARNING_FLAGS)
debug: directories $(SERVER_EXEC) $(CLIENT_EXEC)

# Compilación en modo release optimizado
release: CFLAGS += $(RELEASE_FLAGS)
release: directories $(SERVER_EXEC) $(CLIENT_EXEC)

# ========== REGLAS DE COMPILACIÓN ==========

# Compilar servidor
$(SERVER_EXEC): $(COMMON_OBJECTS) $(SERVER_OBJECTS)
	@echo "Enlazando servidor..."
	$(CC) $(COMMON_OBJECTS) $(SERVER_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Servidor compilado exitosamente: $@"

# Compilar cliente
$(CLIENT_EXEC): $(COMMON_OBJECTS) $(CLIENT_OBJECTS)
	@echo "Enlazando cliente..."
	$(CC) $(COMMON_OBJECTS) $(CLIENT_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Cliente compilado exitosamente: $@"

# Compilar archivos objeto comunes
$(OBJDIR)/chat_common.o: $(SRCDIR)/chat_common.c $(INCDIR)/chat_common.h
	@echo "Compilando módulo común..."
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Compilar archivos objeto del servidor
$(OBJDIR)/chat_server.o: $(SRCDIR)/chat_server.c $(INCDIR)/chat_server.h $(INCDIR)/chat_common.h
	@echo "Compilando servidor..."
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Compilar archivos objeto del cliente
$(OBJDIR)/chat_client.o: $(SRCDIR)/chat_client.c $(INCDIR)/chat_client.h $(INCDIR)/chat_common.h
	@echo "Compilando cliente..."
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# ========== REGLAS DE UTILIDAD ==========

# Crear directorios necesarios
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Limpiar archivos compilados
clean:
	@echo "Limpiando archivos compilados..."
	rm -rf $(OBJDIR)/*.o $(BINDIR)/* core *.core
	@echo "Limpieza completada."

# Limpiar todo incluyendo directorios
distclean: clean
	@echo "Limpieza completa..."
	rm -rf $(OBJDIR) $(BINDIR)

# Instalar ejecutables en el sistema (requiere permisos de administrador)
install: release
	@echo "Instalando ejecutables..."
	sudo cp $(SERVER_EXEC) /usr/local/bin/
	sudo cp $(CLIENT_EXEC) /usr/local/bin/
	sudo chmod +x /usr/local/bin/chat_server
	sudo chmod +x /usr/local/bin/chat_client
	@echo "Instalación completada."

# Desinstalar ejecutables del sistema
uninstall:
	@echo "Desinstalando ejecutables..."
	sudo rm -f /usr/local/bin/chat_server
	sudo rm -f /usr/local/bin/chat_client
	@echo "Desinstalación completada."

# ========== REGLAS DE TESTING Y DESARROLLO ==========

# Ejecutar servidor en modo de prueba
test-server: debug
	@echo "Iniciando servidor en modo de prueba..."
	$(SERVER_EXEC) 8080

# Ejecutar cliente en modo de prueba
test-client: debug
	@echo "Iniciando cliente en modo de prueba..."
	$(CLIENT_EXEC) usuario_test 127.0.0.1 8080

# Verificar sintaxis sin compilar
check:
	@echo "Verificando sintaxis..."
	$(CC) $(CFLAGS) $(WARNING_FLAGS) -I$(INCDIR) -fsyntax-only $(SRCDIR)/*.c
	@echo "Verificación de sintaxis completada."

# Análisis estático con herramientas adicionales (si están disponibles)
static-analysis:
	@echo "Ejecutando análisis estático..."
	@if command -v cppcheck > /dev/null; then \
		cppcheck --enable=all --std=c99 $(SRCDIR)/; \
	else \
		echo "cppcheck no disponible, saltando análisis estático."; \
	fi

# ========== REGLAS DE DOCUMENTACIÓN ==========

# Generar documentación con Doxygen (si está disponible)
docs:
	@echo "Generando documentación..."
	@if command -v doxygen > /dev/null; then \
		doxygen Doxyfile; \
	else \
		echo "Doxygen no disponible. Instale doxygen para generar documentación."; \
	fi

# ========== INFORMACIÓN Y AYUDA ==========

# Mostrar información del proyecto
info:
	@echo "========== INFORMACIÓN DEL PROYECTO =========="
	@echo "Proyecto: Sistema de Chat TCP Cliente-Servidor"
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Directorios:"
	@echo "  - Fuentes: $(SRCDIR)"
	@echo "  - Headers: $(INCDIR)"
	@echo "  - Objetos: $(OBJDIR)"
	@echo "  - Binarios: $(BINDIR)"
	@echo "Ejecutables:"
	@echo "  - Servidor: $(SERVER_EXEC)"
	@echo "  - Cliente: $(CLIENT_EXEC)"
	@echo "=============================================="

# Mostrar ayuda de comandos disponibles
help:
	@echo "========== COMANDOS DISPONIBLES =========="
	@echo "Compilación:"
	@echo "  make all      - Compilar todo (modo release)"
	@echo "  make debug    - Compilar con información de debug"
	@echo "  make release  - Compilar optimizado para producción"
	@echo ""
	@echo "Limpieza:"
	@echo "  make clean    - Limpiar archivos compilados"
	@echo "  make distclean- Limpiar todo incluyendo directorios"
	@echo ""
	@echo "Instalación:"
	@echo "  make install  - Instalar en el sistema"
	@echo "  make uninstall- Desinstalar del sistema"
	@echo ""
	@echo "Testing:"
	@echo "  make test-server - Ejecutar servidor de prueba"
	@echo "  make test-client - Ejecutar cliente de prueba"
	@echo "  make check       - Verificar sintaxis"
	@echo ""
	@echo "Documentación:"
	@echo "  make docs     - Generar documentación"
	@echo ""
	@echo "Utilidades:"
	@echo "  make info     - Mostrar información del proyecto"
	@echo "  make help     - Mostrar esta ayuda"
	@echo "========================================="

# ========== CONFIGURACIÓN ESPECIAL ==========

# Prevenir eliminación de archivos intermedios
.PRECIOUS: $(ALL_OBJECTS)

# Reglas que no corresponden a archivos
.PHONY: all debug release clean distclean install uninstall \
        test-server test-client check static-analysis docs \
        info help directories

# Variables de entorno para debugging
ifdef VERBOSE
    CFLAGS += -v
endif

ifdef PROFILE
    CFLAGS += -pg
    LDFLAGS += -pg
endif

# ========== DEPENDENCIAS AUTOMÁTICAS ==========

# Incluir dependencias automáticas si existen
-include $(ALL_OBJECTS:.o=.d)

# Generar dependencias automáticamente
$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	@$(CC) -MM -I$(INCDIR) $< | sed 's,\($*\)\.o[ :]*,$(OBJDIR)/\1.o $@ : ,g' > $@
