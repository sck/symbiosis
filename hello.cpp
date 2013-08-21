#include <symbiosis.hpp>

#include <algorithm>

using namespace symbiosis;
using namespace std;

void h1(int a, int b) {
  cout << "Hello: " << a << ", " << b << endl;
}

void h2(const char* s, int a, int b) {
  cout << s << a << ", " << b << endl;
}

void h3(const char* s) {
  cout << s << endl;
}

void d(const char *s) {
  printf("strings addr: %zx\n", (size_t)s);
}

//string all = open_file(argv[1]);
//array<string> a = split_string(all, "\n;")
//function eval(a) {
//  a.each {|v|
//  }
//}

// id r1 = id_new(T_INT, 0x1); 
// id r2 = id_new(T_INT); 
// r2.mov(r1); 
//
// void mov(id r2) {
//   ensure_allocated_register();
//   r2.ensure_allocated_register();
//   backend->mov(r1, r2); 
// }
//
// Intel::mov(id a, id b) {
//   ModRM r(a, b);
//   r.emit(I_MOV_r64_r64_8b);
// }


SYMBIOSIS_MAIN({
  
  //auto v = id_new(T_INT);
  //v = 0;
  //loop([](){ 
  //  condition(v < 10) && .end_loop();
  //  add_parameter(v).call(show);
  //});
  //add_parameter(123)(456); call(h1);
  add_parameter(id_new("Hello: "))(123)(456); call(h2);
  //add_parameter(id_new("Hello")); call(h3);
});
