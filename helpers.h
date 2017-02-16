#ifndef __HELPERS_H_
#define __HELPERS_H_

#include <stdint.h>

class range {
 public:
   class iterator {
      friend class range;
    public:
      long int operator *() const { return i_; }
      const iterator &operator ++() { ++i_; return *this; }
      iterator operator ++(int) { iterator copy(*this); ++i_; return copy; }

      bool operator ==(const iterator &other) const { return i_ == other.i_; }
      bool operator !=(const iterator &other) const { return i_ != other.i_; }

    protected:
      iterator(long int start) : i_ (start) { }

    private:
      unsigned long i_;
   };

   iterator begin() const { return begin_; }
   iterator end() const { return end_; }
   range(long int  begin, long int end) : begin_(begin), end_(end) {}
private:
   iterator begin_;
   iterator end_;
};

// NOTE: taken from the chess programming wiki
template<typename moveInDirections>
bool squaresAreConnected(uint64_t sq1, uint64_t sq2, uint64_t path) {
	// With bitboard sq1, do an 8-way flood fill, masking off bits not in
	// path at every step. Stop when fill reaches any set bit in sq2, or
	// fill cannot progress any further

	if (!(sq1 &= path) || !(sq2 &= path)) return false;
	// Drop bits not in path
	// Early exit if sq1 or sq2 not on any path

	while(!(sq1 & sq2))
	{
		uint64_t temp = sq1;
		sq1 |= moveInDirections()(sq1);
		// sq1 |= eastOne(sq1) | westOne(sq1);    // Set all 8 neighbours
		// sq1 |= soutOne(sq1) | nortOne(sq1);
		sq1 &= path;                           // Drop bits not in path
		if (sq1 == temp) return false;         // Fill has stopped
	}
	return true;                              // Found a good path
}


#endif
