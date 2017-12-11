#ifndef BIT_ARRAY
#define BIT_ARRAY
#include <stdexcept>
#include <climits>
#include <string>
#include <vector>
#include <algorithm>

template<class IType = size_t>
class BitArray {
	size_t sz;
	std::vector<IType> blocks;
	enum { BITS_PER_BLOCK = CHAR_BIT * sizeof(IType) };
	friend class bitproxy;
	class bitproxy {
		BitArray& b;
		int pos;
	public:
		bitproxy(BitArray& bs, int p) : b(bs) {
			pos = p;
		}
		bitproxy& operator=(bool val) {
			(val) ? b.set_bit(pos) : b.unset_bit(pos);
			return *this;
		}
		bitproxy& operator=(const bitproxy& bp) {
			(bp.b.read_bit(bp.pos)) ? 
				this->b.set_bit(pos) : this->b.unset_bit(pos);
			return *this;
		}
		operator bool() const {
			return b.read_bit(pos);
		}
	};

public:
	// Object Management
	explicit BitArray::BitArray(size_t count = 0) {
		sz = count;
		if (count > 0)this->blocks.resize(ceil(static_cast<double>(count) / BITS_PER_BLOCK));
	}
	explicit BitArray(const std::string& bits) {
		sz = bits.length();
		size_t index = 0;
		if (sz > 0) this->blocks.resize(ceil(static_cast<double>(sz) / BITS_PER_BLOCK));
		for_each(begin(bits), end(bits), [&index, this](const char& c) {
			if (c != '0' && c != '1') throw runtime_error("Invalid character. Only use '0' or '1'.");
			else if (c == '0') this->unset_bit(index);
			else if (c == '1') this->set_bit(index);
			index++;
		});
	}
	BitArray(const BitArray& b) = default;				// Copy constructor
	BitArray& operator=(const BitArray& b) = default;	// Copy assignment
	BitArray(BitArray&& b) noexcept {					// Move constructor
		this->sz = b.sz;
		auto temp = b.blocks;
		b.blocks = this->blocks;
		this->blocks = temp;
		cout << "Move constructor\n";
	}
	BitArray& operator=(BitArray&& b) noexcept {		// Move assignment
		this->sz = b.sz;
		auto temp = b.blocks;
		b.blocks = this->blocks;
		this->blocks = temp;
		cout << "Move assignment\n";
		return *this;
	}
	size_t capacity() const {	// # of bits the current allocation can hold
		return BITS_PER_BLOCK * this->blocks.size();
	}


	// Mutators
	BitArray& operator+=(bool b) {	// Append a bit
		insert(sz, b);
		return *this;
	}
	BitArray& operator+=(const BitArray& b) {	// Append a BitArray
		vector<IType> temp = b.blocks;		//Needed in case user tries to add a BitArray object to itself
		size_t initSize = b.size();			//Needed in case user tries to add a BitArray object to itself
		for (size_t i = 0; i < initSize; i++) this->operator+=((temp[i / BITS_PER_BLOCK] >> i % BITS_PER_BLOCK) & 1);
		return *this;
	}
	void erase(size_t pos, size_t nbits = 1) {	// Remove “nbits” bits at a position
		if (pos > sz) throw logic_error("Out of bounds.");
		size_t newSize = sz - nbits;
		BitArray temp(slice(pos + nbits, sz - (pos + nbits)));
		insert(pos, temp);
		this->sz = newSize;
	}
	void insert(size_t pos, bool b) {	// Insert a bit at a position (slide "right")
		if (pos > sz) throw logic_error("Out of bounds.");
		else if (blocks.size() == 0) blocks.resize(1);
		else if (sz == BITS_PER_BLOCK * blocks.size()) blocks.resize(blocks.size() + 1);
		for (int i = sz++; i > pos; i--) (read_bit(i - 1)) ? set_bit(i) : unset_bit(i);
		(b) ? set_bit(pos) : unset_bit(pos);
	}
	void insert(size_t pos, const BitArray& b) {	// Insert an entire BitArray object
		for (int i = 0; i < b.sz; i++) insert(pos + i, b.read_bit(i));
	}
	void shrink_to_fit() {
		// Discard unused, trailing vector cells
		if (sz < BITS_PER_BLOCK * (blocks.size() - 1)) {
			cout << "Shrinking from " << blocks.size() << " to " << ceil(static_cast<double>(sz) / BITS_PER_BLOCK) << " blocks.\n";
			blocks.resize(ceil(sz / BITS_PER_BLOCK));
		}
		else cout << "No shrinking needed.\n\n";
	}


	// Bitwise ops
	bitproxy operator[](size_t pos) {
		if (pos >= sz) throw logic_error("Out of bounds.");
		return bitproxy(*this, pos);
	}
	bool operator[](size_t pos) const {
		if (pos >= sz) throw logic_error("Out of bounds.");
		return this->read_bit(pos);
	}
	void toggle(size_t pos) {
		if (pos >= sz) throw logic_error("Out of bounds.");
		size_t block = pos / BITS_PER_BLOCK;
		size_t offset = pos % BITS_PER_BLOCK;
		auto mask = static_cast<IType> (1) << offset;
		this->blocks[block] ^= mask;
	}
	void toggle() {
		*this = this->operator~();
	}
	BitArray operator~() const {
		BitArray temp(this->sz);
		transform(begin(blocks), end(blocks), begin(temp.blocks), [](IType block) {return block ^ static_cast<IType>(~0u); });
		return BitArray<IType>(temp);
	}
	BitArray operator<<(unsigned int offset) const {	//Decreases value
		BitArray temp = *this;
		return temp.operator<<=(offset);
	}
	BitArray operator>>(unsigned int offset) const {	//Increases value
		BitArray temp = *this;
		return temp.operator>>=(offset);
	}
	BitArray& operator<<=(unsigned int offset) {	//Decreases value
		if (this->blocks.size() == 0) throw logic_error("Empty BitArray.");
		auto mask = (static_cast<IType>(1u) << offset) - 1;
		for (size_t i = 0; i < blocks.size(); i++) {
			//shift bits out of container, then copy bits over from next container (e.g. int, size_t, etc)
			blocks[i] >>= offset;
			if (blocks.size() > i + 1) {
				IType moveOver = blocks[i + 1] & mask;
				moveOver <<= (BITS_PER_BLOCK - offset);
				blocks[i] += moveOver;
			}
		}
		return *this;
	}
	BitArray& operator>>=(unsigned int offset) {	//Increases value
		if (this->blocks.size() == 0) throw logic_error("Empty BitArray.");
		for (size_t i = 0; i < offset; i++) {
			for (int j = blocks.size() - 1; j >= 0; j--) {	//shift one element at a time
				blocks[j] <<= 1;
				if (j != 0) (this->operator[]((j * BITS_PER_BLOCK) - 1)) ? set_bit(j * BITS_PER_BLOCK) : unset_bit(j * BITS_PER_BLOCK);
			}
		}
		return *this;
	}


	// Extraction ops
	BitArray slice(size_t pos, size_t count) const {	// Extracts a new sub-array
		BitArray temp;
		for (int i = 0; i < count; i++) temp.operator+=(read_bit(pos + i));
		return BitArray<IType>(temp);
	}


	// Comparison ops
	bool operator==(const BitArray& b) const {
		if (this->sz != b.sz) return false;
		for (size_t i = 0; i < this->blocks.size(); i++) if (this->blocks[i] != b.blocks[i]) return false;
		return true;
	}
	bool operator!=(const BitArray& b) const {
		return !this->operator==(b);
	}
	bool operator<(const BitArray& b) const {
		if (sz == 0 || b.sz == 0) throw logic_error("Empty BitArray.");
		if (this->operator==(b)) return false;
		for (size_t i = 0; i < min(this->sz, b.sz); i++) {
			if (this->read_bit(i) < b.read_bit(i)) return true;
			if (this->read_bit(i) > b.read_bit(i)) return false;
		}
		if (this->sz < b.sz) return true;
		return false;
	}
	bool operator<=(const BitArray& b) const {
		if (this->operator==(b)) return true;
		return this->operator<(b);
	}
	bool operator>(const BitArray& b) const {
		return !this->operator<=(b);
	}
	bool operator>=(const BitArray& b) const {
		return !this->operator<(b);
	}


	// Counting ops
	size_t size() const {	// Number of bits in use in the vector
		return this->sz;
	}
	size_t count() const {	// The number of 1-bits present
		size_t count = 0;
		for (size_t i = 0; i < this->sz; i++) if (this->read_bit(i) == true) count++;
		return count;
	}
	bool any() const {	// Optimized version of count() > 0
		bool anyTrue = false;
		for (size_t i = 0; i < this->sz; i++) if (this->read_bit(i) == true) anyTrue = true;
		return anyTrue;
	}


	// Stream I/O
	friend std::ostream& operator<<(std::ostream& os, const BitArray& b) {
		return os << b.to_string();
	}
	friend std::istream& operator >> (std::istream& is, BitArray& b) {
		std::string tempS = "";
		auto startPos = is.tellg();
		getline(is, tempS);
		is.seekg(startPos);

		char c = ' ';
		while ((is.good() && c != '0' && c != '1') || c == ' ') c = is.get();	//Gets rid of invalid characters and whitespace from the beginning of the stream
		if (is.eof()) is.setstate(std::istream::failbit);
		else {
			b.erase(0, b.size());
			do {
				b += (c == '1') ? true : false;
				c = is.get();
			} while (is.good() && (c == '0' || c == '1'));
		}
		is.unget();

		return is;
	}


	// String conversion
	std::string to_string() const {
		std::string ret = "";
		for (size_t i = 0; i < this->sz; i++) ret += (this->read_bit(i)) ? '1' : '0';
		return ret;
	}


	//Low-level functions to handle individual bits in any position
	bool read_bit(size_t pos) const {
		if (pos >= sz) throw logic_error("Out of bounds.");
		size_t block = pos / BITS_PER_BLOCK;
		size_t offset = pos % BITS_PER_BLOCK;
		return (blocks[block] >> offset) & 1;
	}
	void set_bit(const size_t pos) {
		if (pos >= sz) throw logic_error("Out of bounds.");
		size_t block = pos / BITS_PER_BLOCK;
		size_t offset = pos % BITS_PER_BLOCK;
		auto mask = static_cast<IType> (1) << offset;
		this->blocks[block] |= mask;
	}
	void unset_bit(const size_t pos) {
		if (pos >= sz) throw logic_error("Out of bounds.");
		size_t block = pos / BITS_PER_BLOCK;
		size_t offset = pos % BITS_PER_BLOCK;
		auto mask = ~(static_cast<IType> (1) << offset);
		this->blocks[block] &= mask;
	}
};
#endif // !BIT_ARRAY