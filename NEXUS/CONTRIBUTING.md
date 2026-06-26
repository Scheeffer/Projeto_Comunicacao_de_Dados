# Como contribuir

Padrão de trabalho do repositório.

## Commits (Conventional Commits)

```
feat:     nova funcionalidade
fix:      correção de bug
docs:     documentação
chore:    tarefas de manutenção
refactor: refatoração sem mudança de comportamento
```

Exemplos:
- `feat(mqtt): adiciona LWT no nó atuador`
- `docs(backbone): tabela global de variáveis`
- `fix(can): terminação 120 ohm no barramento`

## Branches

- `main` — estável, sempre funcional para a apresentação
- `dev` — integração
- `feat/<rede>-<tarefa>` — trabalho de cada dupla

## Pull Requests

Cada dupla abre PR da sua branch para `dev`. Revisão por pelo menos 1 pessoa de outra dupla (garante interoperabilidade).
