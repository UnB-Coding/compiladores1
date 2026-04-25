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

Isso executará a regra padrão (`all`), que vai gerar os arquivos `.c` a partir do Flex e do Bison e compilará dois executáveis dentro da pasta `build/`:
- `lexer_exe`: Analisador léxico standalone.
- `parser_exe`: Parser de exemplo.

*(Nota: O Makefile utiliza a pasta `build/`, então certifique-se de que ela existe na raiz do projeto usando `mkdir build` caso o compilador acuse algum erro de pasta não encontrada).*

## 3. Como executar

Após a compilação, os executáveis estarão na pasta `build`. Para rodá-los no terminal, você pode passar um arquivo de código-fonte como entrada:

**Para executar o Analisador Léxico (Lexer):**
```bash
./build/lexer_exe 
```
*(No PowerShell: `Get-Content arquivo_de_teste.c | ./build/lexer_exe.exe`)*

**Para executar o Parser (Analisador Sintático):**
```bash
./build/parser_exe 
```
*(No PowerShell: `Get-Content arquivo_de_teste.c | ./build/parser_exe.exe`)*

## 4. Como limpar os arquivos gerados

Caso queira apagar os executáveis e os códigos em C gerados automaticamente pelo Flex/Bison para recompilar tudo do zero, basta rodar:

```bash
make clean
```
