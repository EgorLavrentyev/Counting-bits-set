#include <iostream>
#include <iomanip>
#include <time.h>
#include <chrono>

#define DUMP_SIZE 15000000
#define W 15


unsigned char BitsSetTable256[256];
double avg_time;

void fill_dump(unsigned char dump[]) {
	for (int i = 0; i < DUMP_SIZE; i++) {
		dump[i] = rand() % 255;
	}
}

uint32_t get_word(unsigned char* mass, int index) {
	size_t bit_index = W * index;
	size_t byte_index = bit_index / 8;
	unsigned bit_in_byte_index = bit_index % 8;
	uint32_t result = mass[byte_index] >> bit_in_byte_index;
	for (unsigned n_bits = 8 - bit_in_byte_index; n_bits < W; n_bits += 8)
		result |= mass[++byte_index] << n_bits;
	return result & ~(~0u << W);
}

unsigned int get_weight_naive(uint32_t word) {
	unsigned int c = 0;
	while (word) {
		c += word & 1;
		word >>= 1;
	}

	return c;
}

unsigned int get_weight_table(uint32_t word) {
	unsigned int c = 0;
	c = BitsSetTable256[word & 0xff] +
		BitsSetTable256[(word >> 8) & 0xff] +
		BitsSetTable256[(word >> 16) & 0xff] +
		BitsSetTable256[word >> 24];
	return c;
}

unsigned int get_weight_kernighan(uint32_t word) {
	unsigned int c = 0; // c accumulates the total bits set in v
	for (c = 0; word; c++)
	{
		word &= word - 1; // clear the least significant bit set
	}
	return c;
}

unsigned int get_weight_64(uint32_t word) {
	unsigned int c = 0;
	c = ((word & 0xfff) * 0x1001001001001ULL & 0x84210842108421ULL) % 0x1f;
	c += (((word & 0xfff000) >> 12) * 0x1001001001001ULL & 0x84210842108421ULL) %
		0x1f;
	c += ((word >> 24) * 0x1001001001001ULL & 0x84210842108421ULL) % 0x1f;
	return c;
}

unsigned int get_weight_parallel(uint32_t word) {
	unsigned int c = 0; // store the total here
	static const int S[] = { 1, 2, 4, 8, 16 }; // Magic Binary Numbers
	static const int B[] = { 0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF };

	c = word - ((word >> 1) & B[0]);
	c = ((c >> S[1]) & B[1]) + (c & B[1]);
	c = ((c >> S[2]) + c) & B[2];
	c = ((c >> S[3]) + c) & B[3];
	c = ((c >> S[4]) + c) & B[4];

	return c;
}

unsigned int get_weight_best(uint32_t word) {
	unsigned int c = 0;
	word = word - ((word >> 1) & 0x55555555);                    // reuse input as temporary
	word = (word & 0x33333333) + ((word >> 2) & 0x33333333);     // temp
	c = ((word + (word >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

	return c;
}

void print_word(uint32_t a, int size) {
	int counter = 1;
	for (int i = 0; i < size; i++, counter++) {
		if (a & 1 << (size - i - 1)) {
			std::cout << "1 ";
		}
		else
			std::cout << "0 ";
		if (counter != 0 && counter % 8 == 0)
		{
			std::cout << " ";
		}
	}
}

void print_bytes(unsigned char* a, int size) {
	for (int k = 0; k < size; k++) {
		for (int i = 0; i < 8; i++)
			if (a[size - k - 1] & 1 << (8 - i - 1)) {
				std::cout << "1 ";
			}
			else
				std::cout << "0 ";
		
		std::cout << " ";
	}
}

int* get_report(unsigned char* dump) {
	int* weights = new int[W + 1];
	for (int i = 0; i <= W; i++) {
		weights[i] = 0;
	}
	
	uint32_t* words = new uint32_t[DUMP_SIZE * 8 / W];
	for (int i = 0; i < DUMP_SIZE * 8 / W; i++) {
		words[i] = get_word(dump, i);
	}

	for (int i = 0; ; i++) {
		if (W * (i + 1) > DUMP_SIZE * 8) {
			return weights;
		}

		unsigned int index = get_weight_best(words[i]);
		weights[index] ++;
	}
	
	return nullptr;
}

void display_report(int* weights, unsigned char* dump) {
	int total_weight = 0;
	int sum = 0;
	//weights = get_report(dump);
	for (int i = 0; i < DUMP_SIZE; i++) {
		total_weight += get_weight_naive(dump[i]);
	}

	std::cout << "\nDump size = " << DUMP_SIZE << " bytes" << std::endl << "Word size = " << W << " bits" << std::endl;

	std::cout << std::endl << " |  Weight | Number of words" << std::endl;
	std::cout << " ____________________________" << std::endl;
	for (int i = 0; i <= W; i++) {
		sum += i * weights[i];
		std::cout <<" |   "<< std::setw(2) << i << "    |       " << weights[i] << "        " << std::endl;
		std::cout << " ____________________________" << std::endl;
	}

	//std::cout << "Total weight = " << total_weight << std::endl << "Sum of weights of all words = " << sum << std::endl;
	//std::cout << "AVG TIME: " << avg_time / (DUMP_SIZE * 8 / W) << " nanoseconds" << std::endl;
}

int main(void) {

	BitsSetTable256[0] = 0;

	for (int i = 0; i < 256; i++)
	{
		BitsSetTable256[i] = (i & 1) + BitsSetTable256[i / 2];
	}

	unsigned char* dump = new  unsigned char[DUMP_SIZE];
	fill_dump(dump);

	//print_bytes(dump, DUMP_SIZE);

	int* weights = get_report(dump);
	display_report(weights, dump);

	return 0;
}