# rubocop:disable all
require 'socket'

def send_chunks
  sock = "/tmp/jamboree.sock"
  socket = UNIXSocket.new(sock)

  File.open('image.jpg', 'rb') do |f|

    until f.eof?
      buf = f.read(128)
      socket.send(buf, 0)
    end

    socket.close_write
    res = socket.read
    puts res
    socket.close
  end
end

def send(data)
  sock = "/tmp/jamboree.sock"
  socket = UNIXSocket.new(sock)
  socket.send(data, 0)
  socket.close_write
  res = socket.read
  puts res
  socket.close
end

threads = []

File.open('image.jpg', 'r') do |f|
  data = f.read

  10.times do |t|
    30.times do |i|
      if i % 2 == 0
        threads << Thread.new { send_chunks }
      else
        threads << Thread.new { send(data) }
      end 
    end

    threads.each do |tr|
      tr.join
    end
  end


end


