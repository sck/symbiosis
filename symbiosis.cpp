// symbiosis: A framework for toy compilers
//
#include "symbiosis.hpp"
#include <sys/stat.h>

#include <iostream>

#include <vector>

#include "cpu_defines.hpp"

using namespace std;

namespace symbiosis {

  void dump(uchar* start, size_t s) {
    for (size_t i = 0; i < s; i++) {
      printf("%02x ", start[i]);
    }
  }

#ifdef __x86_64__
  bool intel = true;
  bool arm = false;
#endif

#ifdef __arm__
  bool intel = false;
  bool arm = true;
#endif

  bool pic_mode;

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
    if (virtual_adr) { _i32 = (size_t)virtual_adr; 
         //printf("virtual_adr: %x\n", _i32);
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
  
  int __offset = 0;
  const char* call_offset(uchar *out_current_code_pos, void *__virt_f) { 
    auto virt_f = (uchar*)__virt_f;
    ssize_t out_start_distance = out_current_code_pos - out_code_start;
    ssize_t virt_dist_from_code_start = virt_f - virtual_code_start;
    __offset = virt_dist_from_code_start - out_start_distance - 5;
    cout << "__virt_f: " << __virt_f << endl;
    //cout << "call_offset: " << __offset << " virt: " << virt_dist_from_code_start << " out: " << out_start_distance << endl;
    return (const char*)&__offset;
  }

  const char* rip_relative_offset(uchar *out_current_code_pos, 
      uchar *virt_adr) { 
    ssize_t distance = out_current_code_pos - out_code_start;
    ssize_t virt_dist_from_code_start = (size_t)virt_adr - 
        (size_t)virtual_code_start - distance;
    __offset = virt_dist_from_code_start - 7;
    printf("virt_dist_from_code_start: %zx %x, string ofs: %zd\n", 
        (size_t)virt_adr, __offset,
        (size_t)(out_s - out_strings_start));
    return (const char*)&__offset;
  }


  int parameter_count = 0;
  const char *register_parameters_intel_32[] = {
      "\xbf", /*edi*/ "\xbe", /*esi*/ "\xba" /*edx*/ };
  
  const char *register_rip_relative_parameters_intel_64[] = {
    "\x48\x8d\x3d", /* rdi */ "\x48\x8d\x35", /* rsi */ 
    "\x48\x8d\x15" /* rdx */ };

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

  void emit(uchar uc) { emit((const char *)&uc, sizeof(uc)); }

  id add_parameter(id p) {
    if (parameter_count >= parameters_max) {
      fprintf(stderr, "Too many parameters!\n");
      return p;
    }
    if (p.is_charp()) {
      if (!pic_mode) {
        emit(register_parameters_intel_32[parameter_count]);
        emit(p.i32(), 4);
      } else {
        uchar *out_current_code_pos = out_c;
        emit(register_rip_relative_parameters_intel_64[parameter_count]);
        emit(rip_relative_offset(out_current_code_pos, p.virtual_adr), 4);
      }
    } else if (p.is_integer()) {
      if (intel) {
        if (p.is_32()) {
          emit(register_parameters_intel_32[parameter_count]);
          emit(p.i32(), 4);
        } else if (p.is_64()) {
          emit(register_parameters_intel_64[parameter_count]);
          emit(p.i64(), 8);
        }
      } else if (arm) {
        emit(register_parameters_intel_32[parameter_count]);
      }
    }
    ++parameter_count;
    return p;
  }

  void jmp(void *f) {
    uchar *out_current_code_pos = out_c;
    emit(I_JMP_e9); 
    emit(call_offset(out_current_code_pos, f), 4);
  }

  void __call(void *f) {
    uchar *out_current_code_pos = out_c;
    emit(I_CALL_e8); 
    emit(call_offset(out_current_code_pos, f), 4);
    parameter_count = 0;
  }

  void __vararg_call(void *f) {
    emit(I_XOR_30); emit(0xc0); // xor    al,al
    call(f);
  }

  void init(char *c, uchar *start, size_t ss, uchar *end, size_t es) {
    //if (!identify_cpu_and_pic_mode()) exit(1);
    cout << "intel: " << intel << ", arm: " << arm << 
        ", pic_mode: " << pic_mode << endl;
    command_file = c;
    virtual_code_start = start;
    virtual_code_end = end;
    find_space(start, ss, end, es, &out_code_start,  
        &out_code_end); 
    virtual_strings_start = (uchar *)STRINGS_START;
    virtual_strings_end = (uchar *)STRINGS_END;
    out_c = out_code_start; 
    find_space(virtual_strings_start, strlen(STRINGS_START), 
        virtual_strings_end, strlen(STRINGS_END), 
            &out_strings_start, &out_strings_end); 
    out_s = out_strings_start; 
  }

  void finish() {
    jmp(virtual_code_end);
    write();
  }

}
