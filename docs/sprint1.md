# Resumo da Sprint 1

**Período:** 01/04 a 13/04  
**Objetivo Principal:** Estabelecer a infraestrutura do projeto e concluir a análise léxica completa.

## Visão Geral das Atividades

A primeira sprint focou na organização do ambiente de desenvolvimento e na definição formal da linguagem a ser processada. O trabalho foi dividido entre infraestrutura, análise gramatical, desenvolvimento do Lexer e documentação técnica.

### Status das Tarefas

| ID | Tarefa | Responsável | Tipo | Estimativa | Status |
|:---|:---|:---|:---:|:---:|:---:|
| S1-01 | Definição da estrutura do repositório (Monorepo, MkDocs, Doxygen e Política de Commit) | Gabriel | Infra | 2d | Concluído |
| S1-02 | Definição do subconjunto da gramática de C suportada (BNF/EBNF) | Gabriel + Pedro | Análise | 3d | Concluído |
| S1-03 | Implementação do Lexer (Tokenização de palavras-chave, operadores, literais e identificadores) | Caio + Hauedy | Dev | 5d | Concluído |
| S1-04 | Escrita de testes unitários do Lexer e casos de teste | Vitor + Gabriel | Teste | 4d | Concluído |
| S1-05 | Documentação da especificação dos tokens e gramática suportada | Pedro | Doc | 2d | Concluído |
| S1-06 | Redação do texto resumo da sprint para o MkDocs | Pedro | Doc | 1d | Concluído |

## Contribuições por Membro

### Gabriel
Liderou a estruturação inicial do repositório e a configuração de ferramentas de documentação (Doxygen). Colaborou na definição da gramática EBNF e na criação da suíte de testes unitários para garantir a estabilidade do Lexer.

### Pedro
Responsável pela formalização documental. Trabalhou na definição do subconjunto da gramática C, detalhou a especificação técnica de cada token suportado e consolidou o relatório final de progresso da sprint.

### Caio & Hauedy
Atuaram no núcleo de desenvolvimento (`Dev`), implementando a lógica de tokenização via Flex. O trabalho resultou em um analisador capaz de reconhecer palavras-chave, operadores aritméticos/relacionais e identificadores conforme o subconjunto definido.

### Vitor
Focou na qualidade do software, desenvolvendo casos de teste específicos para validar se o Lexer identifica corretamente caracteres válidos e aponta erros em entradas malformadas.

## Detalhes Técnicos e Infraestrutura

O projeto foi organizado em um modelo **monorepo**, facilitando a integração entre o código-fonte do compilador e a documentação técnica. 
- **Documentação:** Centralizada no MkDocs.
- **Análise Léxica:** Implementada com suporte à ferramenta Flex.
- **Versionamento:** Política de commits padronizada para manter o histórico organizado.

---