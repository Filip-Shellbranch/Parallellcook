buildFolder := build
implFolder := list_implementations
benchFolder := benchmarks

clean:
	find ${buildFolder}/ -mindepth 1 -delete

course_mutex:
	g++ -Wall -std=c++17 -pthread -O3 ${implFolder}/course_mutex.cpp -o ${buildFolder}/course_mutex

bench_course_mutex: course_mutex
	g++ -Wall -std=c++17 -pthread -O3 ${benchFolder}/course_mutex_bench.cpp -o ${buildFolder}/bench