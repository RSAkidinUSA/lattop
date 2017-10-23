#include <unistd.h>

unsigned int call3(unsigned int i) {
	return sleep(i);
}

unsigned int call2(unsigned int i) {
	return call3(i);
}

unsigned int call1(unsigned int i) {
	return call2(i);
}

int main () {
	return call1(1000);
}
