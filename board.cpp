#include <sstream>
#include <iostream>

#include "board.h"
#include "termcolor.h"

const char* piece_to_string(int8_t piece) {
	switch (piece) {
	case PIECE_FLAT: return "wF";
	case -PIECE_FLAT: return "bF";
	case PIECE_WALL: return "wW";
	case -PIECE_WALL: return "bW";
	case PIECE_CAP: return "wC";
	case -PIECE_CAP: return "bC";
	case 0: return " ";
	default: return "unknown";
	}
}

std::string Board::toTBGEncoding() const {
	std::stringstream ss;

	ss << moveno << "," << (playerTurn > 0 ? "w" : "b") << "," << SIZE << ",";
	ss << piecesleft[0] << "," << piecesleft[1] << "," << capstones[0] << "," << capstones[1];

	bool isFirst = true;

	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 5; ++x) {
			ss << (isFirst ? ";" : ",");
			isFirst = false;
			const Stack& stack = stacks[x + y * Board::SIZE];
			for (int i = 0; i < stack.size(); ++i) {
				if (stack.stack()[i])
					ss << "w";
				else
					ss << "b";
			}
			if (stack.size() >= 1) {
				switch (stack.top()) {
				case PIECE_FLAT:
				case -PIECE_FLAT: ss << "F"; break ;
				case PIECE_WALL:
				case -PIECE_WALL: ss << "S"; break ;
				case PIECE_CAP:
				case -PIECE_CAP: ss << "C"; break ;
				}
			}
		}
	}

	return ss.str();
}

std::ostream& operator << (std::ostream& out, const Board& board) {
	out << "Move #" << board.moveno << "   " << (board.playerTurn > 0 ? "White turn" : "Black turn") << std::endl;

	bool useColors = &out == &std::cout;

	char buffer[80];
	out << "  ";
	for (int i = 0; i < Board::SIZE; ++i) {
		sprintf(buffer, "%15c", 'A' + i);
		out << buffer;
	}
	out << std::endl;

	for (int y = 0; y < Board::SIZE; ++y) {
		out << y + 1 << " ";
		for (int x = 0; x < Board::SIZE; ++x) {
			out << "|";
			const Stack& stack = board.stacks[x + y * Board::SIZE];
			for (int i = 0; i < stack.size() - 1; ++i) {
				if (stack.stack()[i]) {
					if (useColors) out << termcolor::white << "+";
					else out << "w";
				}
				else {
					if (useColors) out << termcolor::red << "+";
					else  out << "b";
				}
			}
			if (stack.size() >= 1) {
				if (useColors) {
					if (stack.top() < 0) out << termcolor::red; else out << termcolor::white;
				}
				out << "[" << piece_to_string(stack.top()) << "]";
			} else
				out << "   ";
			for (int i = 10 - stack.size(); i >= 0; --i) {
				out << " ";
			}
			if (useColors)
				out << termcolor::reset;
		}
		out << "|" << std::endl;
	}

	out << "\t" << "TBGE: " << board.toTBGEncoding() << std::endl;

	return out;
};

/**
	move generation
*/

// TODO: move generation ...
struct MoveInternal {
	const static int8_t TYPE_PLACE = 0;
	const static int8_t TYPE_SPLIT = 1;
	const static int8_t TYPE_SPLIT_SQUASH = 2;

	MoveInternal() { bzero(this, sizeof(MoveInternal)); };

	uint32_t moveid;

	int8_t type;

	int8_t position;
	int8_t piece;

	int8_t split_count;
	int8_t split_positions[Board::SIZE];
	int8_t split_sizes[Board::SIZE];

	inline static int8_t piece_color(const Board& board) {
		if (board.moveno < 2)
			return -board.playerTurn;
		else
			return board.playerTurn;
	}

	void apply(Board& board) const {
		const int8_t piece_color = MoveInternal::piece_color(board);
		if (type == 1) {
			board.place(position, piece * piece_color);
			return ;
		}

		for (int8_t i = 0; i < split_count; ++i) {
			board.move(position, split_positions[i], split_sizes[i]);
		}

		if (type == TYPE_SPLIT_SQUASH) {
			const int8_t last = split_count - 1;
			const int8_t wall = board.stacks[last].top() > 0 ? PIECE_FLAT : -PIECE_FLAT;
			board.remove(last);
			board.place(last, wall);
		}
	}

	void revert(Board& board) const {
		const int8_t piece_color = MoveInternal::piece_color(board);
		if (type == 1) {
			board.place(position, piece * piece_color);
			return ;
		}

		if (type == TYPE_SPLIT_SQUASH) {
			const int8_t last = split_count - 1;
			const int8_t wall = board.stacks[last].top() > 0 ? PIECE_WALL : -PIECE_WALL;
			board.remove(last);
			board.place(last, wall);
		}

		for (int8_t i = split_count - 1; i >= 0; --i) {
			board.move(split_positions[i], position, split_sizes[i]);
		}
	}
};

std::vector<MoveInternal> allMoves;
MoveInternal placements[Board::SQUARES * 3];

struct _InitializeMoveCache {
	_InitializeMoveCache() {
		// placements.
		for (int i = 0; i < Board::SQUARES; ++i) {
			MoveInternal move;

			move.type = MoveInternal::TYPE_PLACE;
			move.position = i;

			move.piece = PIECE_FLAT;
			move.moveid = allMoves.size();
			allMoves.push_back(move);
			placements[i * 3] = move;

			move.piece = PIECE_WALL;
			move.moveid = allMoves.size();
			allMoves.push_back(move);
			placements[i * 3 + 1] = move;

			move.piece = PIECE_CAP;
			move.moveid = allMoves.size();
			allMoves.push_back(move);
			placements[i * 3 + 1] = move;
		}

		// vector moves
		for (int x = 0; x < Board::SQUARES; ++x) {
			
		}
	}
};
