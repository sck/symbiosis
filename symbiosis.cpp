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

  bool __debug = false;

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
  void id::set_register_storage(int r, int size) {
    register_value_ = r;
    storage_type = ST_REGISTER; 
    if (size == sizeof(unsigned long)) { type = T_ULONG; }
    if (size == sizeof(unsigned int)) { type = T_UINT; }
  }

  vector<const char*> type_str = { "0:???", "int", "uint", "long", "ulong", 
      "charp", "float", "double", "register"};
  vector<const char*> storage_type_str = { "immediate", "register",
      "pointer"};

  void id::describe() { 
    if (storage_type < 0 || storage_type > storage_type_str.size() - 1) {
      cout << "[st:" << storage_type  << "]";
    } else {
      cout << storage_type_str[storage_type] << ":"; 
    }
    if (type > type_str.size() - 1) {
      cout << "<id:" << type << ">";
    } else {
      cout << type_str[type];
    }
    cout << endl;
  }
  id  id::operator()(id i) { add_parameter(i); return i; }

  id_new::id_new(const char *p) : id(p) {
    storage_type = ST_POINTER;
    virtual_adr = virtual_strings_start + (out_s - out_strings_start);
    virtual_size = strlen(p);
    if (out_s + virtual_size + 1 > out_strings_end) 
        throw exception("Strings: Out of memory");
    memcpy(out_s, p, virtual_size + 1);
    out_s += virtual_size + 1;
  };  

  id_register::id_register(int r, int size = sizeof(size_t)) : id(0) {
    set_register_storage(r, size);
  }

  
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

 typedef vector<function<void()> > callbacks_t;
  class Backend {
  public:
    callbacks_t after_call;
    callbacks_t function_cleanup;
    int parameter_count = 0; 
    Backend() { }
    virtual ~Backend() { }
    void callback(callbacks_t &cb, function<void()> f) { 
          cb.push_back(f); }
    void perform_callbacks(callbacks_t &cb) { 
      for (int i = cb.size() - 1; i >= 0; --i) { cb[i]();  }
      cb.clear();
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
    }
    virtual void finish() = 0; 
    virtual int new_register_var() = 0;
    virtual void preserve(id v) = 0;
    virtual void store(id dest, id source) = 0;
    virtual void mul(id dest, id b) = 0;
    virtual void div(id dest, id b) = 0;
    virtual void add(id dest, id b) = 0;
    virtual void sub(id dest, id b) = 0;
  };

  Backend* backend;

  void id::mul(id b) { 
    backend->mul(*this, b); 
  }
  void id::div(id b) { backend->div(*this, b); }
  void id::add(id b) { backend->add(*this, b); }
  void id::sub(id b) { backend->sub(*this, b); }


  constexpr int variables_max = 3;
  int variable_count = 0; 

  id new_var(id _v) {
    id v = _v;
    if (variable_count >= variables_max) 
      throw exception("Too many variables!");
    int register_value = backend->new_register_var();
    v.set_register_storage(register_value, sizeof(size_t));
    backend->preserve(v);
    ++variable_count;
    backend->store(v, _v);
    return v;
  }

  class Intel : public Backend {
    vector<int> parameter_register_values = { I_REG_DI, I_REG_SI, I_REG_D };
    vector<int> variable_register_values = { I_REG_B, I_REG_R12, 
      I_REG_R13, I_REG_R14, I_REG_R15 };
  public:
    Intel() : Backend() { }
    // XXX: Check for inside scope/loop
    virtual int new_register_var() {
      return variable_register_values[variable_count];
    }
    virtual void preserve(id v) { 
      push(v); 
      callback(function_cleanup, [=]() { pop(v); });
    }
    void emit_byte(uchar c) {
      if (out_c + 1 > out_code_end) throw exception("Code: Out of memory"); 
      *out_c = c;
      if (__debug) printf("%02x ", c); 
      out_c++;
    }
    void emit_modrm(uchar reg, uchar rm, uchar mod) {
      uchar mod_rm = mod + (reg << 3) + rm;
      emit_byte(mod_rm);
    }
    void emit_rex(id _r, id _rm, bool swapped = false) {
      id r = swapped ? _rm : _r;
      id rm = swapped ? _r : _rm;
      uchar rex = 0x0;
      if (__debug) {
        printf("r.rv: %d\n", r.register_value());
        printf("rm.rv: %d\n", rm.register_value());
      }
      if (r.register_value() > 7) { rex |= I_REX_REGISTER_EXT_41; }
      if (rm.register_value() > 7) { rex |= I_REX_RM_EXT_44; }
      if (r.is_64()) { rex |= I_REX_64_BIT_48; }
      if (rex) emit_byte(rex);
    }
    void emit_rex_single(id r, int _64 = -1) {
      if (_64 == -1) _64 = r.is_64();
      uchar rex = 0x0;
      if (r.register_value() > 7) { rex |= I_REX_REGISTER_EXT_41; }
      if (_64) { rex |= I_REX_64_BIT_48; }
      if (rex) emit_byte(rex);
    }
    void emit_rm_op(uchar opcode, id r, id rm, uchar op_prefix = 0x0,
        bool swapped = false ) {
      emit_rex(r, rm, swapped);
      if (op_prefix) emit_byte(op_prefix);
      emit_byte(opcode);
      uchar mod = 0x0;
      if (__debug) {
        printf("rm.is_register: %d\n", rm.is_register());
        cout << "rm: "; rm.describe();
      }
      if (rm.is_register() || rm.is_imm()) { mod = I_MOD_REGISTER_C0; }
      if (rm.is_pointer()) {
        if (pic_mode) { 
          rm.set_register_value(I_REG_RIP_REL);
          mod = I_MOD_INDEX_00;
        } else {
          mod = I_MOD_INDEX_P_SBYTE_40;
        }
      }
      bool s= false;
      id r1 = s ? rm : r;
      id r2 = s ? r : rm;
      emit_modrm(r1.register_value() & 7, r2.register_value() & 7, mod);
    }
    void emit_one_r_op(uchar opcode, int rv, id rm, 
        uchar op_prefix = 0x0, bool swapped = false ) {
      id_register r(rv, rm.size());
      emit_rm_op(opcode, r, rm, op_prefix, swapped);
    }
    void emit_plus_op(uchar op, id r, int _64 = -1) {
      if (!r.is_register()) throw exception("Register expected");
      if (_64 == -1) _64 = r.is_64();
      emit_rex_single(r, _64);
      emit_byte(op + (r.register_value() & 7)); 
    }
    virtual void store(id dest, id source) {
      if (dest.is_imm()) throw exception("Can't store in immediate value");
      if (dest.is_pointer()) throw exception("Pointers not yet supported");
      uchar *out_current_code_pos = out_c;
      if (source.is_pointer() && pic_mode) {
        emit_rm_op(I_LEA_8d, dest, source);
        emit(rip_relative_offset(out_current_code_pos, 
            source.virtual_adr), 4);
      } else if (source.is_register() && dest.is_register()) {
        if (source.register_value() == dest.register_value()) return;
        emit_rm_op(I_MOV_r64_r64_89, source, dest);
      } else if (source.is_imm() || source.is_pointer()) {
        if (source.long_value() == 0x0) {
          emit_rm_op(I_XOR_r64_31, dest, dest);
        } else {
          bool _64 = source.is_64();
          emit_plus_op(I_MOV_r64_imm64_b8, dest, _64);
          emit(_64 ? source.i64(): source.i32(), source.size());
        }
      }
    }
    void push(id p) { emit_plus_op(I_PUSH_r64_50, p); }
    void pop(id p) { emit_plus_op(I_POP_r64_58, p); }
    virtual void add_parameter(id p) {
      id_register p_reg(parameter_register_values[parameter_count], 
          sizeof(size_t));
      store(p_reg, p);
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
      emit_byte(I_XOR_r8_30); emit_byte(0xc0); // xor    al,al
      __call(f);
    }
    virtual void mul(id dest, id b) { 
      if (dest.is_imm()) throw exception("Can't mul immediate value.");
      if (b.is_register()) {
        emit_rm_op(I_IMUL_r64_r64_AF, dest, b, 0xF, true); 
      } else if (b.is_imm()) {
        if (b.is_64()) throw exception("Value too large!");
        if (b.d.ui <= 0xFF) {
          emit_rm_op(I_IMUL_r64_r64_imm8_6B, dest, dest);
          emit_byte(b.d.ui);
        } else {
          emit_rm_op(I_IMUL_r64_r64_imm32_69, dest, dest);
          emit(b.i32(), 4);
        }
      }
    }
    virtual void div(id dest, id b) { 
      if (!dest.is_register()) throw exception("Dest must be register");
      if (b.is_imm()) throw exception("op can't be immediate");

      if (dest.is_signed()) {
        printf("signed!\n");
        // r: 7
        id_register ax(I_REG_A, dest.size());
        store(ax, dest);
        __debug = true;
        emit_byte(I_CDQ_99);
        emit_one_r_op(I_DIV_rAX_r64_F7, 7, b);
        store(dest, ax);
      } else {
        // r: 6
        id_register dx(I_REG_D, dest.size());
        store(dx, 0);
        id_register ax(I_REG_A, dest.size());
        store(ax, dest);
        emit_one_r_op(I_DIV_rAX_r64_F7, 6, b);
        store(dest, ax);
      }
    }
    virtual void add(id dest, id b) { 
      if (b.is_imm() && b.long_value() == 1) {
        emit_one_r_op(I_INC_r64_FF, 0, dest);
        return;
      }
      if (!dest.is_register()) throw exception("Dest must be register");
      if (b.is_register()) {
        emit_rm_op(I_ADD_r64_r64_01, dest, b);
      } else if (b.is_imm()) {
        if (b.size() > 4) throw exception("Operand is larger than 4 bytes");
        if (dest.register_value() == I_REG_A) {
          emit_rex_single(dest);
          emit_byte(I_ADD_RAX_imm32_05);
          emit(b.i32(), 4);
        } else {
          emit_one_r_op(I_ADD_r64_imm32_81, 0, dest);
          emit(b.i32(), 4);
        }
      }
    }
    virtual void sub(id dest, id b) { 
      if (b.is_imm() && b.long_value() == 1) {
        emit_one_r_op(I_DEC_r64_FF, 1, dest);
        return;
      }
    }
    void finish() { 
      perform_callbacks(function_cleanup);
      jmp(virtual_code_end); 
    }
  };

  const char *register_parameters_arm_32[] = {
      "\xe5\x9f\x00", /*r0*/ "\xe5\x9f\x10", /*r1*/ "\xe5\x9f\x20" /*r2*/ };

  class Arm : public Backend {
    int ofs = 1;
    int swapped;
    char __ofs[3];
  public:
    Arm() : Backend() { alloc_next_32_bits(); }
    virtual void store(id dest, id source) { }
    virtual int new_register_var() { return -1; }
    virtual void preserve(id v) { }
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
      callback(after_call, [=]() { 
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
      if (after_call.size() > 0) {
        uchar *jmp_adr = out_c;
        jmp(out_c);
        perform_callbacks(after_call);
        uchar *o = out_c;
        out_c = jmp_adr;
        jmp(virtual_code_start + (o - out_code_start));
        out_c = o;
      }
    }
    virtual void __vararg_call(void *f) {
      throw exception("No arm support yet!");
    }
    virtual void mul(id dest, id b) { }
    virtual void div(id dest, id b) { }
    virtual void add(id dest, id b) { }
    virtual void sub(id dest, id b) { }
    void finish() { jmp(virtual_code_end + 4); }
  };

  id add_parameter(id p) {
    if (backend->parameter_count >= parameters_max) 
        throw exception("Too many parameters!");
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
