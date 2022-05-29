-- script.lua
-- Receives a table, returns the sum of its components.
io.write("The table the script received has:\n");
x = 0
for i = 1, #foo do
  print(i, foo[i])
  x = x + foo[i]
end

io.write("Asking C to compute average and sum and check the equality\n")
avg, sum = average(unpack(foo));

print(x == sum)
io.write("Returning data back to C\n");
return sum