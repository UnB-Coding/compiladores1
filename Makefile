# Nome do executável
EXEC       = $(BUILD_DIR)/parser_exe
BUILD_DIR  = build

# Arquivos-fonte do Bison e do Flex (parser de exemplo)
BISON_FILE = src/parser.y
FLEX_FILE  = src/scanner.l

# Arquivos que o Bison vai gerar
BISON_C   = $(BUILD_DIR)/parser.tab.c
BISON_H   = $(BUILD_DIR)/parser.tab.h

# Arquivo gerado pelo Flex (parser de exemplo)
FLEX_C    = $(BUILD_DIR)/lex.yy.c

# Parâmetros opcionais ao Bison e Flex
BISON_FLAGS =    # -d gera o arquivo .h (token definitions)
FLEX_FLAGS  =      # deixe vazio ou acrescente opções, se necessário

# Parâmetros de compilação
CC      = gcc
CFLAGS  = -I. -Isrc -Isymbol_table

# parser_exe: parser.y fornece main() e scanner.l fornece yywrap() → sem -lfl
LDFLAGS =

# Regra padrão: compila o interpretador
all: $(EXEC)

# Regra para criar a pasta build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# ========================================================
# Parser de exemplo (expressões aritméticas)
# ========================================================
$(EXEC): $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c src/semantic.c src/ir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symbol_table/symtab.c src/ast.c src/semantic.c src/ir.c $(LDFLAGS)



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
	rm -f $(EXEC) $(BISON_C) $(BISON_H) $(FLEX_C)
