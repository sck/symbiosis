// symbiosis: A hacky and lightweight compiler framework
//
#include "symbiosis.hpp"
#include <sys/stat.h>

#include <iostream>

#include <vector>

using namespace std;

namespace symbiosis {

  class exception : public std::exception {
    const char *what_;
  public:
    exception(const char *w) : what_(w) { }
    virtual ~exception() throw() { }
    virtual const char* what() const throw() { return what_; }
  };

  char *command_file = 0;;

  
  #define M10(v, b) #v #b
  #define M3(a,b) M10(a+1000, v),M10(a+2000, v), M10(a+3000, v),M10(a+4000, v)
  #define M2(a,b) M3(a+100, v) , M3(a+200, v) , M3(a+300, v) , M3(a+400, v)
  #define M1(a,b) M2(a+10, v) , M2(a+20, v) , M2(a+30, v) , M2(a+40, v)
  #define M(v) M1(1, v), M1(2, v), M1(3, v), M1(4, v), M1(5, v)

  uchar *virtual_code_start = 0;
  uchar *virtual_code_end = 0;
  
  uchar *out_code_start = 0;
  uchar *out_code_end = 0;
  uchar *out_c = 0;
  uchar *out_c0 = 0;
  
  uchar *virtual_strings_start = 0;
  uchar *virtual_strings_end = 0;
  uchar *out_strings_start = 0;
  uchar *out_strings_end = 0;
  uchar *out_s = 0;

  #define STRINGS_START "STRINGS_START"
  #define STRINGS_END "STRINGS_END"

  unsigned int _i32 = 0;

  const char* id::i32() {
    if (virtual_adr) { _i32 = (size_t)virtual_adr; printf("virtual_adr: %x\n", _i32);
        return (const char*)&_i32; }
    if (type == T_UINT) return (const char*)&d.ui;
    return (const char*)&d.i;
  }
  const char* id::i64() {
    if (type == T_ULONG) return (const char*)&d.ul;
    return (const char*)&d.l;
  }
  vector<const char*> type_str = { "0:???", "int", "uint", "long", "ulong", 
      "charp", "float", "double"};

  void id::describe() { 
    if (type > type_str.size() - 1) {
      cout << "<id:" << type << ">";
    } else {
      cout << type_str[type];
    }
    cout << endl;
  }
  id  id::operator()(id i) { add_parameter(i); return i; }

  id_new::id_new(const char *p) : id(p) {
    virtual_adr = virtual_strings_start + (out_s - out_strings_start);
    virtual_size = strlen(p);
    if (out_s + virtual_size + 1 > out_strings_end) 
        throw exception("Strings: Out of memory");
    memcpy(out_s, p, virtual_size + 1);
    cout << "new string" << endl;
    out_s += virtual_size + 1;
  };  

  
  const char *reserved_strings[] = { STRINGS_START,
  M(ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc)
  STRINGS_END };

#define FIND(v) { \
  pos = 0; \
  bool found = false; \
  for (size_t i = 0; !found && i < (input_size - v##_s); i++) { \
    for (int j = 0; j < v##_s; j++) { \
      if (buf[i + j] != v[j]) break; \
      if (j == v##_s - 1) { \
        pos = i; \
        found = true; \
      } \
    } \
  } }

  uchar buf[200 * 1024];
  FILE* input = 0;
  size_t input_size = 0;

  void find_space(uchar* start, size_t start_s, 
      uchar* end, size_t end_s, uchar **out_start, uchar **out_end) {
    if (!input) { 
      input = fopen(command_file, "r");
      if (!input) { fprintf(stderr, "Failed\n"); }
      input_size = fread(buf, 1, sizeof(buf), input);
    }
    size_t pos = 0, s = 0, e = 0;
    FIND(start); s = pos;
    FIND(end); e = pos;
    *out_start = &buf[s];
    *out_end = &buf[e];
    //printf("s: %zd, e:%zd, l:%zd\n", s, e, e - s);
  }

  void write() {
    FILE *out = fopen("a.out", "w");
    fwrite(buf, 1, input_size, out);
    fclose(out);
    printf("Writing a.out\n");
    chmod("a.out", 0777);
  }
  
  void dump(uchar* start, size_t s) {
    for (size_t i = 0; i < s; i++) {
      printf("%02x ", start[i]);
    }
    printf("\n");
  }

//#define APPEND_CODE(n) { \
//    out_c0 = c; \
//    uchar *s = &&n##_s; uchar *e = &&n##_e; uchar *b; \
//    for (b  = s; b < e; b++, c++) { \
//      *c = *b; \
//    } }

  //void relocate_call(uchar *buf_p, size_t l, uchar *f) {
  //  uchar *source = (uchar *)ten_parameters;
  //  uchar *p = buf_p;
  //  size_t buf_start_distance = buf_p - code;
  //  size_t virtual_code_address_start = (size_t)code_start + buf_start_distance;
  //  for (size_t i = 0; i < l; i++) {
  //    if (p[i] == 0xe8 && (i + 4 < l) ) {
  //      int *relative_address = (int *)(p + i + 1);
  //      printf("x86-64 call: ");
  //      dump((uchar *)relative_address - 1, 5);
  //      uchar *virtual_jump_relative_address  = 
  //          (uchar *)(virtual_code_address_start + i + 5);
  //      int r = virtual_jump_relative_address - f;
  //      *relative_address = -r;
  //    }
  //    if (p[i] == 0xeb) {
  //      int *relative_address = (int *)(p - 4);
  //      printf("armv6 call: ");
  //      dump((uchar *)relative_address, 4);
  //      uchar *virtual_jump_relative_address  = 
  //          (uchar *)(virtual_code_address_start + i + 5);
  //      int r = virtual_jump_relative_address - f;
  //      *relative_address = -r;
  //    }
  //  }
  //}

#define P(n) (n * (0x1UL << ((sizeof(size_t) - 1) * 8)))

  //void set_parameter(uchar *buf_p, size_t l, int n, size_t v) {
  //  size_t p_id = P(n);
  //  printf("p_id: %zx\n", p_id);
  //  for (size_t i = 0; i + sizeof(size_t) < l; i++) {
  //    size_t *u = (size_t *)(buf_p + i);
  //    if (*u == p_id) {
  //      (*u) = v;
  //      return;
  //    }
  //  }
  //  printf("Set parameter failed!\n");
  //}

//#define APPEND_CALL(f) { \
//    APPEND_CODE(local_call); \
//    relocate_call(c0, c - c0, (uchar *)f); \
//  }

//#define SET_PARAMETER(n, v) set_parameter(c0, c - c0, n, (size_t)v)
//int is_big_endian() { 
//  union { int i; char c[4]; } e = {0x12345678}; 
//  printf("union { int i; char c[4]; } e = {0x12345678} -> ");
//  printf("0x%02x%02x%02x%02x\n", e.c[0], e.c[1], e.c[2], e.c[3]);
//  return e.c[0] == 0x12;
//}

//int read_func_address(uchar *call, size_t l) {
//  uchar *p = call;
//  for (size_t i = 0; i < l; i++) {
//    if (p[i] == 0xe8 && (i + 4 < l) ) {
//      int *relative_address = (int *)(p + i + 1);
//      printf("x86-64 call: ");
//      dump((uchar *)relative_address - 1, 5);
//      return *relative_address;
//    }
//    if (p[i] == 0xeb) {
//      int *relative_address = (int *)(p - 4);
//      printf("armv6 call: ");
//      dump((uchar *)relative_address, 4);
//      return *relative_address;
//    }
//  }
//  fprintf(stderr, "No address found!\n");
//  exit(2);
//  return 0;
//}

//#define __READ_FUNC_ADDR(n,l) \
//  int __##n = read_func_address((uchar *)&&n##_s, l); 
//
//#define READ_FUNC_ADDR(n) __READ_FUNC_ADDR(n, &&n##_e - &&n##_s)
//
//#define GET_FUNC_ADR(n, b) \
//  READ_FUNC_ADDR(n); \
//  n##_s: \
//  b; \
//  n##_e: \
//  void init() {
//    asm("cpuid");
//  }

  // intel
  // first: bf 78 56 34 12          mov    edi,0x12345678
  // TODOS: 
  // - save string parameter
  // - save values for arm
  int parameter_count = 0;
  const char *register_parameters_intel_32[] = {
      "\xbf", /*edi*/ "\xbe", /*esi*/ "\xba" /*edx*/ };
  const char *register_parameters_arm_32[] = {
      "\xe5\x9f\x00", /*r0*/ "\xe5\x9f\x10", /*r1*/ "\xe5\x9f\x20" /*r2*/ };
  const char *register_parameters_intel_64[] = {
      "\x48\xbf" /*rdi*/, "\x48\xbe" ,/*rsi*/ "\x48\xba" /*rdx*/ };

  constexpr int parameters_max = 3;

  void emit(const char* _s, size_t _l = 0) { 
    size_t l = _l > 0 ? _l : strlen(_s);
    uchar *s = (uchar *)_s; uchar *e = s + l;
    if (out_c + l > out_code_end) throw exception("Code: Out of memory"); 
    for (uchar * b  = s; b < e; b++, out_c++) { 
      *out_c = *b; 
    }
    dump(s, l);
  }

  bool intel() { return true; }
  bool arm() { return false; }

  id add_parameter(id p) {
    if (parameter_count >= parameters_max) {
      fprintf(stderr, "Too many parameters!\n");
      return p;
    }
    cout << "parameter_count: " << parameter_count << " "; p.describe();
    if (p.is_integer() || p.is_charp()) {
      cout << "is_integer" << endl;
      if (intel()) {
        cout << "intel" << endl;
        if (p.is_32()) {
          cout << "is_32" << endl;
          emit(register_parameters_intel_32[parameter_count]);
          emit(p.i32(), 4);
        } else if (p.is_64()) {
          cout << "is_64" << endl;
          emit(register_parameters_intel_64[parameter_count]);
          emit(p.i64(), 8);
        }
      } else if (arm()) {
        emit(register_parameters_intel_32[parameter_count]);
      }
    }
    ++parameter_count;
    return p;
    //switch (p_) {
    //  case 0: { emit("\xbf"); emit(i32); break }; // mov edi, i32 }
    //  case 0: { emit("\x48\xbf"); emit(i64); break }; // movabs rdi, i64 }
    //  case 0: { emit("\xe5\x9f\x00"); emit(offset); break }; // ldr r0, [pc, #]

    //  case 1: { emit("\xbe"); emit(i32); break }; // mov esi, i32 }
    //  case 1: { emit("\x48\xbe"); emit(i64); break }; // movabs rsi, i64 }
    //  case 1: { emit("\xe5\x9f\x10"); emit(offset); break }; // ldr r1, [pc, #]

    //  case 2: { emit("\xba"); emit(i32); break }; // mov edx, i32 }
    //  case 2: { emit("\x48\xba"); emit(i64); break }; // movabs rdx, i64 }
    //  case 2: { emit("\xe5\x9f\x20"); emit(offset); break }; // ldr r2, [pc, #]
    //}
  }
  int __offset = 0;
  const char* call_offset(uchar *out_current_code_pos, void *__virt_f) { 
    auto virt_f = (uchar*)__virt_f;
    ssize_t out_start_distance = out_current_code_pos - out_code_start;
    ssize_t virt_dist_from_code_start = virt_f - virtual_code_start;
    __offset = virt_dist_from_code_start - out_start_distance - 5;
    cout << "call_offset: " << __offset << " virt: " << virt_dist_from_code_start << " out: " << out_start_distance << endl;
    return (const char*)&__offset;
  }

  void jmp(void *f) {
    uchar *out_current_code_pos = out_c;
    emit("\xe9"); // JMP
    emit(call_offset(out_current_code_pos, f), 4);
  }

  void __call(void *f) {
    uchar *out_current_code_pos = out_c;
    emit("\xe8"); // call
    emit(call_offset(out_current_code_pos, f), 4);
    parameter_count = 0;
  }

  void __vararg_call(void *f) {
    emit("\x30\xc0"); // xor    al,al
    call(f);
  }

  void init(char *c, uchar *start, size_t ss, uchar *end, size_t es) {
    command_file = c;
    virtual_code_start = start;
    virtual_code_end = end;
    find_space(start, ss, end, es, &out_code_start,  
        &out_code_end); 
    printf("code: %zu\n", out_code_end - out_code_start); 
    virtual_strings_start = (uchar *)STRINGS_START;
    virtual_strings_end = (uchar *)STRINGS_END;
    out_c = out_code_start; 
    find_space(virtual_strings_start, strlen(STRINGS_START), 
        virtual_strings_end, strlen(STRINGS_END), 
            &out_strings_start, &out_strings_end); 
    printf("strings: %zu\n", out_strings_end - out_strings_start); 
    out_s = out_strings_start; 
  }

  void finish() {
    jmp(virtual_code_end);
    write();
  }

  //var save_result_as_var() {
  //  // allocate new var
  //  // mov [bp-4],%eax
  //}


  // for varags:
  // before call:
  // 10:	30 c0                	xor    al,al
  // nothing form arm
  //
  // API should look like: 
  //     call(printf)("hello: %d %d\n")(0x12345671)(0x12345672)

 // #include <iostream>
 //  
 // void call(void(*p)()) {
 //     std::cout << "normal: " << std::endl;
 //     p();
 // }
 //  
 // void call(void(*p)(...)) {
 //     std::cout << "vararg: " << std::endl;
 //     p(1, 2, 3);
 // }
 //  
 // void f1() { }
 // void f2(...) { }
 //  
 // int main() {
 //     call(f1);
 //     call(f2);
 // }
}
