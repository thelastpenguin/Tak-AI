#include <sstream>
#include <iostream>

#include "board.h"
#include "termcolor.h"
#include "helpers.h"

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


bool squaresAreConnected(uint64_t sq1, uint64_t sq2, uint64_t path)
{
	// With bitboard sq1, do an 8-way flood fill, masking off bits not in
	// path at every step. Stop when fill reaches any set bit in sq2, or
	// fill cannot progress any further

	if (!(sq1 &= path) || !(sq2 &= path)) return false;
	// Drop bits not in path
	// Early exit if sq1 or sq2 not on any path

	while(!(sq1 & sq2))
	{
	U64 temp = sq1;
	sq1 |= eastOne(sq1) | westOne(sq1);    // Set all 8 neighbours
	sq1 |= soutOne(sq1) | nortOne(sq1);
	sq1 &= path;                           // Drop bits not in path
	if (sq1 == temp) return false;         // Fill has stopped
	}
	return true;                              // Found a good path
}

int Board::isTerminalState(int8_t team) const {
	uint8_t num_pieces = 0;
	int8_t piece_positions[Board::SQUARES];
	int8_t top[Board::SQUARES];

	for (int i = 0; i < Board::SQUARES; ++i) {
		top[i] = stacks[i].top();
		if (top[i] * team > 0) {
			piece_positions[num_pieces++] = i;
		}
	}

	for (int i = 0; i < Board::SQUARES; ++i) {

	}

	return 0;
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

namespace movegen {
	std::vector<MoveInternal> all_moves;

	MoveInternal placements[Board::SQUARES * 3];

	std::vector<MoveInternal> cuts[Board::SQUARES][Board::SIZE + 1]; // 0 index isn't used
	std::vector<MoveInternal> cuts_flatten[Board::SQUARES][Board::SIZE + 1]; // 0 index isn't used


	// generate all sequences of integers that reach the sum 'n'
	void target_sum(int n, std::vector<int> current, std::vector<std::vector<int>>& results) {
		for (int i = 1; i < n; ++i) {
			std::vector<int> copy = current;
			copy.push_back(i);
			target_sum(n - i, copy, results);
		}
		std::vector<int> copy = current;
		copy.push_back(n);
		results.push_back(copy);
	}

	// generate all moves that reach length given
	std::vector<MoveInternal> generate_moves(int x, int y, int dx, int dy, int length) {
		std::vector<MoveInternal> result;

		int d = dx + dy * Board::SIZE;
		std::vector<std::vector<int>> vectors;
		target_sum(length, std::vector<int>(), vectors);

		std::sort(vectors.begin(), vectors.end(), [](std::vector<int>& a, std::vector<int>& b) {
			return a.size() < b.size();
		});

		for (std::vector<int>& vec : vectors) {
			MoveInternal move;
			move.position = x + y * Board::SIZE;
			move.split_count = vec.size();
			for (int i : range(0, vec.size())) {
				move.split_positions[i] = move.position + d * (i + 1);
				move.split_sizes[i] = vec[i];
			}
		}

		return result;
	}

	struct _InitializeMoveCache {

		void generate_cuts_for_position(int x, int y) {
			const int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

			for (const int* d : directions) {
				int dx = d[0];
				int dy = d[1];
				int maxrange = Board::SIZE;
				if (dx < 0)
					maxrange = std::min(maxrange, x);
				if (dx > 0)
					maxrange = std::min(maxrange, Board::SIZE - x - 1);
				if (dy < 0)
					maxrange = std::min(maxrange, y);
				if (dy > 0)
					maxrange = std::min(maxrange, Board::SIZE - y - 1);


				if (maxrange == 0) continue ;
				for (int distance : range(1, maxrange)) {
					std::vector<MoveInternal> moves = generate_moves(x, y, dx, dy, distance);

					// NOTE: adds normal moves
					for (auto& move : moves) {
						move.type = MoveInternal::TYPE_SPLIT;
						move.moveid = all_moves.size();
						all_moves.push_back(move);
						cuts[x + y * Board::SIZE][distance].push_back(move);
					}

					// NOTE: adds flatten moves
					for (auto& move : moves) {
						if (move.split_sizes[move.split_count - 1] != 1) continue ;
						move.type = MoveInternal::TYPE_SPLIT;
						move.moveid = all_moves.size();
						all_moves.push_back(move);
						cuts_flatten[x + y * Board::SIZE][distance].push_back(move);
					}
				}
			}
		}

		void generate_placements_for_position(int x, int y) {
			int i = x + y * Board::SIZE;

			MoveInternal move;

			move.type = MoveInternal::TYPE_PLACE;
			move.position = i;

			move.piece = PIECE_FLAT;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i * 3] = move;

			move.piece = PIECE_WALL;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i * 3 + 1] = move;

			move.piece = PIECE_CAP;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i * 3 + 1] = move;
		}

		void generate_moves_for_position(int x, int y) {
			generate_placements_for_position(x, y);
			generate_cuts_for_position(x, y);
		}

		_InitializeMoveCache() {
			for (int y : range(0, Board::SIZE)) {
				for (int x : range(0, Board::SIZE)) {
					generate_moves_for_position(x, y);
				}
			}
		}

	};

	_InitializeMoveCache _movecache;

}
