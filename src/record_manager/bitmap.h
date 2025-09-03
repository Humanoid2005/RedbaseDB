#ifndef BITMAP_H
#define BITMAP_H

#include <cinttypes>
#include <cstring>

class Bitmap {
private:
    /*
    since bucket_number = (bitmap_index)/(sizeof bucket)
    */
    static int get_bucket(int pos) { 
        return pos / WIDTH; 
    }
  
    /*
    Gets the mask for that position to set,reset or test the bit in the bucket in a bitmap
    */
    static uint8_t get_bit(int pos) { 
        return HIGHEST_BIT >> (uint8_t)(pos % WIDTH); 
    }

public:
    static constexpr int WIDTH = 8;// We divide the bitmap into buckets of 8 size (1 byte).
    static constexpr unsigned HIGHEST_BIT = 0x80u;//10000000 --> highest bit for 1 byte entry
  
    /*
    Initialising the bitmap
    */
    static void init(uint8_t *bm, int size){ 
        memset(bm, 0, size); 
    }
  
    /*
    Setting the position in the bucket_number to used
    */
    static void set(uint8_t *bm, int pos) { 
        bm[get_bucket(pos)] |= get_bit(pos); 
    }
  
    /*
    Resetting the position in bucket_number to be free
    */
    static void reset(uint8_t *bm, int pos) { 
        bm[get_bucket(pos)] &= (uint8_t)~get_bit(pos); 
    }
  
    /*
    Checking if that position in the bucket_number is used/free
    */
    static bool test(const uint8_t *bm, int pos) { 
        return (bm[get_bucket(pos)] & get_bit(pos)) != 0; 
    }
  
    /*
    Used to search for next free or used space
    */
    static int next_bit(bool bit, const uint8_t *bm, int max_n, int curr) {
          for (int i = curr + 1; i < max_n; i++) {
              if (test(bm, i) == bit) {
                  return i;
              }
          }
          return max_n;
    }
  
    /*
    Search for first free or used space 
    */
    static int first_bit(bool bit, const uint8_t *bm, int max_n) { 
        return next_bit(bit, bm, max_n, -1); 
    }
};

#endif