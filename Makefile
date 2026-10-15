# Nom de l'exécutable
TARGET = fsh

# Les fichiers sources
SRCS = fsh.c parser.c prompt.c exec.c utils.c

# Les fichiers objets générés à partir des sources
OBJS = $(SRCS:.c=.o)

# Compilateur et flags
CC = gcc
CFLAGS = -Wall -g

# Liens pour Readline
LDFLAGS = -lreadline -lm

# Règle par défaut : compile le projet
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Règle pour générer les fichiers objets (.o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJS) $(TARGET)

valgrind:
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)
