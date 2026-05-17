test_buffer: test_buffer.cpp
	g++ $^ -o $@ -levent

run: test_buffer
	./test_buffer

clean:
	rm -f test_buffer

.PHONY: run clean