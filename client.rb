# rubocop:disable all
require 'socket'

def send_data
  sock = "/tmp/jamboree.sock"
  socket = UNIXSocket.new(sock)

  File.open('image.jpg', 'r') do |f|

    f.each_byte do |byte|
      socket.send(byte.chr, 0)
    end
  end

  socket.close_write
  res = socket.read
  puts res
  socket.close
end

def send_words
  sock = "/tmp/jamboree.sock"             
  socket = UNIXSocket.new(sock)

  w = %w{det var en gammal gubbe}
  w.each do |word|
    sleep(2)
    socket.send(word, 0)
  end

  socket.close_write
  res = socket.read
  puts res
  socket.clos
end

def send_data_chunk
  sock = "/tmp/jamboree.sock"
  socket = UNIXSocket.new(sock)

  File.open('image.jpg', 'r') do |f|

    data = f.read
    socket.send(data, 0)
  end

  socket.close_write
  res = socket.read
  puts res
  socket.close
end

#send_data_chunk
send_data

