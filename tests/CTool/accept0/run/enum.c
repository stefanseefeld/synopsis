

struct fob {
  int a;
  int b;
  int c;
  double d;
};

enum Boolean {
     False,
     True,
     Maybe
  };

int main(){

    int a = sizeof(int);
    
    for (a = 0; a < 5 || a > 10; a++)
        continue;

    return(0); 
}

/*  ###############################################################  */