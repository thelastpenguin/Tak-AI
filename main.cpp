#include <iostream>

#include "board.h"

void parseHumanMove() {

}


int main() {
	Board board;
	board.place(1, PIECE_FLAT);
	board.place(1, -PIECE_FLAT);
	board.place(1, PIECE_FLAT);
	board.place(1, -PIECE_FLAT);
	board.place(3, PIECE_FLAT);
	board.place(3, -PIECE_FLAT);
	board.place(3, PIECE_WALL);

	Board board2 = board;
	std::cout << board << std::endl;
	std::cout << board.hash() << std::endl;
	std::cout << board2 << std::endl;
	std::cout << board2.hash() << std::endl;

	while (true) {
		std::cout << " -> " << std::flush;
		std::string line;
		std::getline(std::cin, line);

		if (line == "print")
			std::cout << board << std::endl;
	}

	// http://stackoverflow.com/questions/1508490/how-can-i-erase-the-current-line-printed-on-console-in-c-i-am-working-on-a-lin
}
