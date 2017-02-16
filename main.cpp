#include <iostream>
#include <unistd.h>

#include "board.h"

int main() {
	std::string line;

	while (std::getline(std::cin, line)) {
		Board board(line);

		board.place(rand() % Board::SQUARES, board.playerTurn);
		board.playerTurn *= -1;

		std::cout << board.toTBGEncoding() << "\n";
	}
}
