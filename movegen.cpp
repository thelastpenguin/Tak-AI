#include <algorithm>
#include "board.h"
#include "helpers.h"

/**
	move generation
*/

struct MoveInternal {
	const static int8_t TYPE_PLACE = 1;
	const static int8_t TYPE_SPLIT = 2;
	const static int8_t TYPE_SPLIT_SQUASH = 3;

	MoveInternal() { bzero(this, sizeof(MoveInternal)); };

	uint32_t moveid;

	int8_t type;

	int8_t position;
	int8_t piece;

	int8_t split_count;
	int8_t split_positions[Board::SIZE];
	int8_t split_sizes[Board::SIZE];

	inline static int8_t piece_color(const Board& board) {
		return board.placementColor();
	}

	bool can_move(const Board& board) {
		if (type == TYPE_PLACE) {
			int8_t piece = this->piece * MoveInternal::piece_color(board);
			if (board.moveno < 2) return piece == PIECE_FLAT || piece == -PIECE_FLAT;
			switch (piece) {
			case PIECE_CAP: return board.capstones[0] > 0;
			case -PIECE_CAP: return board.capstones[1] > 0;
			case PIECE_WALL:
			case PIECE_FLAT: return board.piecesleft[0] > 0;
			case -PIECE_WALL:
			case -PIECE_FLAT: return board.piecesleft[1] > 0;
			default:
				exit(0);
			}
		} else if(type == TYPE_SPLIT) {
			const int8_t landingOn = board.stacks[split_positions[split_count - 1]].top();
			return landingOn == 0 || landingOn == -PIECE_FLAT || landingOn == PIECE_FLAT;
		} else if (type == TYPE_SPLIT_SQUASH) {
			const int8_t landingOn = std::abs(board.stacks[split_positions[split_count - 1]].top());
			return landingOn == PIECE_WALL;
		}
		return false;
	}

	void apply(Board& board) const {
		const int8_t piece_color = MoveInternal::piece_color(board);
		board.playerTurn = -board.playerTurn;
		board.moveno++;
		if (type == TYPE_PLACE) {
			board.place(position, piece * piece_color);
			switch (piece * piece_color) {
			case PIECE_CAP: board.capstones[0]--; break ;
			case -PIECE_CAP: board.capstones[1]--; break ;
			case PIECE_FLAT:
			case PIECE_WALL: board.piecesleft[0]--; break ;
			case -PIECE_FLAT:
			case -PIECE_WALL: board.piecesleft[1]--; break ;
			}
			return ;
		}

		for (int8_t i = split_count - 1; i >= 0; --i) {
			board.move(position, split_positions[i], split_sizes[i]);
		}
	}

	void revert(Board& board) const {
		board.moveno--;
		board.playerTurn = -board.playerTurn;
		if (type == TYPE_PLACE) {
			const int8_t piece_color = MoveInternal::piece_color(board);
			board.remove(position);
			switch (piece * piece_color) {
			case PIECE_CAP: board.capstones[0]++; break ;
			case -PIECE_CAP: board.capstones[1]++; break ;
			case PIECE_FLAT:
			case PIECE_WALL: board.piecesleft[0]++; break ;
			case -PIECE_FLAT:
			case -PIECE_WALL: board.piecesleft[1]++; break ;
			}
			return ;
		}

		for (int8_t i = 0; i < split_count; ++i) {
			board.move(split_positions[i], position, split_sizes[i]);
		}

		if (type == TYPE_SPLIT_SQUASH) {
			const int8_t last = split_positions[split_count - 1];
			const int8_t wall = board.stacks[last].top() > 0 ? PIECE_WALL : -PIECE_WALL;
			board.remove(last);
			board.place(last, wall);
		}
	}
};

namespace movegen {
	std::vector<MoveInternal> all_moves;

	MoveInternal placements[Board::SQUARES][3];

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
	std::vector<MoveInternal> generate_moves(int x, int y, int dx, int dy, int distance, int piecesUsed) {
		std::vector<MoveInternal> result;

		int d = dx + dy * Board::SIZE;
		std::vector<std::vector<int>> vectors;
		target_sum(piecesUsed, std::vector<int>(), vectors);

		std::sort(vectors.begin(), vectors.end(), [](std::vector<int>& a, std::vector<int>& b) {
			return a.size() < b.size();
		});

		for (std::vector<int>& vec : vectors) {
			if (vec.size() > (size_t)distance) continue;
			MoveInternal move;
			move.position = x + y * Board::SIZE;
			move.split_count = vec.size();
			for (int i : range(0, vec.size())) {
				move.split_positions[i] = move.position + d * (i + 1);
				move.split_sizes[i] = vec[i];
			}
			result.push_back(move);
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
				for (int pieceCount : range(1, Board::SIZE + 1)) {
					std::vector<MoveInternal> moves = generate_moves(x, y, dx, dy, std::min(maxrange, pieceCount), pieceCount);

					// NOTE: adds normal moves
					for (auto& move : moves) {
						move.type = MoveInternal::TYPE_SPLIT;
						move.moveid = all_moves.size();
						all_moves.push_back(move);
						cuts[INDEX_BOARD(x, y)][pieceCount].push_back(move);
					}

					// NOTE: adds flatten moves
					for (auto& move : moves) {
						if (move.split_sizes[move.split_count - 1] != 1) continue ;
						move.type = MoveInternal::TYPE_SPLIT_SQUASH;
						move.moveid = all_moves.size();
						all_moves.push_back(move);
						cuts_flatten[INDEX_BOARD(x, y)][pieceCount].push_back(move);
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
			placements[i][0] = move;

			move.piece = PIECE_WALL;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i][1] = move;

			move.piece = PIECE_CAP;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i][2] = move;
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


std::vector<Move> Board::get_moves(int8_t team) const {
	uint64_t boardhash = this->hash();

	std::vector<Move> moves;
	for (int i = 0; i < Board::SQUARES; ++i) {
		const Stack& stack = stacks[i];
		int8_t top = stack.top();

		if (top == 0) {
			// NOTE: these moves are for the top of the stack
			for (int j = 0; j < 3; ++j) {
				MoveInternal& move = movegen::placements[i][j];
				if (move.can_move(*this))
					moves.push_back(Move(boardhash, move.moveid));
			}
		} else if (top * team > 0 && moveno >= 2) {
			bool stop = false;
			int limit = 5 < stack.size() ? 5 : stack.size();
			for (int j = 1; j <= limit && !stop; ++j) {
				for (MoveInternal& move : movegen::cuts[i][j]) {
					if (move.can_move(*this)) {
						moves.push_back(Move(boardhash, move.moveid));
					} else
						stop = true;
				}
				if (stop && top * team == PIECE_CAP)
					for (MoveInternal& move : movegen::cuts_flatten[i][j]) {
						if (move.can_move(*this)) {
							moves.push_back(Move(boardhash, move.moveid));
						}
					}
			}
		}
	}

	return moves;
};

void Move::apply(Board& board) const {
	movegen::all_moves[moveid].apply(board);
}

void Move::revert(Board& board) const {
	movegen::all_moves[moveid].revert(board);
}
