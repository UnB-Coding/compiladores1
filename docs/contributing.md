# Contribuição

Este projeto utiliza padrões de commit e de branch para facilitar o desenvolvimento organizado em equipe.

## Convenções de commit

Seguimos o padrão [Conventional Commits](https://www.conventionalcommits.org/pt-br/).

### Formato

```text
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
| `refactor` | Refatoração sem correção de bug nem nova funcionalidade |
| `test` | Adição ou correção de testes |
| `chore` | Tarefas de manutenção, CI, build (sem mudança de código de produção) |
| `build` | Mudanças no sistema de build ou dependências externas |

### Escopos sugeridos

- `lexer`
- `parser`
- `tests`
- `build`
- `docs`

### Exemplos

```text
feat(lexer): adiciona reconhecimento de números de ponto flutuante
fix(parser): corrige precedência de operadores aritméticos
docs: atualiza README com instruções de compilação
test(lexer): adiciona casos de teste para identificadores
chore(build): configura Makefile inicial
```

## Convenções de branching

O modelo é baseado no **Git Flow** simplificado.

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
| `hotfix/<descricao>` | Correção urgente | `main` | `main` e `develop` |
| `release/<versao>` | Preparação de release | `develop` | `main` e `develop` |
| `docs/<descricao>` | Atualização de docs | `develop` | `develop` |

### Regras

1. **Nunca** realize commits diretamente em `main`.
2. Todo código em `main` deve ter passado por Pull Request (ao menos 1 aprovação).
3. Nomes de branches devem usar minúsculas, números e hífens.
4. Ao finalizar uma branch de feature/fix, exclua-a após o merge.
