#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# SPDX-FileCopyrightText: 2026 mdc contributors
# Instala os hooks deste repositório (não copia nada: aponta core.hooksPath).
set -eu

cd "$(dirname "$0")/.."

if [ ! -d .git ]; then
    echo "erro: não é um repositório git. Rode 'git init' primeiro." >&2
    exit 1
fi

chmod +x .githooks/pre-commit .githooks/pre-push
git config core.hooksPath .githooks

echo "hooks instalados: core.hooksPath = $(git config --get core.hooksPath)"
echo "  pre-commit → tools/check_compliance.py --staged"
echo "  pre-push   → tools/check_compliance.py --all"
echo
echo "verificação rápida (testes do próprio gate):"
python3 tools/selftest_compliance.py | tail -2
