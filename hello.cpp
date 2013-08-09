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

SYMBIOSIS_MAIN({
  add_parameter(123)(456); call(h1);
  add_parameter(id_new("Hello: "))(123)(456); call(h2);
  add_parameter(id_new("Hello")); call(h3);
  //add_parameter(id_new("Hello: %d, %d\n"))(123)(456); vararg_call(printf);
  //add_parameter(id_new("Hahah\n")); vararg_call(printf);
});
