// symbiosis: A framework for toy compilers
//
#include "symbiosis.hpp"
#include <sys/stat.h>

#include <iostream>

#include <vector>
#include <functional>

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

  const char* id::i32() const {
    if (virtual_adr) { _i32 = (size_t)virtual_adr; 
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
    cout << "__virt_f: " << __virt_f ;
    printf(" virtual_code_pos: %zx",  (size_t)(virtual_code_start +
        out_start_distance));
    cout << "call_offset: " << __offset << " virt: " << virt_dist_from_code_start << " out: " << out_start_distance << endl;
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


  constexpr int parameters_max = 3;

  class Backend {
  public:
    vector<function<void()> > callbacks;
    int parameter_count = 0; 
    Backend() { }
    virtual ~Backend() { }
    void callback(function<void()> f) { callbacks.push_back(f); }
    void perform_callbacks() { 
      for (size_t i = 0; i < callbacks.size(); ++i) { callbacks[i]();  }
      callbacks.clear();
    }
    virtual void add_parameter(id p) = 0;
    virtual void jmp(void *f) = 0;
    virtual void __call(void *f) = 0;
    virtual void __vararg_call(void *f) = 0;
    virtual void emit_byte(uchar uc) = 0; 
    virtual void emit(const char* _s, size_t _l = 0) { 
      size_t l = _l > 0 ? _l : strlen(_s);
      uchar *s = (uchar *)_s; uchar *e = s + l;
      for (uchar * b  = s; b < e; b++)  emit_byte(*b);
      //dump(out_start, l);
    }
    virtual void finish() = 0; 
  };

  Backend* backend;


  const char *register_parameters_intel_32[] = {
      "\xbf", /*edi*/ "\xbe", /*esi*/ "\xba" /*edx*/ };
  
  const char *register_rip_relative_parameters_intel_64[] = {
    "\x48\x8d\x3d", /* rdi */ "\x48\x8d\x35", /* rsi */ 
    "\x48\x8d\x15" /* rdx */ };

  const char *register_parameters_intel_64[] = {
      "\x48\xbf" /*rdi*/, "\x48\xbe" ,/*rsi*/ "\x48\xba" /*rdx*/ };

  class Intel : public Backend {
    vector<int> parameter_register_values = { I_REG_DI, I_REG_SI, I_REG_D };
    vector<int> variable_register_values = { I_REG_B, I_REG_R12, 
        I_REG_R13, I_REG_R14, I_REG_R15 };
  public:
    Intel() : Backend() { }
    void emit_byte(uchar c) {
      if (out_c + 1 > out_code_end) throw exception("Code: Out of memory"); 
      *out_c = c;
      out_c++;
    }
    void emit_modrm(uchar reg, uchar rm, uchar mod) {
      uchar mod_rm = mod + (reg << 3) + rm;
      emit_byte(mod_rm);
    }
    void emit_rex(id r, id rm) {
      uchar rex = 0x0;
      if (r.register_value() > 7) { rex |= I_REX_REGISTER_EXT_41; }
      if (rm.register_value() > 7) { rex |= I_REX_RM_EXT_44; }
      if (r.is_64()) { rex |= I_REX_64_BIT_48; }
      if (rex) emit_byte(rex);
    }
    void emit_op(uchar opcode, id r, id rm, bool swapped = false) {
      emit_rex(r, rm);
      emit_byte(opcode);
      uchar mod = 0x0;
      if (rm.is_register()) { mod = I_MOD_REGISTER_C0; }
      if (rm.is_p()) {
        if (pic_mode) { 
          rm.set_register_value(I_REG_RIP_REL);
          mod = I_MOD_INDEX_00;
        } else {
          mod = I_MOD_INDEX_P_SBYTE_40;
        }
      }
      id r1 = swapped ? rm : r;
      id r2 = swapped ? r : rm;
      emit_modrm(r1.register_value() & 7, r2.register_value() & 7, mod);
    }
    virtual void add_parameter(id p) {
      if (p.is_charp()) {
        if (!pic_mode) {
          cout << "charp: NO pic_mode" << endl;
          emit(register_parameters_intel_32[parameter_count]);
          emit(p.i32(), 4);
        } else {
          cout << "charp: pic_mode" << endl;
          uchar *out_current_code_pos = out_c;
          id p_reg(0); p_reg.set_register_value(
              parameter_register_values[parameter_count]);
          emit_op(I_LEA_8d, p_reg, p);
          //emit(register_rip_relative_parameters_intel_64[parameter_count]);
          emit(rip_relative_offset(out_current_code_pos, p.virtual_adr), 4);
        }
      } else if (p.is_integer()) {
        if (p.is_32()) {
          emit(register_parameters_intel_32[parameter_count]);
          emit(p.i32(), 4);
        } else if (p.is_64()) {
          emit(register_parameters_intel_64[parameter_count]);
          emit(p.i64(), 8);
        }
      }
    }
    virtual void jmp(void *f)  {
      uchar *out_current_code_pos = out_c;
      emit_byte(I_JMP_e9); 
      emit(call_offset(out_current_code_pos, f), 4);
    }
    virtual void __call(void *f) {
      uchar *out_current_code_pos = out_c;
      emit_byte(I_CALL_e8); 
      emit(call_offset(out_current_code_pos, f), 4);
    }
    virtual void __vararg_call(void *f) {
      emit_byte(I_XOR_30); emit_byte(0xc0); // xor    al,al
      __call(f);
    }
    void finish() { jmp(virtual_code_end); }
  };

  const char *register_parameters_arm_32[] = {
      "\xe5\x9f\x00", /*r0*/ "\xe5\x9f\x10", /*r1*/ "\xe5\x9f\x20" /*r2*/ };

  class Arm : public Backend {
    int ofs = 1;
    int swapped;
    char __ofs[3];
  public:
    Arm() : Backend() { alloc_next_32_bits(); }
    void alloc_next_32_bits() {
      if (out_c + 4 > out_code_end) throw exception("Code: Out of memory"); 
      out_c += 4;
      ofs = 1;
    }
    void emit_byte(uchar c) {
      printf(".%02x", c);
      *(out_c - ofs) = c;
      ofs++;
      if (ofs == 5) {
        if (out_c > out_code_start) { printf("<<"); dump(out_c - 4, 4); printf(">>"); }
        alloc_next_32_bits();
      }
    }
    void load_pc_relative(id p) {
      uchar *ldr_p = out_c - 4;
      emit(register_parameters_arm_32[parameter_count], 3); emit("\x12");
      callback([=]() { 
        ldr_p[0] = (uchar)(out_c - ldr_p) - 12;
        ssize_t out_start_distance = ldr_p - out_code_start;
        size_t virt_pos = (size_t)virtual_code_start + out_start_distance;
        std::cout << "ldr_p: " << (size_t)ldr_p[0];
        printf(" virt_pos: %zx\n", virt_pos);
        emit(swap_int(p.i32()), 4);
      });
    }
    virtual void add_parameter(id p) {
      if (p.is_charp()) {
        if (!pic_mode) { load_pc_relative(p); } else {
          throw exception("pic mode not supported yet!");
          //uchar *out_current_code_pos = out_c;
          //emit(register_rip_relative_parameters_intel_64[parameter_count]);
          //emit(rip_relative_offset(out_current_code_pos, p.virtual_adr), 4);
        }
      } else if (p.is_integer()) {
        if (p.is_32()) { load_pc_relative(p); } else if (p.is_64()) {
          throw exception("64bit not supported yet!");
        }
      } else {
        //cout << "No integer not supported yet" << endl;
        throw exception("No integer not supported yet");
      }
    }
    const char *swap_int(const char* i) {
      auto p = (char*)&swapped;
      p[0] = i[3]; p[1] = i[2]; p[2] = i[1]; p[3] = i[0];
      return p;
    }
    
    const char* arm_offset(uchar *out_current_code_pos, void *__virt_f) { 
      const char *r = call_offset(out_current_code_pos, __virt_f);
      printf("__offset: before: %d ", __offset);
      __offset /= 4;
      printf("-> %d \n", __offset);
      __ofs[0] = r[2]; __ofs[1] = r[1]; __ofs[2] = r[0];
      return (const char*)__ofs;
    }
    virtual void jmp(void *f)  { 
      uchar *out_current_code_pos = out_c;
      emit_byte(A_B_ea);
      emit(arm_offset(out_current_code_pos, f), 3);
    }
    virtual void __call(void *f) {
      uchar *out_current_code_pos = out_c;
      emit_byte(A_BL_eb);
      emit(arm_offset(out_current_code_pos, f), 3);
      if (callbacks.size() > 0) {
        uchar *jmp_adr = out_c;
        jmp(out_c);
        perform_callbacks();
        uchar *o = out_c;
        out_c = jmp_adr;
        jmp(virtual_code_start + (o - out_code_start));
        out_c = o;
      }
    }
    virtual void __vararg_call(void *f) {
      throw exception("No arm support yet!");
    }
    void finish() { jmp(virtual_code_end + 4); }
  };

  id add_parameter(id p) {
    if (backend->parameter_count >= parameters_max) {
      fprintf(stderr, "Too many parameters!\n");
      return p;
    }
    backend->add_parameter(p);
    ++backend->parameter_count;
    return p;
  }

  void jmp(void *f) { backend->jmp(f); }
  void __call(void *f) { backend->__call(f); backend->parameter_count = 0; }
  void __vararg_call(void *f) { backend->__vararg_call(f); }

  void init(char *c, uchar *start, size_t ss, uchar *end, size_t es) {
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
    if (intel) { backend = new Intel(); }
    if (arm) { backend = new Arm(); }
  }

  void finish() {
    backend->finish();
    write();
  }

}
