// group documentation
// @group first group { some comment

// continued and
// extending over multiple lines

// another comment
struct foo
{
private: // helper functions

  // blabla
  //
  // @group make_fn {

  // bla.
  // blabla
  int make_fn() {}
  // bla.
  // blabla
  int make_fn(int) {}
  // }
  // @group } is this lost ?
};
int test1;
int test2;
// @group }
int bar;
// another group
// @group tests {
int test3;
int test4;
// @group }

// @group outer {

// @group inner {
int f;
// @group xxx { not a group
int test5;
// @group } this is accepted
// @group }
// and this too
int test6;
// @group }
