// -*- Comments.SSFilter -*-
// -*- Comments.Grouper -*-

// group documentation
// @group first group {

// some comment
// extending over multiple lines

// another comment
struct foo
{
};
int test1;
int test2;
// }
int bar;
// another group
// @group tests {
int test3;
int test4;
// }

// @group outer {

// @group inner {
int f;
// @group xxx { not a group
int test5;
// } and not a group end
// }
// and not a group either
int test6;
// }

// }
