#define FOO BAR

#define PREFIX(x,y) x##y

#define LOG(x) std::cout << (x) << std::endl;

void foo()
{
   LOG(PREFIX("foo","bar"));
}
