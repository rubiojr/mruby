##
# IO

class IOError < StandardError; end
class EOFError < IOError; end

class IO
  SEEK_SET = 0
  SEEK_CUR = 1
  SEEK_END = 2

  BUF_SIZE = 4096

  def for_fd(fd, mode = "r", opt = {})
    self.new(fd, mode, opt)
  end

  def self.open(fd, mode = "r", opt = {}, &block)
    io = self.new(fd, mode, opt)

    return io  unless block

    begin
      yield io
    ensure
      begin
        io.close unless io.closed?
      rescue StandardError
      end
    end
  end

  def self.popen(command, mode = 'r', &block)
    io = self._popen(command, mode)
    return io unless block

    begin
      yield io
    ensure
      begin
        io.close unless io.closed?
      rescue IOError
        # nothing
      end
    end
  end

  def write(string)
    str = string.is_a?(String) ? string : string.to_s
    return str.size unless str.size > 0

    len = syswrite(str)
    if str.size == len
      @pos += len
      return len
    end

    raise IOError
  end

  def eof?
    ret = false
    char = ''

    begin
      char = sysread(1)
    rescue EOFError => e
      ret = true
    ensure
      _ungets(char)
    end

    ret
  end
  alias_method :eof, :eof?

  def pos
    raise IOError if closed?
    @pos
  end
  alias_method :tell, :pos

  def pos=(i)
    seek(i, SEEK_SET)
  end

  def seek(i, whence = SEEK_SET)
    raise IOError if closed?
    @pos = sysseek(i, whence)
    @buf = ''
    0
  end

  def _read(maxlen = BUF_SIZE)
    raise ArgumentError unless maxlen.is_a?(Fixnum)
    raise ArgumentError if maxlen < 0 || maxlen > BUF_SIZE
    return '' if maxlen == 0

    read_eof = nil
    if BUF_SIZE > @buf.size
      begin
        @buf = sysread(BUF_SIZE - @buf.size)
      rescue EOFError => e
        read_eof = e
      end
    end

    ret = nil
    if @buf.size <= maxlen
      ret = @buf
      @buf = ''
    else
      ret = @buf[0, maxlen]
      @buf = @buf[maxlen, @buf.size - maxlen]
    end

    if ret.empty? && read_eof
      raise EOFError
    end

    @pos += ret.size

    return ret
  end

  def _ungets(substr)
    raise TypeError.new "expect String, got #{substr.class}" unless substr.is_a?(String)
    @pos -= substr.size
    if @buf.empty?
      @buf = substr
    else
      @buf = substr + @buf
    end
    nil
  end

  def ungetc(char)
    raise IOError if @pos == 0 || @pos.nil?
    _ungets(char)
    nil
  end

  def read(length = nil)
    unless length.nil? or length.class == Fixnum
      raise TypeError.new "can't convert #{length.class} into Integer"
    end
    if length && length < 0
      raise ArgumentError.new "negative length: #{length} given"
    end

    str = ''
    while 1
      begin
        buf = _read
      rescue EOFError => e
        buf = nil
      end

      if buf.nil?
        str = nil if str.empty?
        break
      else
        str += buf
      end

      break if length && str.size >= length
    end

    if length && str.size > length
      _ungets(str[length, str.size-length])
      str = str[0, length]
    end

    str
  end

  def readline(arg = $/, limit = nil)
    case arg
    when String
      rs = arg
    when Fixnum
      rs = $/
      limit = arg
    else
      raise ArgumentError
    end

    if rs.nil?
      return read
    end

    if rs == ""
      return _readline_paragraph(limit)
    end

    line = ""
    while 1
      begin
        buf = _read
      rescue EOFError => e
        buf = nil
      end

      if buf.nil?
        line = nil if line.empty?
        break
      elsif idx = buf.index(rs)
        line += buf[0, idx+1]
        _ungets(buf[idx+1, buf.size - (idx+1)])
        break
      else
        line += buf
      end

      break if limit && line && line.size >= limit
    end

    raise EOFError.new "end of file reached" if line.nil?

    if limit && line.size > limit
      _ungets(line[limit, line.size-limit])
      line = line[0, limit]
    end

    line
  end

  def _readline_paragraph(limit = nil)
    line = ''
    eof_raise = nil

    while 1
      begin
        buf = readline
      rescue EOFError => e
        buf = nil
        eof_raise = true
      end

      if buf.nil?
        line = nil if line.empty?
        break
      else
        line += buf
      end

      idx = line.rindex($/)
      if idx > 0 && line[idx] == $/ && line[idx-1] == $/
        if line.size > idx
          _ungets(line[idx+1, line.size - (idx+1)])
          line = line[0, idx+1]
        end
        break
      end

      break if limit && line && line.size >= limit
    end

    raise EOFError.new "end of file reached" if line.nil?

    if limit && line.size > limit
      _ungets(line[limit, line.size-limit])
      line = line[0, limit]
    end

    line
  end

  def gets(*args)
    begin
      readline(*args)
    rescue EOFError => e
      nil
    end
  end

  def readchar
    _read(1)
  end

  def getc
    begin
      readchar
    rescue EOFError => e
      nil
    end
  end

  # 15.2.20.5.3
  def each(&block)
    while line = self.gets
      block.call(line)
    end
    self
  end

  # 15.2.20.5.4
  def each_byte(&block)
    while char = self.getc
      block.call(char)
    end
    self
  end

  # 15.2.20.5.5
  alias each_line each

  alias each_char each_byte

  def readlines
    ary = []
    while (line = gets)
      ary << line
    end
    ary
  end

  def puts(*args)
    i = 0
    len = args.size
    while i < len
      s = args[i].to_s
      write s
      write "\n" if (s[-1] != "\n")
      i += 1
    end
    write "\n" if len == 0
    nil
  end

  def printf(*args)
    write sprintf(*args)
    nil
  end

  alias_method :to_i, :fileno
end
