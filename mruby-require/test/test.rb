assert "Kernel#compile exist" do
  self.methods.include?(:compile)
end

assert "Kernel#load_mrb_str exist" do
  self.methods.include?(:load_mrb_str)
end

assert "compile check" do
  code = "COMPILE_TEST1 = 1"
  str = compile code, "t.rb"
  puts str.size
end

assert "compile & load" do
  assert_equal false, Object.const_defined?(:COMPILE_TEST1)
  str = compile("COMPILE_TEST1 = 1", "t.rb")
  load_mrb_str(str)
  assert_equal true,  Object.const_defined?(:COMPILE_TEST1)
end

