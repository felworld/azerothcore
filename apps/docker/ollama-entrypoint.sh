#!/usr/bin/env bash
#
# Entrypoint for the ac-ollama container. Starts the Ollama server, waits until
# it accepts connections, then makes sure the chat model used by mod-ollama-chat
# is available, pulling it only if it isn't already cached in the ac-ollama
# volume.
#
# NOTE: `ollama pull` is NOT a safe no-op when the model is already cached — it
# still contacts the registry to check the manifest. With no outbound network
# (or DNS) that pull fails, and under `set -e` a failed pull exits the script,
# which kills the container and triggers a restart loop. That makes the server
# flap and mod-ollama-chat see intermittent "no response" errors. So: skip the
# pull when the model is present, and never let a failed pull take the server
# down.
set -euo pipefail

# Model to make available to mod-ollama-chat. Keep in sync with OllamaChat.Model
# in env/dist/etc/modules/mod_ollama_chat.conf. Override via OLLAMA_MODEL.
MODEL="${OLLAMA_MODEL:-gemma4:e4b}"

# Start the server in the background; remember its PID so we can hand control
# back to it (as PID-of-container) once the model is ready.
ollama serve &
serve_pid=$!

echo "Waiting for Ollama server to accept connections..."
until ollama list >/dev/null 2>&1; do
    sleep 1
done

if ollama list 2>/dev/null | grep -qF "${MODEL}"; then
    echo "Model ${MODEL} already present; skipping pull."
else
    echo "Pulling ${MODEL} (first run only; may take a while)..."
    # Don't let a failed pull (e.g. no network/DNS to the registry) abort the
    # script and bounce the container; keep serving whatever is cached.
    ollama pull "${MODEL}" || echo "WARNING: failed to pull ${MODEL}; continuing with cached models only."
fi

echo "Ollama ready; model ${MODEL} available."
wait "${serve_pid}"
