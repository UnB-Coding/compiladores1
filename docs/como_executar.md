# Como executar o projeto

Este projeto utiliza um `Makefile` para automatizar a geração e compilação do interpretador usando **Flex**, **Bison** e **GCC**. 

Como você pode estar utilizando Windows, precisará de um ambiente que suporte essas ferramentas (como WSL, MSYS2 ou MinGW) ou rodar tudo através de um terminal configurado com elas.

## 1. Pré-requisitos

Certifique-se de ter as seguintes ferramentas instaladas e configuradas no seu `PATH`:
- `make` (para ler o `Makefile`)
- `gcc` (compilador C)
- `flex` (gerador de analisador léxico)
- `bison` (gerador de analisador sintático)

## 2. Como compilar

Abra o seu terminal na pasta raiz do projeto (onde está o `Makefile`) e execute:

```bash
make
```

Isso executará a regra padrão (`all`), que vai gerar os arquivos `.c` a partir do Flex e do Bison e compilará o executável dentro da pasta `build/`:
- `parser_exe`: O interpretador principal.

*(Nota: O Makefile cria a pasta `build/` automaticamente; caso o compilador acuse erro de pasta não encontrada, crie-a com `mkdir build`.)*

## 3. Como executar

Após a compilação, o executável estará na pasta `build`. Para executar o interpretador, passe um arquivo de código-fonte C como argumento:

```bash
./build/parser_exe caminho/para/arquivo.c
```

Por exemplo, usando o arquivo de teste de otimização incluso:
```bash
./build/parser_exe examples/teste_otimizacao.c
```

O interpretador irá:
1. Imprimir a **AST Gerada** (árvore sintática abstrata).
2. Executar a **análise semântica** (reportando erros ou avisos).
3. Imprimir o **Código Intermediário (TAC)** original.
4. Imprimir o **IR Otimizado** (após as passes de otimização).
5. **Executar** o programa e exibir os valores declarados/atribuídos.
6. Imprimir a **Tabela de Símbolos** final.

## 4. Como limpar os arquivos gerados

Caso queira apagar os executáveis e os códigos em C gerados automaticamente pelo Flex/Bison para recompilar tudo do zero, basta rodar:

```bash
make clean
```

## 5. Como rodar os testes

Para executar a suíte automatizada de testes com `pytest`:

```bash
pip install -r requirements-test.txt
pytest
```

Os testes compilam o binário automaticamente via fixture e validam scanner, parser, geração de IR e otimizações.
