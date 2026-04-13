# ======================================================================
# Makefile — Compila o Parser (Bison+Flex) e o Driver de Teste do Lexer
# ======================================================================

# --- Executáveis ---
PARSER_EXEC = parser_exe
LEXER_TEST_EXEC = lexer_test
BUILD_DIR   = build

# --- Fontes Bison/Flex ---
BISON_FILE = parser.y
FLEX_FILE  = scanner.l

# --- Arquivos gerados ---
BISON_C = $(BUILD_DIR)/parser.tab.c
BISON_H = $(BUILD_DIR)/parser.tab.h
FLEX_C  = $(BUILD_DIR)/lex.yy.c

# --- Compilador ---
CC      = gcc
CFLAGS  = -Wall
LDFLAGS = -lfl

# ======================================================================
# ALVOS PRINCIPAIS
# ======================================================================

# Compila tudo: parser + driver de teste
all: $(PARSER_EXEC) $(LEXER_TEST_EXEC)

# Parser completo (Bison + Flex)
$(PARSER_EXEC): $(BISON_C) $(FLEX_C)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) $(LDFLAGS)

# Driver de teste do Lexer (SEM o parser — substitui o main do Bison)
$(LEXER_TEST_EXEC): $(FLEX_C) $(BISON_H) lexer_driver.c
	$(CC) $(CFLAGS) -o $@ lexer_driver.c $(FLEX_C) $(LDFLAGS)

# ======================================================================
# REGRAS DE GERAÇÃO (Bison e Flex)
# ======================================================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BISON_C) $(BISON_H): $(BISON_FILE) | $(BUILD_DIR)
	bison --defines=$(BISON_H) -o $(BISON_C) $(BISON_FILE)

$(FLEX_C): $(FLEX_FILE) $(BISON_H) | $(BUILD_DIR)
	flex -o $(FLEX_C) $(FLEX_FILE)

# ======================================================================
# ATALHOS
# ======================================================================

# Compila só o driver de teste (usado pelo pytest)
test-driver: $(LEXER_TEST_EXEC)

# Roda os testes pytest (compila o driver e executa)
test: $(LEXER_TEST_EXEC)
	cd .. && LEXER_BIN=lexer/$(LEXER_TEST_EXEC) python -m pytest tests/lexer/tests/ -v

# Limpeza
clean:
	rm -rf $(BUILD_DIR) $(PARSER_EXEC) $(LEXER_TEST_EXEC)

.PHONY: all clean test test-driver
