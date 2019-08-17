# rubocop:disable all
require 'socket'

threads = []

sock = "/tmp/jamboree.sock"

File.open('image.jpg', 'r') do |f|
  data = f.read

  4.times do |t|
    30.times do |i|
      threads << Thread.new {
        socket = UNIXSocket.new(sock)
        socket.send(data, 0)
        socket.close_write
        res = socket.read
        puts res
        socket.close
      }
    end
    threads.each do |t|
      t.join
    end
  end


end
