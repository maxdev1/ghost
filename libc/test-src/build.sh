
if [ -e $1-test.cpp ]; then
	g++ $1-test.cpp -o $1-test
	if [ $? -ne 0 ]; then
		exit 1
	fi
	./$1-test
else
	echo "test $1 does not exist"
fi