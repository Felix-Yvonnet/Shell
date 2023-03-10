typedef enum { C_PLAIN, C_VOID, C_AND, C_OR, C_PIPE, C_SEQ } cmdtype;

typedef struct cmd {
	int type;
	struct cmd *left;
	struct cmd *right;

	char **args;
	char *input;
	char *output;
	char *append;
	char *error;
	char *equal;
	char *geq;
	char *gneq;
} cmd;

struct arglist {
	char *arg;
	struct arglist *next;
};

extern struct cmd* parser (char*);
extern void output (struct cmd*,int);
