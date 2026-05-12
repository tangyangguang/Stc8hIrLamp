#!/bin/sh
set -eu

BASE_DIR="../../../Stc8hBase"
INCLUDES="-Iinclude -Iboard/stc8h1k08_ir_lamp_remote -I$BASE_DIR/core -I$BASE_DIR/hal -I$BASE_DIR/drivers -I$BASE_DIR/utils"

cc -std=c89 -Wall -Wextra -Dmain=app_entry $INCLUDES -fsyntax-only src/main.c

for file in src/base/*.c; do
    cc -std=c89 -Wall -Wextra $INCLUDES -fsyntax-only "$file"
done
