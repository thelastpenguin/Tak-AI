#include <iostream>

#include "player.h"
#include "board.h"

int main(int argc, const char **argv) {
	// nice debug case 23,b,5,14,13,0,0;,,bF,,wF,,,wF,wF,,,,wbwbwC,,,wF,,bS,bF,,wF,bF,bF,bC,bF
	Board board;

	if (argc > 1) {
		std::cout << "loading board from arguments" << std::endl;
		board = Board(argv[1]);
		std::cout << board << "\n\n---------LOADING COMPLETE. START GAME---------\n\n" << std::endl;
	}

	// TODO: if the first move place the opponent in a corner
	// NOTE: Adjacent corner is best, but mix in opposite corner to keep them on their toes

	Player *white = new HumanPlayer();
	Player *black = new MinmaxPlayer(4);

	while (true) {
		Player *cur = board.playerTurn == 1 ? white : black;

		board = cur->makeAMove(board);
		std::cout << board;
		std::cout << "\tdjikstra white: " << board.getDjikstraScore(1) << std::endl;
		std::cout << "\tdjikstra black: " << board.getDjikstraScore(-1) << std::endl;

		std::cout << "\n\n\n-------------- NEXT TURN ----------------\n\n\n" << std::endl;

		if (board.getWinner() != 0) break ;
	}
	std::cout << "Done." << std::endl;
	std::cout << "\t" << (board.getWinner() < 0 ? "Black" : "White") << " Wins!" << std::endl;
}
