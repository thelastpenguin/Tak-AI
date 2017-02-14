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
	std::cout << board << std::endl;
	printf("%c[2K", 27);
	printf("%c[2K", 27);

	// http://stackoverflow.com/questions/1508490/how-can-i-erase-the-current-line-printed-on-console-in-c-i-am-working-on-a-lin
}
