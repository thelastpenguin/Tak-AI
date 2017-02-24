#include <iostream>
#include <unistd.h>
#include <set>
#include <cassert>
#include <stdint.h>
#include <time.h>

#include "board.h"
#include "mcts.h"

double negamax(Board& board, int depth, Move* result, double alpha = -std::numeric_limits<double>::max(), double beta = std::numeric_limits<double>::max()) {
	int isTerminal = board.isTerminalState();
	if (isTerminal != 0) return isTerminal * 100000;
	if (depth == 0) {
        return board.getScore() * board.playerTurn;
    }

    double max = -std::numeric_limits<double>::max();

	std::vector<Move> moves = board.get_moves();

    for (Move& move : moves) {
        move.apply(board);
        double score = -negamax(board, depth - 1, nullptr, -beta, -alpha);
        move.revert(board);

        if (score > max || (score == max && rand() % 2 == 0)) {
            if (result)
                *result = move;
            max = score;
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break ;
    }

    return max;
}

struct Player {
	virtual Board makeAMove(Board board) = 0;
};

struct AIPlayer {
	virtual Board makeAMove(Board board) {
		Move move;
		double score = negamax(board, 4, &move);
		std::cout << "AI Player generated move with score: " << score << std::endl;
		move.apply(board);
		return board;
	}
};

struct HumanPlayer {
	virtual Board makeAMove(Board board) {
		std::cout << "Human Move (" << (board.playerTurn > 0 ? "White" : "Black") << ") ..." << std::endl;
		std::cout << " -> ";
		// TODO: finish move API
	};
};



int main() {

	// TODO: if the first move place the opponent in a corner
	// NOTE: Adjacent corner is best, but mix in opposite corner to keep them on their toes

	Board board;
	// board.place(0, PIECE_FLAT);
	// board.place(1, PIECE_FLAT);
	// board.place(2, PIECE_FLAT);
	// board.place(3, PIECE_FLAT);
	// board.place(4, PIECE_FLAT);
	// std::cout << board.isTerminalState() << std::endl;

	srand(time(0));


	while (true) {


		std::cout << "\n\n\n\n" << std::endl;
		std::cout << board;
		std::cout << "Move score: " << score << " current board score: " << board.getScore() << std::endl;
		if (board.isTerminalState()) break ;
	}
	std::cout << "Done." << std::endl;
	std::cout << "\t" << (board.playerTurn < 0 ? "Black" : "White") << " Wins!" << std::endl;
}
