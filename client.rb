# rubocop:disable all
require 'socket'

def send_data
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

threads = []
sock = "/tmp/jamboree.sock"

File.open('image.jpg', 'r') do |f|
  data = f.read

  10.times do |t|
    30.times do |i|
      threads << Thread.new {
        send_data

        #socket = UNIXSocket.new(sock)
        #socket.send(data, 0)
        #socket.close_write
        #res = socket.read
        #puts res
        #socket.close
      }
    end

    threads.each do |tr|
      tr.join
    end
  end


end


