import std.stdio;
import std.socket;

void main(string[] args) {
	auto socket = new Socket(AddressFamily.INET, SocketType.DGRAM);
	socket.blocking = true;
	socket.bind(new InternetAddress("0.0.0.0", 8500));

	auto targetAddress = new InternetAddress(args[1], 8500);

	auto buffer = new char[4096];

	writeln("Running...");
	size_t i = 0;

	while(true) {
		const auto rcvd = socket.receive(buffer);
		if(rcvd == 0 || rcvd == Socket.ERROR) {
			writeln(i++, " Error rcvd ", rcvd, ": ", socket.getErrorText);
			continue;
		}

		const auto sent = socket.sendTo(buffer[0 .. rcvd], targetAddress);
		if(sent == 0 || sent == Socket.ERROR) {
			writeln(i++, " Error sent ", sent, ": ", socket.getErrorText);
			continue;
		}

		if(sent != rcvd) {
			writeln(i++, " Sent ", sent, " rcvd", rcvd);
			continue;
		}

		writeln(i++, " PACKET ", sent, "\n", buffer[0..rcvd]);
	}
}
