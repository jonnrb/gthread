# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//makeutils.mak
# info: utilities for make

COM_COLOR = \033[0;34m
RUN_COLOR = \033[0;35m
OBJ_COLOR = \033[0;36m
OK_COLOR = \033[0;32m
ERROR_COLOR = \033[0;31m
WARN_COLOR = \033[0;33m
NO_COLOR = \033[m

OK_STRING = "[OK]"
ERROR_STRING = "[ERROR]"
WARN_STRING = "[WARNING]"
COM_STRING = "Building"
RUN_STRING = "Running"

define build_and_check
  printf " %b" "$(COM_COLOR)$(COM_STRING) $(OBJ_COLOR)$(@F)$(NO_COLOR)\r"; \
  rm -f $(@).fifo;                                                         \
  mkfifo $(@).fifo;                                                        \
  if [ "$$(uname)" = "Darwin" ]; then                                      \
    script -q /dev/null $(1) > $(@).fifo &                                 \
  else                                                                     \
    script -q --return /dev/null -c '$(1)' > $(@).fifo &                   \
  fi;                                                                      \
  SUBPID=$$!;                                                              \
  OLDIFS=$$IFS;                                                            \
  IFS=\n;                                                                  \
  ( FLAG=0; while read -r LINE; do                                         \
    if [ $$FLAG -eq 0 ]; then FLAG=1; printf "\n"; fi;                     \
    echo "$$LINE";                                                         \
  done < $(@).fifo; exit $$FLAG; ) &                                       \
  OUTPID=$$!;                                                              \
  IFS=$$OLDIFS;                                                            \
  wait $$SUBPID; RESULT=$$?;                                               \
  wait $$OUTPID; WARN=$$?;                                                 \
  if [ $$RESULT -ne 0 ]; then                                              \
    printf "\r %-60b%b" "$(COM_COLOR)$(COM_STRING)$(OBJ_COLOR) $@"         \
      "$(ERROR_COLOR)$(ERROR_STRING)$(NO_COLOR)\n";                        \
  elif [ $$WARN -eq 1 ]; then                                              \
    printf "\r %-60b%b" "$(COM_COLOR)$(COM_STRING)$(OBJ_COLOR) $@"         \
      "$(WARN_COLOR)$(WARN_STRING)$(NO_COLOR)\n";                          \
  else                                                                     \
    printf "\r %-60b%b" "$(COM_COLOR)$(COM_STRING)$(OBJ_COLOR) $(@F)"      \
      "$(OK_COLOR)$(OK_STRING)$(NO_COLOR)\n";                              \
  fi;                                                                      \
  rm -f $@.fifo;                                                           \
  exit $$RESULT
endef

define run_and_check
  printf " %b" "$(RUN_COLOR)$(RUN_STRING) $(OBJ_COLOR)$(@F)$(NO_COLOR)\r"; \
  rm -f $(1).fifo;                                                         \
  mkfifo $(1).fifo;                                                        \
  if [ "$$(uname)" = "Darwin" ]; then                                      \
    script -q /dev/null $(1) > $(1).fifo &                                 \
  else                                                                     \
    script -q --return /dev/null -c '$(1)' > $(1).fifo &                   \
  fi;                                                                      \
  SUBPID=$$!;                                                              \
  FLAG=0;                                                                  \
  OLDIFS=$$IFS;                                                            \
  IFS=\n;                                                                  \
  while read -r LINE; do                                                   \
    if [ $$FLAG -eq 0 ]; then FLAG=1; printf "\n"; fi;                     \
    echo "$$LINE";                                                         \
  done < $(1).fifo &                                                       \
  IFS=$$OLDIFS;                                                            \
  OUTPID=$$!;                                                              \
  wait $$SUBPID; RESULT=$$?;                                               \
  wait $$OUTPID;                                                           \
  if [ $$RESULT -ne 0 ]; then                                              \
    printf "\r %-60b%b" "$(RUN_COLOR)$(RUN_STRING)$(OBJ_COLOR) $(1)"       \
      "$(ERROR_COLOR)$(ERROR_STRING)$(NO_COLOR)\n";                        \
  elif [ -s $@.warn ]; then                                                \
    printf "\r %-60b%b" "$(COM_COLOR)$(COM_STRING)$(OBJ_COLOR) $(1)"       \
      "$(WARN_COLOR)$(WARN_STRING)$(NO_COLOR)\n";                          \
  else                                                                     \
    printf "\r %-60b%b" "$(RUN_COLOR)$(RUN_STRING)$(OBJ_COLOR) $(@F)"      \
      "$(OK_COLOR)$(OK_STRING)$(NO_COLOR)\n";                              \
  fi;                                                                      \
  cat $(1).warn;                                                           \
  rm -f $(1).warn;                                                         \
  rm -f $(1).fifo;                                                         \
  exit $$RESULT
endef
