#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#define MAX_LINES 4096
#define MAX_LINE_C 1024
#define MAX_FILENAME 1024

#define ESC "\e"

#define RED "\e[31m"
#define GREEN "\e[32m"

#define RESET "\e[0m"

#define STRING(x) STRINGIFY(x)

#define STRINGIFY(x) #x

#define Mode(end) Mode_##end

#define seql(func, s1, s2)(func(s1, s2) == 0)
#define sneql(func, s1, s2, n)(func(s1, s2, n) == 0)

typedef struct {
	char lines[MAX_LINES][MAX_LINE_C];
	int len[MAX_LINES];
	int current;
	int count;
} MemContent;

typedef enum Mode {
	Mode(WRITE),
	Mode(COMMAND),
} Mode;

void help();

void cat(MemContent *content, bool number);

bool readFromFile(const char *filename, MemContent *content);

bool writeToFile(const char *filename, MemContent *content);

void maybe_switch_to_write(char *buffer, int len, Mode *mode);

void delete(char *buffer, int line, MemContent *content);

void openline(char *buffer, int line, MemContent *content);

int main(int argc, char **argv) {
	MemContent content = {0};

	char filename[MAX_FILENAME] = {0};
	int filename_len = 0;

	for (int i = 0; i < MAX_LINES;i++) {
		strcpy(content.lines[i], "");
	}

	if (argc > 1) {
		strcpy(filename, argv[1]);
		readFromFile(filename, &content);
	}

	help();

	Mode mode = Mode(WRITE);

	bool end = false;

	while (!end) {
		char buffer[2048] = {0};

		switch (mode) {
			case Mode(WRITE):
				printf("%d) ", content.current + 1);
				break;
			case Mode(COMMAND):
				printf(": ");
				break;
		}

		fflush(stdout);
		fgets(buffer, sizeof(buffer), stdin);
		int len = strlen(buffer);

		if (len != 0) {
			do {
				buffer[--len] = '\0';
			} while((mode == Mode(COMMAND)) && (len != 0) && (buffer[len - 1] == ' '));
		} else {
			printf("Unexpected characters detected! Shutting Editor\n");
			end = true;
		}

		switch (mode) {
			case Mode(WRITE):
				if (seql(strcmp, buffer, ESC)) {
					printf("Lines: %d\n", content.count);
					continue;
				}

				if (sneql(strncmp, buffer, ESC "g", 2)) {
					char ch;
					int line;
					char exitmodech;
					
					int ret = sscanf(buffer + 1, "%c%d%c", &ch, &line, &exitmodech);

					if (ret > 1) {
						if (line > content.count) {
							sprintf(buffer, "Cannot access line offset %d", line);
							fprintf(stderr, "Invalid Command: " RED "'%s'" RESET "\n", buffer);
							continue;
						} else if (line < 1) {
							sprintf(buffer, "Invalid line number %d. Line number must be natural number", line);
							fprintf(stderr, "Invalid Command: " RED "'%s'" RESET "\n", buffer);
							continue;
						} else {
							content.current = line - 1;
							if (ret == 3 && exitmodech == ';') {
								maybe_switch_to_write(buffer, len, &mode);
							}
							continue;
						}
					}
				}

				if (sneql(strncmp, buffer, ESC "o", 2)) {
					content.current++;
					if (content.count < content.current) {
						content.count = content.current;
					}
					continue;
				}

				if (sneql(strncmp, buffer, ESC "O", 2)) {
					if (0 < content.current) {
						content.current--;
					}
					continue;
				}

				if (sneql(strncmp, buffer, ESC "l", 2)) {
					printf(ESC "[1;1H" ESC "[2J");
					continue;
				}

				if (sneql(strncmp, buffer, ESC ESC, 2)) {
					mode = Mode(COMMAND);
					puts(GREEN "Switch: W > C" RESET);
					continue;
				}

				if (content.count > MAX_LINES) {
					printf("Content Larger than %d lines\n", MAX_LINES);
					continue;
				}

				snprintf(content.lines[content.current], len + 1, "%s", buffer);
				content.lines[content.current][len] = '\0';

				content.len[content.current] = len;

				content.current++;
				if (content.count < content.current) {
					content.count = content.current;
				}

				printf("W: %d\n", len+1);
				break;
			case Mode(COMMAND):
				if (sneql(strncmp, buffer, "w", 1)) {
					char ch = 0;
					char provided_filename[MAX_FILENAME];

					int ret = sscanf(buffer, "%c %s", &ch, provided_filename);

					if (ret == 1 && filename_len < 1) {
						puts("No filename provided");
						maybe_switch_to_write(buffer, len, &mode);
						continue;
					} else if (ret == 2) {
						if (sneql(strncmp, buffer, "w;", 2)) {
							maybe_switch_to_write(buffer, len, &mode);
							continue;
						}

						strncpy(filename, provided_filename, MAX_FILENAME);
						filename[strlen(provided_filename)] = '\0';
						filename_len = strlen(filename);
					}

					writeToFile(filename, &content);
					maybe_switch_to_write(buffer, len, &mode);
					continue;
				}

				if (sneql(strncmp, buffer, "r", 1)) {
					char ch = 0;
					char provided_filename[MAX_FILENAME];

					int ret = sscanf(buffer, "%c %s", &ch, provided_filename);

					if (ret == 1 && filename_len < 1) {
						puts("No filename provided");
						maybe_switch_to_write(buffer, len, &mode);
						continue;
					} else if (ret == 2) {
						if (sneql(strncmp, buffer, "r;", 2)) {
							maybe_switch_to_write(buffer, len, &mode);
							continue;
						}

						strncpy(filename, provided_filename, MAX_FILENAME);
						filename[strlen(provided_filename)] = '\0';
						filename_len = strlen(filename);
					}

					readFromFile(filename, &content);
					maybe_switch_to_write(buffer, len, &mode);
					continue;
				}

				if (seql(strcmp, buffer, "name")) {
					if (filename_len < 1) {
						puts("No filename provided");
					} else {
						printf("Filename: '%s'\n", filename);
					}
					maybe_switch_to_write(buffer, len, &mode);
					continue;
				}

				if (seql(strcmp, buffer, "cat") || seql(strcmp, buffer, "p")|| seql(strcmp, buffer, "p;") || seql(strcmp, buffer, "cat;")) {
					cat(&content, false);
					maybe_switch_to_write(buffer, len, &mode);
					continue;
				}

				if (seql(strcmp, buffer, "catn") || seql(strcmp, buffer, "pn")|| seql(strcmp, buffer, "pn;") || seql(strcmp, buffer, "catn;")) {
					cat(&content, true);
					maybe_switch_to_write(buffer, len, &mode);
					continue;
				}

				if (seql(strcmp, buffer, "clear") || seql(strcmp, buffer, "clear;")) {
					printf(ESC "[1;1H" ESC "[2J");
					maybe_switch_to_write(buffer, len, &mode);
					continue;
				}

				if (seql(strcmp, buffer, "q") || seql(strcmp, buffer, "quit") || seql(strcmp, buffer, "exit") ||
					seql(strcmp, buffer, "q;") || seql(strcmp, buffer, "quit;") || seql(strcmp, buffer, "exit;")
						) {
					end = true;
					continue;
				}

				if (sneql(strncmp, buffer, "l", 1)) {
					char ch;
					int line;
					char exitmodech;

					int ret = sscanf(buffer, "%c%d%c", &ch, &line, &exitmodech);

					if (ret > 1) {
						if (line > content.count) {
							sprintf(buffer, "Cannot Access Content Offset %d", line);
						} else if (line <= 0) {
							sprintf(buffer, "Invalid Line number %d. Line number must be natural integer", line);
						} else {
							printf("%d: %s\n", line, content.lines[line - 1]);
							if (ret == 3 && exitmodech == ';'){
								maybe_switch_to_write(buffer, len, &mode);
							}
							continue;
						}
					}
				}

				if (sneql(strncmp, buffer, "d", 1)) {
					char ch;
					int line;
					char exitmodech;

					int ret = sscanf(buffer, "%c%d%c", &ch, &line, &exitmodech);

					if (ret > 1) {
						if (line > content.count) {
							sprintf(buffer, "Cannot Access Content Offset %d", line);
						} else if (line <= 0) {
							sprintf(buffer, "Invalid Line number %d. Line number must be natural integer", line);
						} else {
							delete(buffer, line, &content);
							if (ret == 3 && exitmodech == ';') {
								maybe_switch_to_write(buffer, len, &mode);
							}
							continue;
						}
					}
				}

				if (sneql(strncmp, buffer, "o", 1)) {
					char ch;
					int line;
					char exitmodech;

					int ret = sscanf(buffer, "%c%d%c", &ch, &line, &exitmodech);

					if (ret > 1) {
						if (line > content.count) {
							sprintf(buffer, "Cannot Access Content Offset %d", line);
						} else if (line <= 0) {
							sprintf(buffer, "Invalid Line number %d. Line number must be natural integer", line);
						} else {
							openline(buffer, line - 1, &content);

							if (ret == 3 && exitmodech == ';') {
								maybe_switch_to_write(buffer, len, &mode);
							}
							continue;
						}
					}
				}

				if (sneql(strncmp, buffer, "O", 1)) {
					char ch;
					int line;
					char exitmodech;

					int ret = sscanf(buffer, "%c%d%c", &ch, &line, &exitmodech);

					if (ret > 1) {
						if (line > content.count) {
							sprintf(buffer, "Cannot Access Content Offset %d", line);
						} else if (line <= 0) {
							sprintf(buffer, "Invalid Line number %d. Line number must be natural integer", line);
						} else {
							openline(buffer, line - 2, &content);
							if (ret == 3 && exitmodech == ';') {
								maybe_switch_to_write(buffer, len, &mode);
							}
							continue;
						}
					}
				}
				if (sneql(strncmp, buffer, "g", 1)) {
					char ch;
					int line;
					char exitmodech;
					
					int ret = sscanf(buffer, "%c%d%c", &ch, &line, &exitmodech);

					if (ret > 1) {
						if (line > content.count) {
							sprintf(buffer, "Cannot access line offset %d", line);
						} else if (line < 1) {
							sprintf(buffer, "Invalid line number %d. Line number must be natural number", line);
						} else {
							content.current = line - 1;
							if (ret == 3 && exitmodech == ';'){
								maybe_switch_to_write(buffer, len, &mode);
							}
							continue;
						}
					}
				}

				maybe_switch_to_write(buffer, len, &mode);

				if (!sneql(strncmp, buffer, ";", 1)) {
					fprintf(stderr, "Invalid Command: " RED "'%s'" RESET "\n", buffer);
				}
				break;
		}
	}
}

void help() {
	puts("===============");
	puts("|" " [USAGE] " GREEN  "edi" RESET " |");
	puts("===============");
}

bool readFromFile(const char *filename, MemContent *content) {
	FILE *file = fopen(filename, "rb+");

	if (file == NULL) {
		fprintf(stderr, RED "Read Failed for '%s':" RESET " %s\n", filename, strerror(errno));
		return 0;
	}

	fseek(file, 0L, SEEK_END);
	int size = ftell(file);

	rewind(file);

	if (size > (MAX_LINES * MAX_LINE_C)) {
		fprintf(stderr, RED "Read aborted for '%s' due to size limit %ld" RESET "\n", filename, (long)MAX_LINES * (long)MAX_LINE_C);
		fclose(file);
		return 0;
	}

	int lines = 0;
	int count = 0;
	while (fgets(content->lines[lines], sizeof(content->lines[lines]), file) && count <= MAX_LINES) {
		int len = strlen(content->lines[lines]);
		content->lines[lines][len - 1] = '\0';
		content->len[lines] = len - 1;
		count += content->len[lines];
		lines++;
	}

	content->count = lines;
	content->current = lines;
	printf("Read %d lines to '%s' [%d bytes]\n", lines, filename, count);

	fclose(file);
	return 1;
}

bool writeToFile(const char *filename, MemContent *content) {
	FILE *file = fopen(filename, "wb+");

	if (file == NULL) {
		fprintf(stderr, RED "Write Failed for '%s':" RESET " %s\n", filename, strerror(errno));
		return 0;
	}

	int count = 0;
	for (int i = 0; i < content->count; i++) {
		fprintf(file, "%s\n", content->lines[i]);
		count += content->len[i] + 1;
	}

	printf("Written to '%s' [%d Bytes]\n", filename, count);

	fclose(file);
	return 1;
}

void cat(MemContent *content, bool number) {
	if (number) {
		for (int i = 0; i < content->count; i++) {
			printf("%4d) %s\n", i + 1, content->lines[i]);
		}
	} else {
		for (int i = 0; i < content->count; i++) {
			printf("%s\n", content->lines[i]);
		}
	}
}

void maybe_switch_to_write(char *buffer, int len, Mode *mode) {
	if (sneql(strncmp, (buffer + (len - 1)), ";", 1)) {
		*mode = Mode(WRITE);
		puts(GREEN "Switch: C > W" RESET);
	}
}

void delete(char *buffer, int line, MemContent *content) {
	if (line > content->count || line < 0) {
		sprintf(buffer, "Cannot access line offset %d", line);
		return;
	}

	for(int i = line - 1; i < content->count - 1; i++) {
		content->len[i] = content->len[i + 1];
		strcpy(content->lines[i], content->lines[i + 1]);
	}

	content->count--;

	if (content->current > content->count) {
		content->current = content->count;
	}
}

void openline(char *buffer, int line, MemContent *content) {
	if (line > content->count || line < 0) {
		sprintf(buffer, "Cannot access line offset %d", line);
		return;
	}

	for(int i = content->count; i != line - 1; i--) {
		content->len[i + 1] = content->len[i];
		strcpy(content->lines[i + 1], content->lines[i]);
	}

	content->len[line+1] = 0;
	content->lines[line+1][0] = '\0';

	content->count++;
}
