#ifndef __BOARD_H_
#define __BOARD_H_

#include <bitset>
#include <cassert>
#include <cstring>
#include <functional>
#include <stdint.h>
#include <vector>
#include <string>
#include <ostream>

#include "hash.h"

// NOTE: this will not allow for the possibility of a stack height >48

const int8_t PIECE_FLAT = 1;
const int8_t PIECE_WALL = 2;
const int8_t PIECE_CAP = 3;

class Board;
class Stack;
class Move;


class Stack {
private:
	int8_t piece_top = 0;
	int8_t stack_height = 0;
	std::bitset<48> _stack = 0;
public:
	void push(int8_t piece) {
		_stack[stack_height] = piece > 0 ? 1 : 0;
		piece_top = piece;

		stack_height++;
	}

	inline int8_t top() const {
		return piece_top;
	}

	inline void pop() {
		assert(stack_height > 0);
		stack_height--;
		_stack[stack_height] = 0;
		piece_top = stack_height == 0 ? 0 : ((_stack[stack_height - 1] > 0) ? PIECE_FLAT : -PIECE_FLAT);
	}

	int8_t size() const {
		return stack_height;
	}

	inline const std::bitset<48>& stack() const {
		return _stack;
	}

	bool operator == (const Stack& other) const {
		return piece_top == other.piece_top && _stack == other._stack;
	}

	bool operator != (const Stack& other) const { return !(*this == other);}
};

class Move {
public:
	const uint32_t moveid;
	const uint64_t board_hash;

	Move(uint64_t board_hash, uint32_t moveid) : moveid(moveid), board_hash(board_hash) { };

	void apply(Board& board) const;
	void revert(Board& board) const;

	size_t hash() {
		size_t hash = std::hash<uint32_t>()(moveid);
		hash_combine(hash, board_hash);
		return hash;
	}

	bool operator == (const Move& other) const {
		// not much more that can be done in terms of making sure the boards are the same...
		// but probabilistically we should never even see a collision so this is fine.
		return moveid == other.moveid && board_hash == other.board_hash;
	}
};

class Board {
public:
	// TODO: enable support for multiple board sizes... ergh.
	const static int SIZE = 5;
	const static int SQUARES = 25;

	int moveno;
	int playerTurn;
	int capstones[2];
	int piecesleft[2];
	Stack stacks[SQUARES];

	Board() {
		bzero(this, sizeof(Board));
		moveno = 0;
		playerTurn = 1;

		capstones[0] = 1;
		capstones[1] = 1;
		piecesleft[0] = 21;
		piecesleft[1] = 21;
	}

	void move(int8_t fr, int8_t to, int8_t count) {
		if (count == 0) return ;
		int8_t temp = stacks[fr].top();
		move(fr, to, count - 1);
		stacks[to].push(temp);
	}

	void place(int8_t pos, int8_t piece) {
		stacks[pos].push(piece);
	}

	void remove(int8_t pos) {
		stacks[pos].pop();
	}

	uint64_t hash() {
		return murmurhash(this, sizeof(Board), 0);
	}

	std::vector<Move> getMoves() const;

	int isTerminalState(int8_t team) const; // returns 0 if no winner, -1 if black winner, 1 if white winner

	std::string toTBGEncoding() const;

	bool operator == (const Board& other) {
		return memcmp(this, &other, sizeof(Board)) == 0;
	};

	bool operator != (const Board& other) { return !(*this == other); }

	Board& operator=(const Board& other) {
		std::memcpy(this, &other, sizeof(Board));
		return *this;
	}
};

std::ostream& operator << (std::ostream& out, const Board& board);

#endif
