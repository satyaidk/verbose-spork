#!/usr/bin/env bash
# Host-side test harness. Compiles the firmware modules against a stubbed
# Arduino core, so logic can be checked in a second without flashing anything.
#
#   ./run.sh          compile + unit tests + (if node is present) the UI tests
#
# Requires: g++.  The UI tests additionally need node and jsdom:
#   npm install jsdom
set -e
cd "$(dirname "$0")"
mkdir -p .build && cp ../*.h .build/ && cp ../MochiWeb.ino .build/sketch.cpp
cp tests.cpp uitest.js .build/ 2>/dev/null || true
cd .build

echo "== compiling the full sketch =="
g++ -std=gnu++17 -O1 -I. -I../stubs -Wall -Wextra -Wno-unused-parameter \
    sketch.cpp ../stubs/stubmain.cpp -o sketch_run
./sketch_run

echo
echo "== unit tests =="
g++ -std=gnu++17 -O1 -I. -I../stubs tests.cpp -o tests
./tests

if command -v node >/dev/null 2>&1 && [ -d ../node_modules/jsdom -o -d node_modules/jsdom ]; then
  echo
  echo "== web UI tests =="
  python3 - <<'PY'
src=open('web_ui.h').read()
html=src.split('R"HTMLPAGE(')[1].rsplit(')HTMLPAGE"',1)[0]
open('page.html','w').write(html)
open('page.js','w').write(html.split('<script>')[1].split('</script>')[0])
PY
  node --check page.js && echo "page.js syntax ok"
  node uitest.js
else
  echo
  echo "(skipping UI tests: run 'npm install jsdom' in tests/ to enable them)"
fi
