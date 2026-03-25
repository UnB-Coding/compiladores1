# compiladores1

Repositório para um projeto de compilador utilizando **flex** (analisador léxico) e **bison** (analisador sintático).

---

## Estrutura do projeto

```
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
| `build/` | Destino dos artefatos gerados pela compilação (objetos, executáveis). O conteúdo desta pasta é ignorado pelo Git. |

---

## Pré-requisitos

- `flex` ≥ 2.6
- `bison` ≥ 3.0
- `gcc` / `g++`
- `make`

---

## Como compilar

```bash
# A partir da raiz do projeto
make          # compila tudo em build/
make clean    # remove os artefatos
```

---

## Convenções de commit

Este projeto segue o padrão [Conventional Commits](https://www.conventionalcommits.org/pt-br/).

### Formato

```
<tipo>(<escopo>): <descrição curta>

[corpo opcional]

[rodapé(s) opcional(is)]
```

### Tipos permitidos

| Tipo | Quando usar |
|------|-------------|
| `feat` | Adição de nova funcionalidade |
| `fix` | Correção de bug |
| `docs` | Apenas alterações na documentação |
| `style` | Formatação, ponto e vírgula, espaçamento (sem mudança de lógica) |
| `refactor` | Refatoração de código sem correção de bug nem nova funcionalidade |
| `test` | Adição ou correção de testes |
| `chore` | Tarefas de manutenção, CI, build (sem mudança de código de produção) |
| `build` | Mudanças no sistema de build ou dependências externas |

### Escopos sugeridos

`lexer`, `parser`, `tests`, `build`, `docs`

### Exemplos

```
feat(lexer): adiciona reconhecimento de números de ponto flutuante
fix(parser): corrige precedência de operadores aritméticos
docs: atualiza README com instruções de compilação
test(lexer): adiciona casos de teste para identificadores
chore(build): configura Makefile inicial
```

---

## Convenções de branching

Este projeto adota um modelo baseado no **Git Flow** simplificado.

### Branches principais

| Branch | Descrição |
|--------|-----------|
| `main` | Código estável, pronto para entrega. Nenhum commit direto. |
| `develop` | Integração contínua das funcionalidades em desenvolvimento. |

### Branches de suporte

| Prefixo | Propósito | Criada a partir de | Merge em |
|---------|-----------|-------------------|----------|
| `feature/<descricao>` | Nova funcionalidade | `develop` | `develop` |
| `fix/<descricao>` | Correção de bug | `develop` | `develop` |
| `hotfix/<descricao>` | Correção urgente em produção | `main` | `main` e `develop` |
| `release/<versao>` | Preparação de uma release | `develop` | `main` e `develop` |
| `docs/<descricao>` | Atualização de documentação | `develop` | `develop` |

### Regras

1. **Nunca** realize commits diretamente em `main`.
2. Todo código em `main` deve ter passado por Pull Request com ao menos uma aprovação.
3. Nomes de branches devem usar letras minúsculas, números e hífens (sem espaços ou underscores).
4. Ao finalizar uma branch de feature/fix, exclua-a após o merge.

### Exemplos de nomes de branch

```
feature/lexer-numeros-float
fix/parser-precedencia-operadores
hotfix/corrige-crash-entrada-vazia
release/1.0.0
docs/atualiza-convencoes
```
