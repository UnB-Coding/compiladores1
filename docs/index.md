# Compiladores 1

Bem-vindo à documentação do projeto de compilador! Este projeto utiliza **flex** para análise léxica e **bison** para análise sintática.

## Estrutura do Projeto

```text
compiladores1/
├── lexer/      # Analisador léxico (flex / lex)
├── parser/     # Analisador sintático (bison / yacc)
├── tests/      # Casos de teste
├── build/      # Artefatos compilados (gerado durante o build)
└── README.md
```

| Pasta | Descrição |
|-------|-----------|
| `lexer/` | Contém o arquivo `.l` (flex) responsável pela análise léxica. |
| `parser/` | Contém o arquivo `.y` (bison) responsável pela análise sintática. |
| `tests/` | Contém os casos de teste para validar o compilador. |
| `build/` | Destino dos artefatos gerados pela compilação. Ignorado pelo Git. |

## Pré-requisitos

- **flex** ≥ 2.6
- **bison** ≥ 3.0
- **gcc** / **g++**
- **make**

## Como compilar

A partir da raiz do projeto:

```bash
make          # compila tudo em build/
make clean    # remove os artefatos
```
