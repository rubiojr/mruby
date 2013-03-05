##
# IO Test

assert('IO', '15.2.20') do
  assert_equal(Class, IO.class)
end

assert('IO', '15.2.20.2') do
  assert_equal(Object, IO.superclass)
end

assert('IO', '15.2.20.3') do
  assert_include(IO.included_modules, Enumerable)
end

assert('IO.new') do
  IO.new(0)
end

assert('IO gc check') do
  100.times { IO.new(0) }
end

assert('IO TEST SETUP') do
  MRubyIOTestUtil.io_test_setup
end

assert('IO.sysopen, IO#close, IO#closed?') do
  fd = IO.sysopen $mrbtest_io_rfname
  assert_equal Fixnum, fd.class
  io = IO.new fd
  assert_equal IO, io.class
  assert_equal false, io.closed?, "IO not closed"
  assert_equal nil,   io.close,   "IO#close should return nil"
  assert_equal true,  io.closed?, "IO#closed? should return true"
end

assert('IO.sysopen, IO#sysread') do
  fd = IO.sysopen $mrbtest_io_rfname
  io = IO.new fd
  str1 = "     "
  str2 = io.sysread(5, str1)
  assert_equal $mrbtest_io_msg[0,5], str1
  assert_equal $mrbtest_io_msg[0,5], str2
  assert_raise EOFError do
    io.sysread(10000)
    io.sysread(10000)
  end
  io.close
  io.closed?
end

assert('IO.sysopen, IO#syswrite') do
  fd = IO.sysopen $mrbtest_io_wfname, "w"
  io = IO.new fd, "w"
  str = "abcdefg"
  len = io.syswrite(str)
  assert_equal str.size, len
  io.close
  io.closed?
end

assert('IO#_read') do
  fd = IO.sysopen $mrbtest_io_rfname
  io = IO.new fd
  def io._buf
    @buf
  end
  msg_len = $mrbtest_io_msg.size + 1
  assert_equal '', io._buf
  assert_equal '', io._read(0)
  assert_equal '', io._buf
  assert_equal 'mruby', io._read(5)
  assert_equal 5, io.pos
  assert_equal msg_len - 5, io._buf.size
  assert_equal $mrbtest_io_msg[5,100] + "\n", io._read
  assert_equal 0, io._buf.size
  assert_raise EOFError do
    io._read
  end
  assert_raise EOFError do
    io._read(5)
  end
  assert_equal '', io._read(0)
  io.close
  io.closed?
end

assert('IO#read') do
  skip
  fd = IO.sysopen $mrbtest_io_rfname
  io = IO.new fd
  assert_equal 'mruby', io.read(5)
  assert_equal $mrbtest_io_msg[5,100] + "\n", io.read
  assert_equal "",  io.read
  assert_equal nil, io.read(5)
  io.close
  io.closed?
end

assert('IO#gets - 1') do
  fd = IO.sysopen $mrbtest_io_rfname
  io = IO.new fd
  assert_equal $mrbtest_io_msg + "\n", io.gets
  assert_equal nil, io.gets
  io.close
  io.closed?
end

assert('IO#gets - 2') do
  fd = IO.sysopen $mrbtest_io_wfname, "w"
  io = IO.new fd, "w"
  io.write "0123456789" * 5 + "\na"
  assert_equal 52, io.pos
  io.close
  assert_equal true, io.closed?

  fd = IO.sysopen $mrbtest_io_wfname
  io = IO.new fd
  line = io.gets
  assert_equal "0123456789" * 5 + "\n", line
  assert_equal 51, line.size
  assert_equal 51, io.pos
  assert_equal "a", io.gets
  assert_equal nil, io.gets
  io.close
  io.closed?
end

assert('IO TEST CLEANUP') do
  assert_nil MRubyIOTestUtil.io_test_cleanup
end
