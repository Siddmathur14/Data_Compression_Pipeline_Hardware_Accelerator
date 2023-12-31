#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>
// #include "EventTimer.h"
#include <stdio.h>
//****************************************************************************************************************
// #define CAPACITY 32768 // hash output is 15 bits, and we have 1 entry per
// bucket, so capacity is 2^15 #define CAPACITY 4096
#define CAPACITY 65536
// try  uncommenting the line above and commenting line 6 to make the hash table
// smaller and see what happens to the number of entries in the assoc mem (make
// sure to also comment line 27 and uncomment line 28)

unsigned int my_hash(unsigned long key) {
    key &= 0xFFFFF; // make sure the key is only 20 bits

    unsigned int hashed = 0;

    for (int i = 0; i < 20; i++) {
        hashed += (key >> i) & 0x01;
        hashed += hashed << 10;
        hashed ^= hashed >> 6;
    }
    hashed += hashed << 3;
    hashed ^= hashed >> 11;
    hashed += hashed << 15;
    // return hashed & 0x7FFF;          // hash output is 15 bits
    // return hashed & 0xFFF;
    return hashed & 0xFFFF; // hash output is 16 bits
}

void hash_lookup(unsigned long *hash_table, unsigned int key, bool *hit,
                 unsigned int *result) {
    // std::cout << "hash_lookup():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits

    unsigned long lookup = hash_table[my_hash(key)];

    // [valid][value][key]
    unsigned int stored_key = lookup & 0xFFFFF;       // stored key is 20 bits
    unsigned int value = (lookup >> 20) & 0xFFF;      // value is 12 bits
    unsigned int valid = (lookup >> (20 + 12)) & 0x1; // valid is 1 bit

    if (valid && (key == stored_key)) {
        *hit = 1;
        *result = value;
        // std::cout << "\thit the hash" << std::endl;
        // std::cout << "\t(k,v,h) = " << key << " " << value << " " <<
        // my_hash(key) << std::endl;
    } else {
        *hit = 0;
        *result = 0;
        // std::cout << "\tmissed the hash" << std::endl;
    }
}

void hash_insert(unsigned long *hash_table, unsigned int key,
                 unsigned int value, bool *collision) {
    // std::cout << "hash_insert():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits
    value &= 0xFFF; // value is only 12 bits

    unsigned long lookup = hash_table[my_hash(key)];
    unsigned int valid = (lookup >> (20 + 12)) & 0x1;

    if (valid) {
        *collision = 1;
        // std::cout << "\tcollision in the hash" << std::endl;
    } else {
        hash_table[my_hash(key)] = (1UL << (20 + 12)) | (value << 20) | key;
        *collision = 0;
        // std::cout << "\tinserted into the hash table" << std::endl;
        // std::cout << "\t(k,v,h) = " << key << " " << value << " " <<
        // my_hash(key) << std::endl;
    }
}
//****************************************************************************************************************
typedef struct {
    // Each key_mem has a 9 bit address (so capacity = 2^9 = 512)
    // and the key is 20 bits, so we need to use 3 key_mems to cover all the key
    // bits. The output width of each of these memories is 64 bits, so we can
    // only store 64 key value pairs in our associative memory map.

    unsigned long upper_key_mem[512]; // the output of these  will be 64 bits
                                      // wide (size of unsigned long).
    unsigned long middle_key_mem[512];
    unsigned long lower_key_mem[512];
    unsigned int value[64]; // value store is 64 deep, because the lookup mems
                            // are 64 bits wide
    unsigned int fill;      // tells us how many entries we've currently stored
} assoc_mem;

// cast to struct and use ap types to pull out various feilds.

void assoc_insert(assoc_mem *mem, unsigned int key, unsigned int value,
                  bool *collision) {
    // std::cout << "assoc_insert():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits
    value &= 0xFFF; // value is only 12 bits

    if (mem->fill < 64) {
        mem->upper_key_mem[(key >> 18) % 512] |=
            (1 << mem->fill); // set the fill'th bit to 1, while preserving
                              // everything else
        mem->middle_key_mem[(key >> 9) % 512] |=
            (1 << mem->fill); // set the fill'th bit to 1, while preserving
                              // everything else
        mem->lower_key_mem[(key >> 0) % 512] |=
            (1 << mem->fill); // set the fill'th bit to 1, while preserving
                              // everything else
        mem->value[mem->fill] = value;
        mem->fill++;
        *collision = 0;
        // std::cout << "\tinserted into the assoc mem" << std::endl;
        // std::cout << "\t(k,v) = " << key << " " << value << std::endl;
    } else {
        *collision = 1;
        // std::cout << "\tcollision in the assoc mem" << std::endl;
    }
}

void assoc_lookup(assoc_mem *mem, unsigned int key, bool *hit,
                  unsigned int *result) {
    // std::cout << "assoc_lookup():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits

    unsigned int match_high = mem->upper_key_mem[(key >> 18) % 512];
    unsigned int match_middle = mem->middle_key_mem[(key >> 9) % 512];
    unsigned int match_low = mem->lower_key_mem[(key >> 0) % 512];

    unsigned int match = match_high & match_middle & match_low;

    unsigned int address = 0;
    for (int i = 0; i < 64; i++) {
        address |= ((match >> i) & 0x01) * (i + 1);
    }
    // std::cout << "first address is " << address << std::endl;
    // unsigned int address = 0;

    address = 0;
    for (; address < 64; address++) {

        if ((match >> address) & 0x1) {
            break;
        }
    }
    // std::cout << "second address is " << address << std::endl << std::endl;
    if (address != 64) {
        *result = mem->value[address];
        *hit = 1;
        // std::cout << "\thit the assoc" << std::endl;
        // std::cout << "\t(k,v) = " << key << " " << *result << std::endl;
    } else {
        *hit = 0;
        // std::cout << "\tmissed the assoc" << std::endl;
    }
}
//****************************************************************************************************************
void insert(unsigned long *hash_table, assoc_mem *mem, unsigned int key,
            unsigned int value, bool *collision) {
    hash_insert(hash_table, key, value, collision);
    if (*collision) {
        assoc_insert(mem, key, value, collision);
    }
}

void lookup(unsigned long *hash_table, assoc_mem *mem, unsigned int key,
            bool *hit, unsigned int *result) {
    hash_lookup(hash_table, key, hit, result);
    if (!*hit) {
        assoc_lookup(mem, key, hit, result);
    }
}
//****************************************************************************************************************
void hardware_encoding(unsigned char *input, int len, unsigned char *output,
                       unsigned int *output_len) {

    // create hash table and assoc mem
    unsigned long hash_table[CAPACITY];
    assoc_mem my_assoc_mem;

    // make sure the memories are clear
    for (int i = 0; i < CAPACITY; i++) {
        hash_table[i] = 0;
    }
    my_assoc_mem.fill = 0;
    for (int i = 0; i < 512; i++) {
        my_assoc_mem.upper_key_mem[i] = 0;
        my_assoc_mem.middle_key_mem[i] = 0;
        my_assoc_mem.lower_key_mem[i] = 0;
    }

    // init the memories with the first 256 codes
    for (unsigned long i = 0; i < 256; i++) {
        bool collision = 0;
        unsigned int key = (i << 8) + 0UL; // lower 8 bits are the next char,
                                           // the upper bits are the prefix code
        insert(hash_table, &my_assoc_mem, key, i, &collision);
    }
    int next_code = 256;

    unsigned int prefix_code = input[0];
    unsigned int code = 0;
    unsigned char next_char = 0;

    *output_len = 0;

    int i = 0;
    while (i < len) {
        if (i + 1 == len) {
            output[*output_len] = prefix_code;
            (*output_len)++;
            // std::cout << prefix_code;
            // std::cout << "\n";
            break;
        }
        next_char = input[i + 1];

        bool hit = 0;
        // std::cout << "prefix_code " << +prefix_code << " next_char " <<
        // +next_char << std::endl;
        lookup(hash_table, &my_assoc_mem, (prefix_code << 8) + next_char, &hit,
               &code);
        if (!hit) {
            if (prefix_code > 255) {
                printf("%d\n", prefix_code);
            }
            output[*output_len] = prefix_code;
            (*output_len)++;
            // std::cout << (prefix_code&0xFFF);
            // std::cout << "\n";

            bool collision = 0;
            insert(hash_table, &my_assoc_mem, (prefix_code << 8) + next_char,
                   next_code, &collision);
            if (collision) {
                std::cout
                    << "ERROR: FAILED TO INSERT! NO MORE ROOM IN ASSOC MEM!"
                    << std::endl;
                std::cout << std::endl
                          << "assoc mem entry count: " << my_assoc_mem.fill
                          << std::endl;
                return;
            }
            next_code += 1;

            prefix_code = next_char;
        } else {
            prefix_code = code;
            // std::cout << prefix_code << std::endl;
        }
        i += 1;
    }
    std::cout << std::endl
              << "assoc mem entry count: " << my_assoc_mem.fill << std::endl;
}
//****************************************************************************************************************
std::vector<int> encoding(std::string s1) {
    std::cout << "Encoding\n";
    std::unordered_map<std::string, int> table;
    for (int i = 0; i <= 255; i++) {
        std::string ch = "";
        ch += char(i);
        table[ch] = i;
    }

    std::string p = "", c = "";
    p += s1[0];
    int code = 256;
    std::vector<int> output_code;
    std::cout << "String\tOutput_Code\tAddition\n";
    for (int i = 0; i < s1.length(); i++) {
        if (i != s1.length() - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        } else {
            std::cout << p << "\t" << table[p] << "\t\t" << p + c << "\t"
                      << code << std::endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    std::cout << p << "\t" << table[p] << std::endl;
    output_code.push_back(table[p]);
    return output_code;
}

void decoding(std::vector<int> op) {
    std::cout << "\nDecoding\n";
    std::unordered_map<int, std::string> table;
    for (int i = 0; i <= 255; i++) {
        std::string ch = "";
        ch += char(i);
        table[i] = ch;
    }
    int old = op[0], n;
    std::string s = table[old];
    std::string c = "";
    c += s[0];
    std::cout << s;
    int count = 256;
    for (int i = 0; i < op.size() - 1; i++) {
        n = op[i + 1];
        if (table.find(n) == table.end()) {
            s = table[old];
            s = s + c;
        } else {
            s = table[n];
        }
        std::cout << s;
        c = "";
        c += s[0];
        table[count] = table[old] + c;
        count++;
        old = n;
    }
}
//****************************************************************************************************************
int main() {
    // std::string s = "WYS*WYGWYS*WYSWYSG";

    // std::cout << "Our message is: " << s << std::endl << std::endl;
    // std::cout << "Running the software compression we get: " << std::endl;

    // std::vector<int> output_code = encoding(s);
    // std::cout << "The compressed output stream is: ";
    // for (int i = 0; i < output_code.size(); i++) {
    //     std::cout << output_code[i] << " ";
    // }
    // std::cout << std::endl << std::endl;

    // std::cout << "Running the hardware version we get " << std::endl;
    // std::cout << "The compressed output stream is: " << std::endl;

    // EventTimer timer1;

#define BUFF_SIZE 4096

    unsigned char buffer[BUFF_SIZE];
    FILE *fp = fopen("prince.txt", "r");
    if (fp != NULL) {
        size_t newLen = fread(buffer, sizeof(unsigned char), BUFF_SIZE, fp);
        if (ferror(fp) != 0) {
            std::cerr << "Error reading file" << std::endl;
            fclose(fp);
            return 0;
        }
        fclose(fp);
    }

    // timer1.add("Main program");

    unsigned char out_buff[BUFF_SIZE];
    unsigned int output_len = 0;
    hardware_encoding(buffer, BUFF_SIZE, out_buff, &output_len);

    // timer1.finish();
    // timer1.print();
    return 0;
}
