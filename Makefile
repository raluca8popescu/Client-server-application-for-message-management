server:
	g++ server.cpp -o server
subscriber:
	g++ tcp_client.cpp -o subscriber
clean:
	rm server subscriber
