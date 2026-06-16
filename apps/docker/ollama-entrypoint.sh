#!/usr/bin/env bash
#
# Entrypoint for the ac-ollama container. Starts the Ollama server, waits until
# it accepts connections, then pulls the chat model used by mod-ollama-chat.
# The pull is idempotent: once the model is cached in the ac-ollama volume it's
# a no-op on subsequent starts.
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

echo "Pulling ${MODEL} (first run only; may take a while)..."
ollama pull "${MODEL}"

echo "Ollama ready; model ${MODEL} available."
wait "${serve_pid}"
