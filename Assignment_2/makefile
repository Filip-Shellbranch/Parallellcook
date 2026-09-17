buildFolder := build
e1Name := e1.out
e2Name := e2.out
e5Name := e5.out
e6Name := e6.out
VENV := Exercise_5/venv/bin


clean:
	find ${buildFolder}/ -mindepth 1 -delete

e1: 
	g++ Exercise_1/numerical_integration_fast.cpp -o ${buildFolder}/${e1Name}

e2:
	g++ Exercise_2/sieve.cpp -o ${buildFolder}/${e2Name}
	./build/e2.out