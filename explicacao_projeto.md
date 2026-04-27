# Entendendo o Projeto do Compilador

Este documento foi escrito para que qualquer pessoa, mesmo sem conhecimento avançado em engenharia de software, possa entender o que é este projeto, para que serve e como cada parte dele funciona.

---

## 1. O que é um Compilador?

Imagine que você quer pedir para um cozinheiro estrangeiro, que só entende uma língua muito específica (vamos chamá-la de "Linguagem de Máquina"), fazer um bolo. Você, porém, só sabe falar português (ou, neste caso, uma "Linguagem de Programação" como a linguagem C).

O **compilador** é o tradutor. Ele lê as instruções que você escreveu na linguagem de programação e as transforma na linguagem de máquina para que o computador consiga executar as tarefas.

Neste projeto, estamos construindo o nosso próprio compilador!

---

## 2. Como o nosso Compilador funciona? (O Passo a Passo)

A tradução de um código para linguagem de máquina não acontece de uma vez. Ela é dividida em várias etapas. O nosso projeto está organizado exatamente de acordo com essas etapas.

### Passo 1: O Analisador Léxico (Scanner)
**Onde encontrar no projeto:** Pasta `src/`, arquivo `scanner.l`

Pense no Analisador Léxico como alguém lendo um texto palavra por palavra e dizendo qual é a classe gramatical de cada uma (substantivo, verbo, adjetivo). 
No código, ele pega o texto que o programador escreveu e o divide em "pedacinhos" chamados **Tokens**.
- Se ele lê `int`, ele diz: "Isso é um tipo de dado".
- Se ele lê `=`, ele diz: "Isso é um sinal de atribuição".
- Se ele lê `42`, ele diz: "Isso é um número".

Para fazer isso de forma automática, usamos uma ferramenta chamada **Flex**.

### Passo 2: O Analisador Sintático (Parser)
**Onde encontrar no projeto:** Pasta `src/`, arquivo `parser.y`

Depois de separar as "palavras" (tokens), precisamos ver se a frase faz sentido. O Analisador Sintático é o professor de gramática. Ele pega os tokens e verifica se eles seguem as regras da linguagem.
Por exemplo, na nossa linguagem, escrever `x = 10;` é correto, mas escrever `10 = x;` está errado gramaticalmente. O Parser detecta isso.

Neste projeto, usamos uma ferramenta chamada **Bison** para nos ajudar a criar essas regras gramaticais.

### Passo 3: A Árvore Sintática Abstrata (AST)
**Onde encontrar no projeto:** Pasta `src/`, arquivos `ast.c` e `ast.h`

Se a frase está correta, o compilador a transforma em um "mapa mental" ou um "diagrama em árvore", chamado de AST. 
Isso ajuda o compilador a entender a ordem correta das operações. Por exemplo, na conta `2 + 3 * 4`, a árvore garante que a multiplicação (`3 * 4`) seja resolvida antes da adição, seguindo as regras da matemática.

### Passo 4: A Tabela de Símbolos
**Onde encontrar no projeto:** Pasta `symbol_table/`

Imagine a Tabela de Símbolos como a "memória de curto prazo" ou o caderninho de anotações do compilador. 
Quando o programador cria uma variável (por exemplo, guarda o valor `10` na variável `idade`), o compilador anota na Tabela de Símbolos:
- Nome: `idade`
- Tipo: `inteiro` (int)
- Valor: `10`

Assim, se mais tarde o programa pedir para imprimir a `idade`, o compilador olha no caderninho e sabe que o valor é `10`. Nosso projeto suporta números inteiros (`int`), números com vírgula (`float`), letras (`char`) e verdadeiro/falso (`bool`).

---

## 3. Como garantimos que funciona? (Testes)
**Onde encontrar no projeto:** Pasta `tests/`

Como sabemos se o nosso tradutor não está traduzindo "Bom dia" para "Boa noite"? Usando testes!
A pasta de testes contém vários arquivos de código ("provas") que já sabemos qual deve ser o resultado correto. 
Usamos a linguagem Python (através de uma ferramenta chamada `pytest`) para enviar esses arquivos para o nosso compilador e verificar se ele acerta as respostas. Se ele errar, sabemos que temos um bug (erro) para corrigir.

---

## 4. O Sistema de Montagem (Build)
**Onde encontrar no projeto:** Arquivo `Makefile`

Um compilador é feito de vários pequenos arquivos de código. O arquivo `Makefile` é como se fosse um manual de instruções para uma fábrica de montagem. Quando dizemos ao computador para "rodar o make", ele lê o `Makefile` e junta todas as peças (`scanner.l`, `parser.y`, `ast.c`, etc.) para criar o programa final (o compilador pronto para uso). Os arquivos finais são guardados na pasta `build/`.

---

## 5. Funcionalidades Implementadas (O que a nossa linguagem faz?)

Aqui estão as funcionalidades que o nosso compilador suporta, como elas funcionam nos bastidores (back-end) e como os arquivos interagem para torná-las possíveis:

### Tipos de Dados Suportados
- **O que faz:** O compilador entende números inteiros (`int`), números com vírgula (`float`), caracteres únicos (`char` - ex: 'a') e valores verdadeiro/falso (`bool`).
- **Como funciona no back-end:** 
  - O `src/scanner.l` reconhece as palavras-chave (como `int` ou `float`) e literais (como `3.14` ou `'x'`).
  - A Tabela de Símbolos (`symbol_table/symtab.h` e `symtab.c`) reserva um espaço na "memória" capaz de guardar qualquer um desses tipos, usando uma estrutura chamada `union` (que permite que uma mesma variável guarde diferentes formatos de dados de forma eficiente).

### Operações Matemáticas
- **O que faz:** Suporta soma (`+`), subtração (`-`), multiplicação (`*`), divisão (`/`) e o sinal de negativo antes de números (`-`).
- **Como funciona no back-end:** O `src/parser.y` garante que a ordem das operações respeite as regras matemáticas (por exemplo, `*` e `/` são resolvidos antes de `+` e `-`). Quando essas operações são lidas, o compilador cria "nós de operação binária" (`AST_BINOP`) ou "nós de operação unária" (`AST_UNARYOP`) na nossa Árvore Sintática (`src/ast.c`). O cálculo real só acontece mais tarde, quando a árvore é percorrida e avaliada.

### Operações Relacionais e Lógicas
- **O que faz:** Permite comparar valores (maior `>`, menor `<`, igual `==`, diferente `!=`, maior ou igual `>=`, menor ou igual `<=`) e juntar condições usando "E" (`&&`), "OU" (`||`) e "NÃO" (`!`).
- **Como funciona no back-end:** Assim como nas matemáticas, essas operações são validadas pelo `parser.y` e transformadas em nós na Árvore Sintática (`ast.c`). Ao serem avaliadas pela função `eval_ast()`, elas resultam em valores booleanos (1 para verdadeiro ou 0 para falso), que são cruciais para o controle de fluxo.

### Controle de Fluxo (Decisões e Repetições)
- **O que faz:** A linguagem permite o uso de condicionais (`if` e `else`) e laços de repetição (`while` e `for`).
- **Como funciona no back-end:**
  - O `scanner.l` lê os comandos de fluxo como tokens.
  - O `parser.y` agrupa o "teste" (ex: `idade > 18`) e o "bloco de código" que deve ser executado se o teste passar, gerando regras de montagem sem executar o código ainda.
  - A Árvore Sintática (`ast.c`) cria nós específicos (`AST_IF`, `AST_WHILE`, `AST_FOR`). Na hora da execução, o avaliador lê o resultado da condição. Se for verdadeiro, ele entra no bloco de código associado; se for falso, ele pula o bloco. Nos laços (`while` e `for`), o avaliador repete esse processo continuamente até a condição virar falsa.

### Blocos de Código
- **O que faz:** Permite agrupar vários comandos usando chaves `{ e }`. Tudo que está dentro das chaves é tratado como um único bloco.
- **Como funciona no back-end:** O `parser.y` empacota todos os comandos de dentro das chaves em uma "lista encadeada". Na árvore (`ast.c`), isso vira um nó chamado `AST_BLOCK`, que simplesmente diz ao avaliador: "execute toda esta lista de comandos em sequência até as chaves se fecharem".

### Variáveis (Declaração e Atribuição)
- **O que faz:** O usuário pode criar variáveis (ex: `int x;`) e dar valores a elas (`x = 10;`), ou fazer tudo na mesma linha (`int x = 10;`).
- **Como funciona no back-end:** 
  - O `parser.y` identifica essas ações criando nós `AST_DECL` (para criar variáveis) e `AST_ASSIGN` (para atualizar o valor das variáveis).
  - Quando o programa é efetivamente executado pelo `ast.c`, ele "conversa" diretamente com a Tabela de Símbolos (`symtab.c`). Se for uma declaração, a função `sym_set()` adiciona o nome na tabela e reserva a memória para o tipo certo. Se for uma atribuição, a árvore usa o nome para encontrar a gavetinha certa na Tabela de Símbolos e atualiza o valor guardado lá.

---

## Resumo das Pastas do Projeto

- `src/`: O coração do projeto. Onde está o código do Scanner, Parser e Árvore (AST).
- `symbol_table/`: O código responsável pela "memória" do compilador (variáveis e valores).
- `tests/`: Onde ficam as provas/testes para garantir que tudo funciona.
- `docs/`: Onde guardamos os arquivos de documentação mais técnicos do projeto.
- `build/`: Pasta gerada automaticamente. É para onde vai o compilador depois de "montado".

## 6. Particularidades e Diferenças em Relação ao C Padrão

Embora nossa linguagem seja fortemente inspirada na linguagem C, este projeto faz algumas adaptações e simplificações interessantes no *back-end*. Se você já tem alguma familiaridade com engenharia de software ou C, aqui estão as principais diferenças técnicas que implementamos:

### 1. Interpretação Direta da Árvore (AST) vs. Compilação
- **C Padrão:** Um compilador C tradicional transforma o seu código em um arquivo executável de "linguagem de máquina" ou linguagem de montagem (Assembly), que o processador do computador roda diretamente (ex: um `.exe`).
- **Nosso Projeto:** Tecnicamente, o nosso compilador funciona como um **Interpretador Baseado em AST**. Ele lê o código, monta a Árvore Sintática (`parser.y`) e **executa as instruções imediatamente para fins de testagem**, percorrendo a árvore através da função `eval_ast()` no arquivo `ast.c`. Ele ainda não gera um arquivo binário final do código que o usuário escreveu. Essa função será implementada no futuro.

### 2. O Tipo `bool` Nativo
- **C Padrão:** Em C clássico (antes do padrão C99), não existe o tipo `bool` de forma nativa (você precisava incluir a biblioteca `<stdbool.h>`). Verdadeiro ou falso eram apenas números (1 e 0).
- **Nosso Projeto:** Nós adicionamos `bool` como uma palavra-chave integrada, junto com os valores `true` (verdadeiro) e `false` (falso). O `scanner.l` os reconhece diretamente, e a Tabela de Símbolos trata `bool` como uma categoria independente, tornando a linguagem mais moderna.

### 3. Impressão Automática (Comportamento REPL)
- **C Padrão:** Se você faz uma conta como `2 + 2;` ou atribui um valor `x = 10;`, o computador faz a operação na memória de forma silenciosa. Para ver o resultado na tela, você é obrigado a usar funções de saída como o `printf()`.
- **Nosso Projeto:** Para facilitar a visualização e os testes, projetamos a AST (`ast.c`) de forma interativa. Sempre que você declara uma variável, faz uma atribuição ou escreve uma expressão matemática solta com ponto e vírgula (`10 + 5;`), o nosso avaliador imprime o resultado automaticamente no terminal.

### 4. Matemática Interna Unificada
- **C Padrão:** Os cálculos matemáticos são feitos estritamente em seus formatos nativos na memória. Uma soma de dois inteiros (`int`) usa instruções de processador exclusivas para inteiros.
- **Nosso Projeto:** Para simplificar a lógica do avaliador em `ast.c`, a função `eval_ast()` foi programada para resolver **todas** as contas utilizando variáveis temporárias do tipo `double` (que suportam números gigantes e com muitas casas decimais). Somente no final, quando o resultado precisa ser guardado na Tabela de Símbolos (`symtab.c`), o sistema converte esse `double` de volta para o formato exato da variável (por exemplo, cortando as casas decimais se for um `int`).

### 5. Sem Necessidade de `#include`
- **C Padrão:** Em C, para usar qualquer recurso básico (como imprimir coisas na tela ou lidar com textos), você precisa incluir bibliotecas no topo do arquivo (ex: `#include <stdio.h>`).
- **Nosso Projeto:** Como nossa linguagem já traz as operações básicas e a impressão automática construídas diretamente no núcleo (na lógica de execução da AST), não existe a necessidade de importar arquivos e bibliotecas externas.

## Conclusão

Construir um compilador é como criar a ponte entre o pensamento humano e a máquina. Cada pasta e arquivo neste projeto tem uma função específica: o Lexer (`scanner.l`) reconhece as palavras, o Parser (`parser.y`) verifica a gramática, a AST (`ast.c`) mapeia e executa a lógica em tempo real (para fins de testagem), e a Tabela de Símbolos (`symtab.c`) atua como a memória principal. Tudo isso trabalha em conjunto para garantir que o texto escrito pelo programador vire ação de forma segura e confiável.
