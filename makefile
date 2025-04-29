# =============================================================
# ==============---> Variables declerations <---===============
# =============================================================
# -------------------------- defines --------------------------
LIGHT_GREEN = \033[1;32m
GREEN = \033[0;32m
RED = \033[0;31m
CYAN = \033[0;36m
PURPLE = \033[0;35m
BROWN = \033[0;33m
NC = \033[0m

# --------------------------- Flags ---------------------------
CPPFLAGS = -I../../../ds/include
CFLAGS = -ansi -pedantic-errors -Wall -Wextra -g
RELEASE_FLAGS = -ansi -pedantic-errors -Wall -Wextra -DNDEBUG -o3
SO_FLAGS = -shared -fpic
CC = gcc

# --------------------- General Variables ---------------------
TARGET = wd
DEPS = wd.h wd_utils.h
EXE = run_wd
SO_DEBUG = bin/debug/so
O_DEBUG = bin/debug/o
DS_O_DEBUG = ../../../ds/bin/debug/o
SO_RELEASE = bin/release/so
O_RELEASE = bin/release/o
DS_O_RELEASE = ../../../ds/bin/release/o


# =============================================================
# ||                                                         ||
# ||                                                         ||
# ||                       Debug Rules                       ||
# ||                      -------------                      ||
# ||                                                         ||
# =============================================================

# ========================= Watch Dog =========================

#               ------ Executable Rule ------
wd: wd_user.c $(SO_DEBUG)/libwd.so wd_out.c wd.h wd_utils.h
	@bash -c 'echo -e creating wd.out'
	@$(CC) wd_out.c $(CFLAGS) -L$(SO_DEBUG) -lwd -Wl,-rpath=$(SO_DEBUG) $(CPPFLAGS) -o wd.out
	@bash -c 'echo -e creating app.out'
	@$(CC) $< $(CFLAGS) -L$(SO_DEBUG) -lwd -Wl,-rpath=$(SO_DEBUG) $(CPPFLAGS) -o app.out

$(SO_DEBUG)/libwd.so: $(O_DEBUG)/wdlib.o $(DS_O_DEBUG)/vector.o $(DS_O_DEBUG)/heap_pq.o $(DS_O_DEBUG)/heap.o $(DS_O_DEBUG)/scheduler_v2.o $(DS_O_DEBUG)/task.o $(DS_O_DEBUG)/uid.o
	@bash -c 'echo -e creating libwd.so'
	@$(CC) $^ $(CFLAGS) $(SO_FLAGS) -o $@

#                ------ Object Rules ------
$(O_DEBUG)/wdlib.o: wdlib.c
	@bash -c 'echo -e creating wdlib.o'
	@$(CC) $^ -c $(CFLAGS) $(CPPFLAGS) -fpic -o $@


# =============================================================


# =============================================================
# ||                                                         ||
# ||                                                         ||
# ||                      Release Rules                      ||
# ||                      -------------                      ||
# ||                                                         ||
# =============================================================

# ========================= Watch Dog =========================

#               ------ Executable Rule ------
wdr: wd_user.c $(SO_RELEASE)/libwd.so wd_out.c wd.h wd_utils.h
	@bash -c 'echo -e creating wd.out'
	@$(CC) wd_out.c $(RELEASE_FLAGS) -L$(SO_RELEASE) -lwd -Wl,-rpath=$(SO_RELEASE) $(CPPFLAGS) -o wd.out
	@bash -c 'echo -e creating app.out'
	@$(CC) $< $(RELEASE_FLAGS) -L$(SO_RELEASE) -lwd -Wl,-rpath=$(SO_RELEASE) $(CPPFLAGS) -o app.out

$(SO_RELEASE)/libwd.so: $(O_RELEASE)/wdlib.o $(DS_O_RELEASE)/vector.o $(DS_O_RELEASE)/heap_pq.o $(DS_O_RELEASE)/heap.o $(DS_O_RELEASE)/scheduler_v2.o $(DS_O_RELEASE)/task.o $(DS_O_RELEASE)/uid.o
	@bash -c 'echo -e creating libwd.so'
	@$(CC) $^ $(RELEASE_FLAGS) $(SO_FLAGS) -o $@

#                ------ Object Rules ------
$(O_RELEASE)/wdlib.o: wdlib.c
	@bash -c 'echo -e creating wdlib.o'
	@$(CC) $^ -c $(RELEASE_FLAGS) $(CPPFLAGS) -fpic -o $@

# =============================================================


# =============================================================
# ||                                                         ||
# ||                                                         ||
# ||                       Clean Rules                       ||
# ||                      -------------                      ||
# ||                                                         ||
# =============================================================

clean:
	@rm $(O_DEBUG)/wd_out.o $(O_DEBUG)/wdlib.o $(SO_DEBUG)/libwd.so $(O_RELEASE)/wd_out.o $(O_RELEASE)/wdlib.o $(SO_RELEASE)/libwd.so app.out wd.out
	
	