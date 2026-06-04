test_buffer_filter: test_buffer_filter.cpp
	g++ $^ -o $@ -levent

run: test_buffer_filter
	./test_buffer_filter

clean:
	rm -f test_buffer_filter

.PHONY: run clean