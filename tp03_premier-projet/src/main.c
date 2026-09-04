#include "stm32f3xx.h"
#include "functions.h"

int main(void){
	int i;	
	static int observable;
	
	for(i=0;i<100000;i++){
		observable = function1(observable);
	}
	function2();
	return 0;
}
