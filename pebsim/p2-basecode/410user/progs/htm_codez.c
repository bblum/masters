/** @file 410user/progs/htm_codez.c
 *  @author bblum
 *  @brief tests transactional memory
 *  @public yes
 *  @for p2
 *  @covers cond_wait,cond_broadcast
 *  @status done
 */

/* Includes */
#include <syscall.h>
#include <stdlib.h>
#include <thread.h>
#include <mutex.h>
#include <cond.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>
#include <htm.h>

DEF_TEST_NAME("htm-codez:");

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t lock;
static int count = 0;

void txn()
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		count++;
		_xend();
	} else {
		assert(status == _XABORT_RETRY);
	}
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	ERR(mutex_init(&lock));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	txn();

	return 0;
}
