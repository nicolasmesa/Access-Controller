# Makeifle for acl_checker

# List the object files in one place
OBJ=main.o

build:	acl_checker

acl_checker: $(OBJ)
	cc -o $@ $(OBJ)

test:	build
	./acl_checker < test1.txt
	@echo "------------"
	./acl_checker < test2.txt
	@echo "------------"
	./acl_checker < test3.txt
	@echo "------------"
	./acl_checker < test4.txt
	@echo "------------"
	./acl_checker < test5.txt
	@echo "------------"
	./acl_checker < test6.txt
	@echo "------------"
	./acl_checker < test7.txt
	@echo "------------"
	./acl_checker < test8.txt
	@echo "------------"
	./acl_checker < test9.txt
	@echo "------------"
	./acl_checker < test10.txt
	@echo "------------"
	./acl_checker < test11.txt
	@echo "------------"
	./acl_checker < test12.txt
	@echo "------------"
	./acl_checker < test13.txt
	@echo "------------"
	./acl_checker < test14.txt

exec: build
	./acl_checker $(ARG)

clean:
	rm -f acl_checker *.o

