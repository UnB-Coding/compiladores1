# Como executar o projeto

Este projeto utiliza um `Makefile` para automatizar a geração e compilação de analisadores léxicos e sintáticos usando **Flex**, **Bison** e **GCC**. 

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

Isso executará a regra padrão (`all`), que vai gerar os arquivos `.c` a partir do Flex e do Bison e compilará **um único executável** dentro da pasta `build/`:
- `parser_exe`: o interpretador completo (léxico → sintático → semântico → IR → execução).

*(Nota: O Makefile cria a pasta `build/` automaticamente; caso o compilador acuse erro de pasta não encontrada, crie-a com `mkdir build`.)*

## 3. Como executar

Após a compilação, o executável estará em `build/parser_exe`. Ele lê o código-fonte da **entrada padrão** (`stdin`) — ou de um arquivo passado como argumento — e imprime a AST, o TAC (antes e depois da otimização), a saída do programa e a tabela de símbolos final.

**Passando o código por `stdin`:**
```bash
echo 'int x = 1 + 2;' | ./build/parser_exe
```

**Lendo de um arquivo:**
```bash
./build/parser_exe arquivo_de_teste.c
# ou, redirecionando a entrada:
./build/parser_exe < arquivo_de_teste.c
```
*(No PowerShell: `Get-Content arquivo_de_teste.c | ./build/parser_exe.exe`)*

## 4. Como limpar os arquivos gerados

Caso queira apagar os executáveis e os códigos em C gerados automaticamente pelo Flex/Bison para recompilar tudo do zero, basta rodar:

```bash
make clean
```
