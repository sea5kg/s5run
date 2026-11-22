// Test script for `s5run run`.
//
// Usage:
//   ./s5run server --listen 127.0.0.1:7777 &
//   ./s5run run 127.0.0.1:7777 --script test_script.js 21 21
//
// Expected output:
//   hello from test_script.js
//   a = 21, b = 21
//   a + b = 42
//   [warning] this is a warning message
//   {ok: true, sum: 42, args: [21, 21]}

function main(a, b) {
  console.log("hello from test_script.js");
  console.log("a = " + a + ", b = " + b);
  console.log("a + b = " + (a + b));
  console.warning("this is a warning message");

  var total = 0;
  for (var i = 1; i <= 10; ++i)
    total += i;

  return {
    ok: true,
    sum: a + b,
    loopTotal: total,
    args: [a, b]
  };
}
