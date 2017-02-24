#include <iostream>
#include <unistd.h>
#include <set>
#include <cassert>

#include "board.h"

int main() {
	Board board;
	int count = 0;
	for (auto move1 : board.get_moves()) {
		move1.apply(board);
		if (count == 0)
			std::cout << board << std::endl;
		for (auto move2 : board.get_moves()) {
			move2.apply(board);
			if (count == 0)
				std::cout << board << std::endl;
			for (auto move3 : board.get_moves()) {
				move3.apply(board);
				if (count == 0)
					std::cout << board << std::endl;
				for (auto move4 : board.get_moves()) {
					move4.apply(board);
					if (count == 0)
						std::cout << board << std::endl;
					count++;
					move4.revert(board);
				}
				move3.revert(board);
			}
			move2.revert(board);
			break ;
		}
		move1.revert(board);
	}
	std::cout << count << std::endl;
}
