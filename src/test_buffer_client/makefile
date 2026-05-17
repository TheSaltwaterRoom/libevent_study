test_buffer_client: test_buffer_client.cpp
	g++ $^ -o $@ -levent

run: test_buffer_client
	./test_buffer_client

clean:
	rm -f test_buffer_client

.PHONY: run clean