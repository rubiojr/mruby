##
# IO

class IOError < StandardError; end
class EOFError < IOError; end

class IO
  SEEK_SET = 0
  SEEK_CUR = 1
  SEEK_END = 2

  BUF_SIZE = 1024

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

  def pos
    raise IOError if closed?
    @pos
  end

  def pos=(i)
    @pos = i
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
    raise ArgumentError unless substr.is_a?(String)
    @pos -= substr.size
    @buf = substr + @buf
    nil
  end

  def ungetc(char)
    raise IOError if @pos == 0 || @pos.nil?
    _ungets(char)
    nil
  end

  def read(length = nil)
    if length.nil?
      str = ''
      str += buf while (buf = _read)
      return str
    end

    unless length.is_a?(Integer)
      raise TypeError.new "can't convert #{length.class} into Integer"
    end
    if length < 0
      raise ArgumentError.new "negative length: #{length} given"
    end

    str = ''
    while 1
      buf = _read
      str += buf
      break if str.size >= length
    end

    if str.size > length
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

    line = ""
    while 1
      begin
        buf = _read(BUF_SIZE)
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
end
