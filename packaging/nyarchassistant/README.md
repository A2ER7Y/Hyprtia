# Hyprtia Assistant

Hyprtia Assistant is a StratOS patch set for
[NyarchAssistant](https://github.com/NyarchLinux/NyarchAssistant). It remains a
separate GTK application so its GPL-3.0 code, Live2D/LivePNG avatar stack, MCP
support, and local model runtime do not add heavy dependencies to the Hyprtia
Wayland shell.

## Install

```sh
cd packaging/nyarchassistant
makepkg -si
```

The package is pinned to NyarchAssistant commit
`bd632c7148d54f7d7cb8a9290862c99e9f66e61e`. `prepare()` applies
`stratos-minimal.patch`; a failed patch application deliberately stops the
build instead of silently producing an unpatched application.

## StratOS changes

- 420×720 compact phone-style window with collapsed history and canvas panels.
- Narrow waifu panel retained; lightweight LivePNG remains recommended.
- Smaller chat history batches and deterministic avatar renderer cleanup.
- First-class Kimi K2.6 provider with thinking and an accurately labelled swarm
  orchestration prompt.
- Bounded parallel swarm tool with at most eight workers.
- Native Ollama model library presets, including honest hardware warnings for
  Llama 4 Scout and Maverick.
- Permission-gated file create, edit, move, and delete tools. Directory deletion
  is intentionally unavailable to the AI.
- Read-only local Git tools plus GitHub, GitLab, and Gitea-compatible issue
  listing. Tokens are read from environment variables and only sent to a
  matching trusted origin.
- Automatic English, French, Spanish, German, and Italian TTS voice selection
  without changing the saved voice.

NyarchAssistant already supplies native Ollama, Llama.cpp, MCP, terminal,
explorer, and file editor support. The patch extends those existing systems
instead of introducing a second agent runtime. Orbitos Island is credited as an
architectural reference only; no Orbitos source or assets are embedded.

## Credentials

The Git forge integration is read-only. Optional tokens are never stored in
GSettings:

- `GITHUB_TOKEN`, optionally paired with `GITHUB_BASE_URL`
- `GITLAB_TOKEN`, optionally paired with `GITLAB_BASE_URL`
- `GIT_FORGE_TOKEN`, paired with `GIT_FORGE_BASE_URL`

For custom URLs that do not match the configured trusted origin, requests are
sent anonymously.

## Offline mode

Install `ollama` and `python-ollama`, select **Ollama Instance** in the LLM
settings, then open its model library. Small recommended models are suitable for
normal PCs. Llama 4 Scout and Maverick are visible but clearly marked as
workstation/server-class downloads.
