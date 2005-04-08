
bool ok = true;

struct T 
{
  ~T() { ok = false; } 
};

int main() 
{
  { static T t; }
  
  if (!ok) return 1;
  return 0;
}

