# Libraries
LIBS=utils commons pthread readline m crypto

# Custom libraries' paths
SHARED_LIBPATHS=
STATIC_LIBPATHS=../utils

# Compiler flags
CDEBUG=-g -std=gnu11 -Wall -DDEBUG -fdiagnostics-color=always -Werror 
CRELEASE=-O3 -Wall -DNDEBUG -fcommon -Werror
# Source files (*.c) to be excluded from tests compilation
TEST_EXCLUDE=src/main.c
