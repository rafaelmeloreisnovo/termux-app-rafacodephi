# Framework Multi-IA RAFAELIA v1.0

Especificação operacional para governança de dados, auditabilidade, rastreabilidade evolutiva, privacidade e proteção humana.

## 1) Objetivo e Escopo

Este documento define os controles mínimos para implementação do Framework Multi-IA no repositório, com foco em:

- **Proteção humana e dignidade** como requisito de segurança.
- **Proteção infantil reforçada** em qualquer fluxo de entrada/saída.
- **Privacidade por padrão** (minimização, necessidade e proporcionalidade).
- **Auditabilidade ponta-a-ponta** de decisões e mudanças de estado.
- **Rastreabilidade evolutiva** para melhoria contínua sem perda de integridade.

## 2) Núcleo de estado auditável

O conhecimento operacional é representado por:

```text
K_t = (s_t, C_t, H_t, I_t, proofs_t, policy_t, human_context_t)
```

Onde:

- `s_t`: estado toroidal em 7 dimensões normalizadas (`[0,1)^7`).
- `C_t`: coerência global.
- `H_t`: entropia operacional.
- `I_t`: índice semântico multilíngue por camadas.
- `proofs_t`: hashes, CRC e raiz Merkle.
- `policy_t`: versão de políticas vigentes.
- `human_context_t`: contexto de risco humano/cultural.

## 3) Convenções matemáticas obrigatórias

Para evitar ambiguidade formal:

- `phi_ctrl = (1 - H) * C`  (fator de controle)
- `phi_gold = (1 + sqrt(5))/2` (constante geométrica)

É proibido reutilizar o mesmo símbolo para conceitos distintos em código, schema e documentação.

## 4) Arquitetura de governança por camadas

### L0 — Proteção Humana

- Bloqueio de conteúdo com risco de dano, exploração, abuso e autoagressão.
- Rota especial para sinais de vulnerabilidade infantil.
- Decisão em dúvida alta: `escalate_to_human`.

### L1 — Governança de Dados

- Classificação (`public`, `internal`, `sensitive`, `child-sensitive`).
- Retenção por prazo mínimo necessário.
- Pseudonimização/mascaramento em logs operacionais.

### L2 — Coerência Multilíngue

- Validação semântica por língua (`R_L`) e por domínio cultural (`G_L`).
- Tradução orientada a significado e dignidade, não literalidade cega.

### L3 — Integridade Técnica

- `integrity_manifest` assinado.
- Encadeamento de estado por hash do estado anterior.
- Raiz Merkle por build e por release.

### L4 — Supervisão e Auditoria

- Trilhas imutáveis de decisão crítica.
- Rollback seguro para último estado validado.
- Relatórios periódicos de risco, falso positivo e falso negativo.

## 5) Requisitos de conformidade (mapeamento)

| Controle | Evidência técnica | Referência normativa |
|---|---|---|
| Gate de transição ética | decisão `allow/challenge/block/escalate` registrada | NIST SP 800-53 (AC/AU families) |
| Integridade de build | hash, assinatura, Merkle root | ISO/IEC 27001 A.8/A.14 |
| Privacidade por padrão | minimização, retenção, pseudonimização | ISO/IEC 27701, LGPD/GDPR princípios |
| Continuidade operacional | fallback/rollback testado | ISO 22301 |
| V&V de segurança | testes de falha e regressão | IEEE 1012 |

## 6) Eventos auditáveis (schema mínimo)

```json
{
  "event_id": "uuid",
  "timestamp_utc": "2026-05-04T00:00:00Z",
  "agent_id": "string",
  "model_version": "string",
  "policy_version": "string",
  "language": "pt-BR",
  "risk_human": 0.0,
  "risk_child": 0.0,
  "coherence_c": 0.0,
  "entropy_h": 0.0,
  "action": "allow|challenge|block|escalate_to_human",
  "reason_codes": ["HUMAN_DIGNITY", "PRIVACY_MINIMIZATION"],
  "prev_state_hash": "hex",
  "state_hash": "hex",
  "merkle_root": "hex",
  "data_class": "public|internal|sensitive|child-sensitive",
  "retention_days": 30
}
```

## 7) Critérios de aceitação para CI/CD

Pipeline falha automaticamente se:

1. `integrity_manifest` inválido.
2. `policy_version` ausente em eventos críticos.
3. Evento com `risk_child >= threshold_child_block` sem `block` ou `escalate_to_human`.
4. Build sem `merkle_root`.
5. Regressão de cobertura de testes de segurança/ética.

## 8) Operação segura entre agentes

- Mensagens carregam `prev_state_hash` do emissor.
- Receptor valida cadeia antes de processar payload.
- Se validação falhar: descarte + evento `integrity_violation` + isolamento do nó.

## 9) KPIs de governança

- **Human Safety Precision/Recall** por categoria de risco.
- **Child Safety Zero-Miss Rate** (meta: sem falso negativo crítico).
- **Audit Completeness** (% de decisões críticas com trilha completa).
- **Privacy Leakage Rate** (meta: tendência a zero).
- **Cross-Lingual Coherence Drift** por língua/cultura.

## 10) Plano de evolução (v1.1+)

1. Introduzir testes de estresse semântico para pares linguísticos de alta distância.
2. Medir “cadência/entonação semântica” em tradução de conteúdo sensível.
3. Incluir benchmark de equidade cultural por domínio.
4. Expandir matriz de risco para contexto educacional e proteção infantil multilíngue.

---

## Local recomendado no repositório

- Documento de referência: `FRAMEWORK_MULTI_IA_SPEC_V1.md` (raiz).
- Integração futura: apontar este arquivo em `DOCUMENTACAO.md` e `INDICE_DOCUMENTACAO.md`.


## 11) Implementação inicial no repositório

Implementação de referência adicionada para permitir evolução incremental:

- `app/src/main/java/com/termux/app/agents/AgentMessage.java`
- `app/src/main/java/com/termux/app/agents/CommitGate.java`
- `app/src/main/java/com/termux/app/agents/DependencyRobot.java`
- `app/src/test/java/com/termux/app/agents/AgentCoreSelfTest.java`

Objetivo: fornecer base mínima para codificação de agentes e orquestração de dependências com validação de integridade.

