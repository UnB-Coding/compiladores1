# Nome dos executáveis
EXEC       = $(BUILD_DIR)/parser_exe
LEXER_EXEC = $(BUILD_DIR)/lexer_exe
BUILD_DIR  = build

# Arquivos-fonte do Bison e do Flex (parser de exemplo)
BISON_FILE = src/parser.y
FLEX_FILE  = src/scanner.l

# Arquivo-fonte do Lexer standalone (subconjunto C)
LEXER_FILE = examples/lexer.l

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
CFLAGS  = -I. -Isrc -Isymbol_table

# parser_exe: parser.y fornece main() e scanner.l fornece yywrap() → sem -lfl
LDFLAGS =

# lexer_exe: lexer.l não define main() → depende de -lfl para fornecê-la
LEXER_LDFLAGS = -lfl

# Regra padrão: compila ambos os executáveis
all: $(EXEC) $(LEXER_EXEC)

# Regra para criar a pasta build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ========================================================
# Lexer standalone (subconjunto C)
# ========================================================
$(LEXER_EXEC): $(LEXER_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(LEXER_C) $(LEXER_LDFLAGS)

$(LEXER_C): $(LEXER_FILE) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(LEXER_C) $(LEXER_FILE)


# ========================================================
# Parser de exemplo (expressões aritméticas)
# ========================================================
$(EXEC): $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c $(LDFLAGS)



# Regra para rodar o Bison: gera parser.tab.c e parser.tab.h em build/
$(BISON_C) $(BISON_H): $(BISON_FILE) | $(BUILD_DIR)
	bison $(BISON_FLAGS) --defines=$(BISON_H) -o $(BISON_C) $(BISON_FILE)

# Regra para rodar o Flex: gera lex.yy.c em build/
$(FLEX_C): $(FLEX_FILE) $(BISON_H) | $(BUILD_DIR)
	flex $(FLEX_FLAGS) -o $(FLEX_C) $(FLEX_FILE)


# ========================================================
# Executável de teste do lexer (usado pelo pytest)
# Compila sem -lfl pois tests/lexer_test_main.c fornece o main
# ========================================================
LEXER_TEST_EXEC = $(BUILD_DIR)/lexer_test_exe
LEXER_TEST_MAIN = tests/lexer_test_main.c

$(LEXER_TEST_EXEC): $(LEXER_C) $(LEXER_TEST_MAIN) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(LEXER_C) $(LEXER_TEST_MAIN)

lexer_test: $(LEXER_TEST_EXEC)

# ========================================================
# Limpeza
# ========================================================
clean:
	rm -f $(EXEC) $(LEXER_EXEC) $(LEXER_TEST_EXEC) $(BISON_C) $(BISON_H) $(FLEX_C) $(LEXER_C)
