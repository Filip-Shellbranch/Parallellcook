buildFolder := build
implFolder := list_implementations
benchFolder := benchmarks

clean:
	find ${buildFolder}/ -mindepth 1 -delete

bench_course_mutex:
	g++ -Wall -std=c++17 -pthread -O3 ${benchFolder}/course_mutex_bench.cpp -o ${buildFolder}/bench

bench_fine_mutex:
	g++ -Wall -std=c++17 -pthread -O3 ${benchFolder}/fine_mutex_bench.cpp -o ${buildFolder}/bench