#include <cstddef>
#include <cstdio>
#include <string.h>
#include <iostream>

namespace symbiosis {
  extern char *command_file;

  template <typename T, typename T2> T cast_dammit_cast(T2 what){
                  return *((T*) &what); }
  
  #define vararg_call(f) __vararg_call(cast_dammit_cast<void *>(f))
  #define call(f) __call(cast_dammit_cast<void *>(f))

  #define uchar unsigned char
  void find_space(uchar* start, size_t start_s, 
      uchar* end, size_t end_s, uchar **r, size_t *rs);
  #define CODE_START { volatile int i = 1234; i *= i + i; }
  #define CODE_END { volatile int i = 56789; i &= i - i; }
  #define DEF(name, block) \
    name##_s: \
    block \
    name##_e: 
  #define C(n) (uchar *)&&n##_s, (uchar *)&&n##_e - (uchar *)&&n##_s
  #define S ;
  #define R7(b) b S b
  //#define R6(b) R7(b) S R7(b) S R7(b) S R7(b)
  //#define R5(b) R6(b) S R6(b) S R6(b) S R6(b)
  //#define R4(b) R5(b) S R5(b) S R5(b) S R5(b)
  //#define R3(b) R4(b) S R4(b) S R4(b) S R4(b)
  //#define R2(b) R3(b) S R3(b) S R3(b) S R3(b)
  //#define R1(b) R2(b) S R2(b) S R2(b) S R2(b)
  #define R1(b) R7(b) S R7(b)
  #define RESERVE(b) R1(b)

  extern bool pic_mode;

#ifdef __PIC__
#define INIT_PIC_MODE pic_mode = true
#else
#define INIT_PIC_MODE pic_mode = false
#endif

  #define SYMBIOSIS_MAIN(b) int main(int argc, char **argv) { \
    DEF(start, { CODE_START; }); \
    INIT_PIC_MODE; \
    try { \
      symbiosis::init(argv[0], C(start), C(end)); \
      b; \
      symbiosis::finish(); \
    } catch (std::exception& e) { \
      cerr << "Exception: " << e.what() << endl; \
      exit(1); \
    } \
    DEF(reserved_code, { RESERVE({ size_t i = 0x20; }); }); \
    DEF(end, { CODE_END; }); \
  }

  constexpr short int T_INT = 1;
  constexpr short int T_UINT = 2;
  constexpr short int T_LONG = 3;
  constexpr short int T_ULONG = 4;
  constexpr short int T_CHARP = 5;
  constexpr short int T_FLOAT = 6;
  constexpr short int T_DOUBLE = 7;
  constexpr short int T_REGISTER = 8;

  class id {
  public:
    int register_value_ = -1;
    uchar *virtual_adr = 0;
    size_t virtual_size = 0;
    short int type;
    union {
      int i;
      unsigned int ui;
      long l;
      unsigned long ul;
      float f;
      double d;
      char *s;
    } d;
    id(int i) : type(T_INT) { d.i = i; }
    id(unsigned int ui) : type(T_UINT) { d.ui = ui; }
    id(long l) : type(T_LONG) { d.l = l; }
    id(unsigned long ul) : type(T_ULONG) { d.ul = ul; }
    id(const char *s) : type(T_CHARP) { d.s = (char *)s; }
    bool is_32() { 
        return (type == T_INT || type == T_UINT || type == T_CHARP); }
    bool is_64() { return (type == T_ULONG || type == T_LONG); }
    bool is_integer() { return type <= T_ULONG; }
    bool is_charp() { return type == T_CHARP; }
    bool is_p() { return is_charp(); }
    bool is_imm() { return is_integer(); }
    const char* i32() const;
    const char* i64();
    void describe();
    int register_value() { return register_value_; }
    void set_register_value(int rv) { register_value_ = rv; }
    bool is_register() { return register_value_ > -1; }
    id operator()(id p);
  };

  class id_new : public id {
  public:
    id_new(const char *s);
  };

  class id_register : public id {
  public:
    id_register(int r, int size); 
  };

  id add_parameter(id p);
  void __call(void *f);
  void __vararg_call(void *f);
  void init(char *c, uchar *start, size_t ss, uchar *end, size_t es); 
  void finish(); 
}
