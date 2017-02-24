#include <unistd.h>
#include <iostream>
#include "board.h"

void create_piped_proc(const char *fname, int *handles) {
	int stdin[2];
	int stdout[2];
	pipe(stdin);
	pipe(stdout);

	int child = fork();
	if (child == 0) {
		close(stdin[1]);
		close(stdout[0]);
		dup2(stdin[0], 0);
		dup2(stdout[1], 1);

		execve(fname, NULL, NULL);
	} else {
		close(stdin[0]);
		close(stdout[1]);

		handles[0] = stdin[1];
		handles[1] = stdout[0];
	}
}

void read_until_newline(int fd, char *buffer) {
	while (read(fd, buffer, 1)) {
		if (*buffer == '\n') break ;
		buffer++;
	}
	*buffer = 0;
}

int main(int argc, const char** argv, const char **envp) {
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " <proc1> <proc2> " << std::endl;
		exit(0);
	}

	int proc1[2];
	int proc2[2];

	create_piped_proc(argv[1], proc1);
	create_piped_proc(argv[2], proc2);

	char buffer[2048];

	Board board;

	while (true) {
		std::string state;

		state = board.toTBGEncoding() + "\n";
		write(proc1[0], state.c_str(), state.length());

		read_until_newline(proc1[1], buffer);
		board = Board(std::string(buffer));
		std::cout << board << std::endl;
		sleep(1);

		state = board.toTBGEncoding() + "\n";
		write(proc2[0], state.c_str(), state.length());

		read_until_newline(proc2[1], buffer);
		board = Board(std::string(buffer));
		std::cout << board << std::endl;
		sleep(1);
	}
}
