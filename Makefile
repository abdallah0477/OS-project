build:
	gcc process_generator.c -o process_generator.out 
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  process.txt

all: clean build

run:
<<<<<<< HEAD
	./process_generator.out process.txt -sch 4 -q 5
=======
	./process_generator.out process.txt -sch 2
>>>>>>> 2f3ec01 (hpf shaghal)
	
