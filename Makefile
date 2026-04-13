# Nome dos executáveis
EXEC       = parser_exe
LEXER_EXEC = lexer_exe
BUILD_DIR  = build

# Arquivos-fonte do Bison e do Flex (parser de exemplo)
BISON_FILE = parser.y
FLEX_FILE  = scanner.l

# Arquivo-fonte do Lexer standalone (subconjunto C)
LEXER_FILE = lexer/lexer.l

# Arquivos que o Bison vai gerar
BISON_C   = $(BUILD_DIR)/parser.tab.c
BISON_H   = $(BUILD_DIR)/parser.tab.h

# Arquivo gerado pelo Flex (parser de exemplo)
FLEX_C    = $(BUILD_DIR)/lex.yy.c

# Arquivo gerado pelo Flex (lexer standalone)
LEXER_C   = $(BUILD_DIR)/lexer.yy.c

# Parâmetros opcionais ao Bison e Flex
BISON_FLAGS =    # -d gera o arquivo .h (token definitions)
FLEX_FLAGS  =      # deixe vazio ou acrescente opções, se necessário

# Parâmetros de compilação
CC      = gcc
CFLAGS  =
LDFLAGS = -lfl     # biblioteca do Flex (em algumas distros, pode ser -ll)

# Regra padrão: compila ambos os executáveis
all: $(EXEC) $(LEXER_EXEC)


# ========================================================
# Lexer standalone (subconjunto C)
# ========================================================
$(LEXER_EXEC): $(LEXER_C)
	$(CC) $(CFLAGS) -o $@ $(LEXER_C) $(LDFLAGS)

$(LEXER_C): $(LEXER_FILE) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(LEXER_C) $(LEXER_FILE)


# ========================================================
# Parser de exemplo (expressões aritméticas)
# ========================================================
$(EXEC): $(BISON_C) $(FLEX_C)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) $(LDFLAGS)

# Cria diretório de build se não existir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Regra para rodar o Bison: gera parser.tab.c e parser.tab.h em build/
$(BISON_C) $(BISON_H): $(BISON_FILE) | $(BUILD_DIR)
	bison $(BISON_FLAGS) --defines=$(BISON_H) -o $(BISON_C) $(BISON_FILE)

# Regra para rodar o Flex: gera lex.yy.c em build/
$(FLEX_C): $(FLEX_FILE) $(BISON_H) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(FLEX_C) $(FLEX_FILE)


# ========================================================
# Limpeza
# ========================================================
clean:
	rm -f $(EXEC) $(LEXER_EXEC) $(BISON_C) $(BISON_H) $(FLEX_C) $(LEXER_C)