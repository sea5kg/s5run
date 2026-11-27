// Test script for `s5run run`.
//
// Usage:
//   ./s5run server --listen 127.0.0.1:7777 &
//   ./s5run run 127.0.0.1:7777 --script test_run_script.js 21 21
//
// Expected output:
//   hello from test_run_script.js
//   a = 21, b = 21
//   a + b = 42
//   [warning] this is a warning message
//   {ok: true, sum: 42, args: [21, 21]}

function main(key, value) {
  console.log("hello from test_run_script.js");
  console.log("a = " + key + ", b = " + value);
  console.log("a + b = " + (key + value));
  console.warning("this is a warning message from provider_cpp98-provider");
  var total = 0;
  for (var i = 1; i <= 10; ++i)
    total += i;

  var q = getQueue("test_q");
  for (var i = 0; i < 10; i++)
    q.publish(key, value + " (" + i + ")");

  var msg = q.consume();
  while (msg) {
    console.log(msg);
    msg = q.consume();
  }

  var q2 = getQueue("test_scenario5");
  q2.publish("msg1", "Hello from scenario4 to scenario5");

  return {
    ok: true,
    sum: key + value,
    loopTotal: total,
    args: [key, value]
  };
}
