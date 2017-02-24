#include <iostream>
#include <unistd.h>

#include "board.h"

int main() {
	std::string line;

	Board board;
	board.place(0, PIECE_FLAT);
	board.place(0, PIECE_FLAT);
	board.place(0, PIECE_FLAT);
	board.place(0, PIECE_FLAT);
	board.place(0, PIECE_FLAT);
	std::cout << board << std::endl;
	std::vector<Move> moves = board.get_moves(1);
	for (size_t i = 0; i < moves.size(); ++i) {
		moves[i].apply(board);
		std::cout << board << std::endl;
		moves[i].revert(board);
	}
}
