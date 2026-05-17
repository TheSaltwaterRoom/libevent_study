test_signal_loop: test_signal_loop.cpp
	g++ $^ -o $@ -levent

run: test_signal_loop
	./test_signal_loop

clean:
	rm -f test_signal_loop

.PHONY: run clean